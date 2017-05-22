//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "DeviceResources.h"
#include "ShaderStructures.h"
#include "StepTimer.h"
#include "BasicShapes.h"

struct VertexPositionColor
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 color;
};
struct VertexPos
{
	DirectX::XMFLOAT3 pos;
};

struct Triangle
{
	DirectX::XMVECTOR tri1;
	DirectX::XMVECTOR tri2;
	DirectX::XMVECTOR tri3;

	float center[3];
	float  lower_corner[3];
	float  upper_corner[3];

	bool intersected;
	DirectX::XMVECTOR intersectionPoint = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	float distance;
};


namespace ShadowMapping
{
    // This sample renderer instantiates a basic rendering pipeline.
    class ShadowSceneRenderer
    {
    public:
        ShadowSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();
        void ReleaseDeviceDependentResources();
        void Update(DX::StepTimer const& timer);

        void Render();

		// CameraDrive Function
		void CameraDrive();

		// Round Int Value to next 10
		int RoundNum(int num)
		{
			int rem = num % 10;
			return rem >= 5 ? (num - rem + 10) : (num - rem);
		}

		void UpdateCamera();
		void UpdateLight();
		void KeyDownCheck(Windows::System::VirtualKey key);
		void KeyUpCheck(Windows::System::VirtualKey key);
		void DetectInput(DX::StepTimer const & timer);
		void DirectInput(DX::StepTimer const& timer);

		void InitShadowMap();

		void SetEarthNormalMap(bool normal);
		void SetMoonNormalMap(bool normal);

		void SetKDTree(bool draw);

		float GetDistance();

		void TestIntersection(float pointX, float pointY, float HEIGHT, float WIDTH);

		void DrawKDTree(std::vector<BasicVertex> vertices, DirectX::FXMVECTOR color);

		void SetFilterMode(Platform::String ^test);

        __forceinline bool  GetFiltering()                      { return m_useLinear;                 };
        __forceinline void  SetFiltering(bool useLinear)        { m_useLinear = useLinear;            };
        __forceinline float GetShadowDimension()                { return m_shadowMapDimension;        };
        __forceinline void  SetShadowDimension(float dimension) { m_shadowMapDimension = dimension;   };
        __forceinline bool  GetD3D9ShadowsSupported()           { return m_deviceSupportsD3D9Shadows; };


    private:
        void DetermineShadowFeatureSupport();
        void RenderShadowMap();
        void RenderSceneWithShadows();
        void RenderQuad();
        void UpdateAllConstantBuffers();

		// Init Standard Points
		void InitStdPoints();

		//bool XM_CALLCONV Intersects(_In_ DirectX::FXMVECTOR Origin, _In_ DirectX::FXMVECTOR Direction, _In_ DirectX::FXMVECTOR V0, _In_ DirectX::GXMVECTOR V1, _In_ DirectX::HXMVECTOR V2, _Out_ float& Dist);
		DirectX::XMVECTOR VectorToLocal(DirectX::XMVECTOR inVec);

		void DrawRay(DirectX::FXMVECTOR Origin, DirectX::FXMVECTOR Destination, DirectX::FXMVECTOR color);

		void DrawHitTriangle(Triangle& tri, DirectX::FXMVECTOR color);

        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // Direct3D resources for cube geometry.
        Microsoft::WRL::ComPtr<ID3D11InputLayout>   m_inputLayout;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBuffer;
        Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_vertexShader;
        Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_simpleVertexShader;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>  m_lineVertexShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_shadowPixelShader_point;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_shadowPixelShader_linear;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_comparisonShader;
        Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_textureShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_lineShader;

		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_lineVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_lineIndexBuffer;

		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_triangleVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_triangleIndexBuffer;

		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_kdTreeVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_kdTreeIndexBuffer;

        // Shadow buffer Direct3D resources.
        Microsoft::WRL::ComPtr<ID3D11Texture2D>          m_shadowMap;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>   m_shadowDepthView;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shadowResourceView;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>       m_comparisonSampler_point;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>       m_comparisonSampler_linear;
        Microsoft::WRL::ComPtr<ID3D11SamplerState>       m_linearSampler;

		// texture resources
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureResource;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureResourceCube1;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureworldnormals;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_texturemoonnormals;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureResourceCube2;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureResourceCube3;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_textureResourceLight;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_textureSampler;
		// diff filter modi
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_textureSampler_1;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_textureSampler_2;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_textureSampler_3;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_textureSampler_4;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_textureSampler_5;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_textureSampler_6;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_textureSampler_7;
		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_textureSampler_8;

		Microsoft::WRL::ComPtr<ID3D11SamplerState> m_normalSampler;

        // Model, view, projection constant buffers.
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_cubeViewProjectionBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_lightViewProjectionBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_orthoViewProjectionBuffer;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_staticModelBuffer;
      
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_lightModelBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_rotatedModelBuffer1;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_rotatedModelBuffer2;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_rotatedModelBuffer3;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_orthoTransformBuffer;

        // Render states for front face/back face culling.
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_shadowRenderState;
        Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_drawingRenderState;

        // Direct3D resources for displaying the shadow map.
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBufferQuad;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBufferQuad;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBufferFloor;
        Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBufferFloor;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_vertexBufferSphere;
		Microsoft::WRL::ComPtr<ID3D11Buffer>        m_indexBufferSphere;

        D3D11_VIEWPORT                              m_shadowViewport;

        // System resources.
        ViewProjectionConstantBuffer m_cubeViewProjectionBufferData;
        ViewProjectionConstantBuffer m_lightViewProjectionBufferData;
        ViewProjectionConstantBuffer m_orthoViewProjectionBufferData;
        ModelConstantBuffer m_staticModelBufferData;
      
		ModelConstantBuffer m_lightModelBufferData;
		ModelConstantBuffer m_rotatedModelBufferData1;
		ModelConstantBuffer m_rotatedModelBufferData2;
		ModelConstantBuffer m_rotatedModelBufferData3;
        ModelConstantBuffer m_orthoTransformBufferData;
        uint32  m_indexCountQuad;
        uint32  m_indexCountFloor;
        uint32  m_indexCountCube;
		uint32  m_indexCountSphere;                  // mesh index count

		uint32  m_indexCountLine;
		uint32  m_indexCountTriangle;
		uint32  m_indexCountkdTree;

        // Variables used with the rendering loop.
        bool    m_loadingComplete;
        float   m_degreesPerSecond;
        bool    m_useLinear;

        // Controls the size of the shadow map.
        float   m_shadowMapDimension;

        // Cached copy of SupportsDepthAsTextureWithLessEqualComparisonFilter.
        bool    m_deviceSupportsD3D9Shadows;

    };
}

