OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Crescent energy wave (the kick): an arc whose curve points the way it flies, on a billboard oriented along its flight
// (billboard_type oriented_self: the top of the texture, uv.y = 0, is the head). Toon bands: a white-hot leading edge,
// a ki-coloured body that thins towards the tips, a soft glow. UNDERLAY (alpha blended): a dark band under it.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform float time;         // seconds, wrapped (time_0_x)
    // x = brightness (HDR), y = thickness of the crescent, z = glow strength, w = underlay darkness
    uniform vec4 crescentParams;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec4 oColour, TEXCOORD1)
MAIN_DECLARATION
{
    // p.y = -1 at the head (the way it flies), +1 at the tail; p.x across.
    vec2 p = oUv * 2.0 - 1.0;
    vec3 ki = oColour.rgb;

    // Inside the front circle and outside the one behind it: a crescent bulging forwards.
    float outer = length(p - vec2(0.0, 0.3)) - 1.15;
    float inner = length(p - vec2(0.0, 0.3 + crescentParams.y)) - 1.15;
    float body = (1.0 - smoothstep(-0.02, 0.02, outer)) * smoothstep(-0.02, 0.02, inner);
    // Thinner and dimmer towards the tips; a little shimmer along it.
    float tips = 1.0 - smoothstep(0.55, 0.95, abs(p.x));
    body *= tips;
    float edge = (1.0 - smoothstep(-0.09, -0.04, outer)) * body;    // leading edge band
    float glow = exp(-max(outer, 0.0) * 6.0) * smoothstep(-0.2, 0.1, inner) * tips * (1.0 - smoothstep(0.85, 1.0, length(p)));
    float shimmer = 0.85 + 0.15 * sin(p.x * 14.0 - time * 30.0);

#ifdef UNDERLAY
    gl_FragColor = vec4(ki * 0.12, body * crescentParams.w);
#else
    vec3 hot = mix(ki, vec3_splat(1.0), 0.75);
    vec3 colour = (ki * (body - edge) * shimmer + hot * edge * 1.8) * crescentParams.x + ki * glow * crescentParams.z;
    gl_FragColor = vec4(colour, 1.0);
#endif
}
