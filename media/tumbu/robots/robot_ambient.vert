// Robot lighting, ambient pass. One source for Direct3D 11 (HLSL) and OpenGL (GLSL) via OgreUnifiedShader.h.
// Ported from the original 2011 Cg shader (general.cg).
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
