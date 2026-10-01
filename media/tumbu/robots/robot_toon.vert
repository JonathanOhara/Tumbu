OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Robot toon shading, vertex stage: world-space position and tangent frame for the normal map.
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
IN(vec4 tangent, TANGENT)
IN(vec2 uv0, TEXCOORD0)
OUT(vec2 oUv, TEXCOORD0)
OUT(vec3 oWorldPos, TEXCOORD1)
OUT(vec3 oNormal, TEXCOORD2)
OUT(vec3 oTangent, TEXCOORD3)
OUT(vec3 oBinormal, TEXCOORD4)
OUT(vec4 oLightSpacePos, TEXCOORD5)
MAIN_DECLARATION
{
    gl_Position = mul(wvpMat, vertex);
    vec4 worldPos = mul(wMat, vertex);
    oWorldPos = worldPos.xyz;
    oUv = uv0;
    // Robot parts are scaled uniformly, so the world matrix also transforms directions.
    oNormal = mul(wMat, vec4(normal, 0.0)).xyz;
    // Position in the sun's shadow map, pushed out along the normal (normal-offset shadows: no stripes on the
    // surface, with a small depth bias that does not let light leak where two surfaces meet).
    oLightSpacePos = mul(texViewProj, vec4(oWorldPos + normalize(oNormal) * shadowOffset.x, 1.0));
    oTangent = mul(wMat, vec4(tangent.xyz, 0.0)).xyz;
    oBinormal = mul(wMat, vec4(cross(tangent.xyz, normal) * tangent.w, 0.0)).xyz;
}
