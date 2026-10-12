OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// The field's tall grass (Grass.cpp), vertex stage. Each vertex carries its blade's root, height and seed and its place
// along the blade (t: 0 at the root, 1 at the tip), so the blade bends from its root: wind gusts rolling across the
// field and a flutter, and pushed aside by the robots' feet (the contact shadow positions) and by energy balls flying
// low over it (the energy lights). The blades are already in world space.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS_BEGIN
    uniform mat4 wvpMat;
    uniform vec4 contactShadowA;
    uniform vec4 contactShadowB;
    uniform vec4 energyLightPos0;
    uniform vec4 energyLightPos1;
    uniform vec4 energyLightPos2;
    uniform vec4 energyLightPos3;
    uniform vec4 lampParams;
    // xy direction, z strength, w gust wavelength (metres)
    uniform vec4 windParams;
    // x robot radius, y energy ball radius, z strength, w how far above the blades an energy ball still pushes
    uniform vec4 pushParams;
OGRE_UNIFORMS_END

// How far (0..1) and which way a source at src.xyz (w = 0: none) pushes a blade rooted at `root`, `h` tall.
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

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec4 uv0, TEXCOORD0)     // t, across (-1 .. 1), tone, seed
IN(vec4 uv1, TEXCOORD1)     // root xyz, height
// Direct3D 11 matches the vertex outputs to the fragment inputs by their order: keep both lists the same
OUT(vec4 oUv, TEXCOORD0)
OUT(vec3 oWorldPos, TEXCOORD1)
OUT(vec3 oNormal, TEXCOORD2)
OUT(vec4 oExtra, TEXCOORD3)
MAIN_DECLARATION
{
    vec3 root = uv1.xyz;
    float h = uv1.w;
    float t = uv0.x;
    vec3 p = vertex.xyz;
    float time = lampParams.z;
    vec2 wd = windParams.xy;
    // gusts: bands of stronger wind rolling along the wind direction, plus a little per-blade flutter
    float phase = dot(root.xz, wd) / windParams.w * 6.2832 - time * 1.4 + sin(dot(root.xz, vec2(-wd.y, wd.x)) * 0.21) * 1.5;
    float gust = sin(phase) * 0.5 + 0.5;
    gust = gust * gust;
    float flutter = sin(time * 3.7 + uv0.w * 6.2832 + root.x * 1.7) * 0.12 + sin(time * 5.3 + root.z * 2.1) * 0.06;
    vec2 off = wd * windParams.z * (0.15 + gust) + vec2(flutter, flutter * 0.7) * windParams.z;
    vec2 push = grassPush(root, h, contactShadowA, pushParams.x, 1.0) + grassPush(root, h, contactShadowB, pushParams.x, 1.0);
    push += grassPush(root, h, energyLightPos0, pushParams.y, pushParams.w) + grassPush(root, h, energyLightPos1, pushParams.y, pushParams.w);
    push += grassPush(root, h, energyLightPos2, pushParams.y, pushParams.w) + grassPush(root, h, energyLightPos3, pushParams.y, pushParams.w);
    off += push * pushParams.z;
    // the bend grows with the square of t (the root stays put), and lowers the tip so the blade keeps about its length
    vec2 disp = off * h * t * t;
    float lenOff = length(disp);
    p.xz += disp;
    p.y -= min(lenOff, h * 0.9) * 0.55;
    gl_Position = mul(wvpMat, vec4(p, 1.0));
    oWorldPos = p;
    oNormal = normal;
    oUv = uv0;
    oExtra = vec4(gust, saturate(length(push)), 0.0, 0.0);
}
