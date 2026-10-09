OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Torch flames, fragment stage: an unlit toon flame in flat colours (a yellow core where the flame faces the camera,
// orange around it, a deeper orange tip), bright enough in the HDR image for the bloom to give it a glow.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform vec3 camPos;
    // shared: x = keyframe lamps (0..1), y = flame brightness, z = time (s), w = flicker amount
    uniform vec4 lampParams;
)

MAIN_PARAMETERS
IN(vec3 oWorldPos, TEXCOORD0)
IN(vec3 oNormal, TEXCOORD1)
IN(vec2 oFlame, TEXCOORD2)
MAIN_DECLARATION
{
    vec3 n = normalize(oNormal);
    vec3 v = normalize(camPos - oWorldPos);
    float facing = saturate(dot(n, v));
    float h = oFlame.x;
    // The core shrinks towards the tip: three flat tones with short soft edges.
    float core = smoothstep(0.52, 0.6, facing - h * 0.45);
    float tip = smoothstep(0.62, 0.7, h + (1.0 - facing) * 0.25);
    vec3 colour = mix(vec3(1.0, 0.42, 0.08), vec3(1.0, 0.86, 0.45), core);
    colour = mix(colour, vec3(0.95, 0.22, 0.04), tip * (1.0 - core));
    colour *= lampParams.y * oFlame.y;
    gl_FragColor = vec4(colour, smoothstep(0.0, 0.15, lampParams.x));
}
