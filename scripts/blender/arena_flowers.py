"""
The field's flowers and reeds: small toon plant models in art/arena/Flowers.blend, exported to
media/tumbu/arena/flower_<kind>.mesh and reed_<kind>.mesh. Grass.cpp plants them among the tall grass when a match loads
(media/configuration/grass.object, "flowers field" and "reeds field") and grass.vert's wind and push bend them like the
blades (flower.vert). After the games that do flowers well (Ghost of Tsushima, Horizon Zero Dawn, the Xenoblade
meadows): the blades are generated, the flowers are made by hand and placed by the same rules.

Run through scripts/arena-flowers.ps1 (headless Blender 5.2 + blender2ogre):
  - opens art/arena/Flowers.blend (creates it the first time) and builds each plant of KINDS that is not in it yet
    (--rebuild: again, which discards hand edits);
  - exports every mesh object in the "Flowers" collection whose name starts with "flower_" or "reed_";
  - --only a,b: only those plants (object names) are built, rebuilt or exported, the others are left alone;
  - --preview <png>: renders the plants (or the --only ones) side by side into <png> instead, without saving the file
    or exporting anything.

A flower's rules (Grass.cpp reads them):
  - the root is the object's origin and it grows up Blender's z (1 unit = 1 metre); the object's own location only
    spreads them out for viewing, the export takes the mesh as it is around its origin;
  - its colours are the colour attribute "Col" (Vertex Paint; corner domain, byte colour): what Blender shows is what
    the game draws (Grass converts blender2ogre's linear values back to sRGB);
  - no UVs, no textures: the game lights it with the toon shader (flower.frag), seen from both sides.
"""
import math
import os
import sys

import bpy
import addon_utils
from mathutils import Matrix, Vector

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
ROOT = os.path.abspath(ARGS[0])
REBUILD = "--rebuild" in ARGS
ONLY = ARGS[ARGS.index("--only") + 1].split(",") if "--only" in ARGS else None
PREVIEW = ARGS[ARGS.index("--preview") + 1] if "--preview" in ARGS else None
BLEND = os.path.join(ROOT, "art", "arena", "Flowers.blend")
MEDIA = os.path.join(ROOT, "media", "tumbu", "arena")
MATERIAL = "Tumbu/FieldFlower"      # the game's material (media/tumbu/arena/field_flower.material)

# colours (sRGB, as Vertex Paint shows them)
STEM = (0.25, 0.42, 0.14)
LEAF = (0.30, 0.50, 0.17)
LEAF_DARK = (0.22, 0.38, 0.12)
# toon proportions: heads and stems bigger than a real flower's, so they read among the wide toon blades
HEAD = 1.35         # petals and flower centres
STEM_WIDTH = 1.6    # stems


def log(message):
    print("[FLOWERS] " + message, flush=True)


