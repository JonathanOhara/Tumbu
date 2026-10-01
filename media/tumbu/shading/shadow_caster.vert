OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Depth-only shadow caster: position only (the shadow map stores depth; the colour is not used).
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wvpMat;
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
MAIN_DECLARATION
{
    gl_Position = mul(wvpMat, vertex);
}
