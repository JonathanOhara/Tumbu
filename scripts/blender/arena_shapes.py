"""
Arena remake, big shapes (run on art/arena/Arena.blend in Blender 5.2; see Usage).

Models what a texture cannot fake, so the stone reads as cut blocks under the toon lighting:
  1. a plinth course at the foot of the inner arena wall (a base band that stands out from the wall), and
  2. a chamfer on every hard edge of the coliseum (ledge lips, wall tops, window reveals, block corners): the sun
     draws a light or dark band along each edge.
  3. (version 2) weighted normals: on the smooth curved walls, a vertex where a chamfer ends averaged the wall's normal
     with the ledge's, and the toon ramp drew a bright wedge down the wall; with the normals weighted by face area
     (hard edges kept) the big faces decide. They are custom normals: the export keeps them (arena_ao.py).
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

SHAPES_VERSION = 2
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


def main():
    ob = bpy.data.objects.get("coliseum")
    if ob is None:
        raise SystemExit("Arena.blend has no 'coliseum' object")
    done = ob.get("tumbu_shapes", 0)
    if done >= SHAPES_VERSION:
        log("coliseum already has shapes version %d: nothing to do" % done)
        return
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
    ob["tumbu_shapes"] = SHAPES_VERSION
    bpy.ops.wm.save_mainfile()
    log("coliseum: shapes version %d -> %d, %d -> %d vertices, saved %s"
        % (done, SHAPES_VERSION, before, len(ob.data.vertices), bpy.data.filepath))


main()
