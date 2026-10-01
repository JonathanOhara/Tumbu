OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Impact pieces on one billboard each (effects.particle): the particle colour is the ki colour (rgb) and its alpha is
// the life left (1 -> 0, a ColourFader), which drives the animation. One source, one variant per define:
//   FLASH  star-shaped hit flash: white-hot core, four long and four short rays, a ki-coloured glow (additive)
//   RING   shockwave: a ring that thins out as it grows (Scaler affector); UNDERLAY = its dark alpha pass
//   DOME   toon explosion: a banded ball (white core, ki body) that erodes into holes as it dies; UNDERLAY darkens
//   SMOKE  toon smoke puff: two-tone grey with a hard, noisy edge that erodes away (alpha blended)
//   DUST   a soft, faint puff of dust (alpha blended)
//   DEBRIS a dark chunk flung from the ground (alpha blended, oriented along its flight)
#include <OgreUnifiedShader.h>

#ifdef OGRE_HLSL
#define ANGLE(y, x) atan2(y, x)
#else
#define ANGLE(y, x) atan(y, x)
#endif

OGRE_UNIFORMS(
    uniform float time;         // seconds, wrapped (time_0_x)
    // x = brightness (HDR), y = size of the ring / ball (fraction of the half size), z = width / core size,
    // w = underlay darkness (0..1)
    uniform vec4 burstParams;
)

float burstHash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float burstNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(burstHash(i), burstHash(i + vec2(1.0, 0.0)), u.x),
               mix(burstHash(i + vec2(0.0, 1.0)), burstHash(i + vec2(1.0, 1.0)), u.x), u.y);
}

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec4 oColour, TEXCOORD1)
MAIN_DECLARATION
{
    vec2 p = oUv * 2.0 - 1.0;
    float r = length(p);
    float life = saturate(oColour.a);
    vec3 ki = oColour.rgb;
    vec3 hot = mix(ki, vec3_splat(1.0), 0.75);

#if defined(FLASH)
    float a = ANGLE(p.y, p.x);
    float reach = 1.0 - smoothstep(0.0, 1.0, r);
    float ray1 = (1.0 - smoothstep(0.0, 0.12 * reach, abs(sin(a * 2.0)))) * (1.0 - smoothstep(0.5, 0.95, r));
    float ray2 = (1.0 - smoothstep(0.0, 0.10 * reach, abs(sin(a * 2.0 + 0.785398)))) * (1.0 - smoothstep(0.25, 0.55, r));
    float core = 1.0 - smoothstep(burstParams.z, burstParams.z + 0.06, r);
    float glow = exp(-r * 4.0) * (1.0 - smoothstep(0.8, 1.0, r));
    vec3 colour = hot * (core + ray1 + ray2 * 0.7) * 2.0 + ki * glow * 1.2;
    gl_FragColor = vec4(colour * burstParams.x * life * life, 1.0);

#elif defined(RING)
    float radius = burstParams.y;
    float width = burstParams.z * life + 0.02;
    float ring = smoothstep(radius - width - 0.02, radius - width, r) * (1.0 - smoothstep(radius - 0.02, radius, r));
#ifdef UNDERLAY
    gl_FragColor = vec4(ki * 0.12, ring * life * burstParams.w);
#else
    // Bright leading edge, ki-coloured trailing band.
    float edge = smoothstep(radius - 0.06, radius - 0.03, r);
    vec3 colour = mix(ki, hot, edge) * ring;
    gl_FragColor = vec4(colour * burstParams.x * life, 1.0);
#endif

#elif defined(DOME)
    // Turbulent edge; holes open from the noise as the ball dies.
    float n = burstNoise(p * 3.5 + vec2(time * 0.7, -time * 0.9)) * 0.65 + burstNoise(p * 9.0 - vec2(time * 1.3, time)) * 0.35;
    float radius = burstParams.y * (0.92 + 0.12 * n);
    float body = 1.0 - smoothstep(radius - 0.03, radius, r);
    body *= step((1.0 - life) * 1.05, n);
    float coreRadius = burstParams.z * life;
    float core = 1.0 - smoothstep(coreRadius - 0.03, coreRadius, r + (n - 0.5) * 0.15);
#ifdef UNDERLAY
    // Dark ki-coloured shell (the toon shadow band of the explosion).
    float shell = body * smoothstep(radius - 0.22, radius - 0.12, r);
    gl_FragColor = vec4(ki * 0.15, max(shell, body * 0.35) * burstParams.w);
#else
    vec3 colour = ki * body * (0.8 + 0.6 * step(0.55, n)) + hot * core * 2.2;
    gl_FragColor = vec4(colour * burstParams.x * smoothstep(0.0, 0.15, life), 1.0);
#endif

#elif defined(SMOKE)
    float n = burstNoise(p * 3.0 + vec2(0.0, -time * 0.5)) * 0.7 + burstNoise(p * 7.0 + vec2(time * 0.3, 0.0)) * 0.3;
    float radius = burstParams.y * (0.85 + 0.25 * n);
    float puff = 1.0 - smoothstep(radius - 0.04, radius, r);
    puff *= step((1.0 - life) * 0.9, n);
    // Toon two-tone: lit top-left, shadowed bottom-right.
    float lit = step(0.0, p.y * 0.8 - p.x * 0.3 + (n - 0.5) * 0.4);
    vec3 colour = mix(ki * 0.62, ki, lit);
    gl_FragColor = vec4(colour, puff * burstParams.w);

#elif defined(DUST)
    // Soft, faint dust dragged over the floor (the Genki Dama gathering): no hard edge, fades with the life.
    float n = burstNoise(p * 2.5 + vec2(time * 0.4, 0.0));
    float puff = (1.0 - smoothstep(0.2, 1.0, r)) * (0.6 + 0.4 * n);
    gl_FragColor = vec4(ki, puff * life * burstParams.w);

#elif defined(DEBRIS)
    vec2 q = abs(p);
    float chunk = 1.0 - smoothstep(0.6, 0.8, max(q.x, q.y * 0.7) + burstHash(floor(oUv * 3.0)) * 0.2);
    gl_FragColor = vec4(ki, chunk * life * burstParams.w);
#endif
}
