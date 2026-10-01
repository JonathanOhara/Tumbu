"""
Converts a Blender 2.49 file into a modern working file (run by scripts/convert-legacy-blend.ps1 in
Blender 4.5 LTS, the last version that converts pre-2.50 animation data).

Usage: blender --background <source.blend> --python convert_legacy.py -- <target.blend>
The source is never written; the target is saved in the current format (Blender 5.x opens it).
"""
import bpy
import os
import sys

target = sys.argv[sys.argv.index("--") + 1]
os.makedirs(os.path.dirname(target), exist_ok=True)

meshes = [o for o in bpy.data.objects if o.type == 'MESH']
armatures = [o for o in bpy.data.objects if o.type == 'ARMATURE']
print("[CONVERT] %s: %d meshes, %d armatures, %d actions" %
      (bpy.data.filepath, len(meshes), len(armatures), len(bpy.data.actions)), flush=True)
for a in bpy.data.actions:
    print("[CONVERT]   action %s: frames %d-%d" % (a.name, a.frame_range[0], a.frame_range[1]), flush=True)

bpy.ops.file.make_paths_absolute()    # textures keep pointing at media/ after the move
bpy.ops.wm.save_as_mainfile(filepath=target, copy=False)
print("[CONVERT] saved " + target, flush=True)
