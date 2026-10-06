"""
Arena remake textures (run by scripts/arena-textures.ps1 in Blender's bundled Python: numpy, no extra packages).

Every map of a set comes from one height field, so colour, normal and height always agree (what the parallax needs).
All sets tile seamlessly: the block layout, the per-block randomness and the noise are periodic over the tile.
The look was chosen from the mock-ups (docs/IMPROVEMENT_IDEAS.md, "Coliseum retexture"); the numbers here are the
starting point, tuned in the game.

Sets, written to media/tumbu/arena/:
  sandstone_col.png  RGB albedo of the coliseum stone (gamma space, like every game texture)
  sandstone_nrm.png  RGB tangent-space normal (x = +u, y = +v, i.e. down the image; z out of the surface)
  sandstone_hgt.png  R height (1 = block face, 0 = bottom of a joint), G moss patch field (the shader decides where
                     moss grows from it, the wall height and the joints; tunable without running this again)
  ringtiles_col/nrm/hgt.png  the grey tournament tiles of the ring floor (each tile one of four flat greys)
  ring_emblem.png    R the charcoal T, G the neon tube inside its edge, B the border course (the band of tiles along
                     the ring's edge): a decal over the 20 x 20 ring floor (row 0 = Ogre z -10, the far side from
                     the hero's spawn, so the T is upright for the hero)

Usage: blender --background --factory-startup --python arena_textures.py -- <repo root> [--preview <folder>]
"""
import bpy
import numpy as np
import os
import sys

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
if not args:
    raise SystemExit("usage: blender --background --python arena_textures.py -- <repo root> [--preview <folder>]")
ROOT = args[0]
OUT = os.path.join(ROOT, "media", "tumbu", "arena")
PREVIEW = args[args.index("--preview") + 1] if "--preview" in args else None

# --- coliseum sandstone -----------------------------------------------------------------------------------------
STONE_SIZE = 2048           # pixels
STONE_TILE = 4.8            # metres covered by the texture (UVs are laid out at this scale, arena_shapes.py)
STONE_ROWS = 6              # courses per tile (0.8 m each)
STONE_BLOCK = (1.3, 2.9)    # block length range in metres (2.2 in the mock-ups: a random length per block)
STONE_JOINT = 0.03          # half width of a joint (6 cm joints)
STONE_BEVEL = 0.07          # width of the rounded block edge
STONE_JITTER = 0.30         # how much a block face may sit deeper (fraction of the depth)
STONE_DEPTH = 0.12          # metres from a block face to the bottom of a joint (normal map and parallax)
STONE_TONES = ((0.86, 0.67, 0.44), (0.76, 0.57, 0.37), (0.81, 0.62, 0.40))
STONE_PATCH = (0.94, 0.79, 0.56)    # flat lighter patches (posterised, toon)
STONE_JOINT_COLOUR = (0.42, 0.31, 0.24)

# --- ring floor tiles -------------------------------------------------------------------------------------------
TILES_SIZE = 2048
TILES_TILE = 12.8           # 8 x 8 tiles of 1.6 m
TILES_COUNT = 8
TILES_JOINT = 0.012
TILES_BEVEL = 0.025
TILES_DEPTH = 0.03          # shallow: the robots' feet must still sit on the floor
TILES_BASE = ((0.40, 0.41, 0.43), (0.36, 0.37, 0.39))
TILES_SHADES = 0.07         # each tile is one of four flat shades around its base grey (the mock-up's +-12 %
                            # included the lighting's spread; +-7 % in the texture reads the same)
TILES_JOINT_COLOUR = (0.19, 0.20, 0.22)

# --- ring emblem ------------------------------------------------------------------------------------------------
EMBLEM_SIZE = 2048
EMBLEM_EXTENT = 20.0        # metres: the whole ring floor
EMBLEM_SCALE = 1.35
EMBLEM_TUBE = (0.17, 0.29)  # the neon tube runs this far inside the T's edge (metres)
EMBLEM_BORDER = 1.0         # width of the border course along the ring's edge (metres)
# the ring floor's corners in plan view (Blender x, y = the post positions of arena.mesh)
RING_CORNERS = [(-9.89, -5.03), (-0.12, -9.94), (10.0, -4.87), (9.88, 5.06), (-0.12, 9.94), (-10.0, 4.87)]


