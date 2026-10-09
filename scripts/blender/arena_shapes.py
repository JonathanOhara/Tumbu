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
  5. (version 4) sloped walls (the big leaning upper wall: a cone, not a cylinder) unwrap into a curved strip, so one
     turn per piece left the courses diagonal at its ends. Sloped pieces are mapped directly instead: u = the angle
     around the arena x the piece's mean radius, v = the distance up the slope; every course is level.
  6. (version 5) "floors" that lean (a ledge band below the big windows, 0.7 < |normal.z| < LEANING) took v from the
     distance to the centre, which on the oval arena tilts each face's courses and steps them at every face edge; they
     take v from height (up the slope) like the sloped walls.
  7. (version 6) the same for wall pieces leaning only a few degrees (a ledge's front face: its long, slightly conical
     strip still unwrapped into an arc): steps 5 and 6 again with "sloped" from |normal.z| > SLOPED_V6.
  8. (version 7, the mapping now in use; it replaces steps 4 to 7) every face is mapped again by its kind. Faces join
     into pieces through smooth edges between faces of the same kind less than 30 degrees apart, so a piece's mapping
     is continuous. Floors (|normal.z| > 0.7): u = angle around the arena x the piece's mean radius, v = distance from
     the centre (flat) or up the slope (leaning, below LEANING). Walls facing the centre: the same u, v = up the wall.
     Walls facing sideways (block ends, jambs): a flat projection, u along the wall, v up. Steps 4 to 7 measured the
     angle from the arena's +x axis and multiplied it by each vertex's own radius, which smeared the texture across
     the ledge tops, and they mapped sideways walls around the arena, which collapsed them into stripes.
The ring ("arena" object, its own steps in its tumbu_shapes):
  1. its floor and sides get the material arenaFloorMaterial (the grey tiles; world-scale UVs, RING_TILE metres per
     unit) and the ropes arenaRopesMaterial (for the red neon); the posts keep the 2011 material and atlas.
The wall torches (the night lights; "torches" object, its own version in its tumbu_shapes):
  1. 17 torches in iron brackets, one every TORCH_STEP degrees around the arena (symmetric about the gate), placed by
     rays from the centre onto the tall wall and the front of the seating podium: a back plate, an arm, a cup (iron,
     torchIronMaterial) and a toon flame (torchFlameMaterial; its UVs carry the torch's flicker phase and the height
     in the flame for torch_flame.vert). The object goes into the Export collection, so bake-arena-ao.ps1
     (-Only torches) bakes its AO and exports torches.mesh. The torches' light positions and the neon's light segments
     (the ring sides at the ropes' mid height, the T's tube from ring_emblem.png) are written to
     media/configuration/lamps.object (Lighting::loadLamps).
After the coliseum's step 1 its AO UV map is removed (the bake script lays it out again for the new faces) and the
hard edges are marked again. The changes are saved into Arena.blend, which stays the source of truth; run scripts/bake-arena-ao.ps1
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
from mathutils import Matrix, Vector

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
ROOT = os.path.abspath(ARGS[0] if ARGS else os.path.join(os.path.dirname(bpy.data.filepath), "..", ".."))
LAMPS = os.path.join(ROOT, "media", "configuration", "lamps.object")

SHAPES_VERSION = 7
LEANING = 0.95              # floors leaning more than this (|normal.z| below it) get courses from height (v5)
SLOPED = 0.12               # a wall piece whose mean |normal.z| is above this is "sloped" (version 4)
SLOPED_V6 = 0.02            # version 6: any piece that is not truly vertical (a ledge front leaning a few degrees)
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
# Night lights: wall torches (the "torches" object, version TORCHES_VERSION), chosen 2026-10-09 (17, every 20 degrees)
TORCHES_VERSION = 1
TORCH_STEP = 20.0           # degrees between torches
TORCH_FIRST = 30.0          # the first torch; the gate is at about 10 degrees, so 30 and 350 frame it
TORCH_HEIGHT = 3.4          # on the tall wall (below the ledge under the windows)
TORCH_PODIUM_HEIGHT = 1.7   # on the front of the seating podium (a lower wall)
TORCH_PODIUM = (38.0, 148.0)    # azimuths of the seating podium
TORCH_RAY_START = 11.0      # rays start outside the ring
TORCH_IRON = "torchIronMaterial"
TORCH_FLAME = "torchFlameMaterial"
# flame shape (height, radius), a toon teardrop
TORCH_FLAME_PROFILE = [(0.0, 0.07), (0.08, 0.14), (0.18, 0.16), (0.3, 0.13), (0.44, 0.07), (0.6, 0.0)]


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


