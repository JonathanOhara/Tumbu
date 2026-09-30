OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Bloom, step 1: keep only what is brighter than the threshold (glowing eyes, energy balls, hot highlights),
// downsampled from the HDR scene with a 4-tap box filter.
#include <OgreUnifiedShader.h>

SAMPLER2D(scene, 0);

OGRE_UNIFORMS(
    // x = threshold, y = soft knee, z = strength, w = unused
    uniform vec4 bloomParams;
    // 1 / size of the scene texture
    uniform vec4 sceneTexel;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec2 d = sceneTexel.xy;
    vec3 c = (texture2D(scene, oUv + vec2(-d.x, -d.y)).rgb + texture2D(scene, oUv + vec2(d.x, -d.y)).rgb +
              texture2D(scene, oUv + vec2(-d.x, d.y)).rgb + texture2D(scene, oUv + vec2(d.x, d.y)).rgb) * 0.25;

    // Soft threshold: a smooth knee instead of a hard cut, so the glow fades in.
    float brightness = max(c.r, max(c.g, c.b));
    float knee = bloomParams.y;
    float soft = clamp(brightness - bloomParams.x + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.0001);
    float contribution = max(soft, brightness - bloomParams.x) / max(brightness, 0.0001);

    gl_FragColor = vec4(c * contribution, 1.0);
}
