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
    TUMBU_HERO_UNIFORMS
    uniform vec4 matDif;
    // x = reflection amount, y = amount on dark pixels, z = paint tint of the reflection ($metal, robots.material)
    uniform vec4 metalParams;
    uniform float matShininess;
    uniform vec3 camPos;
    // the camera's world axes (the hero fill light follows the camera)
    uniform vec3 camRight;
    uniform vec3 camUp;
    uniform vec3 camForward;
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
    float ao = texture2D(aoMap, oUv).r;
    float shadow = tumbuShadow(shadowMap, oLightSpacePos, dot(normalize(oNormal), sunDirection.xyz), shadowParams);

    // No round highlight: robots get the metal streak instead (tumbuMetal).
    vec3 colour = tumbuToon(albedo, n, v, ao, vec3_splat(0.0), matShininess, shadow,
        sunDirection, sunColour, skyColour, groundColour, shadowColour, rimColour, toonParams, aoParams);
    // Hero lighting: fill light from the camera's side and the rim kept in shadow, so the robot always reads.
    float lit = tumbuSunLit(n, sunDirection, toonParams, shadow);
    colour += tumbuHeroLight(albedo, n, v, ao, lit, camRight, camUp, camForward,
        heroFillColour, heroFillParams, heroRimParams, rimColour, toonParams, aoParams);
    // Painted metal: the toon sky reflection and the sun streak, per part ($metal).
    colour = tumbuMetal(colour, albedo, n, v, ao, lit, matShininess, metalParams, sunDirection, sunColour, skyColour,
        groundColour, metalEnv, metalShape, metalExtra, aoParams);
    // Coloured light from special attacks (energy balls, impacts).
    colour += albedo * TUMBU_ENERGY_LIGHTS(oWorldPos, n);

    // Emissive glow (eyes, lights): the texture colour where the glow map is white. Not lit or shadowed,
    // and above 1 in the HDR buffer, so the bloom picks it up.
    float pulse = 0.85 + 0.15 * sin(glowTime);
    float glow = texture2D(glowMap, oUv).r * glowColour.w * (pulse + 2.0 * glowBoost.x);
    colour += diffuseTex.rgb * glowColour.rgb * glow;

    gl_FragColor = vec4(colour, diffuseTex.a);
}