def sloped_uvs(ob, sloped=SLOPED):
    """Sloped wall pieces: u around the arena, v up the slope (step 5 of the docstring). Uses the seams of step 4."""
    me = ob.data
    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv[me.uv_layers[0].name]
    flat = {f.index for f in bm.faces if abs(f.normal.z) > 0.7}
    walls = [f for f in bm.faces if f.index not in flat]
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

    changed = 0
    for faces in islands.values():
        area = sum(f.calc_area() for f in faces)
        if area <= 0:
            continue
        nz = sum(abs(f.normal.z) * f.calc_area() for f in faces) / area
        if nz <= sloped:
            continue
        # only pieces that lean towards or away from the centre (a cone around the arena); a piece that slopes sideways
        # (the end of the stepped block) keeps its unwrap
        radial = 0.0
        for f in faces:
            h = Vector((f.normal.x, f.normal.y, 0.0))
            c = f.calc_center_median()
            r = Vector((c.x, c.y, 0.0))
            if h.length > 1e-6 and r.length > 1e-6:
                radial += abs(h.normalized().dot(r.normalized())) * f.calc_area()
        if radial / area < 0.8:
            continue
        # the piece's mean direction and radius around the arena's centre
        cx = sum(f.calc_center_median().x * f.calc_area() for f in faces) / area
        cy = sum(f.calc_center_median().y * f.calc_area() for f in faces) / area
        mid = math.atan2(cy, cx)
        radius = sum(f.calc_center_median().xy.length * f.calc_area() for f in faces) / area
        along_slope = 1.0 / max(math.sqrt(max(1.0 - nz * nz, 0.0)), 0.2)    # metres up the slope per metre of height
        for f in faces:
            for l in f.loops:
                p = l.vert.co
                a = math.atan2(p.y, p.x) - mid
                a = (a + math.pi) % (2 * math.pi) - math.pi                    # around the piece's own direction
                l[uv].uv = (a * radius / STONE_TILE, p.z * along_slope / STONE_TILE)
        changed += 1
    # any wall face the unwrap squashed or stretched (a strip at the +x cut) is mapped around the arena instead
    def uv_area(f):
        t = [l[uv].uv for l in f.loops]
        return abs(sum(t[i].x * t[(i + 1) % len(t)].y - t[(i + 1) % len(t)].x * t[i].y for i in range(len(t)))) / 2
    squashed = 0
    for f in walls:
        world = f.calc_area() / (STONE_TILE * STONE_TILE)
        if world <= 1e-8:
            continue
        ratio = uv_area(f) / world
        if 0.5 < ratio < 2.0:
            continue
        c = f.calc_center_median()
        mid = math.atan2(c.y, c.x)
        radius = c.xy.length
        along = 1.0 / max(math.sqrt(max(1.0 - f.normal.z * f.normal.z, 0.0)), 0.2)
        for l in f.loops:
            p = l.vert.co
            a = (math.atan2(p.y, p.x) - mid + math.pi) % (2 * math.pi) - math.pi
            l[uv].uv = (a * radius / STONE_TILE, p.z * along / STONE_TILE)
        squashed += 1
    log("%d squashed wall faces mapped around the arena" % squashed)

    # floors and treads follow the curve too: u around the arena (cut at +x, behind the stepped block), v outwards
    for f in bm.faces:
        if f.index in flat:
            c = f.calc_center_median()
            mid = math.atan2(-c.y, -c.x)
            for l in f.loops:
                p = l.vert.co
                r = p.xy.length
                # each face measures its angles from its own centre: a face lying across the cut keeps one side
                a = mid + ((math.atan2(-p.y, -p.x) - mid + math.pi) % (2 * math.pi) - math.pi)
                l[uv].uv = (a * r / STONE_TILE, r / STONE_TILE)
    bm.to_mesh(me)
    bm.free()
    log("sloped walls: %d pieces mapped around the arena; floors mapped around the arena" % changed)


