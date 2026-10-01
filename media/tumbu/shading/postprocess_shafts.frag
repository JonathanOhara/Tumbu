OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// God rays (volumetric sunlight), at half resolution: march from the camera to the visible surface and add the
// sunlight scattered by the air at every step that the sun's shadow map says is lit. Light coming through the
// coliseum's windows and gaps becomes visible shafts. The alpha channel carries the distance fog.
// Lighting.cpp sets the matrices before the pass renders.
#include <OgreUnifiedShader.h>
#include "TumbuToon.h"

SAMPLER2D(depthMap, 0);
SAMPLER2DSHADOW(shadowMap, 1);

OGRE_UNIFORMS(
    TUMBU_LIGHTING_UNIFORMS
    // x = strength, y = longest march in world units, z = forward scattering (0..0.9), w = steps
    uniform vec4 shaftParams;
    uniform mat4 invViewProj;     // camera clip space -> world
    uniform mat4 shadowViewProj;  // world -> shadow map (texture space)
    uniform vec4 camPos;          // xyz camera position, w = 1 when the depth texture is stored upside down
    uniform vec4 viewportSize;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
MAIN_DECLARATION
{
    // World position of the visible surface.
    float depth = texture2D(depthMap, oUv).r;
    vec2 ndcXY = vec2(oUv.x * 2.0 - 1.0, 1.0 - oUv.y * 2.0);
    if (camPos.w > 0.5)
        ndcXY.y = -ndcXY.y;
#if !defined(OGRE_HLSL) && !defined(OGRE_REVERSED_Z)
    depth = depth * 2.0 - 1.0;
#endif
    vec4 world = mul(invViewProj, vec4(ndcXY, depth, 1.0));
    world.xyz /= world.w;

    vec3 ray = world.xyz - camPos.xyz;

    // Distance fog (alpha, blended in the final pass): far geometry such as the mountains fades into the sky colour.
    // The sky itself (beyond 600 units) gets none.
    float distance = length(ray);
    float fog = distance < 600.0 ? fogParams.z * (1.0 - exp(-max(distance - fogParams.x, 0.0) * fogParams.y)) : 0.0;

    if (shaftParams.x <= 0.0 || shadowParams.x < 0.5)
    {
        gl_FragColor = vec4(0.0, 0.0, 0.0, fog);
        return;
    }

    float rayLength = min(length(ray), shaftParams.y);
    // Sky (nothing behind it): a shorter march, so the open sky does not turn milky.
    if (length(ray) > shaftParams.y * 4.0)
        rayLength = shaftParams.y * 0.35;
    vec3 dir = normalize(ray);

    // A different start offset per pixel turns banding into fine noise, which the blur removes.
    vec2 pixel = oUv * viewportSize.xy;
    float jitter = fract(52.9829189 * fract(dot(pixel, vec2(0.06711056, 0.00583715))));

    float steps = shaftParams.w;
    float stepLength = rayLength / steps;
    float lit = 0.0;
    for (int i = 0; i < 32; i++)
    {
        if (float(i) >= steps)
            break;
        vec3 p = camPos.xyz + dir * (stepLength * (float(i) + jitter));
        vec4 ls = mul(shadowViewProj, vec4(p, 1.0));
        vec3 s = ls.xyz / ls.w;
#if !defined(OGRE_REVERSED_Z) && !defined(OGRE_HLSL)
        s.z = s.z * 0.5 + 0.5;
#endif
        if (s.x < 0.0 || s.x > 1.0 || s.y < 0.0 || s.y > 1.0 || s.z > 1.0)
            lit += 0.25;    // outside the shadow map (open sky, beyond the arena): faint, or the sky turns milky
        else
            lit += TUMBU_SHADOW_CMP(shadowMap, vec3(s.xy, saturate(s.z) - shadowParams.y));
    }
    float litLength = lit / steps * rayLength;

    // Henyey-Greenstein phase: brighter when looking towards the sun (normalised to 1 sideways).
    float g = shaftParams.z;
    float cosTheta = dot(dir, sunDirection.xyz);
    float phase = (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5);
    phase /= (1.0 - g * g) / pow(1.0 + g * g, 1.5);
    phase = min(phase, 7.0);    // looking straight into the sun: at most 7x the sideways glow

    // Soft saturation: linear for thin haze, never more than maxAmount of the sunlight (looking straight into a
    // low sun would otherwise white out the screen).
    const float maxAmount = 2.0;
    float amount = maxAmount * (1.0 - exp(-litLength * phase * shaftParams.x / maxAmount));
    vec3 scattered = sunColour.rgb * amount;
    gl_FragColor = vec4(scattered, fog);
}
