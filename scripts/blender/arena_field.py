"""
The arena's field: the ground around the ring and the hills beyond the walls, the "field" object of art/arena/Arena.blend
(run by scripts/arena-field.ps1 in Blender 5.2). It replaced the 2011 Ogre terrain (Ogitor's page) in 2026-10, so the
field is modelled, painted and exported like the rest of the arena: Blender is the only source.

1. First run (or --rebuild, which DISCARDS hand edits of the field and its map): builds the "field" object from the 2011
   terrain page (art/arena/terrain_2011.ogt: 513 x 513 heights over 196 x 196 units):
     - a grid with the page's heights: full detail (0.38 units between vertices) inside the coliseum, the hills outside
       HILLS_RADIUS decimated to HILLS_RATIO of their vertices (they are only seen far away, above the walls);
     - one UV map, "AO", projected from above over the whole 196 x 196 field: the shader reads the field map and the
       baked ambient occlusion through it (the texture tiles are mapped from world positions in the shader);
     - the material fieldMaterial, whose image node "FieldMap" shows media/tumbu/arena/field_map.png (texture paint it).
   The object goes into the Export collection, so scripts/bake-arena-ao.ps1 bakes its AO (the walls' shade on the ground)
   and exports media/tumbu/arena/field.mesh; the game builds the field's physics from that mesh.
2. The field map, media/tumbu/arena/field_map.png (only made when it does not exist yet, or with --rebuild / --new-map;
   afterwards it is painted by hand in Blender). Over the 196 x 196 field, seen from above like the UV map:
     R  earth (bare ground; 0.5 is the edge, the shader breaks it up with noise): the band of earth around the ring
        (EARTH_INNER to EARTH_OUTER, its outer edge wobbling by EARTH_WOBBLE) and the 2011 "desert" layer on the hills
     G  snow: the 2011 "ice" layer on the hill tops (field_toon.frag shows bare rock on its steepest faces)
     B  tall grass height (x GRASS_FULL metres; 0 = no tall grass): rises from GRASS_SHORT at GRASS_RISE[0] units from the
        centre to 1 at GRASS_RISE[1], and stops at GRASS_OUTER (the grass is generated at load only where B > 0, on the
        field and not under the walls, the ring or the earth; Demo, docs/IMPROVEMENT_IDEAS.md "Grass remake")

The object records the steps already applied in its custom property tumbu_shapes (like arena_shapes.py), so running
this again never touches a field that was edited by hand.

Usage: blender --background art/arena/Arena.blend --python arena_field.py -- <repo root> [--rebuild] [--new-map]
"""
import bpy
import math
import os
import struct
import sys
import numpy as np

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
ROOT = os.path.abspath(ARGS[0] if ARGS else os.path.join(os.path.dirname(bpy.data.filepath), "..", ".."))
REBUILD = "--rebuild" in ARGS
NEW_MAP = "--new-map" in ARGS or REBUILD
PAGE = os.path.join(ROOT, "art", "arena", "terrain_2011.ogt")
MEDIA = os.path.join(ROOT, "media", "tumbu", "arena")
FIELD_MAP = os.path.join(MEDIA, "field_map.png")

FIELD_VERSION = 1
FIELD_MATERIAL = "fieldMaterial"
HILLS_RADIUS = 34.0         # outside this (units from the centre) the hills are decimated
HILLS_RATIO = 0.12          # ... to this fraction of their vertices
AO_SIZE = 2048              # baked AO texture (about 10 cm per texel over the 196 m field)
MAP_SIZE = 1024             # field map (about 19 cm per pixel)
FIELD_SIZE = 196.0          # units covered by the UV map, the field map and the AO (grass.object mapExtent)
EARTH_INNER = 8.3           # the earth band around the ring: from here (the ring's apron) ...
EARTH_OUTER = 13.5          # ... to here, its outer edge wobbling by +-EARTH_WOBBLE / 2
EARTH_WOBBLE = 4.0
EARTH_WOBBLE_SIZE = 4.0      # metres: the wobble's blob size (the noise's gaussian width)
EARTH_SOFT = 1.5            # the band's value goes from 0 to 1 over this width (units) at each edge
DESERT_FROM = 32.0          # the 2011 desert layer is used from here outwards (inside, the field is ours)
GRASS_SHORT = 0.31          # tall grass height near the earth band (x GRASS_FULL in the game) ...
GRASS_RISE = (14.0, 22.0)   # ... rising to full height between these radii
GRASS_OUTER = 30.0          # no tall grass beyond this radius (hidden behind the walls)


def log(message):
    print("[FIELD] " + message, flush=True)


