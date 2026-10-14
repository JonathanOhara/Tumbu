OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// The field's flowers and reeds (Grass.cpp plants the models of art/arena/Flowers.blend), vertex stage: each plant bends
// from its root like a blade of the tall grass (TumbuGrass.h; Grass copies windParams and pushParams from
// Tumbu/FieldGrass), only stiffer (flowerParams) and as much as its set's "wind" in grass.object (uv0.y). Each vertex
// carries its plant's root, height and seed, how far up the plant it is (t) and its colour (Vertex Paint in Blender).
// The plants are already in world space.
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
    uniform vec4 windParams;    // the grass's (set by Grass.cpp)
    uniform vec4 pushParams;    // the grass's (set by Grass.cpp)
    // x how much of the grass's wind a flower takes, y how much of its push
    uniform vec4 flowerParams;
    TUMBU_GRASS_UNIFORMS
OGRE_UNIFORMS_END

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec4 colour, COLOR)
IN(vec4 uv0, TEXCOORD0)     // t, wind (its set's share of the wind), tone, seed
IN(vec4 uv1, TEXCOORD1)     // root xyz, height
// Direct3D 11 matches the vertex outputs to the fragment inputs by their order: keep both lists the same
OUT(vec4 oUv, TEXCOORD0)
OUT(vec3 oWorldPos, TEXCOORD1)
OUT(vec3 oNormal, TEXCOORD2)
OUT(vec4 oExtra, TEXCOORD3)
OUT(vec4 oColour, TEXCOORD4)
MAIN_DECLARATION
{
    vec4 wind = windParams;
    wind.z *= flowerParams.x * uv0.y;
    vec4 push = pushParams;
    push.z *= flowerParams.y;
    // burnt: the plant collapses towards its root (flower.frag chars it)
    vec2 burnt = TUMBU_GRASS_BURNT(uv1.xyz);
    float keep = 1.0 - 0.8 * burnt.x;
    vec3 p = uv1.xyz + (vertex.xyz - uv1.xyz) * vec3(1.0, keep, 1.0);
    float pushed;
    vec4 bent = grassBend(p, uv1.xyz, uv1.w * keep, uv0.x, uv0.w, lampParams.z, wind, push,
        contactShadowA, contactShadowB, energyLightPos0, energyLightPos1, energyLightPos2, energyLightPos3,
        grassWave0, grassWave1, grassWaveWidth, pushed);
    gl_Position = mul(wvpMat, vec4(bent.xyz, 1.0));
    oWorldPos = bent.xyz;
    oNormal = normal;
    oUv = uv0;
    oExtra = vec4(bent.w, pushed, burnt.x, burnt.y);
    oColour = colour;
}
