OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Bloom, upsample: a 3x3 tent filter of the coarser level (already holding every coarser level), plus this level of
// the downsample chain times its weight. Going up from 1/32 to half size, each level adds a narrower glow, so the
// result is a tight hot core with a wide soft falloff.
#include <OgreUnifiedShader.h>

SAMPLER2D(lower, 0);
SAMPLER2D(current, 1);

OGRE_UNIFORMS(
    // 1 / size of the coarser texture
    uniform vec4 lowerTexel;
    // x = this level: 1 = half size ... 5 = 1/32 (its coarser texture is then the 1/64 level), set by each material
    uniform vec4 bloomLevel;
    // Shared (lighting.object bloomLevelWeights, bloomRadius): the weights of the half-size to 1/16 levels; the 1/32
    // and 1/64 weights, 1 / the sum of all six, and the radius of the tent in texels of the coarser texture
    uniform vec4 bloomWeightsA;
    uniform vec4 bloomWeightsB;
)

#define TAP(x, y) texture2D(lower, oUv + vec2(x, y) * t).rgb

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec2 t = lowerTexel.xy * bloomWeightsB.w;
    vec3 up = (TAP(-1.0, -1.0) + TAP(1.0, -1.0) + TAP(-1.0, 1.0) + TAP(1.0, 1.0)
            + (TAP(0.0, -1.0) + TAP(-1.0, 0.0) + TAP(1.0, 0.0) + TAP(0.0, 1.0)) * 2.0
            + TAP(0.0, 0.0) * 4.0) * (1.0 / 16.0);
    float level = bloomLevel.x;
    float weight = level < 1.5 ? bloomWeightsA.x : (level < 2.5 ? bloomWeightsA.y : (level < 3.5 ? bloomWeightsA.z : (level < 4.5 ? bloomWeightsA.w : bloomWeightsB.x)));
    float lowerWeight = level > 4.5 ? bloomWeightsB.y : 1.0;    // the 1/32 pass reads the 1/64 level itself
    float scale = level < 1.5 ? bloomWeightsB.z : 1.0;          // the last pass divides by the sum of the weights
    gl_FragColor = vec4((texture2D(current, oUv).rgb * weight + up * lowerWeight) * scale, 1.0);
}
