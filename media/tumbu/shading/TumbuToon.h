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
    uniform vec4 fogParams; \
    uniform vec4 energyLightPos0; \
    uniform vec4 energyLightPos1; \
    uniform vec4 energyLightPos2; \
    uniform vec4 energyLightPos3; \
    uniform vec4 energyLightColour0; \
    uniform vec4 energyLightColour1; \
    uniform vec4 energyLightColour2; \
    uniform vec4 energyLightColour3;

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
// energyLightPosN: xyz = position of a light cast by a special attack (energy ball, impact), w = radius (0 = off);
// energyLightColourN: rgb = colour x intensity (EffectsManager::updateLights, the four strongest)

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

// One energy light: a toon band (lit side / softer far side, not a smooth gradient) inside its radius, with a
// quadratic falloff so it reads as a glow around the ball rather than a flat disc.
vec3 tumbuEnergyLight(vec3 p, vec3 n, vec4 lightPos, vec4 lightColour)
{
    if (lightPos.w <= 0.0)
        return vec3_splat(0.0);
    vec3 toLight = lightPos.xyz - p;
    float dist = length(toLight);
    float falloff = saturate(1.0 - dist / lightPos.w);
    falloff *= falloff;
    float facing = dot(n, toLight / max(dist, 0.0001)) * 0.5 + 0.5;
    float band = mix(0.25, 1.0, smoothstep(0.45, 0.6, facing));
    return lightColour.rgb * falloff * band;
}

// Light from special attacks (up to four, see EffectsManager), to add to a lit colour as albedo x this.
vec3 tumbuEnergyLights(vec3 p, vec3 n, vec4 pos0, vec4 col0, vec4 pos1, vec4 col1, vec4 pos2, vec4 col2,
                       vec4 pos3, vec4 col3)
{
    return tumbuEnergyLight(p, n, pos0, col0) + tumbuEnergyLight(p, n, pos1, col1)
        + tumbuEnergyLight(p, n, pos2, col2) + tumbuEnergyLight(p, n, pos3, col3);
}

// Rim light on the side the sun misses, relative to the lit side (tumbuToon; robots raise it, see tumbuHeroLight).
#define TUMBU_RIM_SHADOW 0.35

// Robot "hero lighting" (characters lit apart from the scene, as in Genshin or Guilty Gear): robot shaders also put
// TUMBU_HERO_UNIFORMS in OGRE_UNIFORMS. The arena does not use them.
// heroFillColour: rgb = fill colour x strength (per keyframe)
// heroFillParams: xyz = fill direction as weights of the camera's right, up and backwards (towards the viewer)
//                 vectors; w = how much of the fill is left where the sun lights the surface
// heroRimParams: x = rim on the side the sun misses, relative to the lit side (replaces TUMBU_RIM_SHADOW)
// metalEnv / metalShape / metalExtra: robot metal, see tumbuMetal and lighting.object
#define TUMBU_HERO_UNIFORMS \
    uniform vec4 heroFillColour; \
    uniform vec4 heroFillParams; \
    uniform vec4 heroRimParams; \
    uniform vec4 metalEnv; \
    uniform vec4 metalShape; \
    uniform vec4 metalExtra;

// Shorthand for shaders that declare TUMBU_LIGHTING_UNIFORMS.
#define TUMBU_ENERGY_LIGHTS(p, n) tumbuEnergyLights(p, n, energyLightPos0, energyLightColour0, \
    energyLightPos1, energyLightColour1, energyLightPos2, energyLightColour2, energyLightPos3, energyLightColour3)

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
    float rim = pow(1.0 - saturate(dot(n, v)), toonParams.z) * rimColour.w
        * (TUMBU_RIM_SHADOW + (1.0 - TUMBU_RIM_SHADOW) * lit);
    colour += rim * rimColour.rgb * (albedo * 0.5 + 0.5) * aoAmbient;

    return colour;
}

// How much the sun lights a surface after the toon ramp and the cast shadow (the "lit" term of tumbuToon):
// 1 = lit, 0 = the side away from the sun or in a shadow.
float tumbuSunLit(vec3 n, vec4 sunDirection, vec4 toonParams, float shadow)
{
    float halfLambert = dot(n, sunDirection.xyz) * 0.5 + 0.5;
    return smoothstep(toonParams.x - toonParams.y, toonParams.x + toonParams.y, halfLambert) * shadow;
}

