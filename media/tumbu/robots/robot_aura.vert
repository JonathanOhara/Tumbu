OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Ki aura shell, vertex stage: the robot part pushed out along its normals by a width in world units. The material
// draws only the back faces (like the outline), so the robot hides the shell everywhere except around its silhouette.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wMat;
    uniform mat4 vpMat;
    // custom 0 (Robot::updateAura): x = opacity, y = growth, z = shell width in world units
    uniform vec4 auraParams;
    // custom 1: w = how fast the tongues rise
    uniform vec4 auraColour;
    uniform float time;
)

// Value noise, 0..1 (same as robot_aura.frag).
float auraHash(vec3 p)
{
    p = fract(p * 0.3183099 + vec3(0.1, 0.2, 0.3));
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

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
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
OUT(vec3 oWorldPos, TEXCOORD0)
OUT(vec3 oNormal, TEXCOORD1)
MAIN_DECLARATION
{
    // Robot parts are scaled uniformly, so the world matrix also transforms directions.
    vec3 n = normalize(mul(wMat, vec4(normal, 0.0)).xyz);
    vec3 p = mul(wMat, vertex).xyz;
    // Flame tongues in the shape: the inflation swells and shrinks with slow noise climbing up the robot.
    float swell = auraNoise(p * vec3(3.0, 1.6, 3.0) - vec3(0.0, time * auraColour.w * 1.6, 0.0));
    p += n * auraParams.z * (0.5 + swell);
    gl_Position = mul(vpMat, vec4(p, 1.0));
    oWorldPos = p;
    oNormal = n;
}
