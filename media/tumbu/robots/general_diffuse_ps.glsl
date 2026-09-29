#version 120
// Robot lighting, per-light pass (GLSL port of general.cg diffuse_ps).
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
uniform sampler2D diffuseMap;
uniform sampler2D specMap;
uniform sampler2D normalMap;

varying vec2 uv;
varying vec4 wp;
varying vec3 n;
varying vec3 t;
varying vec3 b;
varying vec3 sdir;

void main()
{
    // light direction
    vec3 ld0 = normalize(lightPos0.xyz - (lightPos0.w * wp.xyz));

    // quadratic attenuation
    float lightDist = length(lightPos0.xyz - wp.xyz) / lightAtt0.r;
    float la = 1.0 - lightDist * lightDist;

    // normal map: tangent space -> object space -> world space
    vec3 normalTex = texture2D(normalMap, uv).xyz;
    vec3 normal = mat3(t, b, n) * (normalTex * 2.0 - 1.0);
    normal = normalize(mat3(iTWMat) * normal);

    float diffuse = max(dot(ld0, normal), 0.0);

    // spotlight cone (params 1,0,0,1 mean "not a spotlight")
    float spot = (spotlightParams.x == 1.0 && spotlightParams.y == 0.0 &&
                  spotlightParams.z == 0.0 && spotlightParams.w == 1.0) ? 1.0 :
        clamp((dot(ld0, normalize(-sdir)) - spotlightParams.y) /
              (spotlightParams.x - spotlightParams.y), 0.0, 1.0);

    vec3 camDir = normalize(camPos - wp.xyz);
    vec3 halfVec = normalize(ld0 + camDir);
    float specular = pow(max(dot(normal, halfVec), 0.0), matShininess);

    vec4 diffuseTex = texture2D(diffuseMap, uv);
    vec4 specTex = texture2D(specMap, uv);

    vec3 diffuseContrib = diffuse * lightDif0 * diffuseTex.rgb * matDif.rgb;
    vec3 specularContrib = specular * lightSpec0 * specTex.rgb * matSpec.rgb;
    vec3 light0C = (diffuseContrib + specularContrib) * la * spot;

    gl_FragColor = vec4(light0C, diffuseTex.a);
}
