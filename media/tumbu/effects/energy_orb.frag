OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Energy orb (ki ball, Genki Dama, Rasengan): a procedural ball on a camera-facing billboard, in hard toon bands:
//   white-hot core  ->  saturated ki-coloured ball with turning spiral streaks  ->  soft halo that blooms.
// The silhouette boils (the edge wobbles) and the core pulses, so a ball never looks like a flat sprite.
// UNDERLAY (first pass, alpha blended): a dark, saturated ring just outside the ball, so the orb still reads on
// the near-white arena floor in daylight, where an additive glow alone washes out.
#include <OgreUnifiedShader.h>

#ifdef OGRE_HLSL
#define ANGLE(y, x) atan2(y, x)
#else
#define ANGLE(y, x) atan(y, x)
#endif

OGRE_UNIFORMS(
    uniform float time;         // seconds, wrapped (time_0_x)
    // x = core radius, y = ball radius (fractions of the billboard half size), z = halo strength,
    // w = brightness of the ball (the core is 2.5x: only the core passes the bloom threshold)
    uniform vec4 orbShape;
    // x = spiral speed, y = spiral twist, z = edge wobble, w = underlay strength
    uniform vec4 orbMotion;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec4 oColour, TEXCOORD1)
MAIN_DECLARATION
{
    vec2 p = oUv * 2.0 - 1.0;
    float r = length(p);
    float a = ANGLE(p.y, p.x);
    float seed = oColour.a * 6.2831853;
    float t = time + seed * 3.0;
    vec3 ki = oColour.rgb;

    // Boiling edge: the radius wobbles with the angle and time.
    float wobble = 1.0 + orbMotion.z * (0.5 * sin(a * 6.0 + t * 7.0 + seed) + 0.3 * sin(a * 11.0 - t * 13.0) + 0.2 * sin(a * 17.0 + t * 19.0));
    float rr = r / wobble;
    float ball = orbShape.y;

#ifdef UNDERLAY
    // Dark ink ring hugging the ball's edge.
    float ring = smoothstep(ball - 0.06, ball, rr) * (1.0 - smoothstep(ball + 0.05, ball + 0.16, rr));
    gl_FragColor = vec4(ki * 0.12, ring * orbMotion.w);
#else
    // Two spirals turning in opposite directions (Rasengan), cut into hard streaks.
    float s1 = sin(a * 3.0 - rr * orbMotion.y * 9.0 + t * orbMotion.x * 6.0) * 0.5 + 0.5;
    float s2 = sin(-a * 5.0 - rr * orbMotion.y * 14.0 + t * orbMotion.x * 9.0 + seed) * 0.5 + 0.5;
    float streak = step(0.62, s1 * 0.6 + s2 * 0.4);

    // Toon bands: hard edges with one pixel-ish of softness.
    float inside = 1.0 - smoothstep(ball - 0.02, ball + 0.01, rr);
    float core = orbShape.x * (1.0 + 0.08 * sin(t * 23.0)) + (s1 - 0.5) * 0.04;
    float coreMask = 1.0 - smoothstep(core - 0.03, core + 0.01, rr);

    // Halo: bright near the edge, fading to nothing at the billboard border.
    float outside = max(rr - ball, 0.0);
    float halo = exp(-outside * 9.0) * (1.0 - smoothstep(0.75, 1.0, r)) * (1.0 - inside);
    // The halo breathes a little, out of step between balls.
    halo *= 0.85 + 0.15 * sin(t * 5.0);

    vec3 hot = mix(ki, vec3_splat(1.0), 0.75);
    vec3 colour = ki * halo * orbShape.z
        + inside * ki * orbShape.w * (0.8 + 0.7 * streak)
        + coreMask * hot * orbShape.w * 2.5;
    gl_FragColor = vec4(colour, 1.0);
#endif
}
