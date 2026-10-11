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
    // x = weight of this level, y = weight of the coarser texture (the last level's own weight when it is the 1/64
    // level, else 1), z = scale of the result (1 / the sum of all weights on the last pass, else 1)
    uniform vec4 bloomLevel;
)

#define TAP(x, y) texture2D(lower, oUv + vec2(x, y) * t).rgb

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec2 t = lowerTexel.xy;
    vec3 up = (TAP(-1.0, -1.0) + TAP(1.0, -1.0) + TAP(-1.0, 1.0) + TAP(1.0, 1.0)
            + (TAP(0.0, -1.0) + TAP(-1.0, 0.0) + TAP(1.0, 0.0) + TAP(0.0, 1.0)) * 2.0
            + TAP(0.0, 0.0) * 4.0) * (1.0 / 16.0);
    gl_FragColor = vec4((texture2D(current, oUv).rgb * bloomLevel.x + up * bloomLevel.y) * bloomLevel.z, 1.0);
}
