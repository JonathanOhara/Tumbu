OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Energy trail (Ogre RibbonTrail behind a ki ball): across the ribbon, a white-hot centre line inside a saturated
// ki-coloured band and a soft glow, in hard toon bands. The vertex alpha fades from 1 at the ball to 0 at the tail;
// the bands narrow with it, so the trail tapers in steps like a drawn anime trail instead of a smooth smear.
// UNDERLAY (first pass, alpha blended): a dark band under it, so it keeps its colour over bright surfaces.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    // x = brightness of the band, y = brightness of the centre line, z = glow strength, w = band width (0..1)
    uniform vec4 trailParams;
    // x = darkness of the underlay band (UNDERLAY pass, 0..1)
    uniform vec4 trailUnderlay;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec4 oColour, TEXCOORD1)
MAIN_DECLARATION
{
    float across = abs(oUv.x * 2.0 - 1.0);    // 0 in the middle of the ribbon, 1 at its edges (RibbonTrail: U runs across)
    float fade = saturate(oColour.a);
    vec3 ki = oColour.rgb;

    float bandEdge = trailParams.w * (0.35 + 0.65 * fade);
    float band = 1.0 - smoothstep(bandEdge - 0.06, bandEdge, across);
    float centreEdge = bandEdge * 0.35 * fade;
    float centre = 1.0 - smoothstep(centreEdge - 0.05, centreEdge, across);
    float glow = exp(-across * 3.0) * fade;

#ifdef UNDERLAY
    // Dark, saturated band under the bright one (alpha blended), so the trail keeps its colour over a sunlit wall.
    gl_FragColor = vec4(ki * 0.15, band * fade * trailUnderlay.x);
#else
    vec3 hot = mix(ki, vec3_splat(1.0), 0.5);
    vec3 colour = ki * (band * trailParams.x * fade + glow * trailParams.z) + hot * centre * trailParams.y;
    gl_FragColor = vec4(colour, 1.0);
#endif
}