// Robot hero lighting, added to tumbuToon: a soft fill light that follows the camera (above and to one side, so
// the robot keeps a lit side and a shadow side wherever it stands) through the same toon ramp as the sun, strong
// where the sun does not reach and faint where it does; and the rim kept on the shadow side. Not shadowed: it is a
// character light, not a light in the scene. camRight / camUp / camForward: the camera's world axes; lit: tumbuSunLit.
vec3 tumbuHeroLight(vec3 albedo, vec3 n, vec3 v, float ao, float lit, vec3 camRight, vec3 camUp, vec3 camForward,
                    vec4 heroFillColour, vec4 heroFillParams, vec4 heroRimParams, vec4 rimColour, vec4 toonParams,
                    vec4 aoParams)
{
    vec3 l = normalize(camRight * heroFillParams.x + camUp * heroFillParams.y - camForward * heroFillParams.z);
    float halfLambert = dot(n, l) * 0.5 + 0.5;
    float band = smoothstep(toonParams.x - toonParams.y, toonParams.x + toonParams.y, halfLambert);
    float aoAmbient = mix(1.0, ao, aoParams.x);
    vec3 fill = heroFillColour.rgb * band * mix(1.0, heroFillParams.w, lit) * aoAmbient;

    // tumbuToon gives the shadow side TUMBU_RIM_SHADOW of the rim; this adds the rest up to heroRimParams.x.
    float rim = pow(1.0 - saturate(dot(n, v)), toonParams.z) * rimColour.w;
    float rimLift = max(heroRimParams.x - TUMBU_RIM_SHADOW, 0.0) * (1.0 - lit);
    return albedo * fill + rim * rimLift * rimColour.rgb * (albedo * 0.5 + 0.5) * aoAmbient;
}

// Robot metal (anime painted metal): the armour reflects a toon environment in flat bands (sky, a bright band, a dark
// horizon line, ground; colours of the current keyframe, so it follows the time of day and also shows in shadow),
// tinted by the paint; plus one sharp sun streak stretched along the part (vertical axis), cut into a hard toon edge.
// colour: the lit colour so far; metalParams (per part, $metal in robotNNN.material): x = reflection amount,
// y = amount on dark pixels (engraved lines stay crisp when low; black armour shines when high), z = how much the
// reflection takes the paint colour. lit: tumbuSunLit. n: the normal-mapped normal (reflection); ng: the smooth
// surface normal, for the streak (a hard toon edge on fine normal-map grooves sparkles).
vec3 tumbuMetal(vec3 colour, vec3 albedo, vec3 n, vec3 ng, vec3 v, float ao, float lit, float shininess, vec4 metalParams,
                vec4 sunDirection, vec4 sunColour, vec4 skyColour, vec4 groundColour, vec4 metalEnv,
                vec4 metalShape, vec4 metalExtra, vec4 aoParams)
{
    float luma = dot(albedo, vec3(0.299, 0.587, 0.114));
    float amount = mix(metalParams.y, metalParams.x, smoothstep(0.08, 0.3, luma));

    // The reflected direction's height picks the band (edges a little soft, so they do not shimmer).
    vec3 r = reflect(-v, n);
    vec3 env = groundColour.rgb * metalEnv.w;
    env = mix(env, groundColour.rgb * metalEnv.z, smoothstep(metalShape.z - 0.02, metalShape.z + 0.02, r.y));
    env = mix(env, skyColour.rgb * metalEnv.y + vec3_splat(0.1), smoothstep(metalShape.y - 0.02, metalShape.y + 0.02, r.y));
    env = mix(env, skyColour.rgb * metalEnv.x, smoothstep(metalShape.x - 0.03, metalShape.x + 0.03, r.y));
    // The sun's glint, where the sun reaches.
    float glint = smoothstep(metalExtra.y - 0.004, metalExtra.y + 0.004, dot(r, sunDirection.xyz));
    env += sunColour.rgb * glint * metalExtra.x * lit;

    float brightest = max(albedo.r, max(albedo.g, albedo.b));
    vec3 tint = mix(vec3_splat(1.0), albedo / max(brightest, 0.05), metalParams.z);
    // True metal reflects in its own colour and brightness: dark metal gives a darker reflection.
    tint *= mix(1.0, 0.35 + 0.65 * brightest, metalParams.z);
    float nv = saturate(dot(n, v));
    float fresnel = mix(metalExtra.z, 1.0, (1.0 - nv) * (1.0 - nv));
    float k = amount * fresnel * mix(1.0, ao, aoParams.x);
    colour = colour * (1.0 - metalExtra.w * k) + env * tint * k;

    // Sun streak: the normal and half vector without their vertical part, so the highlight runs along the part.
    vec3 h = normalize(sunDirection.xyz + v);
    vec3 na = vec3(ng.x, 0.0, ng.z);
    vec3 ha = vec3(h.x, 0.0, h.z);
    float lengthN = length(na);
    float s = pow(max(dot(na / max(lengthN, 0.0001), ha / max(length(ha), 0.0001)), 0.0), shininess * 0.6);
    s = smoothstep(0.55, 0.62, s) * smoothstep(0.1, 0.3, lengthN) * smoothstep(0.0, 0.25, dot(ng, sunDirection.xyz));
    float streak = s * lit * metalShape.w * (0.35 + 0.65 * max(metalParams.x, 0.4)) * mix(1.0, ao, aoParams.y);
    colour += streak * mix(sunColour.rgb, vec3_splat(1.0), 0.4) * mix(vec3_splat(1.0), albedo, 0.25);
    return colour;
}