def leaning_floor_uvs(ob):
    """Leaning "floors" (step 6 of the docstring): v from height, so their courses stay level across faces."""
    me = ob.data
    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv[me.uv_layers[0].name]
    changed = 0
    for f in bm.faces:
        nz = abs(f.normal.z)
        if not (0.7 < nz < LEANING):
            continue
        along = 1.0 / math.sqrt(1.0 - nz * nz)          # metres up the slope per metre of height
        c = f.calc_center_median()
        mid = math.atan2(-c.y, -c.x)
        for l in f.loops:
            p = l.vert.co
            r = p.xy.length
            a = mid + ((math.atan2(-p.y, -p.x) - mid + math.pi) % (2 * math.pi) - math.pi)
            l[uv].uv = (a * r / STONE_TILE, p.z * along / STONE_TILE)
        changed += 1
    bm.to_mesh(me)
    bm.free()
    log("leaning floors: %d faces with courses from height" % changed)


def final_uvs(ob):
    """Version 7: every coliseum face mapped again from its own kind, replacing steps 4 to 7 (see the docstring)."""
    me = ob.data
    bm = bmesh.new()
    bm.from_mesh(me)
    uv = bm.loops.layers.uv[me.uv_layers[0].name]

    def kind(f):
        n = f.normal
        if abs(n.z) > 0.7:
            return "floor"
        h = Vector((n.x, n.y, 0.0))
        c = f.calc_center_median()
        r = Vector((c.x, c.y, 0.0))
        if h.length < 1e-6 or r.length < 1e-6:
            return "side"
        return "radial" if abs(h.normalized().dot(r.normalized())) > 0.7 else "side"

    kinds = {f.index: kind(f) for f in bm.faces}
    parent = {f.index: f.index for f in bm.faces}

    def find(i):
        while parent[i] != i:
            parent[i] = parent[parent[i]]
            i = parent[i]
        return i
    for e in bm.edges:
        if len(e.link_faces) != 2 or not e.smooth:
            continue
        a, b = e.link_faces
        if kinds[a.index] == kinds[b.index] and a.normal.angle(b.normal, 0.0) < math.radians(30):
            parent[find(a.index)] = find(b.index)
    islands = {}
    for f in bm.faces:
        islands.setdefault(find(f.index), []).append(f)

    def unwrap(angle, mid):
        return (angle - mid + math.pi) % (2 * math.pi) - math.pi

    def around(p, f, mid):
        """Angle of p around the arena, measured from the piece's middle `mid`, unwrapped through the face's own
        centre, so a face never spans the cut opposite `mid` (the cut falls between faces)."""
        c = f.calc_center_median()
        fa = unwrap(math.atan2(c.y, c.x), mid)
        return fa + unwrap(math.atan2(p.y, p.x), math.atan2(c.y, c.x))

    counts = {"floor": 0, "radial": 0, "side": 0}
    for faces in islands.values():
        k = kinds[faces[0].index]
        counts[k] += 1
        area = sum(f.calc_area() for f in faces) or 1.0
        cx = sum(f.calc_center_median().x * f.calc_area() for f in faces) / area
        cy = sum(f.calc_center_median().y * f.calc_area() for f in faces) / area
        mid = math.atan2(cy, cx)
        radius = sum(f.calc_center_median().xy.length * f.calc_area() for f in faces) / area
        nz = sum(abs(f.normal.z) * f.calc_area() for f in faces) / area
        if k == "side":
            n = sum((f.normal * f.calc_area() for f in faces), Vector())
            h = Vector((-n.y, n.x, 0.0))
            h = h.normalized() if h.length > 1e-6 else Vector((1.0, 0.0, 0.0))
            along = 1.0 / max(math.sqrt(max(1.0 - nz * nz, 0.0)), 0.2)
            for f in faces:
                for l in f.loops:
                    p = l.vert.co
                    l[uv].uv = (p.dot(h) / STONE_TILE, p.z * along / STONE_TILE)
        elif k == "radial":
            along = 1.0 / max(math.sqrt(max(1.0 - nz * nz, 0.0)), 0.2)
            for f in faces:
                for l in f.loops:
                    p = l.vert.co
                    a = around(p, f, mid)
                    l[uv].uv = ((mid + a) * radius / STONE_TILE, p.z * along / STONE_TILE)
        else:
            # "leaning" only when the piece slopes towards or away from the centre; one that slopes sideways (an
            # underside along a jamb) keeps v outwards, or u and v would both run along its slope
            n = sum((f.normal * f.calc_area() for f in faces), Vector())
            hn = Vector((n.x, n.y, 0.0))
            rc = Vector((cx, cy, 0.0))
            radial = hn.length > 1e-6 and rc.length > 1e-6 and abs(hn.normalized().dot(rc.normalized())) > 0.7
            leaning = nz < LEANING and radial
            along = 1.0 / max(math.sqrt(max(1.0 - nz * nz, 0.0)), 0.05)
            for f in faces:
                for l in f.loops:
                    p = l.vert.co
                    a = around(p, f, mid)
                    # v: up the slope for a leaning floor (courses parallel to its edges on the oval arena),
                    # outwards for a flat one
                    v = p.z * along if leaning else p.xy.length
                    l[uv].uv = ((mid + a) * radius / STONE_TILE, v / STONE_TILE)
    bm.to_mesh(me)
    bm.free()
    log("final UVs: %(floor)d floor, %(radial)d centre-facing and %(side)d side-facing pieces" % counts)


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


