OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// Dust motes: a soft dot that only shines where the sun reaches it (inside the god rays), additive.
#include <OgreUnifiedShader.h>
#include "TumbuToon.h"

SAMPLER2DSHADOW(shadowMap, 0);

OGRE_UNIFORMS(
    TUMBU_LIGHTING_UNIFORMS
    // x = brightness in sunlight, y = brightness in shadow
    uniform vec4 dustParams;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec4 oLightSpacePos, TEXCOORD1)
IN(vec4 oColour, TEXCOORD2)
MAIN_DECLARATION
{
    float dist = length(oUv - vec2(0.5, 0.5)) * 2.0;
    float disc = 1.0 - smoothstep(0.2, 1.0, dist);
    float lit = 0.0;
    if (shadowParams.x > 0.5)
    {
        vec3 s = oLightSpacePos.xyz / oLightSpacePos.w;
#if !defined(OGRE_REVERSED_Z) && !defined(OGRE_HLSL)
        s.z = s.z * 0.5 + 0.5;
#endif
        if (s.x >= 0.0 && s.x <= 1.0 && s.y >= 0.0 && s.y <= 1.0 && s.z <= 1.0)
            lit = TUMBU_SHADOW_CMP(shadowMap, vec3(s.xy, saturate(s.z) - shadowParams.y));
    }
    float brightness = mix(dustParams.y, dustParams.x, lit) * disc * oColour.a;
    gl_FragColor = vec4(sunColour.rgb * brightness, 1.0);
}