class Builder:
    """Collects vertices and faces with a colour and a smooth flag per face."""

    def __init__(self):
        self.verts = []
        self.faces = []
        self.colours = []
        self.smooth = []

    def vert(self, p):
        self.verts.append(Vector(p))
        return len(self.verts) - 1

    def face(self, idx, colour, smooth=False):
        self.faces.append(tuple(idx))
        self.colours.append(colour)
        self.smooth.append(smooth)

    def tube(self, path, radii, colour, sides=5, widen=True):
        """A stem along `path` (points), `radii` per point (x STEM_WIDTH unless widen=False), closed at neither end (the
        ends are hidden)."""
        rings = []
        for i, p in enumerate(path):
            d = (path[min(i + 1, len(path) - 1)] - path[max(i - 1, 0)]).normalized()
            side = d.cross(Vector((0, 1, 0)) if abs(d.y) < 0.9 else Vector((1, 0, 0))).normalized()
            up = d.cross(side)
            rings.append([self.vert(p + (side * math.cos(a) + up * math.sin(a)) * radii[i] * (STEM_WIDTH if widen else 1))
                          for a in (k * 2 * math.pi / sides for k in range(sides))])
        for i in range(len(rings) - 1):
            for k in range(sides):
                a, b = rings[i][k], rings[i][(k + 1) % sides]
                c, d = rings[i + 1][(k + 1) % sides], rings[i + 1][k]
                self.face((a, b, c, d), colour, True)

    def blade(self, base, direction, normal, length, width, curl, colour, tip_colour=None, profile=(0.35, 1.0, 0.8)):
        """A petal or a leaf: a strip of 3 segments from `base` along `direction`, tapering to a tip, its width across
        direction x normal, curling by `curl` radians (towards `normal`) along its length."""
        direction = Vector(direction).normalized()
        normal = Vector(normal).normalized()
        across = direction.cross(normal).normalized()
        normal = across.cross(direction).normalized()
        steps = len(profile)
        rows = []
        p = Vector(base)
        d = direction.copy()
        seg = length / steps
        for i in range(steps):
            w = width * profile[i] * 0.5
            rows.append((self.vert(p - across * w), self.vert(p + across * w)))
            # bend the direction towards the normal a little each segment
            rot = Matrix.Rotation(curl / steps, 3, across)
            d = (rot @ d).normalized()
            p = p + d * seg
        tip = self.vert(p)
        for i in range(steps - 1):
            c = colour if tip_colour is None or i < steps - 2 else tip_colour
            self.face((rows[i][0], rows[i][1], rows[i + 1][1], rows[i + 1][0]), c)
        self.face((rows[-1][0], rows[-1][1], tip), colour if tip_colour is None else tip_colour)

    def dome(self, centre, normal, radius, height, colour, sides=8):
        """A low cone (a flower's centre), its apex `height` above `centre` along `normal`."""
        normal = Vector(normal).normalized()
        side = normal.cross(Vector((0, 1, 0)) if abs(normal.y) < 0.9 else Vector((1, 0, 0))).normalized()
        up = normal.cross(side)
        radius *= HEAD
        height *= HEAD
        apex = self.vert(Vector(centre) + normal * height)
        ring = [self.vert(Vector(centre) + (side * math.cos(a) + up * math.sin(a)) * radius)
                for a in (k * 2 * math.pi / sides for k in range(sides))]
        for k in range(sides):
            self.face((ring[k], ring[(k + 1) % sides], apex), colour, True)


def curve(top, bend, points=5):
    """A stem from the origin up to `top`, its middle pushed sideways by `bend` (a gentle arc)."""
    top = Vector(top)
    bend = Vector(bend)
    return [top * t + bend * (4 * t * (1 - t)) for t in (i / (points - 1) for i in range(points))]


def head_frame(path):
    """Direction the stem ends in, and two axes across it."""
    n = (path[-1] - path[-2]).normalized()
    side = n.cross(Vector((0, 1, 0))).normalized()
    return n, side, n.cross(side).normalized()


def radial(b, centre, normal, side, up, count, length, width, tilt, curl, colour, tip_colour=None, offset=0.0,
           profile=(0.35, 1.0, 0.8)):
    """A ring of petals around `centre`, each leaving at `tilt` radians above the head's plane."""
    length *= HEAD
    width *= HEAD
    for k in range(count):
        a = offset + k * 2 * math.pi / count
        out = side * math.cos(a) + up * math.sin(a)
        d = (out * math.cos(tilt) + normal * math.sin(tilt)).normalized()
        b.blade(centre + out * 0.004, d, normal, length, width, curl, colour, tip_colour, profile)


def leaves(b, specs):
    """Leaves at the foot of the stem: (height, angle in degrees, length, width, lift, droop)."""
    for z, angle, length, width, lift, droop in specs:
        a = math.radians(angle)
        out = Vector((math.cos(a), math.sin(a), 0))
        d = (out + Vector((0, 0, lift))).normalized()
        b.blade(Vector((0, 0, z)) + out * 0.004, d, Vector((0, 0, 1)), length, width, -droop, LEAF,
                profile=(0.5, 1.0, 0.7))