def read_terrain_page(path):
    """Reads an Ogre terrain page (StreamSerialiser chunks: id, version, length, checksum) as Ogitor 0.4.4 saved it.
    Returns the heights h[y][x] (Ogre's terrain index space: x = +x, y = -z) and the blend maps in the same layout."""
    data = open(path, "rb").read()

    def chunk(offset):
        cid = data[offset:offset + 4]
        version, length = struct.unpack_from("<HI", data, offset + 4)
        return cid, length, offset + 14

    cid, length, body = chunk(0)                  # the stream header (endianness)
    cid, length, body = chunk(body + length)
    if cid != b"TERR":
        raise SystemExit("%s: no TERR chunk" % path)
    align, size, world = struct.unpack_from("<BHf", data, body)
    offset = body + 1 + 2 + 4 + 2 + 2 + 12        # align, size, world size, max / min batch, position
    heights = np.frombuffer(data, "<f4", size * size, offset).reshape(size, size).copy()
    offset += 4 * size * size
    cid, length, body = chunk(offset)              # TDCL: the layer declaration
    offset = body + length
    layers = data[offset]
    offset += 1
    for _ in range(layers):
        cid, length, body = chunk(offset)          # TLAY: world size and texture names
        offset = body + length
    blend_size, = struct.unpack_from("<H", data, offset)
    offset += 2
    blends = []
    for _ in range((layers - 1 + 3) // 4):
        image = np.frombuffer(data, np.uint8, blend_size * blend_size * 4, offset).reshape(blend_size, blend_size, 4)
        offset += blend_size * blend_size * 4
        # the blend map's row 0 is the far edge (terrain y = size - 1): flip it into the heights' layout
        blends.append(np.flipud(image).astype(np.float32) / 255.0)
    log("terrain page: %d x %d heights over %.0f units (%.2f .. %.2f), %d layers, blend map %d"
        % (size, size, world, heights.min(), heights.max(), layers, blend_size))
    return heights, world, blends


def smooth_noise(size, features, seed):
    """Smooth periodic noise in 0..1 (gaussian-filtered white noise, rank-normalised): `features` = blob size in pixels."""
    rng = np.random.default_rng(seed)
    k = np.fft.fftfreq(size)
    kx, ky = np.meshgrid(k, k)
    field = np.real(np.fft.ifft2(np.fft.fft2(rng.standard_normal((size, size)))
                                 * np.exp(-2 * (np.pi ** 2) * (features ** 2) * (kx ** 2 + ky ** 2))))
    ranks = np.empty(size * size)
    ranks[np.argsort(field.ravel())] = np.arange(size * size) / (size * size)
    return ranks.reshape(size, size)


def make_field_map(blends, world):
    """The first field map (see the module text), as Blender pixels: row 0 at the bottom (Blender y = -world / 2)."""
    n = MAP_SIZE
    centres = (np.arange(n) + 0.5) / n * world - world / 2
    x, y = np.meshgrid(centres, centres)            # [row][column] = Blender (y, x)
    r = np.hypot(x, y)
    broad = smooth_noise(n, EARTH_WOBBLE_SIZE / world * n, 31)
    outer = EARTH_OUTER + (broad - 0.5) * EARTH_WOBBLE
    band = 0.5 + np.clip(np.minimum(r - EARTH_INNER, outer - r) / EARTH_SOFT, -0.5, 0.5)

    def resample(channel):
        size = channel.shape[0]
        idx = np.clip(((np.arange(n) + 0.5) / n * size).astype(int), 0, size - 1)
        return channel[idx][:, idx]

    desert = resample(blends[0][..., 0]) * (r >= DESERT_FROM)
    rock = resample(blends[0][..., 1])
    rise = np.clip((r - GRASS_RISE[0]) / (GRASS_RISE[1] - GRASS_RISE[0]), 0.0, 1.0)
    grass = (GRASS_SHORT + (1.0 - GRASS_SHORT) * rise) * (r <= GRASS_OUTER)
    rgba = np.ones((n, n, 4), np.float32)
    rgba[..., 0] = np.maximum(band, desert)
    rgba[..., 1] = rock
    rgba[..., 2] = grass
    return rgba


def save_field_map(rgba):
    img = bpy.data.images.get("field_map")
    if img is None:
        img = bpy.data.images.new("field_map", MAP_SIZE, MAP_SIZE, alpha=False, float_buffer=False)
    img.colorspace_settings.name = "Non-Color"
    img.pixels.foreach_set(rgba.ravel())
    img.filepath_raw = FIELD_MAP
    img.file_format = "PNG"
    img.save()
    log("wrote " + FIELD_MAP)
    return img


def field_material(image):
    """fieldMaterial with the field map in an image node (texture paint target), shown as the base colour in Blender."""
    mat = bpy.data.materials.get(FIELD_MATERIAL) or bpy.data.materials.new(FIELD_MATERIAL)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    node = nodes.get("FieldMap")
    if node is None:
        node = nodes.new("ShaderNodeTexImage")
        node.name = node.label = "FieldMap"
        node.location = (-400, 200)
        bsdf = nodes.get("Principled BSDF")
        if bsdf is not None:
            mat.node_tree.links.new(node.outputs["Color"], bsdf.inputs["Base Color"])
    node.image = image
    nodes.active = node
    return mat


def project_uv(ob):
    """The field's only UV map, "AO", projected from above over the FIELD_SIZE x FIELD_SIZE field (the bake script keeps
    an existing "AO" map). Done on every run, so moving vertices sideways (sculpting) never shifts the field map or the
    AO under them. Returns the vertex positions."""
    me = ob.data
    uv = me.uv_layers.get("AO") or me.uv_layers.new(name="AO")
    co = np.zeros(len(me.vertices) * 3, np.float32)
    me.vertices.foreach_get("co", co)
    co = co.reshape(-1, 3)
    if ob.matrix_world != ob.matrix_world.Identity(4):
        raise SystemExit("the field object must not be moved, turned or scaled (edit its vertices instead)")
    loop_vertex = np.zeros(len(me.loops), np.int32)
    me.loops.foreach_get("vertex_index", loop_vertex)
    uvs = (co[loop_vertex, :2] + FIELD_SIZE / 2) / FIELD_SIZE
    uv.data.foreach_set("uv", uvs.astype(np.float32).ravel())
    return co


def build_field(heights, world, image):
    size = heights.shape[0]
    spacing = world / (size - 1)
    coords = np.arange(size) * spacing - world / 2
    # Ogre terrain (ALIGN_X_Z): game x = x index, game z = -(y index); Blender (Z up, exported with SWAP_AXIS xz-y):
    # Blender x = game x, Blender y = -game z = y index, Blender z = height
    gx, gy = np.meshgrid(coords, coords)
    verts = np.stack([gx, gy, heights], -1).reshape(-1, 3)
    idx = np.arange(size * size).reshape(size, size)
    faces = np.stack([idx[:-1, :-1].ravel(), idx[:-1, 1:].ravel(), idx[1:, 1:].ravel(), idx[1:, :-1].ravel()], -1)
    me = bpy.data.meshes.new("field")
    me.vertices.add(len(verts))
    me.vertices.foreach_set("co", verts.astype(np.float32).ravel())
    me.loops.add(faces.size)
    me.loops.foreach_set("vertex_index", faces.astype(np.int32).ravel())
    me.polygons.add(len(faces))
    me.polygons.foreach_set("loop_start", (np.arange(len(faces)) * 4).astype(np.int32))
    me.update()
    me.validate()
    ob = bpy.data.objects.new("field", me)
    export = bpy.data.collections.get("Export")
    (export or bpy.context.scene.collection).objects.link(ob)
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)

    # decimate the hills outside the coliseum (vertex group weight 1 = decimated, 0 = kept)
    r = np.hypot(verts[:, 0], verts[:, 1])
    group = ob.vertex_groups.new(name="hills")
    group.add(np.nonzero(r > HILLS_RADIUS)[0].tolist(), 1.0, 'REPLACE')
    dec = ob.modifiers.new("decimate", 'DECIMATE')
    dec.decimate_type = 'COLLAPSE'
    dec.ratio = HILLS_RATIO
    dec.vertex_group = "hills"
    dec.use_collapse_triangulate = True
    bpy.ops.object.modifier_apply(modifier=dec.name)
    if ob.vertex_groups.get("hills") is not None:
        ob.vertex_groups.remove(ob.vertex_groups["hills"])
    me.shade_smooth()

    co = project_uv(ob)
    me.materials.append(field_material(image))
    ob["tumbu_ao_size"] = AO_SIZE
    ob["tumbu_shapes"] = FIELD_VERSION
    log("field: %d vertices, %d faces (%d inside %.0f units: full detail)"
        % (len(me.vertices), len(me.polygons), int(np.sum(np.hypot(co[:, 0], co[:, 1]) < HILLS_RADIUS)), HILLS_RADIUS))
    return ob


def main():
    ob = bpy.data.objects.get("field")
    if ob is not None and REBUILD:
        bpy.data.meshes.remove(ob.data)
        ob = None
    built = False
    if ob is None or NEW_MAP or not os.path.isfile(FIELD_MAP):
        heights, world, blends = read_terrain_page(PAGE)
        if NEW_MAP or not os.path.isfile(FIELD_MAP):
            image = save_field_map(make_field_map(blends, world))
        else:
            image = bpy.data.images.load(FIELD_MAP, check_existing=True)
        image.colorspace_settings.name = "Non-Color"
        if ob is None:
            ob = build_field(heights, world, image)
            built = True
        else:
            field_material(image)
    done = ob.get("tumbu_shapes", 0)
    # (later versions of the field's steps go here, each run once: `if done < 2: ...`)
    project_uv(ob)
    ob["tumbu_shapes"] = FIELD_VERSION
    log("field: version %d -> %d%s" % (done, FIELD_VERSION, " (built)" if built else ""))
    bpy.ops.file.make_paths_relative()
    bpy.ops.wm.save_mainfile()
    log("saved " + bpy.data.filepath)


main()
