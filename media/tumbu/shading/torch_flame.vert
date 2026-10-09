OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Torch flames (torches.mesh, torchFlameMaterial), vertex stage: the flame sways and flickers, and grows from its cup
// as the keyframe value lamps rises at dusk (0 by day: the flame is gone).
// UV set 0: u = the torch's flicker phase (0..1, arena_shapes.py), v = height in the flame (0 at the cup, 1 at the tip).
#include <OgreUnifiedShader.h>

// A torch's flicker (1 = steady): three sines, phase 0..1 per torch. Lighting::updateLamps computes the same for the
// torches' light, so a flame and its light pool flicker together.
float flameFlicker(float time, float phase, float amount)
{
    return 1.0 + amount * (0.5 * sin(time * 8.3 + phase * 6.2832) + 0.3 * sin(time * 13.7 + phase * 17.0)
                           + 0.2 * sin(time * 23.1 + phase * 31.0));
}

OGRE_UNIFORMS(
    uniform mat4 wMat;
    uniform mat4 vpMat;
    // shared: x = keyframe lamps (0..1), y = flame brightness, z = time (s), w = flicker amount
    uniform vec4 lampParams;
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec2 uv0, TEXCOORD0)
OUT(vec3 oWorldPos, TEXCOORD0)
OUT(vec3 oNormal, TEXCOORD1)
OUT(vec2 oFlame, TEXCOORD2)
MAIN_DECLARATION
{
    vec4 worldPos = mul(wMat, vertex);
    float h = uv0.y;
    float time = lampParams.z;
    float flicker = flameFlicker(time, uv0.x, lampParams.w);
    // Height: the flicker stretches it, and it grows out of the cup with the lamps value (the flame is 0.6 tall,
    // TORCH_FLAME_PROFILE in arena_shapes.py).
    float grow = smoothstep(0.0, 0.6, lampParams.x);
    worldPos.y += (flicker * grow - 1.0) * h * 0.6;
    // Sway: the tip moves most, two frequencies per axis.
    float sway = h * h;
    float phase = uv0.x * 6.2832;
    worldPos.x += sway * (0.045 * sin(time * 4.1 + phase) + 0.02 * sin(time * 9.7 + phase * 3.0));
    worldPos.z += sway * (0.045 * sin(time * 3.3 + phase * 2.0) + 0.02 * sin(time * 11.3 + phase));
    gl_Position = mul(vpMat, worldPos);
    oWorldPos = worldPos.xyz;
    oNormal = mul(wMat, vec4(normal, 0.0)).xyz;
    oFlame = vec2(h, flicker);
}