def daisy(b):
    """White daisy: a long slender stem, a flat head of twelve white petals around a yellow dome."""
    path = curve((0.03, 0.0, 0.42), (0.025, 0.01, 0.0))
    b.tube(path, [0.007, 0.006, 0.0055, 0.005, 0.005], STEM)
    leaves(b, [(0.02, 20, 0.11, 0.030, 0.5, 0.6), (0.03, 150, 0.10, 0.028, 0.6, 0.5), (0.03, 260, 0.09, 0.026, 0.7, 0.5)])
    n, side, up = head_frame(path)
    radial(b, path[-1], n, side, up, 12, 0.085, 0.026, math.radians(8), math.radians(-10), (0.97, 0.96, 0.92))
    b.dome(path[-1], n, 0.024, 0.014, (0.98, 0.76, 0.18))


def poppy(b):
    """Red poppy: a cup of five broad petals, slightly nodding, a dark centre."""
    path = curve((0.06, 0.02, 0.48), (0.04, -0.02, 0.0))
    b.tube(path, [0.008, 0.007, 0.006, 0.0055, 0.0055], STEM)
    leaves(b, [(0.02, 60, 0.13, 0.034, 0.4, 0.7), (0.025, 210, 0.12, 0.032, 0.5, 0.6)])
    n, side, up = head_frame(path)
    radial(b, path[-1], n, side, up, 5, 0.075, 0.075, math.radians(35), math.radians(40), (0.86, 0.14, 0.10),
           (0.93, 0.26, 0.16), profile=(0.45, 1.0, 0.9))
    b.dome(path[-1] + n * 0.004, n, 0.016, 0.016, (0.16, 0.11, 0.10), 6)


def lupine(b):
    """Purple lupine spike: whorls of small petals up the top half of the stem, darker below, lilac at the tip; a
    star of leaflets at the foot."""
    path = curve((0.0, 0.02, 0.58), (0.02, 0.0, 0.0), 7)
    b.tube(path, [0.008, 0.0075, 0.007, 0.0065, 0.006, 0.005, 0.004], STEM)
    for k in range(7):
        a = math.radians(k * 360 / 7)
        out = Vector((math.cos(a), math.sin(a), 0))
        b.blade(Vector((0, 0, 0.05)) + out * 0.003, (out + Vector((0, 0, 0.5))).normalized(), Vector((0, 0, 1)), 0.08,
                0.022, -0.5, LEAF if k % 2 else LEAF_DARK, profile=(0.4, 1.0, 0.6))
    whorls = 11
    for w in range(whorls):
        t = w / (whorls - 1)
        z = 0.27 + t * 0.30
        centre = Vector((0.02 * 4 * (z / 0.58) * (1 - z / 0.58), 0.02 * (z / 0.58), z))
        dark, light = Vector((0.40, 0.26, 0.74)), Vector((0.72, 0.60, 0.95))
        colour = tuple(dark.lerp(light, t))
        size = 1.0 - t * 0.5
        for k in range(5):
            a = math.radians(k * 72 + w * 36)
            out = Vector((math.cos(a), math.sin(a), 0))
            b.blade(centre + out * 0.004, (out + Vector((0, 0, 0.8))).normalized(), Vector((0, 0, 1)), 0.042 * HEAD * size,
                    0.036 * HEAD * size, math.radians(25), colour, profile=(0.5, 1.0, 0.9))
    b.dome(path[-1], Vector((0, 0, 1)), 0.008, 0.02, (0.74, 0.64, 0.95), 5)


