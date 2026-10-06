"""
Arena remake, big shapes (run on art/arena/Arena.blend in Blender 5.2; see Usage).

Models what a texture cannot fake, so the stone reads as cut blocks under the toon lighting:
  1. a plinth course at the foot of the inner arena wall (a base band that stands out from the wall), and
  2. a chamfer on every hard edge of the coliseum (ledge lips, wall tops, window reveals, block corners): the sun
     draws a light or dark band along each edge.
  3. (version 2) weighted normals: on the smooth curved walls, a vertex where a chamfer ends averaged the wall's normal
     with the ledge's, and the toon ramp drew a bright wedge down the wall; with the normals weighted by face area
     (hard edges kept) the big faces decide. They are custom normals: the export keeps them (arena_ao.py).
  4. (version 3) texture UVs at world scale (the first UV set; the AO set is untouched): 1 UV unit = one sandstone
     tile (STONE_TILE metres, arena_textures.py). Walls are unwrapped (seams on hard edges, where walls meet floors,
     and one cut at +x behind the stepped block); each piece is turned so that "up" is +v, scaled to world size and
     shifted so that v = height / tile, so the block courses run level all around the arena. Floors and treads are
     mapped from above (u = x, v = y).
The ring ("arena" object, its own steps in its tumbu_shapes):
  1. its floor and sides get the material arenaFloorMaterial (the grey tiles; world-scale UVs, RING_TILE metres per
     unit) and the ropes arenaRopesMaterial (for the red neon); the posts keep the 2011 material and atlas.
Then the AO UV map is removed (the bake script lays it out again for the new faces) and the hard edges are marked
again. The changes are saved into Arena.blend, which stays the source of truth; run scripts/bake-arena-ao.ps1
afterwards to bake the AO and export the meshes.

The object records the steps already applied in its custom property tumbu_shapes, and only the newer steps run (a
second chamfer would cut the chamfers again). Settings: the constants below.

Usage: blender --background art/arena/Arena.blend --python arena_shapes.py -- <repo root>
"""
import bpy
import bmesh
import math
import os
import sys
from mathutils import Vector

SHAPES_VERSION = 3
RING_VERSION = 1
STONE_TILE = 4.8            # metres per texture repeat on the coliseum (arena_textures.py STONE_TILE)
RING_TILE = 12.8            # metres per texture repeat on the ring floor (arena_textures.py TILES_TILE)
RING_FLOOR = "arenaFloorMaterial"
RING_ROPES = "arenaRopesMaterial"
PLINTH_HEIGHT = 0.9         # units above the ground (the terrain covers the first ~0.2 near the wall)
PLINTH_DEPTH = 0.15         # how far the plinth stands out from the wall
PLINTH_MAX_RADIUS = 28.5    # only the inner arena wall: it is an oval, 15 to 27 units from the centre
CHAMFER = 0.08              # chamfer width on the hard edges (about 3 pixels from the chase camera)
HARD_ANGLE = 30.0           # degrees: edges sharper than this are hard (as clean_mesh in arena_ao.py)


def log(message):
    print("[SHAPES] " + message, flush=True)


def inner_wall_faces(bm):
    """Vertical faces of the inner arena wall that touch the ground and face the centre."""
    faces = []
    for f in bm.faces:
        n = f.normal
        if abs(n.z) > 0.2:
            continue
        zs = [v.co.z for v in f.verts]
        if min(zs) > 0.01:
            continue
        c = f.calc_center_median()
        radial = Vector((c.x, c.y, 0.0))
        if radial.length > PLINTH_MAX_RADIUS or radial.length < 1.0:
            continue
        if n.dot(-radial.normalized()) < 0.7:   # facing the arena's centre
            continue
        faces.append(f)
    return faces


def add_plinth(bm):
    faces = inner_wall_faces(bm)
    if not faces:
        raise SystemExit("no inner wall faces found for the plinth")
    geom = list({e for f in faces for e in f.edges}) + faces + list({v for f in faces for v in f.verts})
    bmesh.ops.bisect_plane(bm, geom=geom, plane_co=Vector((0, 0, PLINTH_HEIGHT)), plane_no=Vector((0, 0, 1)))
    bm.faces.ensure_lookup_table()
    low = [f for f in inner_wall_faces(bm) if max(v.co.z for v in f.verts) <= PLINTH_HEIGHT + 1e-4]
    ret = bmesh.ops.extrude_face_region(bm, geom=low)
    new_verts = [e for e in ret["geom"] if isinstance(e, bmesh.types.BMVert)]
    # move each new vertex outwards from the wall, towards the arena's centre (the wall is round)
    for v in new_verts:
        inward = -Vector((v.co.x, v.co.y, 0.0)).normalized()
        v.co += inward * PLINTH_DEPTH
    # extrude_face_region removes the original faces unless they are still needed
    leftover = [f for f in low if f.is_valid]
    if leftover:
        bmesh.ops.delete(bm, geom=leftover, context='FACES')
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.normal_update()
    log("plinth: %d wall faces, %.2f high, %.2f deep" % (len(low), PLINTH_HEIGHT, PLINTH_DEPTH))


