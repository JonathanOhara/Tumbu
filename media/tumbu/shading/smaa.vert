OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// SMAA vertex shader of the three passes (postprocess.compositor, Tumbu/SMAA): the full-screen quad plus the
// neighbour coordinates each pass reads. SMAA_PASS is set per program in shading.program: 1 = edge detection,
// 2 = blending weights, 3 = neighbourhood blending.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    uniform mat4 wvpMat;
    uniform vec4 smaaMetrics;
)

#define SMAA_INCLUDE_VS 1
#define SMAA_INCLUDE_PS 0
#include "smaa_common.h"

MAIN_PARAMETERS
IN(vec4 vertex, POSITION)
IN(vec2 uv0, TEXCOORD0)
OUT(vec2 oUv, TEXCOORD0)
OUT(vec4 oOffset0, TEXCOORD1)
OUT(vec4 oOffset1, TEXCOORD2)
OUT(vec4 oOffset2, TEXCOORD3)
MAIN_DECLARATION
{
    gl_Position = mul(wvpMat, vertex);
    oUv = uv0;
    vec4 offset[3];
    vec2 pixel;    // not passed on: the pixel shader computes it from oUv
    offset[1] = vec4_splat(0.0);
    offset[2] = vec4_splat(0.0);
#if SMAA_PASS == 1
    SMAAEdgeDetectionVS(uv0, offset);
#elif SMAA_PASS == 2
    SMAABlendingWeightCalculationVS(uv0, pixel, offset);
#else
    SMAANeighborhoodBlendingVS(uv0, offset[0]);
#endif
    oOffset0 = offset[0];
    oOffset1 = offset[1];
    oOffset2 = offset[2];
}