def log(message):
    print("[TEX] " + message, flush=True)


def periodic_noise(size, cells, rng, octaves=1, gain=0.5):
    """Value noise on a `cells` x `cells` lattice, smooth (quintic) and periodic over the image."""
    total = np.zeros((size, size), np.float32)
    amp, norm = 1.0, 0.0
    for o in range(octaves):
        c = cells * (2 ** o)
        lattice = rng.random((c, c)).astype(np.float32)
        t = (np.arange(size, dtype=np.float32) + 0.5) / size * c
        i0 = np.floor(t).astype(int) % c
        i1 = (i0 + 1) % c
        f = t - np.floor(t)
        f = f * f * f * (f * (f * 6 - 15) + 10)
        a = lattice[i0][:, i0] * (1 - f)[None, :] + lattice[i0][:, i1] * f[None, :]
        b = lattice[i1][:, i0] * (1 - f)[None, :] + lattice[i1][:, i1] * f[None, :]
        total += amp * (a * (1 - f)[:, None] + b * f[:, None])
        norm += amp
        amp *= gain
    return total / norm


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def normal_from_height(height, metres_per_pixel, depth):
    """Tangent-space normal: x along +u (right), y along +v (down the image), z out of the surface; periodic."""
    dx = (np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)) / (2 * metres_per_pixel) * depth
    dy = (np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)) / (2 * metres_per_pixel) * depth
    n = np.stack([-dx, -dy, np.ones_like(height)], axis=-1)
    return n / np.linalg.norm(n, axis=-1, keepdims=True)


def row_layout(rng):
    """Random block lengths per course, wrapped over the tile; vertical joints of neighbouring courses never meet."""
    rows = []
    for r in range(STONE_ROWS):
        for _ in range(1000):
            n = int(rng.integers(2, 4))
            lengths = rng.uniform(*STONE_BLOCK, size=n)
            lengths *= STONE_TILE / lengths.sum()
            if lengths.min() < STONE_BLOCK[0] * 0.9 or lengths.max() > STONE_BLOCK[1] * 1.1:
                continue
            start = rng.uniform(0, STONE_TILE)
            joints = np.sort((start + np.concatenate([[0], np.cumsum(lengths)[:-1]])) % STONE_TILE)
            neighbours = [rows[-1]] if rows else []
            if r == STONE_ROWS - 1:
                neighbours.append(rows[0])      # the last course meets the first one across the tile edge
            ok = True
            for other in neighbours:
                d = np.abs(joints[:, None] - other[None, :])
                d = np.minimum(d, STONE_TILE - d)
                if d.min() < 0.4:
                    ok = False
            if ok:
                rows.append(joints)
                break
        else:
            raise SystemExit("could not lay out course %d" % r)
    return rows