def wall_hit(scene, coliseum, azimuth, height):
    """Where a horizontal ray from the arena's centre, at `height`, first meets the coliseum: (point, normal) or None.
    azimuth in game degrees (0 = game +Z = Blender -Y, 90 = +X), as lighting.object's sunAzimuth."""
    a = math.radians(azimuth)
    d = Vector((math.sin(a), -math.cos(a), 0.0))
    depsgraph = bpy.context.evaluated_depsgraph_get()
    hit, loc, nrm, _, ob, _ = scene.ray_cast(depsgraph, Vector((0.0, 0.0, height)) + d * TORCH_RAY_START, d,
                                             distance=40.0)
    return (loc, nrm) if hit and ob == coliseum else None


def add_lathe(bm, base, profile, segments, material, u, smooth):
    """A closed surface of revolution around +Z at `base`: profile = [(height, radius)], from the bottom to the tip
    (a radius of 0 closes it). UV u = `u` (the lamp's phase), v = height / the profile's height (the flame shader)."""
    top = profile[-1][0]
    rings = []
    for h, r in profile:
        if r <= 0.0:
            rings.append([bm.verts.new(base + Vector((0.0, 0.0, h)))])
        else:
            rings.append([bm.verts.new(base + Vector((math.cos(2 * math.pi * i / segments) * r,
                                                      math.sin(2 * math.pi * i / segments) * r, h)))
                          for i in range(segments)])
    faces = []
    for lo, hi in zip(rings, rings[1:]):
        for i in range(segments):
            j = (i + 1) % segments
            if len(hi) == 1:
                faces.append(bm.faces.new((lo[i], lo[j], hi[0])))
            elif len(lo) == 1:
                faces.append(bm.faces.new((lo[0], hi[j], hi[i])))
            else:
                faces.append(bm.faces.new((lo[i], lo[j], hi[j], hi[i])))
    if len(rings[0]) > 1:
        faces.append(bm.faces.new(list(reversed(rings[0]))))
    uv = bm.loops.layers.uv.verify()
    for f in faces:
        f.material_index = material
        f.smooth = smooth
        for loop in f.loops:
            loop[uv].uv = (u, (loop.vert.co.z - base.z) / top)
    return faces


def add_box(bm, centre, size, rotation, material):
    m = Matrix.Translation(centre) @ rotation @ Matrix.Diagonal((size[0], size[1], size[2], 1.0))
    ret = bmesh.ops.create_cube(bm, size=1.0, matrix=m)
    faces = {f for v in ret["verts"] for f in v.link_faces}
    for f in faces:
        f.material_index = material
        f.smooth = False
    return faces


