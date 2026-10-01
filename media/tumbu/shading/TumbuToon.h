// Shared toon lighting of TUMBU (robots and arena). The values come from the "TumbuLighting" shared parameters,
// which Lighting.cpp fills from media/configuration/lighting.object and blends with the time of day.
//
// Put TUMBU_LIGHTING_UNIFORMS inside the shader's OGRE_UNIFORMS( ... ) and reference the shared parameters in
// the program definition (shared_params_ref TumbuLighting).

#define TUMBU_LIGHTING_UNIFORMS \
    uniform vec4 sunDirection; \
    uniform vec4 sunColour; \
    uniform vec4 skyColour; \
    uniform vec4 groundColour; \
    uniform vec4 shadowColour; \
    uniform vec4 rimColour; \
    uniform vec4 toonParams; \
    uniform vec4 shadowParams; \
    uniform vec4 aoParams; \
    uniform vec4 contactShadowA; \
    uniform vec4 contactShadowB; \
    uniform vec4 fogParams;

// sunDirection.xyz: unit vector towards the sun (world space)
// rimColour.w: rim strength
// toonParams: x = ramp threshold, y = ramp softness, z = rim power, w = specular softness
// aoParams: x = AO strength on the ambient light, y = on the sun, z = tint of occluded areas towards the shadow
//           colour (all 0..1), w = contact shadow strength
// contactShadowA/B: a robot's feet position (xyz) and its contact shadow radius (w, 0 = none)
// fogParams: x = fog start distance, y = density, z = maximum amount, w = brightness of the fog colour
//            (fog colour = sky ambient colour x w)
// shadowParams: x = 1 when the shadow map is in use, y = depth bias, z = 1 / shadow map size, w = filter radius
//               in texels

// One depth comparison: 1 = lit. Some GLSL compilers return a vec4 from shadow2D, HLSL a float; the splat
// accepts both.
#define TUMBU_SHADOW_CMP(map, coord) vec4_splat(shadow2D(map, coord)).x

// Sun visibility from the depth shadow map (Ogre integrated texture shadows): 1 = lit, 0 = in shadow.
// lightSpacePos = texture_viewproj_matrix * world position. 3x3 taps of hardware 2x2 PCF give a soft edge.
float tumbuShadow(sampler2DShadow shadowMap, vec4 lightSpacePos, float ndl, vec4 shadowParams)
{
    if (shadowParams.x < 0.5)
        return 1.0;

    vec3 pos = lightSpacePos.xyz / lightSpacePos.w;
#if !defined(OGRE_REVERSED_Z) && !defined(OGRE_HLSL)
    pos.z = pos.z * 0.5 + 0.5;    // OpenGL clip depth -1..1 -> 0..1
#endif
    // Outside the shadow map: lit.
    if (pos.x < 0.0 || pos.x > 1.0 || pos.y < 0.0 || pos.y > 1.0 || pos.z > 1.0)
        return 1.0;

    // More bias on surfaces that face away from the sun (grazing angles cause shadow acne).
    float depth = saturate(pos.z) - shadowParams.y * (1.0 + 4.0 * (1.0 - saturate(ndl)));
    float texelStep = shadowParams.z * shadowParams.w;
    float visible = 0.0;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
            visible += TUMBU_SHADOW_CMP(shadowMap, vec3(pos.xy + vec2(float(x), float(y)) * texelStep, depth));
    }
    return visible / 9.0;
}

// Soft dark disc under a robot (contact shadow): on upward-facing surfaces below its feet, fading with the
// distance from its centre and with the height of a jump. 0 = none, 1 = full.
float tumbuContact(vec3 p, vec3 n, vec4 robot)
{
    if (robot.w <= 0.0)
        return 0.0;
    float height = robot.y - p.y;
    float r = length(p.xz - robot.xz) / robot.w;
    float facingUp = smoothstep(0.5, 0.9, n.y);
    float below = smoothstep(-0.25, -0.05, height);
    float fade = 1.0 - saturate(height / 1.5);
    return facingUp * below * fade * (1.0 - smoothstep(0.2, 1.0, r));
}

// albedo: surface colour; n: unit world normal; v: unit vector towards the camera; ao: ambient occlusion
// (1 = open); specMask: specular colour (0 = matte); shadow: 1 = lit, 0 = in a cast shadow.
vec3 tumbuToon(vec3 albedo, vec3 n, vec3 v, float ao, vec3 specMask, float shininess, float shadow,
               vec4 sunDirection, vec4 sunColour, vec4 skyColour, vec4 groundColour, vec4 shadowColour,
               vec4 rimColour, vec4 toonParams, vec4 aoParams)
{
    // Toon ramp on the half-Lambert term: a soft band instead of a gradient that fades to black.
    float halfLambert = dot(n, sunDirection.xyz) * 0.5 + 0.5;
    float lit = smoothstep(toonParams.x - toonParams.y, toonParams.x + toonParams.y, halfLambert) * shadow;

    // The unlit side is the sun tinted by the shadow colour, never grey.
    vec3 direct = sunColour.rgb * mix(shadowColour.rgb, vec3_splat(1.0), lit);
    // Hemispheric ambient: ground colour below, sky colour above.
    vec3 hemi = mix(groundColour.rgb, skyColour.rgb, n.y * 0.5 + 0.5);
    // Ambient occlusion: full on the ambient light, partial on the sun, and occluded areas lean towards the
    // shadow colour (coloured, never grey).
    float aoAmbient = mix(1.0, ao, aoParams.x);
    float aoDirect = mix(1.0, ao, aoParams.y);
    vec3 aoTint = mix(vec3_splat(1.0), mix(shadowColour.rgb, vec3_splat(1.0), ao), aoParams.z);
    vec3 colour = albedo * (direct * aoDirect + hemi * aoAmbient) * aoTint;

    // Stylised specular: a highlight shape with a soft edge, only on the lit side.
    vec3 h = normalize(sunDirection.xyz + v);
    float spec = pow(max(dot(n, h), 0.0), shininess);
    spec = smoothstep(0.5 - toonParams.w, 0.5 + toonParams.w, spec) * lit;
    colour += spec * specMask * sunColour.rgb;

    // Rim light: brighter on the sun side, tinted by the surface so it does not look like a white halo.
    float rim = pow(1.0 - saturate(dot(n, v)), toonParams.z) * rimColour.w * (0.35 + 0.65 * lit);
    colour += rim * rimColour.rgb * (albedo * 0.5 + 0.5) * aoAmbient;

    return colour;
}
