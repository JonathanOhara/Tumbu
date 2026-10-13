OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// The field's tall grass (Grass.cpp), vertex stage. Each vertex carries its blade's root, height and seed and its place
// along the blade (t: 0 at the root, 1 at the tip), so the blade bends from its root (TumbuGrass.h): wind gusts rolling
// across the field and a flutter, pushed aside by the robots' feet, by energy balls flying low over it and by the
// shockwaves of impacts; where an impact burnt the field the blade is flattened (and grass.frag chars it). The blades
// are already in world space.
#include <OgreUnifiedShader.h>
#include "TumbuGrass.h"

OGRE_UNIFORMS_BEGIN
    uniform mat4 wvpMat;
    uniform vec4 contactShadowA;
    uniform vec4 contactShadowB;
    uniform vec4 energyLightPos0;
    uniform vec4 energyLightPos1;
    uniform vec4 energyLightPos2;
    uniform vec4 energyLightPos3;
    uniform vec4 lampParams;
    // xy direction, z strength, w gust wavelength (metres)
    uniform vec4 windParams;
    // x robot radius, y energy ball radius, z strength, w how far above the blades an energy ball still pushes
    uniform vec4 pushParams;
    TUMBU_GRASS_UNIFORMS
OGRE_UNIFORMS_END

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec4 uv0, TEXCOORD0)     // t, across (-1 .. 1), tone, seed
IN(vec4 uv1, TEXCOORD1)     // root xyz, height
// Direct3D 11 matches the vertex outputs to the fragment inputs by their order: keep both lists the same
OUT(vec4 oUv, TEXCOORD0)
OUT(vec3 oWorldPos, TEXCOORD1)
OUT(vec3 oNormal, TEXCOORD2)
OUT(vec4 oExtra, TEXCOORD3)
MAIN_DECLARATION
{
    // burnt: the blade shrinks to a charred stub (its height and its bend with it)
    vec2 burnt = TUMBU_GRASS_BURNT(uv1.xz);
    float keep = 1.0 - 0.8 * burnt.x;
    vec3 p = vertex.xyz;
    p.y = uv1.y + (p.y - uv1.y) * keep;
    float pushed;
    vec4 bent = grassBend(p, uv1.xyz, uv1.w * keep, uv0.x, uv0.w, lampParams.z, windParams, pushParams,
        contactShadowA, contactShadowB, energyLightPos0, energyLightPos1, energyLightPos2, energyLightPos3,
        grassWave0, grassWave1, grassWaveWidth, pushed);
    gl_Position = mul(wvpMat, vec4(bent.xyz, 1.0));
    oWorldPos = bent.xyz;
    oNormal = normal;
    oUv = uv0;
    oExtra = vec4(bent.w, pushed, burnt.x, burnt.y);
}
