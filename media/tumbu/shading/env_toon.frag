OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Arena and coliseum toon shading, fragment stage: same lighting as the robots (TumbuToon.h), baked AO.
// TUMBU_STONE (env_stone_ps): the arena remake's stone, with a normal map, parallax occlusion with toon self-shadows and
// moss (TumbuStone.h); TUMBU_EMBLEM adds the ring floor's decal (the charcoal T, its tube and the border course).
// TUMBU_NEON (env_neon_ps, the ring ropes) and the T's tube glow red at dusk and night (neonParams, Lighting).
#include <OgreUnifiedShader.h>
#include "TumbuToon.h"
#ifdef TUMBU_STONE
#include "TumbuStone.h"
#endif

SAMPLER2D(diffuseMap, 0);
SAMPLER2DSHADOW(shadowMap, 1);
SAMPLER2D(aoMap, 2);
#ifdef TUMBU_STONE
SAMPLER2D(normalMap, 3);
SAMPLER2D(heightMap, 4);    // r = height, g = moss patch field
#ifdef TUMBU_EMBLEM
SAMPLER2D(emblemMap, 5);    // r = T, g = tube, b = border course; over the 20 x 20 ring floor
#endif
#endif

// BEGIN/END instead of OGRE_UNIFORMS(...): GLSL does not allow #ifdef inside a macro's arguments (OpenGL rejected the
// whole program, Direct3D 11 accepted it).
OGRE_UNIFORMS_BEGIN
    TUMBU_LIGHTING_UNIFORMS
    uniform vec4 matDif;
    uniform vec4 matSpec;
    uniform float matShininess;
    uniform vec3 camPos;
#ifdef TUMBU_STONE
    // x: depth of the height field in UV units (metres / tile size), y: parallax steps (0 = normal map only),
    // z: distance where the parallax has faded out, w: self-shadow strength (0..1)
    uniform vec4 stoneParams;
    uniform vec4 detailParams;      // shared: x 0 = no parallax (the low setting: shadows off), 1 = parallax
    // moss: x threshold on the patch field, y height of the "foot" band (world units), z extra at the foot,
    // w extra on surfaces facing up
    uniform vec4 mossParams;
    // moss: x extra in the joints, y offset of the darker tone's threshold, z how much moss fills the joints (height)
    uniform vec4 mossParams2;
    uniform vec4 mossColour;
    uniform vec4 mossDeepColour;
#ifdef TUMBU_EMBLEM
    uniform vec4 emblemColour;      // the T
    uniform vec4 tubeColour;        // the tube by day (rgb)
    uniform vec4 borderTint;        // multiplies the border course
#endif
#endif
#if defined(TUMBU_EMBLEM) || defined(TUMBU_NEON)
    uniform vec4 neonParams;        // shared: x how much the neon glows (keyframe neon), y brightness of the core
    uniform vec4 neonColour;        // the neon's colour
#endif
OGRE_UNIFORMS_END

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
    vec2 uv = oUv;
#ifdef TUMBU_STONE
    vec3 ng = n;    // the surface's own normal (the normal map changes n)
    vec3 tng;
    vec3 btg;
    tumbuTangentFrame(oWorldPos, oUv, n, tng, btg);
    vec2 dx = dFdx(oUv);
    vec2 dy = dFdy(oUv);
    vec3 viewTs = vec3(dot(v, tng), dot(v, btg), dot(v, n));
    // parallax fades out with distance (the far walls only get the normal map)
    float pomFade = 1.0 - saturate(length(camPos - oWorldPos) / max(stoneParams.z, 0.001));
    // more layers at grazing angles (where layering would show), fewer when looking at the surface head-on
    float steps = floor(stoneParams.y * detailParams.x * pomFade * mix(2.0, 0.5, saturate(viewTs.z)) + 0.5);
    float hitHeight;
    uv = tumbuParallax(heightMap, oUv, viewTs, stoneParams.x * pomFade, steps, dx, dy, hitHeight);
    vec4 hm = tumbuSampleGrad(heightMap, uv, dx, dy);
    // BC5 normal maps keep x and y only: z is rebuilt (the surface's own side of the hemisphere)
    vec3 nTs;
    nTs.xy = tumbuSampleGrad(normalMap, uv, dx, dy).xy * 2.0 - 1.0;
    nTs.z = sqrt(saturate(1.0 - dot(nTs.xy, nTs.xy)));

    // moss: the patch field plus more at the foot of the walls, on surfaces facing up and in the joints
    // the moss field is sampled at the tile scale and at a larger, unrelated scale, so the patches do not repeat
    // with the 4.8 m texture
    float broad = tumbuSampleGrad(heightMap, uv * 0.29 + vec2(0.37, 0.11), dx * 0.29, dy * 0.29).g;
    float field = mix(hm.g, broad, 0.6) + saturate(1.0 - oWorldPos.y / max(mossParams.y, 0.001)) * mossParams.z
                + saturate(ng.y) * mossParams.w + (1.0 - hm.r) * mossParams2.x;
    float moss = step(mossParams.x, field) * mossColour.w;
    float mossDeep = step(mossParams.x + mossParams2.y, field);
    nTs = normalize(mix(nTs, vec3(0.0, 0.0, 1.0), moss * 0.7));
    n = normalize(tng * nTs.x + btg * nTs.y + n * nTs.z);
