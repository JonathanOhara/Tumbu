OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Robot shadow caster (Tumbu/RobotShadowCaster): the skinned part (RobotSkinning.h) into the sun's depth map. The robots
// need their own caster: Ogre's default one does not skin, so the shadow would keep the rest pose.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 vpMat;
)

#include "RobotSkinning.h"

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
ROBOT_SKIN_INPUTS
MAIN_DECLARATION
{
    vec4 row0, row1, row2;
    robotSkinMatrix(vec4(blendIndices), blendWeights, row0, row1, row2);
    gl_Position = mul(vpMat, vec4(robotSkinPoint(row0, row1, row2, vertex), 1.0));
}
