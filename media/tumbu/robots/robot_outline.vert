OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Robot outline ("inverted hull"): the back faces pushed out along the normal by a fixed width in pixels.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wMat;
    uniform mat4 wvpMat;
    uniform mat4 vpMat;
    uniform vec4 viewportSize;
    uniform float outlineWidth;
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec3 normal, NORMAL)
IN(vec2 uv0, TEXCOORD0)
OUT(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec4 clipPos = mul(wvpMat, vertex);
    vec3 worldNormal = mul(wMat, vec4(normal, 0.0)).xyz;
    vec2 clipNormal = mul(vpMat, vec4(worldNormal, 0.0)).xy;
    float len = length(clipNormal);
    if (len > 0.0001)
        clipNormal /= len;
    // outlineWidth pixels, converted to clip space (times w, so it keeps its size at any distance).
    clipPos.xy += clipNormal * outlineWidth * 2.0 * viewportSize.zw * clipPos.w;
    gl_Position = clipPos;
    oUv = uv0;
}
