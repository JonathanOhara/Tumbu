// SMAA 1x (Jimenez et al., smaa/SMAA.hlsl, MIT licence) on Ogre's unified shaders: the porting macros SMAA asks for
// (SMAA_CUSTOM_SL), written on top of OgreUnifiedShader.h so the same source compiles as HLSL (Direct3D 11) and GLSL.
// Every texture is a combined sampler, so its filtering comes from the material: linear for the colour, edges and
// area textures, point for the search texture (what SMAA's LinearSampler / PointSampler would be).
// The including file defines SMAA_INCLUDE_VS or SMAA_INCLUDE_PS (0/1) and declares the uniform smaaMetrics
// (viewport_size: width, height, 1/width, 1/height) before including this.

#define SMAA_CUSTOM_SL
#define SMAA_PRESET_HIGH
#define SMAA_RT_METRICS smaaMetrics.zwxy

#define SMAATexture2D(tex) sampler2D tex
#define SMAATexturePass2D(tex) tex
#define SMAASampleLevelZero(tex, coord) texture2DLod(tex, coord, 0.0)
#define SMAASampleLevelZeroPoint(tex, coord) texture2DLod(tex, coord, 0.0)
#define SMAASampleLevelZeroOffset(tex, coord, offset) texture2DLod(tex, (coord) + vec2(offset) * SMAA_RT_METRICS.xy, 0.0)
#define SMAASample(tex, coord) texture2D(tex, coord)
#define SMAASamplePoint(tex, coord) texture2D(tex, coord)
#define SMAASampleOffset(tex, coord, offset) texture2D(tex, (coord) + vec2(offset) * SMAA_RT_METRICS.xy)

#if defined(OGRE_HLSL)
#define SMAA_FLATTEN [flatten]
#define SMAA_BRANCH [branch]
#else
// GLSL names of the HLSL types and intrinsics SMAA.hlsl uses (saturate comes from OgreUnifiedShader.h).
#define SMAA_FLATTEN
#define SMAA_BRANCH
#define lerp(a, b, t) mix(a, b, t)
#define mad(a, b, c) ((a) * (b) + (c))
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define int2 ivec2
#define int3 ivec3
#define int4 ivec4
#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4
#endif

#include "SMAA.hlsl"
