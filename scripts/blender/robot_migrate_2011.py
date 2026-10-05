"""
One-time migration (2026-10-05): copies into art/robots/robot00N.blend what the game's 2011 robot files have and the
converted Blender files lost, so the export (scripts/export-robots.ps1) reproduces the game exactly. Run once per
robot, BEFORE the new exports replace media/tumbu/robot00N/:
    blender --background art/robots/robot00N.blend --python robot_migrate_2011.py -- <dir with the 2011 *.mesh.xml>

  - Normals: the 2011 exporter computed its own vertex normals (separately for each copy of a vertex split at a UV
    seam). They differ from the normals Blender computes by about 4 degrees typically and up to 90, and give the robots
    the soft shading the game has. Each face corner of each part gets the normal of the 2011 vertex with the same
    position and UV, as a custom normal (Blender then shows the game's shading).
  - right_punch on robot003 and robot005: the game plays 20 frames (the punch fires at half the animation, ends at the
    end); the action has 30. The action gets a manual frame range 1-21, so Blender plays and exports what the game
    plays.
Saves the .blend. Running it again is harmless (it sets the same values).
"""
import math
import os
import re
import sys
import xml.etree.ElementTree as ET

import bpy
from mathutils import Vector

PARTS = ("head", "body", "leftArm", "rightArm", "legs")
PUNCH_20_FRAMES = ("robot003", "robot005")
POSITION_TOLERANCE = 1e-4
UV_TOLERANCE = 1e-3


def log(message):
    print("[MIGRATE] " + message, flush=True)


def load_2011_vertices(path):
    """(position, normal, uv) of every vertex of a 2011 .mesh.xml (its buffers split the attributes)."""
    root = ET.parse(path).getroot()
    sub = root.find("submeshes").find("submesh")
    geometry = root.find("sharedgeometry") if root.find("sharedgeometry") is not None else sub.find("geometry")
    buffers = [vb.findall("vertex") for vb in geometry.findall("vertexbuffer")]
    vertices = []
    for i in range(len(buffers[0])):
        e = {}
        for b in buffers:
            for child in b[i]:
                e[child.tag] = child
        p, n, t = e["position"], e["normal"], e["texcoord"]
        vertices.append((Vector([float(p.get(k)) for k in "xyz"]), Vector([float(n.get(k)) for k in "xyz"]),
                         (float(t.get("u")), float(t.get("v")))))
    return vertices


def migrate_normals(ob, old):
    me = ob.data
    uv = me.uv_layers.active.data
    normals = []
    by_position = 0
    for loop in me.loops:
        co = me.vertices[loop.vertex_index].co
        u, v = uv[loop.index].uv
        candidates = [o for o in old if (o[0] - co).length < POSITION_TOLERANCE]
        if not candidates:
            raise SystemExit("%s: vertex %d has no 2011 vertex at its position" % (ob.name, loop.vertex_index))
        # Ogre's V runs down the texture, Blender's up.
        exact = [o for o in candidates if abs(o[2][0] - u) < UV_TOLERANCE and abs(o[2][1] - (1.0 - v)) < UV_TOLERANCE]
        if not exact:
            by_position += 1
            # Same position, a UV the 2011 file does not have (a UV tile offset): take the closest normal to Blender's.
            exact = sorted(candidates, key=lambda o: o[1].angle(loop.normal, math.pi))
        normals.append(exact[0][1].normalized())
    me.normals_split_custom_set(normals)
    return by_position


def main():
    old_dir = sys.argv[sys.argv.index("--") + 1]
    name = os.path.splitext(os.path.basename(bpy.data.filepath))[0]     # robot00N
    number = name[-3:]
    for ob in bpy.data.objects:
        m = re.match(r"(%s)_%s$" % ("|".join(PARTS), number), ob.name)
        if not m or ob.type != 'MESH':
            continue
        old = load_2011_vertices(os.path.join(old_dir, ob.name + ".mesh.xml"))
        by_position = migrate_normals(ob, old)
        log("%s %s: %d corners got their 2011 normal%s" % (name, ob.name, len(ob.data.loops),
            (" (%d matched by position only)" % by_position) if by_position else ""))
    if name in PUNCH_20_FRAMES:
        action = bpy.data.actions["right_punch"]
        action.use_frame_range = True
        action.frame_start = 1
        action.frame_end = 21
        log("%s: right_punch plays frames 1-21 (20 frames, as in the game)" % name)
    bpy.ops.wm.save_mainfile()
    log("%s: saved" % name)


main()