def add_rod(bm, start, end, radius, material, segments=6):
    axis = end - start
    rotation = axis.to_track_quat('Z', 'Y').to_matrix().to_4x4()
    m = Matrix.Translation((start + end) / 2) @ rotation
    ret = bmesh.ops.create_cone(bm, cap_ends=True, segments=segments, radius1=radius, radius2=radius,
                                depth=axis.length, matrix=m)
    faces = {f for v in ret["verts"] for f in v.link_faces}
    for f in faces:
        f.material_index = material
        f.smooth = False
    return faces


def add_torches(scene, coliseum):
    """The night lights (docs/IMPROVEMENT_IDEAS.md idea 4): wall torches in iron brackets, one every TORCH_STEP
    degrees around the arena, symmetric about the gate (TORCH_FIRST and its mirror), at TORCH_HEIGHT on the tall wall
    and TORCH_PODIUM_HEIGHT on the front of the seating podium. One object, "torches", with two materials: the iron
    (toon, like the stone) and the flames (unlit, media/tumbu/arena/torches.material). Returns the light positions in
    Blender coordinates, at the flames."""
    me = bpy.data.meshes.new("torches")
    ob = bpy.data.objects.new("torches", me)
    export = bpy.data.collections.get("Export")
    (export or scene.collection).objects.link(ob)
    me.materials.append(bpy.data.materials.get(TORCH_IRON) or bpy.data.materials.new(TORCH_IRON))
    me.materials.append(bpy.data.materials.get(TORCH_FLAME) or bpy.data.materials.new(TORCH_FLAME))
    ob["tumbu_ao_size"] = 512

    bm = bmesh.new()
    bm.loops.layers.uv.verify()
    lights = []
    azimuths = [TORCH_FIRST + TORCH_STEP * k for k in range(int(round(360.0 / TORCH_STEP)) - 1)]
    for index, azimuth in enumerate(azimuths):
        podium = TORCH_PODIUM[0] <= azimuth <= TORCH_PODIUM[1]
        hit = wall_hit(scene, coliseum, azimuth, TORCH_PODIUM_HEIGHT if podium else TORCH_HEIGHT)
        if hit is None:
            log("torch at %.0f degrees: no wall, skipped" % azimuth)
            continue
        loc, nrm = hit
        out = Vector((nrm.x, nrm.y, 0.0)).normalized()     # away from the wall, towards the arena
        side = Vector((-out.y, out.x, 0.0))
        facing = Matrix((side, out, Vector((0.0, 0.0, 1.0)))).transposed().to_4x4()   # local x along the wall
        # back plate with two rivet bars, the arm, a ring under the cup, the cup
        add_box(bm, loc + out * 0.03, (0.24, 0.06, 0.36), facing, 0)
        add_box(bm, loc + out * 0.065 + Vector((0, 0, 0.12)), (0.28, 0.03, 0.04), facing, 0)
        add_box(bm, loc + out * 0.065 - Vector((0, 0, 0.12)), (0.28, 0.03, 0.04), facing, 0)
        cup = loc + out * 0.42 + Vector((0.0, 0.0, 0.3))
        add_rod(bm, loc + out * 0.05 - Vector((0, 0, 0.05)), cup - Vector((0, 0, 0.17)), 0.035, 0)
        add_lathe(bm, cup - Vector((0, 0, 0.22)), [(0.0, 0.03), (0.05, 0.05), (0.07, 0.06), (0.22, 0.17), (0.25, 0.15),
                                                   (0.25, 0.0)], 8, 0, 0.0, False)
        phase = (index * 0.618034) % 1.0       # golden-ratio steps: neighbouring flames flicker out of step
        add_lathe(bm, cup + Vector((0, 0, 0.0)), TORCH_FLAME_PROFILE, 8, 1, phase, True)
        lights.append(cup + Vector((0.0, 0.0, 0.3)) + out * 0.15)
    bm.to_mesh(me)
    bm.free()
    ob["tumbu_shapes"] = TORCHES_VERSION
    log("torches: %d of %d placed" % (len(lights), len(azimuths)))
    return lights


