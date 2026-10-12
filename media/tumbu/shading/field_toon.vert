OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// The field (field.mesh: the ground around the ring and the hills, from Arena.blend), vertex stage. One UV set, projected
// from above over the whole field: the field map and the baked AO (scripts/blender/arena_field.py).
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wMat;
    uniform mat4 wvpMat;
    uniform mat4 texViewProj;
    // x: normal offset of the shadow lookup in world units (Lighting.cpp, shadowNormalOffset)
    uniform vec4 shadowOffset;
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec2 uv0, TEXCOORD0)
// Direct3D 11 matches the vertex outputs to the fragment inputs by their order: keep both lists the same
OUT(vec2 oUv, TEXCOORD0)
OUT(vec3 oWorldPos, TEXCOORD1)
OUT(vec3 oNormal, TEXCOORD2)
OUT(vec4 oLightSpacePos, TEXCOORD3)
MAIN_DECLARATION
{
    gl_Position = mul(wvpMat, vertex);
    oWorldPos = mul(wMat, vertex).xyz;
    oNormal = mul(wMat, vec4(normal, 0.0)).xyz;
    oLightSpacePos = mul(texViewProj, vec4(oWorldPos + normalize(oNormal) * shadowOffset.x, 1.0));
    oUv = uv0;
}
