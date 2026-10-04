OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Robot outline colour: the part's own texture, darkened and tinted (coloured lines instead of black ones).
#include <OgreUnifiedShader.h>

SAMPLER2D(diffuseMap, 0);

OGRE_UNIFORMS(
    uniform vec4 matDif;
    uniform vec4 outlineColour;
    // time-of-day tint of the outlines (shared TumbuLighting parameter, lighting.object outlineTint)
    uniform vec4 outlineTint;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec3 albedo = texture2D(diffuseMap, oUv).rgb * matDif.rgb;
    gl_FragColor = vec4(albedo * outlineColour.rgb * outlineTint.rgb, 1.0);
}
