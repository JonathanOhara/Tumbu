OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// The painted toon sky (Tumbu/ToonSky on a big box around the arena, Sky.cpp): the fragment stage works from the view
// direction, so the box's size and the camera's place in it do not matter.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wvpMat;
    uniform mat4 wMat;
    uniform vec3 camPos;
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
OUT(vec3 oDir, TEXCOORD0)
MAIN_DECLARATION
{
    gl_Position = mul(wvpMat, vertex);
    oDir = mul(wMat, vertex).xyz - camPos;
}