def sandstone():
    rng = np.random.default_rng(2026)
    S, T = STONE_SIZE, STONE_TILE
    mpp = T / S
    coord = (np.arange(S, dtype=np.float32) + 0.5) * mpp           # metres along u (columns) and v (rows)
    x = np.broadcast_to(coord[None, :], (S, S))
    y = np.broadcast_to(coord[:, None], (S, S))
    row_h = T / STONE_ROWS
    row = np.minimum((y / row_h).astype(int), STONE_ROWS - 1)
    dh = np.minimum(y - row * row_h, (row + 1) * row_h - y)          # distance to the horizontal joints

    layout = row_layout(rng)
    dv = np.full((S, S), 1e9, np.float32)
    block = np.zeros((S, S), int)
    for r, joints in enumerate(layout):
        mask = row == r
        xs = x[mask]
        d = np.abs(xs[:, None] - joints[None, :])
        d = np.minimum(d, T - d)
        dv[mask] = d.min(axis=1)
        k = (np.searchsorted(joints, xs, side='right') - 1) % len(joints)   # the block left of x (wraps)
        block[mask] = r * 8 + k
    # per-block randomness (periodic: one value per block of the tile)
    n_ids = STONE_ROWS * 8
    tone = rng.integers(0, len(STONE_TONES), n_ids)
    depth = rng.random(n_ids) * STONE_JITTER
    tilt = rng.normal(0, 0.15, (n_ids, 2))

    # rounded, slightly chipped corners: a soft minimum of the two distances, roughened by fine noise
    chip = (periodic_noise(S, 40, rng, octaves=1) - 0.5) * 0.022
    k = 0.08
    dist = np.minimum(dv, dh) - np.maximum(k - np.abs(dv - dh), 0.0) * 0.35 + chip
    edge = smoothstep(STONE_JOINT, STONE_JOINT + STONE_BEVEL, dist)
    # each block face sits at its own depth, with a slight tilt and a soft surface undulation
    face = 1.0 - depth[block] + 0.04 * (tilt[block, 0] * np.sin(x / T * 2 * np.pi * STONE_TILE / 2.4)
                                      + tilt[block, 1] * np.cos(y / row_h * np.pi))
    surface = (periodic_noise(S, 12, rng, octaves=3) - 0.5) * 0.06
    height = np.clip(0.06 + (face + surface - 0.06) * edge, 0.0, 1.0)

    # colour: block tone, flat lighter patches (posterised noise: toon, not photo), darker joints
    colour = np.array(STONE_TONES, np.float32)[tone[block]]
    patches = np.floor(periodic_noise(S, 5, rng, octaves=2) * 3) / 3
    colour = colour + (np.array(STONE_PATCH, np.float32) - colour) * (patches * 0.3)[..., None]
    joint = 1.0 - smoothstep(STONE_JOINT * 0.6, STONE_JOINT + STONE_BEVEL * 0.35, dist)
    colour = colour + (np.array(STONE_JOINT_COLOUR, np.float32) - colour) * joint[..., None]

    # moss patch field (the shader thresholds it): broad patches plus a finer break-up, mean about 0.5
    moss = periodic_noise(S, 2, rng, octaves=3) + (periodic_noise(S, 10, rng, octaves=2) - 0.5) * 0.25
    moss = (moss - moss.mean()) / (moss.std() * 8.0) + 0.5

    normal = normal_from_height(height, mpp, STONE_DEPTH)
    return colour, normal, np.stack([height, np.clip(moss, 0, 1)], axis=-1)


