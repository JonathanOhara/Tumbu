OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Bloom, step 2: separable 9-tap Gaussian blur along blurDirection (run horizontally, then vertically).
#include <OgreUnifiedShader.h>

SAMPLER2D(source, 0);

OGRE_UNIFORMS(
    // xy: direction (1,0) or (0,1), in texels
    uniform vec4 blurDirection;
    // 1 / size of the source texture
    uniform vec4 sourceTexel;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    // Linear-sampling Gaussian: 5 fetches cover 9 texels.
    vec2 texelStep = blurDirection.xy * sourceTexel.xy;
    vec4 c = texture2D(source, oUv) * 0.2270270270;    // alpha too: the god-ray texture carries the fog there
    c += (texture2D(source, oUv + texelStep * 1.3846153846) + texture2D(source, oUv - texelStep * 1.3846153846)) * 0.3162162162;
    c += (texture2D(source, oUv + texelStep * 3.2307692308) + texture2D(source, oUv - texelStep * 3.2307692308)) * 0.0702702703;
    gl_FragColor = c;
}
