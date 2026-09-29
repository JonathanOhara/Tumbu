// Robot lighting, per-light pass (normal + specular maps). Ported from the original 2011 Cg shader (general.cg).
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wMat;
    uniform mat4 wvpMat;
    uniform vec4 spotlightDir;
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec4 tangent, TANGENT)
IN(vec2 uv0, TEXCOORD0)
OUT(vec2 oUv, TEXCOORD0)
OUT(vec4 oWorldPos, TEXCOORD1)
OUT(vec3 oNormal, TEXCOORD2)
OUT(vec3 oTangent, TEXCOORD3)
OUT(vec3 oBinormal, TEXCOORD4)
OUT(vec3 oSpotDir, TEXCOORD5)
MAIN_DECLARATION
{
    oWorldPos = mul(wMat, vertex);
    gl_Position = mul(wvpMat, vertex);
    oUv = uv0;
    oNormal = normal;
    oTangent = tangent.xyz;
    oBinormal = cross(tangent.xyz, normal) * tangent.w;
    oSpotDir = mul(wMat, spotlightDir).xyz;    // spotlight direction in world space
}
