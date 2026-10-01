"""
Robot ambient occlusion (run by scripts/bake-robot-ao.ps1, in Blender 5.2).

Bakes Cycles AO for each part of art/robots/robot00N.blend (converted from the 2011 file by
scripts/convert-legacy-blend.ps1) into media/tumbu/robot00N/AO<part>UV_00N.png, through the part's own
texture UVs, so the game's existing .mesh files use it as they are (robots.material $aoMap).

- Each part occludes only itself: parts are swapped between robots and animated, so occlusion from the
  body on an arm (or from another robot's parts) would be wrong most of the time.
- Rest pose (the armature is set to its rest position while baking).
- The texture has the size of the part's diffuse texture.

Settings are scene custom properties of each robot file: tumbu_ao_distance (world units, default 0.25),
tumbu_ao_samples (default 512).

Usage: blender --background art/robots/robot00N.blend --python robot_ao.py -- <repo root>
"""
import bpy
import os
import re
import sys

ROOT = sys.argv[sys.argv.index("--") + 1]
PARTS = ("head", "body", "leftArm", "rightArm", "legs")


def log(message):
    print("[ROBOT-AO] " + message, flush=True)


def clean_ao(image, passes=1):
    """Median filter (3x3): removes bake noise and one-texel speckles."""
    import numpy as np
    w, h = image.size
    px = np.array(image.pixels[:], dtype=np.float32).reshape(h, w, 4)
    ao = px[:, :, 0]
    # The bake writes exactly 0 outside the UV islands (and their margin): make that white, or texture
    # filtering at a distance pulls black into the island edges.
    ao = np.where(ao == 0.0, 1.0, ao)
    for _ in range(passes):
        padded = np.pad(ao, 1, mode='edge')
        stack = np.stack([padded[y:y + h, x:x + w] for y in range(3) for x in range(3)])
        ao = np.median(stack, axis=0)
    px[:, :, 0] = px[:, :, 1] = px[:, :, 2] = ao
    image.pixels[:] = px.ravel()


def bake_target(ob, image):
    for slot in ob.material_slots:
        mat = slot.material
        if mat is None:
            continue
        mat.use_nodes = True
        nodes = mat.node_tree.nodes
        node = nodes.get("TumbuAO")
        if node is None:
            node = nodes.new("ShaderNodeTexImage")
            node.name = node.label = "TumbuAO"
        node.image = image
        nodes.active = node


def main():
    name = os.path.splitext(os.path.basename(bpy.data.filepath))[0]          # robot00N
    number = name[-3:]
    media = os.path.join(ROOT, "media", "tumbu", name)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = int(scene.get("tumbu_ao_samples", 512))
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.light_settings.distance = float(scene.get("tumbu_ao_distance", 0.25))
    log("%s: AO distance %.2f, %d samples" % (name, scene.world.light_settings.distance, scene.cycles.samples))

    for arm in (o for o in bpy.data.objects if o.type == 'ARMATURE'):
        arm.data.pose_position = 'REST'

    parts = {}
    for ob in bpy.data.objects:
        m = re.match(r"(%s)_%s$" % ("|".join(PARTS), number), ob.name)
        if ob.type == 'MESH' and m:
            parts[m.group(1)] = ob
    missing = [p for p in PARTS if p not in parts]
    if missing:
        raise SystemExit("%s: parts not found: %s" % (name, ", ".join(missing)))

    for part, ob in parts.items():
        # Only this part renders (and so occludes) during its bake.
        for other in bpy.data.objects:
            other.hide_render = other is not ob
        diffuse = bpy.data.images.load(os.path.join(media, "%sUV_%s.tga" % (part, number)), check_existing=True)
        size = max(diffuse.size[0], 64)

        image_name = "AO%sUV_%s" % (part, number)
        old = bpy.data.images.get(image_name)
        if old is not None:
            bpy.data.images.remove(old)
        image = bpy.data.images.new(image_name, size, size, alpha=False)
        image.colorspace_settings.name = "Non-Color"
        bake_target(ob, image)

        bpy.ops.object.select_all(action='DESELECT')
        bpy.context.view_layer.objects.active = ob
        ob.select_set(True)
        bpy.ops.object.bake(type='AO', uv_layer=ob.data.uv_layers[0].name, margin=8, margin_type='EXTEND')
        clean_ao(image)

        path = os.path.join(media, image_name + ".png")
        image.filepath_raw = path
        image.file_format = 'PNG'
        image.save()
        log("baked %s (%dx%d) -> %s" % (ob.name, size, size, path))

    for other in bpy.data.objects:
        other.hide_render = False
    for arm in (o for o in bpy.data.objects if o.type == 'ARMATURE'):
        arm.data.pose_position = 'POSE'
    bpy.ops.wm.save_mainfile()
    log("saved " + bpy.data.filepath)


main()
