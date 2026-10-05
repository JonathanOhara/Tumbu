"""
Checks that an exported robot part plays in Ogre exactly as it does in Blender (used by robot_export.py).

  - Mesh: every exported vertex (Ogre splits vertices at UV seams and normal changes) carries the same bones and
    weights as the Blender vertex at that position, and the normal of a Blender face corner with that position and UV.
  - Animation: for every action and every frame, each bone's position and orientation as Ogre computes them from the
    exported .skeleton (binding pose, then the keyframes: translate in the parent's space, rotate in the bone's own
    space, as Ogre's NodeAnimationTrack applies them) match Blender's pose bones at that frame.
Both run on the XML files blender2ogre keeps next to the binaries.
"""
import math
import xml.etree.ElementTree as ET

import bpy
from mathutils import Quaternion, Vector

POSITION_TOLERANCE = 1e-3     # world units (the robots are about 2 units tall)
ANGLE_TOLERANCE = 0.01        # radians (about 0.6 degrees)
WEIGHT_TOLERANCE = 1e-3


def _vec(e):
    return Vector((float(e.get("x")), float(e.get("y")), float(e.get("z"))))


def _quat(e):
    axis = _vec(e.find("axis"))
    if axis.length < 1e-9:
        return Quaternion()
    return Quaternion(axis.normalized(), float(e.get("angle")))


def load_skeleton(path):
    root = ET.parse(path).getroot()
    bones = {}
    for b in root.find("bones"):
        bones[b.get("name")] = (_vec(b.find("position")), _quat(b.find("rotation")))
    names = [b.get("name") for b in root.find("bones")]
    parents = {p.get("bone"): p.get("parent") for p in root.find("bonehierarchy")}
    animations = {}
    for a in root.find("animations") if root.find("animations") is not None else []:
        tracks = {}
        for t in a.find("tracks"):
            keys = []
            for k in t.find("keyframes"):
                tr = k.find("translate")
                ro = k.find("rotate")
                keys.append((float(k.get("time")), _vec(tr) if tr is not None else Vector(),
                             _quat(ro) if ro is not None else Quaternion()))
            tracks[t.get("bone")] = keys
        animations[a.get("name")] = (float(a.get("length")), tracks)
    return names, bones, parents, animations


def _sample(keys, time):
    """Ogre's linear keyframe interpolation (shortest-path slerp for rotations)."""
    if time <= keys[0][0]:
        return keys[0][1], keys[0][2]
    for (t0, p0, q0), (t1, p1, q1) in zip(keys, keys[1:]):
        if time <= t1:
            f = (time - t0) / (t1 - t0) if t1 > t0 else 0.0
            return p0.lerp(p1, f), q0.slerp(q1, f)
    return keys[-1][1], keys[-1][2]


def ogre_pose(bones, parents, tracks, time):
    """Model-space position and orientation of every bone at a time of one animation."""
    world = {}

    def solve(name):
        if name in world:
            return world[name]
        pos, rot = bones[name]
        if tracks is not None and name in tracks:
            t, r = _sample(tracks[name], time)
            pos = pos + t             # Node::translate, parent space
            rot = rot @ r             # Node::rotate, local space
        parent = parents.get(name)
        if parent:
            ppos, prot = solve(parent)
            world[name] = (ppos + prot @ pos, prot @ rot)
        else:
            world[name] = (pos, rot)
        return world[name]

    for name in bones:
        solve(name)
    return world


def _angle(a, b):
    d = abs(a.dot(b))
    return 2.0 * math.acos(min(1.0, d))