def ring_tiles():
    rng = np.random.default_rng(1606)
    S, T, N = TILES_SIZE, TILES_TILE, TILES_COUNT
    mpp = T / S
    tile = T / N
    coord = (np.arange(S, dtype=np.float32) + 0.5) * mpp
    x = np.broadcast_to(coord[None, :], (S, S))
    y = np.broadcast_to(coord[:, None], (S, S))
    fx, fy = x % tile, y % tile
    dist = np.minimum(np.minimum(fx, tile - fx), np.minimum(fy, tile - fy))
    ix, iy = (x // tile).astype(int) % N, (y // tile).astype(int) % N
    ids = iy * N + ix
    shade = rng.integers(0, 4, N * N)                       # one of four flat shades per tile
    level = 1.0 - TILES_SHADES + shade * (2 * TILES_SHADES / 3)
    mix = rng.random(N * N) * 0.5                           # a little of the second grey: tiles are not all one hue
    base = np.array(TILES_BASE, np.float32)
    colour = (base[0] + (base[1] - base[0]) * mix[ids][..., None]) * level[ids][..., None]
    edge = smoothstep(TILES_JOINT, TILES_JOINT + TILES_BEVEL, dist)
    joint = 1.0 - smoothstep(TILES_JOINT * 0.6, TILES_JOINT + TILES_BEVEL * 0.3, dist)
    colour = colour + (np.array(TILES_JOINT_COLOUR, np.float32) - colour) * joint[..., None]
    face = 1.0 - rng.random(N * N) * 0.1
    height = 0.05 + (face[ids] - 0.05) * edge
    normal = normal_from_height(height, mpp, TILES_DEPTH)
    return colour, normal, np.stack([height, np.zeros_like(height)], axis=-1)


def emblem():
    """The T (the robots' chest emblem: a bevelled bar and a tapering stem) and a neon tube inside its edge."""
    S, E = EMBLEM_SIZE, EMBLEM_EXTENT
    k = EMBLEM_SCALE
    bar = [(-1.9, 1.35), (1.9, 1.35), (1.55, 0.75), (-1.55, 0.75)]
    stem = [(-0.45, 0.75), (0.45, 0.75), (0.30, -1.75), (0.0, -2.05), (-0.30, -1.75)]
    # drawn in plan view: x right, "up" (the T's top) towards Ogre -z, which is row 0 of the image
    polys = [[(px * k, (py + 0.35) * k) for px, py in p] for p in (bar, stem)]
    coord = ((np.arange(S, dtype=np.float32) + 0.5) / S - 0.5) * E
    X = np.broadcast_to(coord[None, :], (S, S))
    Y = -np.broadcast_to(coord[:, None], (S, S))          # row 0 = the T's top side

    def inside_distance(poly):
        """Signed distance to a convex polygon's edges (positive inside); polygons listed clockwise or not."""
        d = np.full((S, S), 1e9, np.float32)
        pts = np.array(poly, np.float32)
        centre = pts.mean(axis=0)
        for i in range(len(pts)):
            a, b = pts[i], pts[(i + 1) % len(pts)]
            e = b - a
            n = np.array([e[1], -e[0]]) / np.linalg.norm(e)
            if np.dot(centre - a, n) < 0:
                n = -n
            d = np.minimum(d, (X - a[0]) * n[0] + (Y - a[1]) * n[1])
        return d

    d = np.maximum(inside_distance(polys[0]), inside_distance(polys[1]))
    px = E / S
    fill = smoothstep(-px, px, d)
    tube = smoothstep(EMBLEM_TUBE[0] - px, EMBLEM_TUBE[0] + px, d) * (1 - smoothstep(EMBLEM_TUBE[1] - px, EMBLEM_TUBE[1] + px, d))
    # the image's "up" (row 0) is Blender +y, the ring corners are in Blender x, y
    border = 1 - smoothstep(EMBLEM_BORDER - px, EMBLEM_BORDER + px, inside_distance(RING_CORNERS))
    return np.stack([fill, tube, border], axis=-1)


def save(name, rgb, folder):
    """Writes an 8-bit RGB PNG exactly as given (no colour management): row 0 of the array is the image's top."""
    h, w = rgb.shape[:2]
    img = bpy.data.images.new(name, w, h, alpha=False, float_buffer=False)
    img.colorspace_settings.name = 'Non-Color'
    rgba = np.ones((h, w, 4), np.float32)
    rgba[..., :rgb.shape[2]] = np.clip(rgb, 0, 1)
    img.pixels.foreach_set(np.flipud(rgba).ravel())     # Blender's pixels start at the bottom row
    path = os.path.join(folder, name + ".png")
    img.filepath_raw = path
    img.file_format = 'PNG'
    img.save()
    bpy.data.images.remove(img)
    log("wrote " + path)


def main():
    os.makedirs(OUT, exist_ok=True)
    colour, normal, hm = sandstone()
    save("sandstone_col", colour, OUT)
    save("sandstone_nrm", normal * 0.5 + 0.5, OUT)
    save("sandstone_hgt", np.concatenate([hm, np.zeros_like(hm[..., :1])], axis=-1), OUT)
    colour, normal, hm = ring_tiles()
    save("ringtiles_col", colour, OUT)
    save("ringtiles_nrm", normal * 0.5 + 0.5, OUT)
    save("ringtiles_hgt", np.concatenate([hm, np.zeros_like(hm[..., :1])], axis=-1), OUT)
    save("ring_emblem", emblem(), OUT)
    if PREVIEW:
        # the same maps tiled 2 x 2, to check the seams by eye
        for n in ("sandstone_col", "sandstone_nrm", "ringtiles_col"):
            img = bpy.data.images.load(os.path.join(OUT, n + ".png"))
            w, h = img.size
            px = np.array(img.pixels[:], np.float32).reshape(h, w, 4)
            save(n + "_2x2", np.flipud(np.tile(px, (2, 2, 1)))[::2, ::2, :3], PREVIEW)


main()
