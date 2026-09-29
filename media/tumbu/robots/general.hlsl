// Robot lighting (HLSL, Direct3D 9, shader model 3). Ported 1:1 from the original general.cg.
// Two passes: an ambient pass (ambient_vs/ambient_ps) plus one additive pass per light (diffuse_vs/diffuse_ps)
// with a normal map and a specular map.

struct VIn
{
    float4 p    : POSITION;
    float3 n    : NORMAL;
    float4 t    : TANGENT;
    float2 uv   : TEXCOORD0;
};

struct VOut
{
    float4 p    : POSITION;
    float2 uv   : TEXCOORD0;
    float4 wp   : TEXCOORD1;
    float3 n    : TEXCOORD2;
    float3 t    : TEXCOORD3;
    float3 b    : TEXCOORD4;
    float3 sdir : TEXCOORD5;
};

struct PIn
{
    float2 uv   : TEXCOORD0;
    float4 wp   : TEXCOORD1;
    float3 n    : TEXCOORD2;
    float3 t    : TEXCOORD3;
    float3 b    : TEXCOORD4;
    float3 sdir : TEXCOORD5;
};

void ambient_vs(VIn IN,
    uniform float4x4 wvpMat,
    out float4 oPos : POSITION,
    out float2 oUV : TEXCOORD0)
{
    oPos = mul(wvpMat, IN.p);
    oUV = IN.uv;
}

float4 ambient_ps(float2 uv : TEXCOORD0,
    uniform float3 ambient,
    uniform float4 matDif,
    uniform sampler2D dMap : register(s0),
    uniform sampler2D aoMap : register(s1)) : COLOR0
{
    return tex2D(dMap, uv) * tex2D(aoMap, uv) * float4(ambient, 1) * float4(matDif.rgb, 1);
}

VOut diffuse_vs(VIn IN,
    uniform float4x4 wMat,
    uniform float4x4 wvpMat,
    uniform float4 spotlightDir)
{
    VOut OUT;
    OUT.wp = mul(wMat, IN.p);
    OUT.p = mul(wvpMat, IN.p);
    OUT.uv = IN.uv;
    OUT.n = IN.n;
    OUT.t = IN.t.xyz;
    OUT.b = cross(IN.t.xyz, IN.n) * IN.t.w;
    OUT.sdir = mul(wMat, spotlightDir).xyz; // spotlight direction in world space
    return OUT;
}

float4 diffuse_ps(PIn IN,
    uniform float3 lightDif0,
    uniform float4 lightPos0,
    uniform float4 lightAtt0,
    uniform float3 lightSpec0,
    uniform float4 matDif,
    uniform float4 matSpec,
    uniform float matShininess,
    uniform float3 camPos,
    uniform float4 spotlightParams,
    uniform float4x4 iTWMat,
    uniform sampler2D diffuseMap : register(s0),
    uniform sampler2D specMap : register(s1),
    uniform sampler2D normalMap : register(s2)) : COLOR0
{
    // light direction
    float3 ld0 = normalize(lightPos0.xyz - (lightPos0.w * IN.wp.xyz));

    // quadratic attenuation
    float lightDist = length(lightPos0.xyz - IN.wp.xyz) / lightAtt0.r;
    float la = 1.0 - lightDist * lightDist;

    // normal map: tangent space -> object space -> world space
    float4 normalTex = tex2D(normalMap, IN.uv);
    float3x3 tbn = float3x3(IN.t, IN.b, IN.n);
    float3 normal = mul(transpose(tbn), normalTex.xyz * 2 - 1);
    normal = normalize(mul((float3x3)iTWMat, normal));

    float3 diffuse = max(dot(ld0, normal), 0);

    // spotlight cone (params 1,0,0,1 mean "not a spotlight")
    float spot = (spotlightParams.x == 1 && spotlightParams.y == 0 &&
                  spotlightParams.z == 0 && spotlightParams.w == 1) ? 1 :
        saturate((dot(ld0, normalize(-IN.sdir)) - spotlightParams.y) /
                 (spotlightParams.x - spotlightParams.y));

    float3 camDir = normalize(camPos - IN.wp.xyz);
    float3 halfVec = normalize(ld0 + camDir);
    float3 specular = pow(max(dot(normal, halfVec), 0), matShininess);

    float4 diffuseTex = tex2D(diffuseMap, IN.uv);
    float4 specTex = tex2D(specMap, IN.uv);

    float3 diffuseContrib = diffuse * lightDif0 * diffuseTex.rgb * matDif.rgb;
    float3 specularContrib = specular * lightSpec0 * specTex.rgb * matSpec.rgb;
    float3 light0C = (diffuseContrib + specularContrib) * la * spot;

    return float4(light0C, diffuseTex.a);
}