def forgetmenot(b):
    """Blue forget-me-nots: one stem branching into five short stalks, each with a small flat five-petal star and a
    pale yellow eye."""
    trunk = curve((0.0, 0.0, 0.22), (0.015, 0.0, 0.0), 4)
    b.tube(trunk, [0.006, 0.0055, 0.005, 0.0045], STEM)
    leaves(b, [(0.02, 0, 0.08, 0.026, 0.6, 0.4), (0.02, 120, 0.075, 0.024, 0.7, 0.4), (0.025, 240, 0.08, 0.026, 0.6, 0.4)])
    stalks = [(30, 0.10, 0.07), (100, 0.07, 0.11), (175, 0.11, 0.08), (250, 0.06, 0.12), (320, 0.09, 0.09)]
    for angle, reach, rise in stalks:
        a = math.radians(angle)
        top = Vector((math.cos(a) * reach, math.sin(a) * reach, 0)) * 0.6 + Vector((0, 0, 0.22 + rise))
        path = [trunk[-1] + (top - trunk[-1]) * t + Vector((0, 0, 0.02 * 4 * t * (1 - t))) for t in (0, 0.5, 1)]
        b.tube(path, [0.0035, 0.003, 0.003], STEM, 4)
        n = Vector((math.cos(a) * 0.3, math.sin(a) * 0.3, 1)).normalized()
        side = n.cross(Vector((0, 1, 0))).normalized()
        up = n.cross(side).normalized()
        radial(b, path[-1], n, side, up, 5, 0.032, 0.03, math.radians(5), 0.0, (0.40, 0.64, 0.98),
               profile=(0.5, 1.0, 0.9), offset=a)
        b.dome(path[-1], n, 0.008, 0.004, (0.99, 0.93, 0.56), 5)


def marigold(b):
    """Orange marigold: two layers of short broad petals, the inner one darker and more upright."""
    path = curve((-0.03, 0.02, 0.36), (-0.02, 0.0, 0.0))
    b.tube(path, [0.0075, 0.0065, 0.006, 0.0055, 0.0055], STEM)
    leaves(b, [(0.02, 80, 0.10, 0.032, 0.5, 0.6), (0.03, 200, 0.10, 0.030, 0.6, 0.5), (0.025, 320, 0.09, 0.03, 0.5, 0.5)])
    n, side, up = head_frame(path)
    radial(b, path[-1], n, side, up, 10, 0.06, 0.04, math.radians(10), math.radians(-5), (0.99, 0.62, 0.14),
           profile=(0.5, 1.0, 0.85))
    radial(b, path[-1] + n * 0.006, n, side, up, 8, 0.04, 0.035, math.radians(40), math.radians(10), (0.93, 0.44, 0.08),
           offset=0.4, profile=(0.5, 1.0, 0.85))
    b.dome(path[-1] + n * 0.006, n, 0.014, 0.01, (0.62, 0.30, 0.06), 6)


REED_LEAF = (0.36, 0.50, 0.20)
REED_LEAF_DRY = (0.60, 0.58, 0.30)


def reed_leaves(b, count, length, seed):
    """Long leaves at the foot of a reed clump, rising steeply and arching over, some of them dry."""
    for k in range(count):
        a = math.radians(k * 360 / count + (seed * 47 + k * 71) % 40)
        out = Vector((math.cos(a), math.sin(a), 0))
        l = length * (0.75 + 0.25 * ((k * 37 + seed) % 5) / 4)
        b.blade(out * 0.05, (out + Vector((0, 0, 2.2))).normalized(), out, l, 0.034, -1.1,
                REED_LEAF_DRY if k % 3 == 2 else REED_LEAF, profile=(0.6, 1.0, 0.9, 0.7, 0.4))


