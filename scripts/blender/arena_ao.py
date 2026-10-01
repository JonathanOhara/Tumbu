"""
Arena ambient occlusion (run by scripts/bake-arena-ao.ps1, in Blender 5.2 with blender2ogre installed).

1. First run (or --rebuild): builds art/arena/Arena.blend from the 2011 sources (coliseum.blend, gym.blend).
   Collections:
     Export     meshes that are baked and exported to media/tumbu/arena/<object name>.mesh
     Occluders  extra geometry that only casts AO (not exported); empty at first
   Every exported mesh gets a second UV map, "AO" (Smart UV Project), next to its texture UVs.
2. Every run: bakes Cycles AO for each Export mesh into media/tumbu/arena/<name>_ao.png through its AO UV
   map (the whole scene occludes), then exports the meshes with blender2ogre and saves Arena.blend.

Settings live in the .blend, so they can be adjusted in Blender and the script run again:
  Scene custom properties: tumbu_ao_distance (world units), tumbu_ao_samples
  Object custom property:  tumbu_ao_size (texture size in pixels)

Usage: blender --background --python arena_ao.py -- <repo root> [--rebuild]
"""
import bpy
import os
import sys
import addon_utils
import math
from mathutils import Matrix

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
if not args:
    raise SystemExit("usage: blender --background --python arena_ao.py -- <repo root> [--rebuild]")
ROOT = args[0]
REBUILD = "--rebuild" in args

MEDIA = os.path.join(ROOT, "media", "tumbu", "arena")
ART = os.path.join(ROOT, "art", "arena")
BLEND = os.path.join(ART, "Arena.blend")
AO_UV = "AO"

# (source file, object, material name the game uses, AO texture size, modelled Y-up)
# The 2011 files do not agree on "up": coliseum.blend (like the robots) is Y-up, the Ogre convention, and was
# exported without axis conversion; gym.blend is Z-up. blender2ogre converts Blender Z-up to Ogre Y-up, so Y-up
# sources are turned upright (+90 degrees around X) when Arena.blend is built.
SOURCES = [
    ("coliseum.blend", "coliseum", "coliseumMaterial", 2048, True),
    ("gym.blend", "arena", "arenaMaterial/TEXFACE/gym_arena.png", 1024, False),
]


def log(message):
    print("[AO] " + message, flush=True)


def collection(name):
    col = bpy.data.collections.get(name)
    if col is None:
        col = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(col)
    return col


def build():
    """Creates Arena.blend from the 2011 .blend files."""
    bpy.ops.wm.read_homefile(use_empty=True)    # keeps the preferences (blender2ogre stays enabled)
    export = collection("Export")
    collection("Occluders")
    for source, name, material, size, y_up in SOURCES:
        with bpy.data.libraries.load(os.path.join(MEDIA, source), link=False) as (src, dst):
            dst.objects = [name]
        ob = dst.objects[0]
        export.objects.link(ob)
        if y_up:
            ob.data.transform(Matrix.Rotation(math.radians(90), 4, 'X'))
        ob["tumbu_ao_size"] = size
        if ob.material_slots and ob.material_slots[0].material:
            ob.material_slots[0].material.name = material
        add_ao_uv(ob)
        log("imported %s from %s (%d vertices)" % (name, source, len(ob.data.vertices)))

    scene = bpy.context.scene
    scene["tumbu_ao_distance"] = 3.0
    scene["tumbu_ao_samples"] = 1024
    os.makedirs(ART, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=BLEND)
    log("created " + BLEND)


def add_ao_uv(ob):
    """Second UV map without overlaps (texture UVs stay first: Ogre texture coordinate 0)."""
    mesh = ob.data
    if AO_UV not in mesh.uv_layers:
        mesh.uv_layers.new(name=AO_UV)
    mesh.uv_layers.active = mesh.uv_layers[AO_UV]

    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.004, area_weight=0.0, scale_to_bounds=True)
    bpy.ops.object.mode_set(mode='OBJECT')
    mesh.uv_layers.active = mesh.uv_layers[0]


def bake_target(ob, image):
    """Makes `image` the bake target (active image node) of every material of `ob`."""
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
            node.location = (-600, -300)
        node.image = image
        nodes.active = node


def bake(ob, scene):
    size = int(ob.get("tumbu_ao_size", 1024))
    name = ob.name + "_ao"
    image = bpy.data.images.get(name)
    if image is None or tuple(image.size) != (size, size):
        if image is not None:
            bpy.data.images.remove(image)
        image = bpy.data.images.new(name, size, size, alpha=False)
    image.colorspace_settings.name = "Non-Color"
    bake_target(ob, image)

    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    bpy.ops.object.bake(type='AO', uv_layer=AO_UV, margin=8, margin_type='EXTEND')

    path = os.path.join(MEDIA, name + ".png")
    image.filepath_raw = path
    image.file_format = 'PNG'
    image.save()
    log("baked %s (%dx%d) -> %s" % (ob.name, size, size, path))


def export(ob):
    from io_ogre import api
    # blender2ogre silently skips unselected objects (its SELECTED_ONLY setting).
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    api.dot_mesh(ob, MEDIA, force_name=ob.name, overwrite=True)
    log("exported %s.mesh" % ob.name)
    # blender2ogre leaves the converter's log next to the mesh.
    converter_log = os.path.join(MEDIA, "OgreXMLConverter.log")
    if os.path.isfile(converter_log):
        os.remove(converter_log)


def main():
    if REBUILD or not os.path.isfile(BLEND):
        build()
    bpy.ops.wm.open_mainfile(filepath=BLEND)

    if "io_ogre" not in bpy.context.preferences.addons:
        addon_utils.enable("io_ogre")

    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = int(scene.get("tumbu_ao_samples", 1024))
    if scene.world is None:
        scene.world = bpy.data.worlds.new("World")
    scene.world.light_settings.distance = float(scene.get("tumbu_ao_distance", 3.0))
    log("AO distance %.2f, %d samples" % (scene.world.light_settings.distance, scene.cycles.samples))

    export_col = bpy.data.collections.get("Export")
    meshes = [ob for ob in export_col.all_objects if ob.type == 'MESH'] if export_col else []
    if not meshes:
        raise SystemExit("Arena.blend has no meshes in the 'Export' collection")
    for ob in meshes:
        if AO_UV not in ob.data.uv_layers:
            add_ao_uv(ob)
        bake(ob, scene)
    for ob in meshes:
        export(ob)

    bpy.ops.file.make_paths_relative()
    bpy.ops.wm.save_as_mainfile(filepath=BLEND)
    log("saved " + BLEND)


main()
