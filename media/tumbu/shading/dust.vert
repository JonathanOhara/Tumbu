OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Dust motes (Tumbu/ArenaDust): billboards in world space; the position in the sun's shadow map tells the
// fragment stage whether the mote is in sunlight.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wMat;
    uniform mat4 vpMat;
    uniform mat4 texViewProj;
    uniform vec3 camPos;
)

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec4 colour, COLOR)
IN(vec2 uv0, TEXCOORD0)
OUT(vec2 oUv, TEXCOORD0)
OUT(vec4 oLightSpacePos, TEXCOORD1)
OUT(vec4 oColour, TEXCOORD2)
MAIN_DECLARATION
{
    vec4 world = mul(wMat, vertex);
    gl_Position = mul(vpMat, world);
    oLightSpacePos = mul(texViewProj, world);
    oUv = uv0;
    // Visible from about 3 to 10 units: closer motes would be big blobs, farther ones look like stars in the sky.
    float distance = length(world.xyz - camPos);
    oColour = vec4(colour.rgb, colour.a * smoothstep(1.5, 3.0, distance) * (1.0 - smoothstep(10.0, 16.0, distance)));
}