def pampas(b):
    """Pampas grass: five tall straw stems leaning out a little, each with a cream plume of slender feathers that flags
    to the side the stem leans to."""
    stems = [(0, 1.75, 0.16), (72, 1.55, 0.24), (144, 1.85, 0.18), (216, 1.45, 0.26), (288, 1.65, 0.2)]
    for angle, height, lean in stems:
        a = math.radians(angle)
        out = Vector((math.cos(a), math.sin(a), 0))
        top = out * (lean * height * 0.5) + Vector((0, 0, height))
        path = [out * 0.07 + q for q in curve(top, out * 0.03, 6)]
        b.tube(path, [0.008, 0.0075, 0.007, 0.0065, 0.006, 0.0055], (0.70, 0.66, 0.42))
        n, side, up = head_frame(path)
        # a plume: slender feathers close around the top, all flagging to the side the stem leans to
        for k in range(12):
            fa = k * 2 * math.pi / 12 + angle
            spread = side * math.cos(fa) + up * math.sin(fa)
            outer = k % 2
            d = (n + spread * (0.28 if outer else 0.14)).normalized()
            colour = (0.95, 0.90, 0.76) if outer else (0.88, 0.82, 0.66)
            b.blade(path[-1] - n * (0.26 + 0.04 * outer) + spread * 0.008, d, out, 0.40 + 0.06 * ((k * 5) % 3) / 2, 0.06,
                    0.55 + 0.25 * outer, colour, (0.99, 0.96, 0.86), profile=(0.4, 0.9, 1.0, 0.8))
    reed_leaves(b, 9, 0.85, 1)


def cattail(b):
    """Cattails (bulrush): four tall green stems, each with a brown velvet head and a thin spike above it, among long
    arching leaves."""
    stems = [(20, 1.35, 0.12), (130, 1.5, 0.14), (230, 1.25, 0.18), (320, 1.45, 0.1)]
    for angle, height, lean in stems:
        a = math.radians(angle)
        out = Vector((math.cos(a), math.sin(a), 0))
        top = out * (lean * height * 0.5) + Vector((0, 0, height))
        path = [out * 0.06 + q for q in curve(top, out * 0.02, 6)]
        b.tube(path, [0.007, 0.0065, 0.006, 0.0055, 0.005, 0.005], (0.32, 0.46, 0.18))
        n = (path[-1] - path[-2]).normalized()
        head = [path[-1] - n * 0.24 + n * (0.2 * t) for t in (0, 0.1, 0.5, 0.9, 1.0)]
        b.tube(head, [0.012, 0.028, 0.031, 0.027, 0.012], (0.42, 0.25, 0.12), 6, widen=False)
        b.tube([head[-1], head[-1] + n * 0.1], [0.004, 0.002], (0.48, 0.40, 0.22), 4, widen=False)
    reed_leaves(b, 10, 1.0, 2)


# (object name, builder): flower_* are the flowers, reed_* the reeds
KINDS = [("flower_daisy", daisy), ("flower_poppy", poppy), ("flower_lupine", lupine),
         ("flower_forgetmenot", forgetmenot), ("flower_marigold", marigold),
         ("reed_pampas", pampas), ("reed_cattail", cattail)]


def srgb_to_linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def make_object(name, build, collection, x):
    b = Builder()
    build(b)
    me = bpy.data.meshes.new(name)
    me.from_pydata([tuple(v) for v in b.verts], [], b.faces)
    me.update()
    col = me.color_attributes.new("Col", 'BYTE_COLOR', 'CORNER')
    for poly, colour, smooth in zip(me.polygons, b.colours, b.smooth):
        poly.use_smooth = smooth
        for li in poly.loop_indices:
            col.data[li].color_srgb = (colour[0], colour[1], colour[2], 1.0)
    mat = bpy.data.materials.get(MATERIAL) or bpy.data.materials.new(MATERIAL)
    me.materials.append(mat)
    ob = bpy.data.objects.new(name, me)
    ob.location = (x, 0, 0)
    collection.objects.link(ob)
    log("built %s: %d vertices, %d faces" % (name, len(me.vertices), len(me.polygons)))
    return ob