def check_animation(arm, skeleton_xml, fps, offset):
    """Compares Blender's pose bones with the exported skeleton at every frame of every action. offset turns Blender's
    armature space into the part's space (the exported skeleton's model space)."""
    names, bones, parents, animations = load_skeleton(skeleton_xml)
    blender_bones = {b.name for b in arm.data.bones}
    problems = []
    if set(names) != blender_bones:
        problems.append("bones differ: %s" % sorted(set(names) ^ blender_bones))
        return problems, 0.0, 0.0

    # Binding pose: the exported rest pose against Blender's rest pose.
    rest = ogre_pose(bones, parents, None, 0.0)
    worst_pos = worst_rot = 0.0
    for b in arm.data.bones:
        pos, rot = rest[b.name]
        worst_pos = max(worst_pos, (pos - (b.head_local + offset)).length)
        worst_rot = max(worst_rot, _angle(rot, b.matrix_local.to_quaternion()))

    ad = arm.animation_data
    saved_action, saved_nla = ad.action, ad.use_nla
    ad.use_nla = False
    scene = bpy.context.scene
    for action in bpy.data.actions:
        if action.name not in animations:
            # blender2ogre leaves out an action that moves no bone (no_pose, the rest pose). Fine only if it really
            # moves nothing.
            ad.action = action
            for pb in arm.pose.bones:
                pb.location, pb.rotation_quaternion = (0.0, 0.0, 0.0), (1.0, 0.0, 0.0, 0.0)
                pb.rotation_euler, pb.scale = (0.0, 0.0, 0.0), (1.0, 1.0, 1.0)
            moves = False
            for frame in range(int(action.frame_range[0]), int(action.frame_range[1]) + 1):
                scene.frame_set(frame)
                for pb in arm.pose.bones:
                    pos, rot = rest[pb.name]
                    if ((pos - (pb.matrix.to_translation() + offset)).length > POSITION_TOLERANCE
                            or _angle(rot, pb.matrix.to_quaternion()) > ANGLE_TOLERANCE):
                        moves = True
            if moves:
                problems.append("animation %s was not exported" % action.name)
            continue
        length, tracks = animations[action.name]
        start, end = action.frame_range
        expected = (end - start) / fps
        if abs(length - expected) > 1e-4:
            problems.append("%s: length %.4f, Blender %.4f" % (action.name, length, expected))
        ad.action = action
        # Ogre starts every animation from the binding pose; Blender keeps the pose the previous action left on the
        # bones this one does not key. Clear it, as blender2ogre does before exporting.
        for pb in arm.pose.bones:
            pb.location = (0.0, 0.0, 0.0)
            pb.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
            pb.rotation_euler = (0.0, 0.0, 0.0)
            pb.scale = (1.0, 1.0, 1.0)
        for frame in range(int(start), int(end) + 1):
            scene.frame_set(frame)
            pose = ogre_pose(bones, parents, tracks, (frame - start) / fps)
            for pb in arm.pose.bones:
                pos, rot = pose[pb.name]
                matrix = pb.matrix                              # armature space, posed
                dp = (pos - (matrix.to_translation() + offset)).length
                dr = _angle(rot, matrix.to_quaternion())
                if dp > worst_pos:
                    worst_pos = dp
                if dr > worst_rot:
                    worst_rot = dr
                if dp > POSITION_TOLERANCE or dr > ANGLE_TOLERANCE:
                    problems.append("%s frame %d bone %s: %.4f units, %.4f rad" % (action.name, frame, pb.name, dp, dr))
    ad.action, ad.use_nla = saved_action, saved_nla
    return problems[:20], worst_pos, worst_rot


def check_weights(ob, mesh_xml, skeleton_xml):
    """Every exported vertex has the bones and weights of the Blender vertex at its position."""
    names, _, _, _ = load_skeleton(skeleton_xml)
    root = ET.parse(mesh_xml).getroot()
    sub = root.find("submeshes").find("submesh")
    geometry = root.find("sharedgeometry") if root.find("sharedgeometry") is not None else sub.find("geometry")
    buffers = [vb.findall("vertex") for vb in geometry.findall("vertexbuffer")]
    positions, normals, uvs = [], [], []
    for i in range(len(buffers[0])):
        e = {}
        for b in buffers:
            for child in b[i]:
                e[child.tag] = child
        positions.append(_vec(e["position"]))
        normals.append(_vec(e["normal"]))
        uvs.append((float(e["texcoord"].get("u")), float(e["texcoord"].get("v"))))
    exported = [dict() for _ in positions]
    # With shared geometry they sit at the root, and the converter also writes an empty list in the submesh.
    assignments = [a for parent in (sub.find("boneassignments"), root.find("boneassignments")) if parent is not None
                   for a in parent]
    for a in assignments:
        exported[int(a.get("vertexindex"))][names[int(a.get("boneindex"))]] = float(a.get("weight"))

    groups = {g.index: g.name for g in ob.vertex_groups}
    blender = [(v.co.copy(), {groups[g.group]: g.weight for g in v.groups if g.weight > 0}) for v in ob.data.vertices]
    worst = 0.0
    problems = []
    for i, p in enumerate(positions):
        co, weights = min(blender, key=lambda b: (b[0] - p).length)
        if (co - p).length > 1e-4:
            problems.append("vertex %d at %s has no Blender vertex" % (i, tuple(p)))
            continue
        if set(weights) != set(exported[i]):
            problems.append("vertex %d: bones %s, Blender %s" % (i, sorted(exported[i]), sorted(weights)))
            continue
        for bone, w in weights.items():
            worst = max(worst, abs(w - exported[i][bone]))
        if len(exported[i]) > 4:
            problems.append("vertex %d has %d weights" % (i, len(exported[i])))
    if worst > WEIGHT_TOLERANCE:
        problems.append("weights differ by up to %.5f" % worst)

    # Normals: the exported normal of each vertex is the normal of a Blender face corner at its position and UV
    # (Ogre's V runs down the texture, Blender's up).
    me = ob.data
    uv_layer = me.uv_layers.active.data
    corners = [(me.vertices[l.vertex_index].co.copy(), tuple(uv_layer[l.index].uv), me.corner_normals[l.index].vector.copy())
               for l in me.loops]
    worst_normal = 0.0
    for p, n, (u, v) in zip(positions, normals, uvs):
        match = [c for c in corners if (c[0] - p).length < 1e-4 and abs(c[1][0] - u) < 1e-4 and abs(c[1][1] - (1.0 - v)) < 1e-4]
        if not match:
            problems.append("vertex at %s, uv (%.4f, %.4f) has no Blender corner" % (tuple(round(c, 4) for c in p), u, v))
            continue
        worst_normal = max(worst_normal, min(math.degrees(n.angle(c[2], math.pi)) for c in match))
    if worst_normal > 0.5:
        problems.append("normals differ by up to %.2f degrees" % worst_normal)
    return problems[:20], worst, len(positions), worst_normal
