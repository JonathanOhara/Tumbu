// Stone surfaces of the arena remake (env_toon.frag with TUMBU_STONE): normal map, parallax occlusion with toon
// self-shadows, moss. The texture sets come from scripts/blender/arena_textures.py; every map of a set comes from one
// height field, so colour, normal and height agree.
//
// The tangent frame is built per pixel from screen-space derivatives (no tangents in the mesh): T follows +u, B
// follows +v (down the texture), N is the surface normal. It is the same on Direct3D 11 and OpenGL, because the
// derivatives of the position and of the UVs change sign together.

#if defined(OGRE_HLSL)
// Sampling with explicit gradients: needed inside the parallax loop (its length varies per pixel, so the hardware
// cannot take derivatives there). HLSL_SM4Support.hlsl has no tex2Dgrad.
vec4 tumbuSampleGrad(sampler2D s, vec2 uv, vec2 dx, vec2 dy) { return s.t.SampleGrad(s.s, uv, dx, dy); }
#else
#define tumbuSampleGrad(s, uv, dx, dy) textureGrad(s, uv, dx, dy)
#endif

// Tangent frame from the derivatives of the world position and of the UVs (Schueler's cotangent frame).
void tumbuTangentFrame(vec3 p, vec2 uv, vec3 n, out vec3 t, out vec3 b)
{
    vec3 dp1 = dFdx(p);
    vec3 dp2 = dFdy(p);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, n);
    vec3 dp1perp = cross(n, dp1);
    t = dp2perp * duv1.x + dp1perp * duv2.x;
    b = dp2perp * duv1.y + dp1perp * duv2.y;
    float invmax = inversesqrt(max(max(dot(t, t), dot(b, b)), 1e-20));
    t *= invmax;
    b *= invmax;
}

// Parallax occlusion: marches the view ray down into the height field (1 = surface, 0 = depth below it) and returns
// the UV where it hits. viewTs is the direction towards the camera in tangent space (x along T, y along B, z along N).
// depth = how deep the height field goes, in UV units; steps = number of layers (0: no parallax).
vec2 tumbuParallax(sampler2D heightMap, vec2 uv, vec3 viewTs, float depth, float steps, vec2 dx, vec2 dy,
                   out float hitHeight)
{
    hitHeight = 1.0;
    if (steps < 1.0)
        return uv;
    float layer = 1.0 / steps;
    // UV change per layer: the view ray projected on the surface, scaled so that it reaches `depth` at height 0
    vec2 delta = -viewTs.xy / max(viewTs.z, 0.15) * depth * layer;
    vec2 cur = uv;
    float curLayer = 1.0;
    float h = tumbuSampleGrad(heightMap, cur, dx, dy).r;
    vec2 prevUv = cur;
    float prevH = h;
    float prevLayer = curLayer;
    for (int i = 0; i < 48; i++)
    {
        if (float(i) >= steps || h >= curLayer)
            break;
        prevUv = cur;
        prevH = h;
        prevLayer = curLayer;
        cur += delta;
        curLayer -= layer;
        h = tumbuSampleGrad(heightMap, cur, dx, dy).r;
    }
    // refine between the last two layers (linear intersection of the ray with the height profile)
    float after = h - curLayer;
    float before = prevH - prevLayer;
    float w = after / min(after - before, -1e-4);
    w = saturate(w);
    hitHeight = mix(curLayer, prevLayer, w);
    return mix(cur, prevUv, w);
}

// Self-shadow: from the hit point, a short march towards the sun; 0 where a higher stone blocks the sun (a hard toon
// shadow, no gradient), 1 where it is lit. sunTs is the direction towards the sun in tangent space.
float tumbuParallaxShadow(sampler2D heightMap, vec2 uv, float height, vec3 sunTs, float depth, float steps,
                          vec2 dx, vec2 dy)
{
    if (steps < 1.0 || sunTs.z <= 0.0 || height > 0.98)
        return 1.0;
    float rise = (1.0 - height) / steps;
    vec2 delta = sunTs.xy / max(sunTs.z, 0.1) * depth * rise;
    vec2 cur = uv;
    float curH = height;
    for (int i = 0; i < 16; i++)
    {
        if (float(i) >= steps)
            break;
        cur += delta;
        curH += rise;
        if (tumbuSampleGrad(heightMap, cur, dx, dy).r > curH + 0.02)
            return 0.0;
    }
    return 1.0;
}
