// Constant buffer that stores two of the basic column-major matrices for composing geometry.
cbuffer ViewProjectionConstantBuffer : register(b0)
{
    matrix view;
    matrix projection;
    float4 eyePos; // eye position
};

// Constant buffer that stores the per-model transform matrix for composing geometry.
cbuffer ModelConstantBuffer : register(b1)
{
    matrix model;
};

// A constant buffer that stores matrices for composing geometry for the shadow buffer.
cbuffer LightViewProjectionConstantBuffer : register(b2)
{
    matrix lView;
    matrix lProjection;
    float4 lPos; // light position
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD0;
    float3 norm : NORMAL0;
    float3 color : COLOR0;
	float3 utan : TANGENT0;
	float3 vtan : TANGENT1;
};

// Per-pixel color data passed to the pixel shader.
struct PixelShaderInput
{
    float4 pos : SV_POSITION;
    float2 tex : TEXCOORD0;
    //float3 color : COLOR0;
    float4 lightSpacePos : POSITION1;  
    float3 norm : NORMAL0;
    float3 lRay : NORMAL1;
    float3 view : NORMAL2;   
	float3 utan : TANGENT0;
	float3 vtan : TANGENT1;
};

// Shader to do vertex processing for camera view position and light view position.
PixelShaderInput main(VertexShaderInput input)
{
    PixelShaderInput output;
    float4 pos = float4(input.pos, 1.0f);

    // Transform the vertex position into projected space.
    float4 modelPos = mul(pos, model);
    pos = mul(modelPos, view);
    pos = mul(pos, projection);
    output.pos = pos;

    // Transform the vertex position into projected space from the POV of the light.
    float4 lightSpacePos = mul(modelPos, lView);
    lightSpacePos = mul(lightSpacePos, lProjection);
    output.lightSpacePos = lightSpacePos;

    // Light ray
    float3 lRay = lPos.xyz - modelPos.xyz;
    output.lRay = lRay;
    
    // Camera ray
	output.view = eyePos.xyz - modelPos.xyz;

	// Calculate the normal vector against the world matrix only.
	output.norm = mul(input.norm, (float3x3)model);
	// Normalize the normal vector.
	output.norm = normalize(output.norm);
	/*
    // Transform the vertex normal into world space.
    float4 norm = float4(input.norm, 1.0f);
    norm = mul(norm, model);
    output.norm = norm.xyz;*/
    
    // Pass through the color and texture coordinates without modification.
    //output.color = input.color;
    output.tex = input.tex;

	// normal mapping
	output.utan = mul(float4(input.utan, 1.0f), model).xyz;		// convert u-tangent to world coords
	output.vtan = mul(float4(input.vtan, 1.0f), model).xyz;		// convert v-tangent to world coords

    return output;
}