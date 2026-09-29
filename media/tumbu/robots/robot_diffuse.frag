// Robot lighting, per-light pass: normal-mapped diffuse + specular map, with quadratic attenuation and spotlights.
#include <OgreUnifiedShader.h>

SAMPLER2D(diffuseMap, 0);
SAMPLER2D(specMap, 1);
SAMPLER2D(normalMap, 2);

OGRE_UNIFORMS(
    uniform vec3 lightDif0;
    uniform vec4 lightPos0;
    uniform vec4 lightAtt0;
    uniform vec3 lightSpec0;
    uniform vec4 matDif;
    uniform vec4 matSpec;
    uniform float matShininess;
    uniform vec3 camPos;
    uniform vec4 spotlightParams;
    uniform mat4 iTWMat;
)

MAIN_PARAMETERS
IN(vec2 oUv, TEXCOORD0)
IN(vec4 oWorldPos, TEXCOORD1)
IN(vec3 oNormal, TEXCOORD2)
IN(vec3 oTangent, TEXCOORD3)
IN(vec3 oBinormal, TEXCOORD4)
IN(vec3 oSpotDir, TEXCOORD5)
MAIN_DECLARATION
{
    // light direction (w = 0 for directional lights)
    vec3 ld0 = normalize(lightPos0.xyz - (lightPos0.w * oWorldPos.xyz));

    // quadratic attenuation
    float lightDist = length(lightPos0.xyz - oWorldPos.xyz) / lightAtt0.r;
    float la = 1.0 - lightDist * lightDist;

    // normal map: tangent space -> object space -> world space
    vec3 n = texture2D(normalMap, oUv).xyz * 2.0 - 1.0;
    vec3 objNormal = oTangent * n.x + oBinormal * n.y + oNormal * n.z;
    vec3 normal = normalize(mul(iTWMat, vec4(objNormal, 0.0)).xyz);

    float diffuse = max(dot(ld0, normal), 0.0);

    // spotlight cone (params 1,0,0,1 mean "not a spotlight")
    float spot = 1.0;
    if (!(spotlightParams.x == 1.0 && spotlightParams.y == 0.0 && spotlightParams.z == 0.0 && spotlightParams.w == 1.0))
        spot = saturate((dot(ld0, normalize(-oSpotDir)) - spotlightParams.y) / (spotlightParams.x - spotlightParams.y));

    vec3 camDir = normalize(camPos - oWorldPos.xyz);
    vec3 halfVec = normalize(ld0 + camDir);
    float specular = pow(max(dot(normal, halfVec), 0.0), matShininess);

    vec4 diffuseTex = texture2D(diffuseMap, oUv);
    vec4 specTex = texture2D(specMap, oUv);

    vec3 diffuseContrib = diffuse * lightDif0 * diffuseTex.rgb * matDif.rgb;
    vec3 specularContrib = specular * lightSpec0 * specTex.rgb * matSpec.rgb;
    vec3 light0C = (diffuseContrib + specularContrib) * la * spot;

    gl_FragColor = vec4(light0C, diffuseTex.a);
}