#endif
#ifdef TUMBU_STONE
    vec3 albedo = tumbuSampleGrad(diffuseMap, uv, dx, dy).rgb * matDif.rgb;
    albedo = mix(albedo, mix(mossColour.rgb, mossDeepColour.rgb, mossDeep), moss);
#ifdef TUMBU_EMBLEM
    // the ring decal, mapped from above over the 20 x 20 floor (the ring is at the origin)
    vec3 emblem = texture2D(emblemMap, oWorldPos.xz / 20.0 + 0.5).rgb;
    float joint = 1.0 - smoothstep(0.15, 0.4, hm.r);      // the tile joints show through the paint
    albedo = mix(albedo, emblemColour.rgb, emblem.r * (1.0 - joint));
    albedo = mix(albedo, tubeColour.rgb, emblem.g * (1.0 - joint));
    albedo *= mix(vec3_splat(1.0), borderTint.rgb, emblem.b);
#endif
#else
    vec3 albedo = texture2D(diffuseMap, uv).rgb * matDif.rgb;
#endif
    float ao = texture2D(aoMap, oAoUv).r;
    float shadow = tumbuShadow(shadowMap, oLightSpacePos, dot(n, sunDirection.xyz), shadowParams);
#ifdef TUMBU_STONE
    // the stones' own shadows towards the sun (hard, toon), where the parallax runs
    // (only where the sun still reaches the surface: in the shadow map's shade or facing away it changes nothing)
    vec3 sunTs = vec3(dot(sunDirection.xyz, tng), dot(sunDirection.xyz, btg), dot(sunDirection.xyz, ng));
    if (shadow > 0.0 && sunTs.z > 0.0 && steps >= 1.0)
    {
        float selfShadow = tumbuParallaxShadow(heightMap, uv, hitHeight, normalize(sunTs), stoneParams.x * pomFade,
                                               6.0, dx, dy);
        shadow *= mix(1.0, selfShadow, stoneParams.w * (1.0 - moss));
    }
#endif
    // Contact shadows under the robots (Lighting::updateContactShadows).
    float contact = max(tumbuContact(oWorldPos, n, contactShadowA), tumbuContact(oWorldPos, n, contactShadowB)) * aoParams.w;
    ao *= 1.0 - contact;
    shadow *= 1.0 - contact * 0.6;

    vec3 colour = tumbuToon(albedo, n, v, ao, matSpec.rgb, matShininess, shadow,
        sunDirection, sunColour, skyColour, groundColour, shadowColour, rimColour, toonParams, aoParams);
    // Coloured light from special attacks (energy balls, impacts).
    colour += albedo * TUMBU_ENERGY_LIGHTS(oWorldPos, n);

    // The neon: an unlit core, bright enough in the HDR image for the bloom to give it a halo.
#ifdef TUMBU_NEON
    colour = mix(colour, neonColour.rgb * neonParams.y, neonParams.x);
#endif
#ifdef TUMBU_EMBLEM
    colour = mix(colour, neonColour.rgb * neonParams.y, emblem.g * (1.0 - joint) * neonParams.x);
#endif

    gl_FragColor = vec4(colour, 1.0);
}
