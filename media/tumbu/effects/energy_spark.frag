OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Spark: a short streak of energy on a velocity-oriented billboard (billboard_type oriented_self), thick and hot at
// its head, thin at its tail, in the particle's colour; the particle's alpha fades it out.
#include <OgreUnifiedShader.h>

OGRE_UNIFORMS(
    // x = brightness (HDR: above 1 blooms), y = how white the head is (0..1)
    uniform vec4 sparkParams;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec4 oColour, TEXCOORD1)
MAIN_DECLARATION
{
    float across = abs(oUv.x * 2.0 - 1.0);
    float along = oUv.y;    // 0 = head (direction of travel), 1 = tail
    float width = mix(1.0, 0.15, along);
    float streak = (1.0 - smoothstep(width * 0.45, width, across)) * (1.0 - smoothstep(0.6, 1.0, along))
        * smoothstep(0.0, 0.08, along);
    vec3 colour = mix(oColour.rgb, vec3_splat(1.0), sparkParams.y * (1.0 - along));
    gl_FragColor = vec4(colour * streak * sparkParams.x * oColour.a, 1.0);
}
