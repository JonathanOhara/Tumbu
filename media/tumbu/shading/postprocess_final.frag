// Final image: exposure, tone mapping (HDR scene -> screen), saturation, contrast and vignette.
#include <OgreUnifiedShader.h>

SAMPLER2D(scene, 0);

OGRE_UNIFORMS(
    // x = exposure, y = saturation, z = contrast, w = vignette
    uniform vec4 postParams;
)

// Leaves values below the knee untouched (the art keeps its colours) and rolls everything above it smoothly
// towards 1, so bright surfaces never clip to flat white.
vec3 softShoulder(vec3 x)
{
    const float knee = 0.6;
    vec3 over = max(x - vec3_splat(knee), vec3_splat(0.0));
    vec3 rolled = vec3_splat(knee) + (1.0 - knee) * (vec3_splat(1.0) - exp(-over / (1.0 - knee)));
    return mix(x, rolled, step(vec3_splat(knee), x));
}

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    vec3 colour = texture2D(scene, oUv).rgb * postParams.x;
    colour = softShoulder(colour);

    float luma = dot(colour, vec3(0.2126, 0.7152, 0.0722));
    colour = mix(vec3_splat(luma), colour, postParams.y);
    colour = saturate((colour - 0.5) * postParams.z + 0.5);

    vec2 fromCentre = oUv - vec2(0.5, 0.5);
    colour *= 1.0 - postParams.w * smoothstep(0.35, 0.9, length(fromCentre) * 1.4142);

    gl_FragColor = vec4(colour, 1.0);
}