def neon_segments(ring):
    """The neon as light segments (Blender coordinates): one per ring side at the ropes' mid height, and the T's tube
    as the centre lines of its bar and stem (from ring_emblem.png, mapped from above over the 20 x 20 floor)."""
    import numpy as np
    me = ring.data
    corners = []
    for v in me.vertices:
        if v.co.z > 1.45 and all((v.co.xy - k).length > 1.0 for k in corners):
            corners.append(v.co.xy.copy())
    corners.sort(key=lambda c: math.atan2(c.y, c.x))
    rope_slot = me.materials.find(RING_ROPES)
    zs = [me.vertices[i].co.z for f in me.polygons if f.material_index == rope_slot for i in f.vertices]
    z = (min(zs) + max(zs)) / 2
    segments = []
    for a, b in zip(corners, corners[1:] + corners[:1]):
        # stop short of the posts (the ropes end there)
        d = (b - a).normalized() * 0.3
        segments.append((Vector((a.x + d.x, a.y + d.y, z)), Vector((b.x - d.x, b.y - d.y, z))))
    image = bpy.data.images.load(os.path.join(ROOT, "media", "tumbu", "arena", "ring_emblem.png"), check_existing=True)
    w, h = image.size
    px = np.array(image.pixels[:], dtype=np.float32).reshape(h, w, 4)
    ys, xs = np.nonzero(px[:, :, 1] > 0.5)
    # Blender image rows go up; game uv = world xz / 20 + 0.5 with v down, which is Blender (x, y) / 20 + 0.5
    wx = (xs + 0.5) / w * 20.0 - 10.0
    wy = (ys + 0.5) / h * 20.0 - 10.0
    rows = {}
    for x, y in zip(wx, wy):
        rows.setdefault(round(y, 1), []).append(x)
    widest = max(max(v) - min(v) for v in rows.values())
    bar = [y for y, v in rows.items() if max(v) - min(v) > widest * 0.6]
    stem = [y for y, v in rows.items() if max(v) - min(v) <= widest * 0.6]
    bar_x = [x for y in bar for x in rows[y]]
    stem_x = [x for y in stem for x in rows[y]]
    tz = 0.25
    bar_y = (min(bar) + max(bar)) / 2
    stem_mid = (min(stem_x) + max(stem_x)) / 2
    far = min(stem) if abs(min(stem) - bar_y) > abs(max(stem) - bar_y) else max(stem)
    segments.append((Vector((min(bar_x), bar_y, tz)), Vector((max(bar_x), bar_y, tz))))
    segments.append((Vector((stem_mid, bar_y, tz)), Vector((stem_mid, far, tz))))
    return segments


def write_lamps(lights, segments):
    """media/configuration/lamps.object: the torch lights and the neon segments in game coordinates (Y up), read by
    Lighting (Lighting::loadLamps)."""
    def game(p):
        return "%.3f %.3f %.3f" % (p.x, p.z, -p.y)
    lines = ["// Generated by scripts/blender/arena_shapes.py from art/arena/Arena.blend: do not edit by hand.",
             "// The arena's night lights in game coordinates (Y up): each lamp is the light of a wall torch (at its",
             "// flame); each neon segment runs from one point to another (the ring's ropes, one per side, and the T's",
             "// tube). Their colour, reach and strength are in lighting.object.",
             "lamps arena{"]
    lines += ["\tlamp %s" % game(p) for p in lights]
    lines += ["\tneon %s %s" % (game(a), game(b)) for a, b in segments]
    lines.append("}")
    path = LAMPS
    with open(path, "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    log("wrote %s: %d lamps, %d neon segments" % (path, len(lights), len(segments)))


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
    if done < 4:
        sloped_uvs(ob)
    if done < 5:
        leaning_floor_uvs(ob)
    if done < 6:
        sloped_uvs(ob, SLOPED_V6)
        leaning_floor_uvs(ob)
    if done < 7:
        final_uvs(ob)
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

    torches = bpy.data.objects.get("torches")
    if torches is None or torches.get("tumbu_shapes", 0) < TORCHES_VERSION or not os.path.isfile(LAMPS):
        if torches is not None:
            bpy.data.meshes.remove(torches.data)
        lights = add_torches(bpy.context.scene, ob)
        write_lamps(lights, neon_segments(ring))
    bpy.ops.wm.save_mainfile()
    log("saved " + bpy.data.filepath)


main()
