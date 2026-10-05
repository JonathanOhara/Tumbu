"""
Compares a new robot export with the files the game uses now (scripts/export-robots.ps1 runs it before installing):
for every part, the skinned position of every vertex at every frame of every animation, as Ogre computes it
(binding pose, keyframes, at most 4 bone weights per vertex, renormalised, as Ogre does when it loads a mesh).
    blender --background --python robot_compare.py -- <old xml dir> <new xml dir> <part> [<part> ...]
Prints the largest vertex movement per part and per animation, in world units.
"""
import os
import sys
import xml.etree.ElementTree as ET

from mathutils import Matrix, Vector

sys.dont_write_bytecode = True
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import robot_check  # noqa: E402

FPS = 30


def load_mesh(path, bone_names):
    root = ET.parse(path).getroot()
    sub = root.find("submeshes").find("submesh")
    geometry = root.find("sharedgeometry") if root.find("sharedgeometry") is not None else sub.find("geometry")
    vertices = [robot_check._vec(v.find("position")) for vb in geometry.findall("vertexbuffer")
                for v in vb.findall("vertex") if v.find("position") is not None]
    weights = [dict() for _ in vertices]
    # With shared geometry they sit at the root, and the converter also writes an empty list in the submesh.
    assignments = [a for parent in (sub.find("boneassignments"), root.find("boneassignments")) if parent is not None
                   for a in parent]
    for a in assignments:
        weights[int(a.get("vertexindex"))][bone_names[int(a.get("boneindex"))]] = float(a.get("weight"))
    # Ogre keeps the 4 largest weights of a vertex and renormalises them (Mesh::_compileBoneAssignments).
    for i, w in enumerate(weights):
        top = sorted(w.items(), key=lambda kv: -kv[1])[:4]
        total = sum(v for _, v in top) or 1.0
        weights[i] = {k: v / total for k, v in top}
    return vertices, weights, sub.get("material")


def matrices(pose):
    return {name: Matrix.Translation(pos) @ rot.to_matrix().to_4x4() for name, (pos, rot) in pose.items()}


def skinned(vertices, weights, bind_inverse, posed):
    out = []
    for v, w in zip(vertices, weights):
        p = Vector()
        for bone, weight in w.items():
            p += weight * (posed[bone] @ bind_inverse[bone] @ v)
        out.append(p)
    return out


def compare(old_dir, new_dir, part):
    old_names, old_bones, old_parents, old_anims = robot_check.load_skeleton(os.path.join(old_dir, part + ".skeleton.xml"))
    new_names, new_bones, new_parents, new_anims = robot_check.load_skeleton(os.path.join(new_dir, part + ".skeleton.xml"))
    old_v, old_w, old_mat = load_mesh(os.path.join(old_dir, part + ".mesh.xml"), old_names)
    new_v, new_w, new_mat = load_mesh(os.path.join(new_dir, part + ".mesh.xml"), new_names)
    print("[COMPARE] %s: vertices %d -> %d, material %s -> %s" % (part, len(old_v), len(new_v), old_mat, new_mat))

    # Pair the vertices by position (both files split them at UV seams; the order may differ).
    pairs = []
    for i, v in enumerate(new_v):
        j = min(range(len(old_v)), key=lambda k: (old_v[k] - v).length)
        pairs.append((j, i, (old_v[j] - v).length))
    print("[COMPARE] %s: rest positions differ by up to %.6f" % (part, max(d for _, _, d in pairs)))

    old_bind = {k: m.inverted() for k, m in matrices(robot_check.ogre_pose(old_bones, old_parents, None, 0.0)).items()}
    new_bind = {k: m.inverted() for k, m in matrices(robot_check.ogre_pose(new_bones, new_parents, None, 0.0)).items()}
    worst_all = 0.0
    for name in sorted(set(old_anims) | set(new_anims)):
        if name not in old_anims or name not in new_anims:
            print("[COMPARE] %s %-17s only in the %s files" % (part, name, "old" if name in old_anims else "new"))
            continue
        length = max(old_anims[name][0], new_anims[name][0])
        worst, at = 0.0, 0
        for frame in range(int(round(length * FPS)) + 1):
            t = frame / FPS
            old_pos = skinned(old_v, old_w, old_bind, matrices(robot_check.ogre_pose(old_bones, old_parents, old_anims[name][1], t)))
            new_pos = skinned(new_v, new_w, new_bind, matrices(robot_check.ogre_pose(new_bones, new_parents, new_anims[name][1], t)))
            d = max((old_pos[j] - new_pos[i]).length for j, i, _ in pairs)
            if d > worst:
                worst, at = d, frame
        worst_all = max(worst_all, worst)
        print("[COMPARE] %s %-17s length %.3f -> %.3f s, vertices move up to %.5f (frame %d)" % (
            part, name, old_anims[name][0], new_anims[name][0], worst, at))
    print("[COMPARE] %s: largest movement %.5f" % (part, worst_all))


args = sys.argv[sys.argv.index("--") + 1:]
for part in args[2:]:
    compare(args[0], args[1], part)
