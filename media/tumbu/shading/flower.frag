OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// The field's flowers, fragment stage: the colours painted in Blender (Vertex Paint), each flower a little lighter or
// darker (tone), darker at the foot in the grass, petals glowing a little against the sun. Thin petals and leaves are seen
// from both sides, so the normal turns to the camera. Burnt plants are charred, glowing as embers at first (oExtra.zw).
// Lit like the grass (TumbuToon.h): the toon ramp, the sun's shadow map, energy lights, torches and neon.
#include <OgreUnifiedShader.h>
#include "TumbuToon.h"

SAMPLER2DSHADOW(shadowMap, 0);

OGRE_UNIFORMS_BEGIN
    TUMBU_LIGHTING_UNIFORMS
    TUMBU_LAMP_UNIFORMS
    uniform vec4 shadowOffset;
    uniform vec3 camPos;
    uniform mat4 texViewProj;
    // x occlusion at the foot (in the grass), y translucency against the sun, z tone variation between flowers
    uniform vec4 flowerShade;
    // burnt: the charred colour, and the embers' colour x glow (w)
    uniform vec4 burntColour;
    uniform vec4 emberColour;
OGRE_UNIFORMS_END

MAIN_PARAMETERS
IN(vec4 oUv, TEXCOORD0)
IN(vec3 oWorldPos, TEXCOORD1)
IN(vec3 oNormal, TEXCOORD2)
IN(vec4 oExtra, TEXCOORD3)
IN(vec4 oColour, TEXCOORD4)
MAIN_DECLARATION
{
    float t = oUv.x;
    vec3 v = normalize(camPos - oWorldPos);
    vec3 n = normalize(oNormal);
    n = dot(n, v) < 0.0 ? -n : n;
    vec3 albedo = oColour.rgb * (1.0 + (oUv.z - 0.5) * flowerShade.z);
    albedo = mix(albedo, burntColour.rgb * (0.45 + 0.55 * t), oExtra.z);
    float ao = mix(flowerShade.x, 1.0, saturate(t * 2.5));
    vec4 lightSpacePos = mul(texViewProj, vec4(oWorldPos + n * shadowOffset.x, 1.0));
    float shadow = tumbuShadow(shadowMap, lightSpacePos, dot(n, sunDirection.xyz), shadowParams);
    vec3 colour = tumbuToon(albedo, n, v, ao, vec3_splat(0.0), 16.0, shadow,
        sunDirection, sunColour, skyColour, groundColour, shadowColour, rimColour, toonParams, aoParams);
    float back = pow(saturate(dot(-v, sunDirection.xyz)), 3.0);
    colour += albedo * sunColour.rgb * back * t * flowerShade.y * shadow;
    colour += albedo * TUMBU_ENERGY_LIGHTS(oWorldPos, n);
    colour += albedo * TUMBU_ARENA_LIGHTS(oWorldPos, n);
    colour += emberColour.rgb * emberColour.w * oExtra.w * t * t;
    gl_FragColor = vec4(colour, 1.0);
}