def chamfer(bm):
    bm.edges.ensure_lookup_table()
    hard = [e for e in bm.edges if len(e.link_faces) == 2
            and math.degrees(e.calc_face_angle(0.0)) > HARD_ANGLE
            and not all(v.co.z < 0.01 for v in e.verts)]    # not along the ground
    ret = bmesh.ops.bevel(bm, geom=hard, offset=CHAMFER, offset_type='OFFSET', segments=1, profile=0.5,
                          affect='EDGES', clamp_overlap=True)
    # every edge of a chamfer is hard, so it shades as one flat band (a shallow corner would blend otherwise)
    for f in ret["faces"]:
        for e in f.edges:
            e.smooth = False
    log("chamfer: %d hard edges, %.2f wide" % (len(hard), CHAMFER))


def weighted_normals(ob):
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    mod = ob.modifiers.new("tumbu_weighted_normals", 'WEIGHTED_NORMAL')
    mod.mode = 'FACE_AREA'
    mod.weight = 100
    mod.keep_sharp = True
    bpy.ops.object.modifier_apply(modifier=mod.name)
    log("weighted normals (face area, hard edges kept)")


def world_uvs(ob):
    """Texture UVs at world scale (step 4 of the docstring)."""
    me = ob.data
    uv_name = me.uv_layers[0].name           # the texture UV set (Ogre texture coordinate 0)
    me.uv_layers.active = me.uv_layers[0]
    bm = bmesh.new()
    bm.from_mesh(me)
    flat = {f.index for f in bm.faces if abs(f.normal.z) > 0.7}
    for e in bm.edges:
        faces = e.link_faces
        seam = (not e.smooth) or len(faces) != 2
        if not seam:
            a, b = faces
            if (a.index in flat) != (b.index in flat):
                seam = True
            else:
                ca, cb = a.calc_center_median(), b.calc_center_median()
                aa, ab = math.atan2(ca.y, ca.x), math.atan2(cb.y, cb.x)
                # the one cut that opens each closed ring: at +x, behind the stepped block
                seam = (aa < 0) != (ab < 0) and abs(aa) < math.pi / 2 and abs(ab) < math.pi / 2
        e.seam = seam
    bm.to_mesh(me)
    bm.free()

    for p in me.polygons:
        p.select = p.index not in flat
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_mode(type='FACE')
    bpy.ops.uv.unwrap(method='ANGLE_BASED', margin=0.0)
    bpy.ops.object.mode_set(mode='OBJECT')

    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv[uv_name]
    walls = [f for f in bm.faces if f.index not in flat]
    # the wall pieces: faces joined through edges that are not seams
    parent = {f.index: f.index for f in walls}

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for e in bm.edges:
        if not e.seam and len(e.link_faces) == 2:
            a, b = e.link_faces
            if a.index in parent and b.index in parent:
                parent[find(a.index)] = find(b.index)
    islands = {}
    for f in walls:
        islands.setdefault(find(f.index), []).append(f)

    for faces in islands.values():
        # direction of increasing height in UV space, UV area and world area (fans of triangles)
        gx = gy = area_uv = area_3d = 0.0
        for f in faces:
            loops = f.loops
            p0, t0 = loops[0].vert.co, loops[0][uv].uv
            for i in range(1, len(loops) - 1):
                p1, t1 = loops[i].vert.co, loops[i][uv].uv
                p2, t2 = loops[i + 1].vert.co, loops[i + 1][uv].uv
                d1, d2 = t1 - t0, t2 - t0
                det = d1.x * d2.y - d1.y * d2.x
                if abs(det) < 1e-12:
                    continue
                z1, z2 = p1.z - p0.z, p2.z - p0.z
                w = abs(det)
                gx += (z1 * d2.y - z2 * d1.y) / det * w
                gy += (d1.x * z2 - d2.x * z1) / det * w
                area_uv += w / 2
                area_3d += (p1 - p0).cross(p2 - p0).length / 2
        if area_uv <= 0:
            continue
        turn = math.pi / 2 - math.atan2(gy, gx)
        c, s = math.cos(turn), math.sin(turn)
        scale = math.sqrt(area_3d / area_uv) / STONE_TILE
        shift, count = 0.0, 0
        for f in faces:
            for l in f.loops:
                u, v = l[uv].uv
                l[uv].uv = ((c * u - s * v) * scale, (s * u + c * v) * scale)
                shift += l[uv].uv.y - l.vert.co.z / STONE_TILE
                count += 1
        shift /= count
        for f in faces:
            for l in f.loops:
                l[uv].uv.y -= shift
    for f in bm.faces:
        if f.index in flat:
            for l in f.loops:
                l[uv].uv = (l.vert.co.x / STONE_TILE, l.vert.co.y / STONE_TILE)
    bm.to_mesh(me)
    bm.free()
    log("world UVs: %d wall pieces, %d floor faces, %.1f m per tile" % (len(islands), len(flat), STONE_TILE))


