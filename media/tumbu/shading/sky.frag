OGRE_NATIVE_GLSL_VERSION_DIRECTIVE
// The painted toon sky (arena remake, "painted bands"; replaces Caelum): hard colour bands from the horizon up, a
// yellow sun disc with a smooth glow, flat two-tone clouds, a crescent moon and stars at night. Everything follows
// the lighting rig: the colours are lighting.object keyframe values and the sun (or moon) is the light's direction.
#include <OgreUnifiedShader.h>

#ifndef OGRE_HLSL
#define atan2(y, x) atan(y, x)
#endif

OGRE_UNIFORMS(
    // shared (TumbuLighting, Lighting.cpp)
    uniform vec4 sunDirection;      // towards the sun (or the moon at night)
    uniform vec4 sunColour;
    uniform vec4 skyZenith;         // band colours: zenith, middle, horizon
    uniform vec4 skyMid;
    uniform vec4 skyHorizon;
    uniform vec4 cloudLit;          // w: cloud cover (0 none .. 1 overcast)
    uniform vec4 cloudShade;
    uniform vec4 skyParams;         // x: moon (1) or sun (0), y: stars, z: quality (1 clouds and twinkling stars), w: sun brightness
    uniform float time;
)

MAIN_PARAMETERS
IN(vec3 oDir, TEXCOORD0)
MAIN_DECLARATION
{
    vec3 dir = normalize(oDir);
    float el = asin(clamp(dir.y, -1.0, 1.0)) * 57.2958;    // elevation in degrees

    // Five hard bands up to 50 degrees, then the zenith colour; below the horizon the horizon colour, a little darker.
    float t = clamp(floor(saturate(el / 50.0) * 5.0 + 0.5) / 5.0, 0.0, 1.0);
    vec3 sky = t < 0.5 ? mix(skyHorizon.rgb, skyMid.rgb, t * 2.0) : mix(skyMid.rgb, skyZenith.rgb, (t - 0.5) * 2.0);
    if (el < 0.0)
        sky = skyHorizon.rgb * 0.85;

    vec3 towards = normalize(sunDirection.xyz);
    float angle = acos(clamp(dot(dir, towards), -1.0, 1.0));
    float pixel = max(fwidth(angle), 1e-4);
    float discRadius = 0.045;                                    // about 2.6 degrees

    // Stars (night): one candidate per cell of a grid on the direction, faded towards the horizon.
    if (skyParams.y > 0.0 && el > 4.0)
    {
        vec3 p = dir * 90.0;
        vec3 cell = floor(p);
        float h = fract(sin(dot(cell, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
        // the star's centre lies on the sky sphere (a centre inside the cell but off the sphere was never hit)
        vec3 centre = normalize(cell + 0.5 + (fract(vec3(h * 13.1, h * 71.7, h * 37.3)) - 0.5) * 0.6) * 90.0;
        // at least about 1.5 pixels wide, bigger ones for the brightest candidates
        float size = max(0.06 + 0.06 * fract(h * 91.3), fwidth(p.x) * 1.5);
        float star = step(0.97, h) * (1.0 - smoothstep(size * 0.5, size, length(p - centre)));
        float twinkle = mix(1.0, 0.65 + 0.35 * sin(time * 2.5 + h * 60.0), skyParams.z);
        sky = mix(sky, vec3(1.3, 1.32, 1.4), star * twinkle * skyParams.y * saturate((el - 4.0) / 10.0));
    }

    // Clouds: flat two-tone cumulus, each a cluster of round lumps with a flat bottom, placed by direction (azimuth
    // and elevation in degrees, no perspective stretch). Each 30-degree sector of the sky may hold one cloud; they
    // drift slowly around the arena. The lower part of each lump is the shaded tone.
    if (skyParams.z > 0.5 && el > 2.0 && cloudLit.w > 0.0)
    {
        vec2 a = vec2(atan2(dir.x, dir.z) * 57.2958 + time * 0.25, el);
        float sector = floor(a.x / 30.0);
        float pixelDeg = max(fwidth(a.y), 0.02);
        float cloud = 0.0;
        float lit = 0.0;
        for (int k = -1; k <= 1; k++)
        {
            float id = sector + float(k);
            float wrapped = id - 12.0 * floor(id / 12.0);    // 12 sectors around the sky: the same clouds come back
            vec4 h = fract(sin(vec4(wrapped * 12.9898, wrapped * 78.233, wrapped * 37.719, wrapped * 4.581)) * 43758.5453);
            if (h.x > 0.35 + 0.6 * cloudLit.w)
                continue;
            vec2 centre = vec2(id * 30.0 + 15.0 + (h.y - 0.5) * 12.0, 12.0 + h.z * 26.0);
            float size = (3.6 + h.w * 2.8) * (1.0 - centre.y / 90.0);    // degrees; smaller higher up
            float inside = 0.0;
            float top = 0.0;
            for (int l = 0; l < 5; l++)
            {
                float fl = float(l);
                float r = size * (0.55 + 0.35 * fract(h.w * 7.3 + fl * 0.37)) * (l == 2 ? 1.25 : 1.0);
                vec2 c = centre + vec2((fl - 2.0) * size * 0.6, size * 0.15 * (1.0 - abs(fl - 2.0) * 0.5));
                float d = length(a - c);
                inside = max(inside, 1.0 - smoothstep(r - pixelDeg, r + pixelDeg, d));
                // the lit part: the same lump moved up
                top = max(top, 1.0 - smoothstep(r - pixelDeg, r + pixelDeg, length(a - c - vec2(0.0, size * 0.22))));
            }
            inside *= smoothstep(centre.y - size * 0.45 - pixelDeg, centre.y - size * 0.45 + pixelDeg, a.y);    // flat bottom
            cloud = max(cloud, inside);
            lit = max(lit, top * inside);
        }
        // above 1 in the HDR image: the tone mapping would turn a white cloud grey
        vec3 cloudColour = mix(cloudShade.rgb, cloudLit.rgb * 1.35, lit);
        sky = mix(sky, cloudColour, cloud * saturate((el - 2.0) / 4.0));
    }

    if (skyParams.x < 0.5)
    {
        // The sun: a warm yellow disc (orange when the light is orange) with a smooth glow, no rings.
        vec3 core = vec3(1.0, 0.95 * sunColour.g, 0.35 * sunColour.b);
        vec3 halo = mix(skyHorizon.rgb, vec3(1.0, 0.96, 0.82), 0.65);
        float glow = exp(-pow(angle / (discRadius * 1.8), 2.0));
        sky = mix(sky, halo, glow * 0.7);
        float disc = 1.0 - smoothstep(discRadius - pixel, discRadius + pixel, angle);
        sky = mix(sky, core * skyParams.w, disc);
    }
    else
    {
        // The moon: a pale disc with a faint halo; a second disc offset up and to the side cuts the crescent.
        float glow = exp(-pow(angle / (discRadius * 2.5), 2.0));
        sky = mix(sky, mix(sky, vec3(0.65, 0.72, 0.95), 0.5), glow * 0.5);
        vec3 side = normalize(cross(towards, vec3(0.0, 1.0, 0.0)) + vec3(0.0, 0.001, 0.0));
        vec3 up = cross(side, towards);
        vec3 shadowCentre = normalize(towards + (side * 0.55 + up * 0.35) * discRadius);
        float inShadow = 1.0 - smoothstep(discRadius - pixel, discRadius + pixel, acos(clamp(dot(dir, shadowCentre), -1.0, 1.0)));
        float disc = (1.0 - smoothstep(discRadius - pixel, discRadius + pixel, angle)) * (1.0 - inShadow);
        sky = mix(sky, vec3(0.93, 0.95, 1.0) * skyParams.w * 0.7, disc);
    }

    gl_FragColor = vec4(sky, 1.0);
}
