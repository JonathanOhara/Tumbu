// The motion of the field's plants (the tall grass, grass.vert, and the flowers and reeds, flower.vert; Grass.cpp builds
// them): a plant bends from its root, swayed by wind gusts rolling across the field and a flutter, pushed aside by the
// robots' feet (the contact shadow positions), by energy balls flying low over it (the energy lights) and by the
// shockwaves of impacts; and where an impact burnt the field, flattened and charred until it grows back.

// What the impacts do to the field (EffectsManager::updateGrass, the "grass" layer of effects.object):
// grassWave0/1: x z of the centre, the front's distance from it, its strength now (0 = none); grassWaveWidth: their
// widths (x, y) and how many burnt patches there are (z: the loop below stops there, so a field with none costs
// nothing); grassBurn[i]: x z, radius, how burnt (0..1) plus the embers (0..1) above 1; grassBurnY[i].x: the height of
// its centre (the ground where the impact landed). A burn is a sphere: it marks the field, the plants, the ring floor,
// the walls and the seats it reaches (2026-10-14: a hit on the seats burnt the grass below and left the seats clean).
// The array sizes must match EffectsManager::GRASS_BURNS. Put TUMBU_GRASS_UNIFORMS in OGRE_UNIFORMS.
#define TUMBU_GRASS_UNIFORMS \
    uniform vec4 grassWave0; \
    uniform vec4 grassWave1; \
    uniform vec4 grassWaveWidth; \
    uniform vec4 grassBurn[8]; \
    uniform vec4 grassBurnY[8];

// How burnt a point is (x) and how much its embers glow (y), 0..1: full inside 70 % of a patch's radius (3D distance),
// fading out to its edge.
vec2 tumbuGrassBurnt(vec3 p, vec4 grassBurn[8], vec4 grassBurnY[8], float count)
{
    vec2 b = vec2_splat(0.0);
    for (int i = 0; i < 8; i++)
    {
        if (float(i) >= count)
            break;
        vec4 g = grassBurn[i];
        float inside = saturate((g.z - length(p - vec3(g.x, grassBurnY[i].x, g.y))) / (g.z * 0.3));
        b.x = max(b.x, inside * min(g.w, 1.0));
        b.y = max(b.y, inside * max(g.w - 1.0, 0.0));
    }
    return b;
}
#define TUMBU_GRASS_BURNT(p) tumbuGrassBurnt(p, grassBurn, grassBurnY, grassWaveWidth.z)

// A cheap value noise (0..1, about one feature per metre) to break up a scorch's edge on surfaces without the meadow's
// noise texture (the stone).
float tumbuScorchNoise(vec3 p)
{
    vec2 q = vec2(p.x + p.y * 0.7, p.z - p.y * 0.5) * 1.3;
    vec2 i = floor(q);
    vec2 f = q - i;
    f = f * f * (3.0 - 2.0 * f);
    float a = fract(sin(dot(i, vec2(127.1, 311.7))) * 43758.5453);
    float b = fract(sin(dot(i + vec2(1.0, 0.0), vec2(127.1, 311.7))) * 43758.5453);
    float c = fract(sin(dot(i + vec2(0.0, 1.0), vec2(127.1, 311.7))) * 43758.5453);
    float d = fract(sin(dot(i + vec2(1.0, 1.0), vec2(127.1, 311.7))) * 43758.5453);
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

// How far and which way a shockwave (w: x z centre, front distance, strength) bends a plant rooted at `root`.
vec2 grassWavePush(vec3 root, vec4 w, float width)
{
    if (w.w <= 0.0)
        return vec2_splat(0.0);
    vec2 d = root.xz - w.xy;
    float dist = max(length(d), 0.001);
    float x = (dist - w.z) / width;
    return d / dist * exp(-x * x) * w.w;
}

// How far (0..1) and which way a source at src.xyz (w = 0: none) pushes a plant rooted at `root`, `h` tall.
vec2 grassPush(vec3 root, float h, vec4 src, float radius, float reach)
{
    if (src.w <= 0.0)
        return vec2_splat(0.0);
    vec2 d = root.xz - src.xz;
    float dist = length(d);
    float above = src.y - root.y;
    float f = saturate(1.0 - dist / radius) * saturate(1.0 - (above - h) / reach);
    return d / max(dist, 0.001) * f;
}

// Bends the point `p` of a plant rooted at `root`, `h` tall, `t` of the way up (0 root, 1 top; `seed` 0..1 per plant).
// windParams: xy direction, z strength, w gust wavelength (metres); pushParams: x robot radius, y energy ball radius,
// z strength, w how far above the plant an energy ball still pushes; wave0/1 and waveWidth: the shockwaves
// (TUMBU_GRASS_UNIFORMS). Returns the bent position in xyz and the gust (0..1) in w; `pushed` = how hard it is pushed
// (0..1).
vec4 grassBend(vec3 p, vec3 root, float h, float t, float seed, float time, vec4 windParams, vec4 pushParams,
               vec4 footA, vec4 footB, vec4 ball0, vec4 ball1, vec4 ball2, vec4 ball3,
               vec4 wave0, vec4 wave1, vec4 waveWidth, out float pushed)
{
    vec2 wd = windParams.xy;
    // gusts: bands of stronger wind rolling along the wind direction, plus a little per-plant flutter
    float phase = dot(root.xz, wd) / windParams.w * 6.2832 - time * 1.4 + sin(dot(root.xz, vec2(-wd.y, wd.x)) * 0.21) * 1.5;
    float gust = sin(phase) * 0.5 + 0.5;
    gust = gust * gust;
    float flutter = sin(time * 3.7 + seed * 6.2832 + root.x * 1.7) * 0.12 + sin(time * 5.3 + root.z * 2.1) * 0.06;
    vec2 off = wd * windParams.z * (0.15 + gust) + vec2(flutter, flutter * 0.7) * windParams.z;
    vec2 push = grassPush(root, h, footA, pushParams.x, 1.0) + grassPush(root, h, footB, pushParams.x, 1.0);
    push += grassPush(root, h, ball0, pushParams.y, pushParams.w) + grassPush(root, h, ball1, pushParams.y, pushParams.w);
    push += grassPush(root, h, ball2, pushParams.y, pushParams.w) + grassPush(root, h, ball3, pushParams.y, pushParams.w);
    off += push * pushParams.z;
    // the shockwaves: a bend away from the impact at the rolling front
    vec2 wave = grassWavePush(root, wave0, waveWidth.x) + grassWavePush(root, wave1, waveWidth.y);
    off += wave;
    push += wave * 0.5;
    // the bend grows with the square of t (the root stays put), and lowers the top so the plant keeps about its length
    vec2 disp = off * h * t * t;
    float lenOff = length(disp);
    p.xz += disp;
    p.y -= min(lenOff, h * 0.9) * 0.55;
    pushed = saturate(length(push));
    return vec4(p, gust);
}
