OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Robot toon shading, vertex stage: world-space position and tangent frame for the normal map.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wMat;
    uniform mat4 wvpMat;
    uniform mat4 texViewProj;
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
    oLightSpacePos = mul(texViewProj, worldPos);    // position in the sun's shadow map
    oUv = uv0;
    // Robot parts are scaled uniformly, so the world matrix also transforms directions.
    oNormal = mul(wMat, vec4(normal, 0.0)).xyz;
    oTangent = mul(wMat, vec4(tangent.xyz, 0.0)).xyz;
    oBinormal = mul(wMat, vec4(cross(tangent.xyz, normal) * tangent.w, 0.0)).xyz;
}
