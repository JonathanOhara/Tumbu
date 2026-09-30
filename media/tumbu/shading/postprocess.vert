OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Full-screen quad of the compositor passes.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wvpMat;
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec2 uv0, TEXCOORD0)
OUT(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    gl_Position = mul(wvpMat, vertex);
    oUv = uv0;
}
