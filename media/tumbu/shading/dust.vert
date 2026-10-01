OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Dust motes (Tumbu/ArenaDust): billboards in world space. Each billboard is rebuilt around its centre, so every
// mote gets its own size and a slow swirl; its colour is a random seed (dust.particle). The position in the
// sun's shadow map tells the fragment stage whether the mote is in sunlight.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wMat;
    uniform mat4 vpMat;
    uniform mat4 invViewMat;
    uniform mat4 texViewProj;
    uniform vec3 camPos;
    uniform float time;             // seconds, wrapped (time_0_x)
    uniform float billboardSize;    // particle_width of dust.particle
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec4 colour, COLOR)
IN(vec2 uv0, TEXCOORD0)
OUT(vec2 oUv, TEXCOORD0)
OUT(vec4 oLightSpacePos, TEXCOORD1)
OUT(vec4 oColour, TEXCOORD2)
MAIN_DECLARATION
{
    vec3 seed = colour.rgb;
    vec3 right = mul(invViewMat, vec4(1.0, 0.0, 0.0, 0.0)).xyz;
    vec3 up = mul(invViewMat, vec4(0.0, 1.0, 0.0, 0.0)).xyz;

    // Ogre places a point billboard's corner at centre + right * (u - 0.5) * size + up * (0.5 - v) * size.
    vec2 corner = vec2(uv0.x - 0.5, 0.5 - uv0.y);
    vec3 world = mul(wMat, vertex).xyz;
    vec3 centre = world - (right * corner.x + up * corner.y) * billboardSize;

    // Slow swirl: each mote loops on its own small path (4..10 s per loop, up to 0.25 units away).
    float t = time;
    vec3 speed = vec3(0.6, 0.45, 0.7) + seed * 0.9;
    vec3 phase = seed.zxy * 6.2831853;
    centre += vec3(sin(t * speed.x + phase.x), sin(t * speed.y + phase.y) * 0.6, cos(t * speed.z + phase.z)) * 0.25;

    // Size: most motes small, a few larger. Far motes grow so they always cover a few soft pixels (a 1-pixel mote
    // looks like a star), and lose brightness to match (below), so far dust reads as a faint shimmer.
    float distance = length(centre - camPos);
    float grow = max(1.0, distance / 6.0);
    float size = billboardSize * mix(0.4, 1.4, seed.x * seed.x) * grow;
    vec3 p = centre + (right * corner.x + up * corner.y) * size;

    gl_Position = mul(vpMat, vec4(p, 1.0));
    oLightSpacePos = mul(texViewProj, vec4(centre, 1.0));
    oUv = uv0;
    // Hidden right in front of the camera (big blobs) and beyond about 30 units.
    float fade = smoothstep(1.5, 3.0, distance) * (1.0 - smoothstep(25.0, 35.0, distance)) / grow;
    // Twinkle: brightness pulses slowly and out of step (flat specks turning in the light), plus a fixed
    // per-mote brightness so they are not all alike.
    float twinkle = 0.35 + 0.65 * (0.5 + 0.5 * sin(t * (1.5 + seed.y * 2.5) + seed.x * 6.2831853));
    float brightness = mix(0.3, 1.0, seed.z) * twinkle;
    oColour = vec4(seed, colour.a * fade * brightness);
}
