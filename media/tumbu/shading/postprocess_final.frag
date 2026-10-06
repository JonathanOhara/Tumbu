OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Final image: distance fog, god rays, bloom, lens flare, exposure, tone mapping (HDR scene -> screen),
// saturation, contrast
// and vignette.
#include <OgreUnifiedShader.h>

SAMPLER2D(scene, 0);
SAMPLER2D(bloom, 1);
SAMPLER2D(shafts, 2);
SAMPLER2D(ssao, 3);

OGRE_UNIFORMS(
    // x = exposure, y = saturation, z = contrast, w = vignette
    uniform vec4 postParams;
    // z = bloom strength (x, y: threshold and knee, used by the bright pass)
    uniform vec4 bloomParams;
    // Lens flare (Lighting::notifyMaterialRender): xy = sun position in texture coordinates, z = strength
    // times how much of the sun is visible (0 = hidden by a wall, behind the camera, moon, or flare off)
    uniform vec4 flareSun;
    uniform vec4 sunColour;
    uniform vec4 skyHorizon;       // the toon sky's horizon band: far terrain fades into it
    // x = start, y = density, z = maximum, w = brightness of the fog colour (sky ambient x w)
    uniform vec4 fogParams;
    uniform vec4 viewportSize;
    // Screen flash of an impact (EffectsManager::flash): rgb = colour, w = amount (0..1)
    uniform vec4 screenFlash;
)

// Leaves values below the knee untouched (the art keeps its colours) and rolls everything above it smoothly
// towards 1, so bright surfaces never clip to flat white.
vec3 softShoulder(vec3 x)
{
    const float knee = 0.6;
    vec3 over = max(x - vec3_splat(knee), vec3_splat(0.0));
    vec3 rolled = vec3_splat(knee) + (1.0 - knee) * (vec3_splat(1.0) - exp(-over / (1.0 - knee)));
    return mix(x, rolled, step(vec3_splat(knee), x));
}

// One soft ghost disc with a brighter rim.
float ghost(vec2 p, vec2 centre, float radius)
{
    float d = length(p - centre) / radius;
    return smoothstep(1.0, 0.75, d) * (0.4 + 0.6 * smoothstep(0.4, 0.95, d));
}

vec3 lensFlare(vec2 uv)
{
    vec2 sun = flareSun.xy;
    // flareSun.z already holds how much of the sun is visible (Lighting::setLensFlare). Fade out at the screen edge.
    vec2 edge = min(sun, vec2_splat(1.0) - sun);
    float visibility = smoothstep(-0.05, 0.1, min(edge.x, edge.y));
    float strength = flareSun.z * visibility;
    if (strength <= 0.0)
        return vec3_splat(0.0);

    float aspect = viewportSize.x / viewportSize.y;
    vec2 p = vec2(uv.x * aspect, uv.y);
    vec2 s = vec2(sun.x * aspect, sun.y);
    vec2 c = vec2(0.5 * aspect, 0.5);
    vec2 axis = c - s;    // the ghosts line up through the screen centre

    // Glow and a horizontal anamorphic streak around the sun.
    float dist = length(p - s);
    vec3 flare = sunColour.rgb * (exp(-dist * 9.0) * 0.6 + exp(-abs(p.y - s.y) * 160.0) * exp(-abs(p.x - s.x) * 2.5) * 0.35);

    // Ghosts: small tinted discs along the axis, on both sides of the centre.
    flare += vec3(1.0, 0.75, 0.45) * ghost(p, s + axis * 0.45, 0.035) * 0.25;
    flare += vec3(0.55, 0.85, 1.00) * ghost(p, s + axis * 0.80, 0.060) * 0.18;
    flare += vec3(0.80, 1.00, 0.70) * ghost(p, s + axis * 1.25, 0.025) * 0.30;
    flare += vec3(1.00, 0.60, 0.80) * ghost(p, s + axis * 1.60, 0.090) * 0.12;
    flare += vec3(0.60, 0.70, 1.00) * ghost(p, s + axis * 2.05, 0.045) * 0.20;
    return flare * strength;
}

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec4 shaftsAndFog = texture2D(shafts, oUv);    // rgb = god rays, a = distance fog
    vec3 colour = mix(texture2D(scene, oUv).rgb * texture2D(ssao, oUv).r, skyHorizon.rgb * fogParams.w, shaftsAndFog.a);
    colour = (colour + texture2D(bloom, oUv).rgb * bloomParams.z + shaftsAndFog.rgb + lensFlare(oUv)) * postParams.x;
    colour = softShoulder(colour);

    float luma = dot(colour, vec3(0.2126, 0.7152, 0.0722));
    colour = mix(vec3_splat(luma), colour, postParams.y);
    colour = saturate((colour - 0.5) * postParams.z + 0.5);

    vec2 fromCentre = oUv - vec2(0.5, 0.5);
    colour *= 1.0 - postParams.w * smoothstep(0.35, 0.9, length(fromCentre) * 1.4142);

    // Impact flash: the whole frame leans towards the flash colour for an instant (anime "white frame").
    colour = mix(colour, screenFlash.rgb, saturate(screenFlash.w));

    gl_FragColor = vec4(colour, 1.0);
}
