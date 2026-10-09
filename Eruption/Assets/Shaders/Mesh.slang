// Mesh.slang - basic lit, textured mesh shader compiled to SPIR-V by Slang at runtime.
//
// Descriptor layout (matched by the engine via reflection):
//   set 0, binding 0 : SceneData      (per-frame camera + light)
//   set 1, binding 0 : albedo texture (per-material)
//   set 1, binding 1 : albedo sampler (per-material)
//   push constant     : per-object model matrix

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 uv       : TEXCOORD0;
    float4 tangent  : TANGENT;
};

struct VSOutput
{
    float4 clipPosition : SV_Position;
    float3 worldNormal  : NORMAL;
    float2 uv           : TEXCOORD0;
    float3 worldPosition : TEXCOORD1;
};

struct SceneData
{
    float4x4 viewProjection;
    float4   cameraPosition;
    float4   lightDirection; // xyz: direction the light travels
    float4   lightColor;     // rgb: colour, a: intensity
};

struct PushConstants
{
    float4x4 model;
};

[[vk::binding(0, 0)]] ConstantBuffer<SceneData> uScene;
[[vk::binding(0, 1)]] Texture2D                  uAlbedoTexture;
[[vk::binding(1, 1)]] SamplerState               uAlbedoSampler;
[[vk::push_constant]] PushConstants              uPush;

[shader("vertex")]
VSOutput vertexMain(VSInput input)
{
    VSOutput output;

    float4 worldPosition = mul(uPush.model, float4(input.position, 1.0));
    output.worldPosition = worldPosition.xyz;
    output.clipPosition  = mul(uScene.viewProjection, worldPosition);

    // Assumes uniform scale; fine for the sample. Use the inverse-transpose for non-uniform scale.
    output.worldNormal = normalize(mul((float3x3) uPush.model, input.normal));
    output.uv          = input.uv;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOutput input) : SV_Target
{
    float3 normal      = normalize(input.worldNormal);
    float3 toLight     = normalize(-uScene.lightDirection.xyz);
    float  diffuse     = max(dot(normal, toLight), 0.0);

    float3 albedo  = uAlbedoTexture.Sample(uAlbedoSampler, input.uv).rgb;
    float3 ambient = 0.04 * albedo;
    float3 lit     = albedo * uScene.lightColor.rgb * uScene.lightColor.a * diffuse;

    return float4(ambient + lit, 1.0);
}
