OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Screen-space ambient occlusion, at half resolution: rebuilds the world position and normal of each pixel from
// the depth buffer and checks how many points in a small hemisphere around it are inside geometry. Adds the
// contact darkening that the baked AO cannot have (robots on the floor, robots next to walls). Lighting.cpp
// sets the matrices before the pass renders (identifier 30); the final pass multiplies the scene by the result.
#include <OgreUnifiedShader.h>
#include "TumbuToon.h"

SAMPLER2D(depthMap, 0);

OGRE_UNIFORMS(
    TUMBU_LIGHTING_UNIFORMS
    // x = radius in world units, y = strength (0 = off), z = projection scale (pixels per unit at distance 1,
    // in texture coordinates), w = 1 when the depth texture is upside down
    uniform vec4 ssaoParams;
    uniform mat4 invViewProj;
    uniform vec4 camPos;
    uniform vec4 viewportSize;
)

vec3 worldAt(vec2 uv)
{
    float depth = texture2D(depthMap, uv).r;
    vec2 ndcXY = vec2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    if (ssaoParams.w > 0.5)
        ndcXY.y = -ndcXY.y;
#if !defined(OGRE_HLSL) && !defined(OGRE_REVERSED_Z)
    depth = depth * 2.0 - 1.0;
#endif
    vec4 world = mul(invViewProj, vec4(ndcXY, depth, 1.0));
    return world.xyz / world.w;
}

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec3 p = worldAt(oUv);
    float distance = length(p - camPos.xyz);
    if (ssaoParams.y <= 0.0 || distance > 120.0)
    {
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);    // off, or sky / far away: no occlusion
        return;
    }

    // Normal from the neighbouring pixels (the smaller difference on each axis, so edges stay clean).
    vec2 texel = 1.0 / viewportSize.xy;    // viewport_size is the half-size target of this pass
    vec3 px1 = worldAt(oUv + vec2(texel.x, 0.0)) - p;
    vec3 px2 = p - worldAt(oUv - vec2(texel.x, 0.0));
    vec3 py1 = worldAt(oUv + vec2(0.0, texel.y)) - p;
    vec3 py2 = p - worldAt(oUv - vec2(0.0, texel.y));
    vec3 dx = dot(px1, px1) < dot(px2, px2) ? px1 : px2;
    vec3 dy = dot(py1, py1) < dot(py2, py2) ? py1 : py2;
    vec3 n = normalize(cross(dx, dy));
    if (dot(n, camPos.xyz - p) < 0.0)
        n = -n;

    // 12 samples on a golden-angle spiral, rotated per pixel (the blur removes the noise).
    float radiusUv = ssaoParams.x * ssaoParams.z / distance;
    vec2 pixel = oUv * viewportSize.xy;
    float rotation = 6.2831853 * fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));
    float occlusion = 0.0;
    for (int i = 0; i < 12; i++)
    {
        float t = (float(i) + 0.5) / 12.0;
        float angle = rotation + float(i) * 2.3999632;
        vec2 offset = vec2(cos(angle), sin(angle)) * sqrt(t) * radiusUv;
        vec3 s = worldAt(oUv + offset) - p;
        float len = length(s);
        // Occluders in front of the surface, within the radius (farther ones fade out: no dark halos).
        float facing = max(dot(n, s / max(len, 0.0001)) - 0.15, 0.0);
        occlusion += facing * (1.0 - smoothstep(ssaoParams.x * 0.5, ssaoParams.x, len));
    }
    float ao = 1.0 - ssaoParams.y * occlusion / 12.0;
    gl_FragColor = vec4(vec3_splat(saturate(ao)), 1.0);
}