def export(ob):
    from io_ogre import api, config
    # blender2ogre keeps its settings between runs (io_ogre.json): Flowers.blend is Z-up, like Arena.blend
    config.update(SWAP_AXIS='xz-y')
    bpy.ops.object.select_all(action='DESELECT')
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    api.dot_mesh(ob, MEDIA, force_name=ob.name, overwrite=True, tangents=0)
    # blender2ogre leaves the XML and the converter's log next to the mesh
    for left in (ob.name + ".mesh.xml", "OgreXMLConverter.log"):
        if os.path.exists(os.path.join(MEDIA, left)):
            os.remove(os.path.join(MEDIA, left))
    lo = [min(v.co[i] for v in ob.data.vertices) for i in range(3)]
    hi = [max(v.co[i] for v in ob.data.vertices) for i in range(3)]
    log("exported %s.mesh (Blender bounds %s .. %s)" % (ob.name, ["%.3f" % v for v in lo], ["%.3f" % v for v in hi]))


def preview(objects, path):
    """Renders the plants side by side (Workbench, their vertex colours) from the front with an orthographic camera
    that frames them all."""
    from mathutils import Vector
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_WORKBENCH'
    scene.display.shading.light = 'STUDIO'
    scene.display.shading.color_type = 'VERTEX'
    scene.display.shading.background_type = 'VIEWPORT'
    scene.display.shading.background_color = (0.55, 0.70, 0.85)
    scene.render.resolution_x, scene.render.resolution_y = 1600, 700
    corners = [ob.matrix_world @ Vector(c) for ob in objects for c in ob.bound_box]
    lo = Vector([min(c[i] for c in corners) for i in range(3)])
    hi = Vector([max(c[i] for c in corners) for i in range(3)])
    centre = (lo + hi) / 2
    cam = bpy.data.cameras.new("preview")
    cam.type = 'ORTHO'
    cam.ortho_scale = max(hi.x - lo.x, (hi.z - lo.z) * 1600 / 700) * 1.1
    camera = bpy.data.objects.new("preview", cam)
    scene.collection.objects.link(camera)
    # from the front (-y), a little above
    camera.location = centre + Vector((0, -10, 1.5))
    camera.rotation_euler = (centre - camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.camera = camera
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    log("preview of %d plants: %s" % (len(objects), path))


def main():
    if os.path.exists(BLEND):
        bpy.ops.wm.open_mainfile(filepath=BLEND)
    else:
        # an empty file, keeping the preferences: read_factory_settings would reset them too, which switches the
        # blender2ogre add-on off and makes enabling it again fail halfway (a KeyError in its register)
        bpy.ops.wm.read_homefile(use_empty=True)
    if "io_ogre" not in bpy.context.preferences.addons:
        addon_utils.enable("io_ogre")
    collection = bpy.data.collections.get("Flowers")
    if collection is None:
        collection = bpy.data.collections.new("Flowers")
        bpy.context.scene.collection.children.link(collection)
    for i, (name, build) in enumerate(KINDS):
        if ONLY is not None and name not in ONLY:
            continue
        ob = bpy.data.objects.get(name)
        if PREVIEW is not None and ob is not None:
            continue
        if ob is not None and REBUILD:
            me = ob.data
            bpy.data.objects.remove(ob)
            bpy.data.meshes.remove(me)
            ob = None
        if ob is None:
            make_object(name, build, collection, i * 0.4)
    plants = [ob for ob in sorted(collection.objects, key=lambda o: o.name)
              if ob.type == 'MESH' and (ob.name.startswith("flower_") or ob.name.startswith("reed_"))
              and (ONLY is None or ob.name in ONLY)]
    if PREVIEW is not None:
        preview(plants, PREVIEW)
        return
    bpy.ops.wm.save_as_mainfile(filepath=BLEND)
    log("saved " + BLEND)
    for ob in plants:
        export(ob)


main()