def ring_materials(ob):
    """The ring's floor and sides get the tile material with world-scale UVs, the ropes their own material."""
    me = ob.data
    for name in (RING_FLOOR, RING_ROPES):
        if name not in me.materials:
            mat = bpy.data.materials.get(name) or bpy.data.materials.new(name)
            me.materials.append(mat)
    floor_slot = me.materials.find(RING_FLOOR)
    rope_slot = me.materials.find(RING_ROPES)
    corners = []
    for v in me.vertices:
        if v.co.z > 1.45 and all((v.co.xy - k).length > 1.0 for k in corners):
            corners.append(v.co.xy.copy())
    if len(corners) != 6:
        raise SystemExit("expected the 6 ring posts, found %d" % len(corners))
    uv = me.uv_layers[0].data
    counts = {"floor": 0, "side": 0, "rope": 0}
    for f in me.polygons:
        c, n = f.center, f.normal
        if n.z > 0.9 and c.z < 0.25:
            f.material_index = floor_slot
            for li in f.loop_indices:
                p = me.vertices[me.loops[li].vertex_index].co
                uv[li].uv = (p.x / RING_TILE, p.y / RING_TILE)
            counts["floor"] += 1
        elif c.z < 0.22 and abs(n.z) < 0.5:
            f.material_index = floor_slot
            along = Vector((-n.y, n.x, 0.0)).normalized()
            for li in f.loop_indices:
                p = me.vertices[me.loops[li].vertex_index].co
                uv[li].uv = (p.dot(along) / RING_TILE, p.z / RING_TILE)
            counts["side"] += 1
        elif min((c.xy - k).length for k in corners) > 0.7:
            f.material_index = rope_slot
            counts["rope"] += 1
    log("ring: %(floor)d floor faces, %(side)d side faces, %(rope)d rope faces" % counts)


def main():
    ob = bpy.data.objects.get("coliseum")
    if ob is None:
        raise SystemExit("Arena.blend has no 'coliseum' object")
    done = ob.get("tumbu_shapes", 0)
    before = len(ob.data.vertices)
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    if done < 1:
        bm = bmesh.new()
        bm.from_mesh(ob.data)
        add_plinth(bm)
        chamfer(bm)
        bm.to_mesh(ob.data)
        bm.free()
        # smooth shading by angle, keeping the chamfer edges hard
        bpy.ops.object.shade_smooth_by_angle(angle=math.radians(HARD_ANGLE), keep_sharp_edges=True)
        # the AO lightmap layout is made again by the bake script for the new faces
        ao = ob.data.uv_layers.get("AO")
        if ao is not None:
            ob.data.uv_layers.remove(ao)
    if done < 2:
        weighted_normals(ob)
    if done < 3:
        world_uvs(ob)
    ob["tumbu_shapes"] = SHAPES_VERSION
    log("coliseum: shapes version %d -> %d, %d -> %d vertices" % (done, SHAPES_VERSION, before, len(ob.data.vertices)))

    ring = bpy.data.objects.get("arena")
    if ring is None:
        raise SystemExit("Arena.blend has no 'arena' object")
    ring_done = ring.get("tumbu_shapes", 0)
    if ring_done < 1:
        ring_materials(ring)
    ring["tumbu_shapes"] = RING_VERSION
    log("ring: shapes version %d -> %d" % (ring_done, RING_VERSION))
    bpy.ops.wm.save_mainfile()
    log("saved " + bpy.data.filepath)


main()
