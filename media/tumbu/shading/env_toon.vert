OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Arena and coliseum toon shading, vertex stage. The meshes have two UV sets: texture and baked AO.
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
IN(vec2 uv1, TEXCOORD1)
OUT(vec2 oUv, TEXCOORD0)
OUT(vec3 oWorldPos, TEXCOORD1)
OUT(vec3 oNormal, TEXCOORD2)
OUT(vec4 oLightSpacePos, TEXCOORD3)
OUT(vec2 oAoUv, TEXCOORD4)
MAIN_DECLARATION
{
    gl_Position = mul(wvpMat, vertex);
    vec4 worldPos = mul(wMat, vertex);
    oWorldPos = worldPos.xyz;
    oNormal = mul(wMat, vec4(normal, 0.0)).xyz;
    // Position in the sun's shadow map, pushed out along the normal (normal-offset shadows: no stripes on the
    // surface, with a small depth bias that does not let light leak where two surfaces meet).
    oLightSpacePos = mul(texViewProj, vec4(oWorldPos + normalize(oNormal) * shadowOffset.x, 1.0));
    oUv = uv0;
    oAoUv = uv1;    // second UV set: the baked ambient occlusion (scripts/bake-arena-ao.ps1)
}
