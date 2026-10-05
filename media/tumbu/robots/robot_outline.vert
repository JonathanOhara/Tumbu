OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Robot outline ("inverted hull"): the back faces of the skinned part (RobotSkinning.h) pushed out along the normal by a
// fixed width in pixels.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 vpMat;
    uniform vec4 viewportSize;
    uniform float outlineWidth;
)

#include "RobotSkinning.h"

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec2 uv0, TEXCOORD0)
ROBOT_SKIN_INPUTS
OUT(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec4 row0, row1, row2;
    robotSkinMatrix(vec4(blendIndices), blendWeights, row0, row1, row2);
    vec4 clipPos = mul(vpMat, vec4(robotSkinPoint(row0, row1, row2, vertex), 1.0));
    vec3 worldNormal = robotSkinDirection(row0, row1, row2, normal);
    vec2 clipNormal = mul(vpMat, vec4(worldNormal, 0.0)).xy;
    float len = length(clipNormal);
    if (len > 0.0001)
        clipNormal /= len;
    // outlineWidth pixels, converted to clip space (times w, so it keeps its size at any distance).
    clipPos.xy += clipNormal * outlineWidth * 2.0 * viewportSize.zw * clipPos.w;
    gl_Position = clipPos;
    oUv = uv0;
}
