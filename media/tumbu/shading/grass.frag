OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// The field's tall grass, fragment stage: toon blades. The colour runs from a dark root tone to a light tip in hard
// bands (the toon blades of the grass remake), each clump between two tip tones; the gusts lighten the tips; the root is
// occluded; blades against the sun glow at the tips (translucency); burnt blades are charred, their tips glowing as
// embers at first (oExtra.zw, grass.vert). Lit like the arena (TumbuToon.h): the toon ramp, the sun's shadow map,
// energy lights, torches and neon.
#include <OgreUnifiedShader.h>
#include "TumbuToon.h"

SAMPLER2DSHADOW(shadowMap, 0);

OGRE_UNIFORMS_BEGIN
    TUMBU_LIGHTING_UNIFORMS
    TUMBU_LAMP_UNIFORMS
    uniform vec4 shadowOffset;
    uniform vec3 camPos;
    uniform mat4 texViewProj;
    uniform vec4 rootColour;
    uniform vec4 tipColour;
    uniform vec4 tipColour2;    // the second tip tone (clumps vary between the two)
    // x root occlusion, y translucency, z gust lightening, w tone bands along the blade (0 = smooth)
    uniform vec4 grassShade;
    // burnt: rgb the charred colour at the tip (darker at the root), w how strongly the embers glow
    uniform vec4 burntColour;
    uniform vec4 emberColour;
OGRE_UNIFORMS_END

MAIN_PARAMETERS
IN(vec4 oUv, TEXCOORD0)
IN(vec3 oWorldPos, TEXCOORD1)
IN(vec3 oNormal, TEXCOORD2)
IN(vec4 oExtra, TEXCOORD3)
MAIN_DECLARATION
{
    float t = oUv.x;
    vec3 v = normalize(camPos - oWorldPos);
    vec3 n = normalize(oNormal);
    float tb = grassShade.w > 0.0 ? floor(t * grassShade.w + 0.5) / grassShade.w : t;
    vec3 tip = mix(tipColour.rgb, tipColour2.rgb, oUv.z);
    vec3 albedo = mix(rootColour.rgb, tip, tb);
    albedo *= 1.0 + oExtra.x * grassShade.z * t;
    albedo = mix(albedo, burntColour.rgb * (0.45 + 0.55 * t), oExtra.z);
    float ao = mix(grassShade.x, 1.0, saturate(t * 1.4));
    // the shadow lookup in this stage: the blades bend in the vertex stage, after their normal was fixed
    vec4 lightSpacePos = mul(texViewProj, vec4(oWorldPos + n * shadowOffset.x, 1.0));
    float shadow = tumbuShadow(shadowMap, lightSpacePos, dot(n, sunDirection.xyz), shadowParams);
    vec3 colour = tumbuToon(albedo, n, v, ao, vec3_splat(0.0), 16.0, shadow,
        sunDirection, sunColour, skyColour, groundColour, shadowColour, rimColour, toonParams, aoParams);
    float back = pow(saturate(dot(-v, sunDirection.xyz)), 3.0);
    colour += albedo * sunColour.rgb * back * t * grassShade.y * shadow;
    colour += albedo * TUMBU_ENERGY_LIGHTS(oWorldPos, n);
    colour += albedo * TUMBU_ARENA_LIGHTS(oWorldPos, n);
    // embers: the burnt tips glow (above the bloom threshold), fading out
    colour += emberColour.rgb * emberColour.w * oExtra.w * t * t;
    gl_FragColor = vec4(colour, 1.0);
}
