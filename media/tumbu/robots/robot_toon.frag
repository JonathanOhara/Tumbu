OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Robot toon shading, fragment stage: diffuse, normal, specular and ambient-occlusion maps lit by the shared
// toon lighting (TumbuToon.h) in a single pass, plus the emissive glow map.
#include <OgreUnifiedShader.h>
#include "TumbuToon.h"

SAMPLER2D(diffuseMap, 0);
SAMPLER2D(normalMap, 1);
SAMPLER2D(specMap, 2);
SAMPLER2D(aoMap, 3);
SAMPLER2DSHADOW(shadowMap, 4);
SAMPLER2D(glowMap, 5);

OGRE_UNIFORMS(
    TUMBU_LIGHTING_UNIFORMS
    uniform vec4 matDif;
    uniform vec4 matSpec;
    uniform float matShininess;
    uniform vec3 camPos;
    // rgb: tint of the glow (multiplies the texture colour), w: strength (0 = no glow)
    uniform vec4 glowColour;
    // x: flare while a special attack charges (0..1, set per robot by Robot::updateEyeGlow)
    uniform vec4 glowBoost;
    // time wrapped to 0..2pi (slow pulse)
    uniform float glowTime;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec3 oWorldPos, TEXCOORD1)
IN(vec3 oNormal, TEXCOORD2)
IN(vec3 oTangent, TEXCOORD3)
IN(vec3 oBinormal, TEXCOORD4)
IN(vec4 oLightSpacePos, TEXCOORD5)
MAIN_DECLARATION
{
    vec3 nMap = texture2D(normalMap, oUv).xyz * 2.0 - 1.0;
    vec3 n = normalize(oTangent * nMap.x + oBinormal * nMap.y + oNormal * nMap.z);
    vec3 v = normalize(camPos - oWorldPos);

    vec4 diffuseTex = texture2D(diffuseMap, oUv);
    vec3 albedo = diffuseTex.rgb * matDif.rgb;
    vec3 specMask = texture2D(specMap, oUv).rgb * matSpec.rgb;
    float ao = texture2D(aoMap, oUv).r;
    float shadow = tumbuShadow(shadowMap, oLightSpacePos, dot(normalize(oNormal), sunDirection.xyz), shadowParams);

    vec3 colour = tumbuToon(albedo, n, v, ao, specMask, matShininess, shadow,
        sunDirection, sunColour, skyColour, groundColour, shadowColour, rimColour, toonParams);

    // Emissive glow (eyes, lights): the texture colour where the glow map is white. Not lit or shadowed,
    // and above 1 in the HDR buffer, so the bloom picks it up.
    float pulse = 0.85 + 0.15 * sin(glowTime);
    float glow = texture2D(glowMap, oUv).r * glowColour.w * (pulse + 2.0 * glowBoost.x);
    colour += diffuseTex.rgb * glowColour.rgb * glow;

    gl_FragColor = vec4(colour, diffuseTex.a);
}
