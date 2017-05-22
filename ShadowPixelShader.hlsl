// Constant buffer that stores the per-model transform matrix for composing geometry.
cbuffer ModelConstantBuffer : register(b0)
{
	matrix model;
	bool hasNormalMap;
	float3 padding;
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

// shadow Map
Texture2D shadowMap : register(t0);
SamplerComparisonState shadowSampler : register(s0);

// texture
Texture2D simpleTexture : register(t1);
SamplerState simpleSampler : register(s1);
Texture2D NormalTexture : register(t2);
SamplerState NormalSampler : register(s2);


float3 DplusS(float3 N, float3 L, float NdotL, float3 view);

float4 main(PixelShaderInput input) : SV_TARGET
{
	float3 textureColor;
	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
	textureColor = simpleTexture.Sample(simpleSampler, input.tex).rgb;
	
	if (hasNormalMap)
	{
		// load normal map texture
		float4 nmap = NormalTexture.Sample(NormalSampler, input.tex);
		float3 normalMap = nmap.rgb * 2 - 1;                         // convert to range [-1 .. 1]
		float height = nmap.a;                                  // height is in alpha channel

		normalMap = normalize(normalMap);

		//input.utan = normalize(input.utan - dot(input.utan, input.norm)*input.norm);
		//input.vtan = cross(input.norm, input.utan);

		// Assemble the tangent basis matrix
		float3x3 mTangent = float3x3(normalize(input.utan),
			normalize(input.vtan),
			input.norm);

		input.norm = normalize(mul(normalMap, mTangent));

		float3 vToLight = mul(mTangent, normalize(input.lRay));			// transform light vector into tangent coords
		float intensity = saturate(dot(normalMap, vToLight));
		// Apply a shadow based on height above a sphere
		float costh = dot(input.norm, normalize(input.lRay));              // vertex/sphere normal
		float sin = sqrt(1 - costh*costh);                      // compute sin
		if ((costh < 0) && (sin + height*0.02 < 1))          // assume max bump height is 2% of radius
			intensity = 0;
			
		intensity = 0.99f*intensity + 0.01f;                    // add a small ambient light contribution
		intensity = sqrt(intensity);                          // gamma correct the lighting term

		//textureColor = saturate(textureColor * intensity);*/
		textureColor *= intensity;
	}

    const float3 ambient = float3(0.4f, 0.4f, 0.4f);

    // NdotL for shadow offset, lighting.
    float3 N = (input.norm);
    float3 L = normalize(input.lRay);
    float NdotL = dot(N, L);

    // Compute texture coordinates for the current point's location on the shadow map.
    float2 shadowTexCoords;
    shadowTexCoords.x = 0.5f + (input.lightSpacePos.x / input.lightSpacePos.w * 0.5f);
    shadowTexCoords.y = 0.5f - (input.lightSpacePos.y / input.lightSpacePos.w * 0.5f);

    //float pixelDepth = input.lightSpacePos.z / input.lightSpacePos.w;
	input.lightSpacePos /= input.lightSpacePos.w;

    float lighting = 0.f;
	
    // Check if the pixel texture coordinate is in the view frustum of the 
    // light before doing any shadow work.
    /*if ((saturate(shadowTexCoords.x) == shadowTexCoords.x) &&
        (saturate(shadowTexCoords.y) == shadowTexCoords.y) &&
        (pixelDepth > 0))*/
	if (input.lightSpacePos.x >= -1.0f && input.lightSpacePos.x <= 1.0f &&
		input.lightSpacePos.y >= -1.0f && input.lightSpacePos.y <= 1.0f &&
		input.lightSpacePos.z >= 0.0f && input.lightSpacePos.z <= 1.0f)
    {
        
		const float bias = 0.0005f;

        // sample from the shadow map
        lighting = float(shadowMap.SampleCmpLevelZero(
            shadowSampler,
            shadowTexCoords,
			input.lightSpacePos.z - bias
            )
            );
		
        if (lighting <= 0.0001f && lighting >= -0.0001f)
        {
            return float4(textureColor * ambient, 1.f);  //* input.color
        }

		else if (lighting < 1.0f)
		{
			// Blends the shadow area into the lit area.
			float3 light = lighting * (ambient + DplusS(N, L, NdotL, input.view));
			float3 shadow = (1.0f - lighting) * ambient;
			return float4(textureColor * (light + shadow), 1.f);
		}
    }
	else
	{
		return float4(textureColor * ambient, 1.f);  //* input.color
	}
	
	return float4(textureColor * (ambient + DplusS(N, L, NdotL, input.view)), 1.f); // * input.color
}

// Performs very basic Phong lighting for example purposes.
float3 DplusS(float3 N, float3 L, float NdotL, float3 view)
{
	const float3 Kdiffuse = float3(.5f, .5f, .4f);
	const float3 Kspecular = float3(.2f, .2f, .3f);
	const float exponent = 3.f;

    // Compute the diffuse coefficient.
    float diffuseConst = saturate(NdotL);

    // Compute the diffuse lighting value.
    float3 diffuse = Kdiffuse * diffuseConst;

	float d = length(L);

    // Compute the specular highlight.
    float3 R = reflect(-L, N);
    float3 V = normalize(view);
    float3 RdotV = dot(R, V);
    float3 specular = Kspecular * pow(saturate(RdotV), exponent);

    return (diffuse + specular);
}