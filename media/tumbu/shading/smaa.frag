OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// SMAA 1x, the three passes of Tumbu/SMAA (postprocess.compositor) on the tone-mapped image (display-space colours,
// as SMAA expects). SMAA_PASS is set per program in shading.program:
// 1 = luma edge detection (colour -> edges), 2 = blending weights (edges + the area and search lookup textures,
// made by Lighting.cpp from smaa/AreaTex.h and SearchTex.h), 3 = neighbourhood blending (colour + weights -> screen).
#include <OgreUnifiedShader.h>

SAMPLER2D(source, 0);
#if SMAA_PASS == 2
SAMPLER2D(areaTex, 1);
SAMPLER2D(searchTex, 2);
#elif SMAA_PASS == 3
SAMPLER2D(blendTex, 1);
#endif

OGRE_UNIFORMS(
    uniform vec4 smaaMetrics;
)

#define SMAA_INCLUDE_VS 0
#define SMAA_INCLUDE_PS 1
#include "smaa_common.h"

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec4 oOffset0, TEXCOORD1)
IN(vec4 oOffset1, TEXCOORD2)
IN(vec4 oOffset2, TEXCOORD3)
MAIN_DECLARATION
{
    vec4 offset[3];
    offset[0] = oOffset0;
    offset[1] = oOffset1;
    offset[2] = oOffset2;
#if SMAA_PASS == 1
    // Pixels without an edge are discarded (the target is cleared to 0 first).
    gl_FragColor = vec4(SMAALumaEdgeDetectionPS(oUv, offset, source), 0.0, 0.0);
#elif SMAA_PASS == 2
    // Pixel coordinates from oUv (SMAA's vertex shader would pass them as one more varying).
    gl_FragColor = SMAABlendingWeightCalculationPS(oUv, oUv * smaaMetrics.xy, offset, source, areaTex, searchTex, vec4_splat(0.0));
#else
    gl_FragColor = SMAANeighborhoodBlendingPS(oUv, offset[0], source, blendTex);
#endif
}
