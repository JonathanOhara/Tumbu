OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Robot toon shading, vertex stage: the part skinned by its bones (RobotSkinning.h), world-space position and tangent
// frame for the normal map.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 vpMat;
    uniform mat4 texViewProj;
    // x: normal offset of the shadow lookup in world units (Lighting.cpp, shadowNormalOffset)
    uniform vec4 shadowOffset;
)

#include "RobotSkinning.h"

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec4 tangent, TANGENT)
IN(vec2 uv0, TEXCOORD0)
ROBOT_SKIN_INPUTS
OUT(vec2 oUv, TEXCOORD0)
OUT(vec3 oWorldPos, TEXCOORD1)
OUT(vec3 oNormal, TEXCOORD2)
OUT(vec3 oTangent, TEXCOORD3)
OUT(vec3 oBinormal, TEXCOORD4)
OUT(vec4 oLightSpacePos, TEXCOORD5)
MAIN_DECLARATION
{
    vec4 row0, row1, row2;
    robotSkinMatrix(vec4(blendIndices), blendWeights, row0, row1, row2);
    oWorldPos = robotSkinPoint(row0, row1, row2, vertex);
    gl_Position = mul(vpMat, vec4(oWorldPos, 1.0));
    oUv = uv0;
    oNormal = robotSkinDirection(row0, row1, row2, normal);
    // Position in the sun's shadow map, pushed out along the normal (normal-offset shadows: no stripes on the
    // surface, with a small depth bias that does not let light leak where two surfaces meet).
    oLightSpacePos = mul(texViewProj, vec4(oWorldPos + normalize(oNormal) * shadowOffset.x, 1.0));
    oTangent = robotSkinDirection(row0, row1, row2, tangent.xyz);
    oBinormal = robotSkinDirection(row0, row1, row2, cross(tangent.xyz, normal) * tangent.w);
}
