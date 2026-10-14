OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// The field, fragment stage: the wild meadow. The painted field map (arena_field.py) chooses between grass, earth and
// snow; their edges are broken up by a noise and stay crisp at any distance (toon), the earth has a thin shaded rim along
// the grass and the snow one along its lower edge; the steepest faces in the snow are bare earth. Every texture tiles at
// world scale (layerScale metres). Lit like the arena (TumbuToon.h): the toon ramp, the sun's shadow map, the baked AO
// (the walls' shade on the ground), contact shadows, energy lights, torches and neon.
#include <OgreUnifiedShader.h>
#include "TumbuToon.h"
#include "TumbuGrass.h"

SAMPLER2D(fieldMap, 0);     // r earth (0.5 = the edge), g snow, b tall grass height (read by the grass, not here)
SAMPLER2DSHADOW(shadowMap, 1);
SAMPLER2D(aoMap, 2);
SAMPLER2D(grassMap, 3);
SAMPLER2D(earthMap, 4);
SAMPLER2D(noiseMap, 5);     // r mid-scale breakup, g broad tone field
SAMPLER2D(snowMap, 6);

OGRE_UNIFORMS_BEGIN
    TUMBU_LIGHTING_UNIFORMS
    TUMBU_LAMP_UNIFORMS
    uniform vec3 camPos;
    uniform vec4 layerScale;    // tile size in metres: grass, earth, (unused), noise
    // x earth edge (threshold on the field map's r), y edge breakup (noise amount), z rim darkness, w broad tone
    // variation of the grass
    uniform vec4 fieldParams;
    // the earth on the hills beyond the walls: x from this distance to the centre, y over this width, z its texture
    // scale (larger cracks far away)
    uniform vec4 hillParams;
    uniform vec4 hillEarthTint;
    // the snow: x bare earth where the slope (1 - normal.y) is above this, y its breakup by the noise, z the width of the
    // shaded rim along the snow's lower edge (on the field map's g), w how dark that rim is
    uniform vec4 snowParams;
    uniform vec4 snowTile;      // x the snow's tile size in metres
    // where an impact burnt the field (TUMBU_GRASS_UNIFORMS): rgb the scorched ground, w how dark it gets (0..1)
    uniform vec4 scorchColour;
    // the glowing embers on it at first: rgb x w (HDR)
    uniform vec4 emberColour;
    TUMBU_GRASS_UNIFORMS
OGRE_UNIFORMS_END

// A hard edge, anti-aliased over about a pixel (the toon look at any distance).
float fieldStep(float edge, float x)
{
    float w = max(fwidth(x), 0.0001) * 0.75;
    return smoothstep(edge - w, edge + w, x);
}

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec3 oWorldPos, TEXCOORD1)
IN(vec3 oNormal, TEXCOORD2)
IN(vec4 oLightSpacePos, TEXCOORD3)
MAIN_DECLARATION
{
    vec3 n = normalize(oNormal);
    vec3 v = normalize(camPos - oWorldPos);
    vec2 w = oWorldPos.xz;
    vec4 field = texture2D(fieldMap, oUv);
    float breakup = texture2D(noiseMap, w / layerScale.w).r - 0.5;
    // a second, larger scale of the noise: flat lighter and darker regions, so the grass's repeats do not show
    float broad = texture2D(noiseMap, w / (layerScale.w * 4.3) + vec2(0.31, 0.77)).g;

    vec3 grass = texture2D(grassMap, w / layerScale.x).rgb;
    grass *= 1.0 + (fieldStep(0.5, broad) - 0.5) * fieldParams.w;
    float earthField = field.r + breakup * fieldParams.y;
    float earthMask = fieldStep(fieldParams.x, earthField);
    float rim = earthMask - fieldStep(fieldParams.x + 0.06, earthField);
    float hills = saturate((length(w) - hillParams.x) / hillParams.y);
    vec3 earth = texture2D(earthMap, w / (layerScale.y * mix(1.0, hillParams.z, hills))).rgb * (1.0 - rim * fieldParams.z);
    earth *= mix(vec3_splat(1.0), hillEarthTint.rgb, hills);
    vec3 albedo = mix(grass, earth, earthMask);
    float snowField = field.g + breakup * 0.4;
    float snowMask = fieldStep(0.5, snowField);
    float snowRim = snowMask - fieldStep(0.5 + snowParams.z, snowField);
    vec3 snow = texture2D(snowMap, w / snowTile.x).rgb * (1.0 - snowRim * snowParams.w);
    // the hills' earth shows through on the steepest faces
    float bare = fieldStep(snowParams.x, 1.0 - n.y + breakup * snowParams.y);
    albedo = mix(albedo, mix(snow, earth, bare), snowMask);
    // scorched where an impact burnt the field, its edge broken up by the noise
    vec2 burnt = TUMBU_GRASS_BURNT(oWorldPos);
    albedo = mix(albedo, scorchColour.rgb, scorchColour.w * fieldStep(0.35, burnt.x + breakup * 0.5));

    float ao = texture2D(aoMap, oUv).r;
    float shadow = tumbuShadow(shadowMap, oLightSpacePos, dot(n, sunDirection.xyz), shadowParams);
    // Contact shadows under the robots (Lighting::updateContactShadows).
    float contact = max(tumbuContact(oWorldPos, n, contactShadowA), tumbuContact(oWorldPos, n, contactShadowB)) * aoParams.w;
    ao *= 1.0 - contact;
    shadow *= 1.0 - contact * 0.6;

    vec3 colour = tumbuToon(albedo, n, v, ao, vec3_splat(0.0), 16.0, shadow,
        sunDirection, sunColour, skyColour, groundColour, shadowColour, rimColour, toonParams, aoParams);
    colour += albedo * TUMBU_ENERGY_LIGHTS(oWorldPos, n);
    colour += albedo * TUMBU_ARENA_LIGHTS(oWorldPos, n);
    // embers glowing in the scorch for the first seconds (the plants' tips glow too)
    if (burnt.y > 0.0)
        colour += emberColour.rgb * emberColour.w * tumbuGroundEmbers(oWorldPos, burnt);
    gl_FragColor = vec4(colour, 1.0);
}
