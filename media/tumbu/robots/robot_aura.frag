OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Ki aura shell, fragment stage (Saint Seiya cosmos with a soft DBZ touch): a glow that hugs the robot, strongest
// next to the body and fading out to the shell's rim, broken into flame tongues by noise scrolling upwards. Toon
// bands: a near-white inner edge, a light ki-coloured body and a faint darker fringe, so it still reads over the
// white arena floor. Alpha blended (an additive glow alone vanishes over bright ground).
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform vec3 camPos;
    uniform float time;
    // custom 0: x = opacity, y = growth (how far the glow reaches into the shell), z = width (vertex stage)
    uniform vec4 auraParams;
    // custom 1: rgb = ki colour, w = how fast the tongues rise
    uniform vec4 auraColour;
)

float auraHash(vec3 p)
{
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Value noise, 0..1.
float auraNoise(vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    float a = mix(mix(auraHash(i), auraHash(i + vec3(1.0, 0.0, 0.0)), f.x),
                  mix(auraHash(i + vec3(0.0, 1.0, 0.0)), auraHash(i + vec3(1.0, 1.0, 0.0)), f.x), f.y);
    float b = mix(mix(auraHash(i + vec3(0.0, 0.0, 1.0)), auraHash(i + vec3(1.0, 0.0, 1.0)), f.x),
                  mix(auraHash(i + vec3(0.0, 1.0, 1.0)), auraHash(i + vec3(1.0, 1.0, 1.0)), f.x), f.y);
    return mix(a, b, f.z);
}

MAIN_PARAMETERS
IN(vec3 oWorldPos, TEXCOORD0)
IN(vec3 oNormal, TEXCOORD1)
MAIN_DECLARATION
{
    vec3 n = normalize(oNormal);
    vec3 v = normalize(camPos - oWorldPos);
    // Back faces of the shell: facing the camera most right next to the body's silhouette, edge-on at the rim.
    float edge = saturate(abs(dot(n, v)) * 1.8);

    // Flame tongues: noise stretched upwards and scrolling up.
    vec3 q = oWorldPos * vec3(4.0, 2.2, 4.0) - vec3(0.0, time * auraColour.w * 2.2, 0.0);
    float tongues = 0.65 * auraNoise(q) + 0.35 * auraNoise(q * 2.1 + vec3(17.0, 5.0, 3.0));
    float f = edge * (0.45 + 0.95 * tongues) * auraParams.y;

    // Toon bands.
    float fringe = smoothstep(0.30, 0.36, f);
    float body = smoothstep(0.50, 0.56, f);
    float inner = smoothstep(0.92, 0.98, f);
    vec3 ki = auraColour.rgb;
    vec3 colour = mix(ki * 0.5, ki, body);
    colour = mix(colour, mix(ki, vec3_splat(1.0), 0.6), inner);
    float flicker = 0.9 + 0.1 * sin(time * 8.8);
    float alpha = (0.16 * fringe + 0.20 * body + 0.14 * inner) * auraParams.x * flicker;

    // The inner edge goes a little above 1 (HDR), so the bloom gives it a faint halo.
    gl_FragColor = vec4(colour * (1.0 + 0.6 * inner), saturate(alpha));
}
