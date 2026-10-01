OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Arena and coliseum toon shading, fragment stage: same lighting as the robots (TumbuToon.h), no normal map, baked AO.
#include <OgreUnifiedShader.h>
#include "TumbuToon.h"

SAMPLER2D(diffuseMap, 0);
SAMPLER2DSHADOW(shadowMap, 1);
SAMPLER2D(aoMap, 2);

OGRE_UNIFORMS(
    TUMBU_LIGHTING_UNIFORMS
    uniform vec4 matDif;
    uniform vec4 matSpec;
    uniform float matShininess;
    uniform vec3 camPos;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec3 oWorldPos, TEXCOORD1)
IN(vec3 oNormal, TEXCOORD2)
IN(vec4 oLightSpacePos, TEXCOORD3)
IN(vec2 oAoUv, TEXCOORD4)
MAIN_DECLARATION
{
    vec3 n = normalize(oNormal);
    vec3 v = normalize(camPos - oWorldPos);
    vec3 albedo = texture2D(diffuseMap, oUv).rgb * matDif.rgb;
    float ao = texture2D(aoMap, oAoUv).r;
    float shadow = tumbuShadow(shadowMap, oLightSpacePos, dot(n, sunDirection.xyz), shadowParams);

    vec3 colour = tumbuToon(albedo, n, v, ao, matSpec.rgb, matShininess, shadow,
        sunDirection, sunColour, skyColour, groundColour, shadowColour, rimColour, toonParams, aoParams);

    gl_FragColor = vec4(colour, 1.0);
}
