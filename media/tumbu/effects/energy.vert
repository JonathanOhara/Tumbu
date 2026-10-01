OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Energy effects (orbs, sparks, rings): camera-facing billboards built by Ogre. Each particle's colour carries the
// attack's ki colour (rgb) and a random seed (alpha) for the fragment shader.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wvpMat;
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec4 colour, COLOR)
IN(vec2 uv0, TEXCOORD0)
OUT(vec2 oUv, TEXCOORD0)
OUT(vec4 oColour, TEXCOORD1)
MAIN_DECLARATION
{
    gl_Position = mul(wvpMat, vertex);
    oUv = uv0;
    oColour = colour;
}
