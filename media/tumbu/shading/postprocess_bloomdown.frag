OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Bloom, downsample (the mip-chain bloom of Jimenez, "Next Generation Post Processing in Call of Duty: Advanced
// Warfare", SIGGRAPH 2014): 13 bilinear taps that cover 6x6 texels of the source, so no bright pixel falls between taps.
// BLOOM_PREFILTER, the first step from the HDR scene: the five 2x2 groups are weighted by 1 / (1 + luma) (the Karis
// average), so one very bright pixel cannot make the glow flicker as it moves, then the soft threshold keeps what glows.
#include <OgreUnifiedShader.h>

SAMPLER2D(source, 0);

// BEGIN/END instead of OGRE_UNIFORMS(...): GLSL does not allow #ifdef inside a macro's arguments.
OGRE_UNIFORMS_BEGIN
#ifdef BLOOM_PREFILTER
    // x = threshold, y = soft knee (z = strength, used by the final pass)
    uniform vec4 bloomParams;
#endif
    // 1 / size of the source texture
    uniform vec4 sourceTexel;
OGRE_UNIFORMS_END

#define TAP(x, y) texture2D(source, oUv + vec2(x, y) * t).rgb

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec2 t = sourceTexel.xy;
    vec3 a = TAP(-2.0, -2.0); vec3 b = TAP(0.0, -2.0); vec3 c = TAP(2.0, -2.0);
    vec3 d = TAP(-1.0, -1.0); vec3 e = TAP(1.0, -1.0);
    vec3 f = TAP(-2.0, 0.0);  vec3 g = TAP(0.0, 0.0);  vec3 h = TAP(2.0, 0.0);
    vec3 i = TAP(-1.0, 1.0);  vec3 j = TAP(1.0, 1.0);
    vec3 k = TAP(-2.0, 2.0);  vec3 l = TAP(0.0, 2.0);  vec3 m = TAP(2.0, 2.0);

#ifdef BLOOM_PREFILTER
    // The centre group counts half, the four corner groups an eighth each, each also by 1 / (1 + luma).
    vec3 g0 = (d + e + i + j) * 0.25;
    vec3 g1 = (a + b + f + g) * 0.25;
    vec3 g2 = (b + c + g + h) * 0.25;
    vec3 g3 = (f + g + k + l) * 0.25;
    vec3 g4 = (g + h + l + m) * 0.25;
    vec3 lumaWeights = vec3(0.2126, 0.7152, 0.0722);
    float w0 = 0.5 / (1.0 + dot(g0, lumaWeights));
    float w1 = 0.125 / (1.0 + dot(g1, lumaWeights));
    float w2 = 0.125 / (1.0 + dot(g2, lumaWeights));
    float w3 = 0.125 / (1.0 + dot(g3, lumaWeights));
    float w4 = 0.125 / (1.0 + dot(g4, lumaWeights));
    vec3 colour = (g0 * w0 + g1 * w1 + g2 * w2 + g3 * w3 + g4 * w4) / (w0 + w1 + w2 + w3 + w4);

    // Soft threshold: a smooth knee instead of a hard cut, so the glow fades in.
    float brightness = max(colour.r, max(colour.g, colour.b));
    float knee = bloomParams.y;
    float soft = clamp(brightness - bloomParams.x + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.0001);
    colour *= max(soft, brightness - bloomParams.x) / max(brightness, 0.0001);
#else
    vec3 colour = g * 0.125 + (a + c + k + m) * 0.03125 + (b + f + h + l) * 0.0625 + (d + e + i + j) * 0.125;
#endif
    gl_FragColor = vec4(colour, 1.0);
}
