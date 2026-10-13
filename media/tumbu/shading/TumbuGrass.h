// The motion of the field's plants (the tall grass, grass.vert, and the flowers, flower.vert; Grass.cpp builds both): a
// plant bends from its root, swayed by wind gusts rolling across the field and a flutter, and pushed aside by the robots'
// feet (the contact shadow positions) and by energy balls flying low over it (the energy lights).

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
// z strength, w how far above the plant an energy ball still pushes. Returns the bent position in xyz and the gust
// (0..1) in w; `pushed` = how hard it is pushed (0..1).
vec4 grassBend(vec3 p, vec3 root, float h, float t, float seed, float time, vec4 windParams, vec4 pushParams,
               vec4 footA, vec4 footB, vec4 ball0, vec4 ball1, vec4 ball2, vec4 ball3, out float pushed)
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
    // the bend grows with the square of t (the root stays put), and lowers the top so the plant keeps about its length
    vec2 disp = off * h * t * t;
    float lenOff = length(disp);
    p.xz += disp;
    p.y -= min(lenOff, h * 0.9) * 0.55;
    pushed = saturate(length(push));
    return vec4(p, gust);
}
