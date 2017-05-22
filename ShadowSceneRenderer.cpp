//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include <string>
#include <cassert>

#include "ShadowSceneRenderer.h"
#include <DirectXColors.h>

#include "DirectXHelper.h"

#include "DDSTextureLoader.h"
#include "BasicShapes.h"

#include "Kdtree.h"

#include "Timer.h"

//#include <DirectXMath.h>
#include <DirectXCollision.h>

using namespace ShadowMapping;

using namespace DirectX;
using namespace Windows::Foundation;

using namespace Windows::UI::Core;
using namespace Windows::System;
using namespace Windows::UI::Xaml;

XMGLOBALCONST XMVECTORF32 g_RayEpsilon = { 1e-20f, 1e-20f, 1e-20f, 1e-20f };
XMGLOBALCONST XMVECTORF32 g_RayNegEpsilon = { -1e-20f, -1e-20f, -1e-20f, -1e-20f };
XMGLOBALCONST XMVECTORF32 g_FltMin = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };
XMGLOBALCONST XMVECTORF32 g_FltMax = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };

// cube Indices
std::vector<short> m_cubeIndices;
std::vector<BasicVertex> m_cubeVertices;

std::vector<short> m_sphereIndices;
std::vector<BasicVertex> m_sphereVertices;

float mPickedTriangle = -1;
XMVECTOR rayOrigin;
XMVECTOR rayDirection;
float closestDist = FLT_MAX;

Triangle hitTriangle;
Triangle tempTriangle;

bool pointToCamera = true;

XMVECTOR pointInPlane = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
XMVECTOR pointOnNearest = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
XMVECTOR pointOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

float aspectRatio;
float fovAngleY;

// struct to use for camera positions of camera drive function
struct CameraViewPoint
{
	DirectX::XMVECTOR Eye = DirectX::XMVectorSet(0.0f, 14.f, 30.f, 0.0f );
	float camYaw = 0.0f;
	float camPitch = 0.0f;
	float camRoll = 0.0f;
	DirectX::XMVECTOR quaternion;
};

float ShadowSceneRenderer::GetDistance() { return closestDist; };

//float pick(XMVECTOR pickRayInWorldSpacePos, XMVECTOR pickRayInWorldSpaceDir, std::vector<BasicVertex>& vertPosArray, std::vector<short>& indexPosArray, XMMATRIX & worldSpace);

std::vector<Triangle> VerticesToTrianglesWorld(std::vector<BasicVertex>& vertPosArray, std::vector<short>& indexPosArray, XMMATRIX & worldSpace);

float pick(XMVECTOR& rayOrigin, XMVECTOR& rayDirection, std::vector<Triangle>& vertices);

float intersectRayTriangle(Triangle & tri, XMVECTOR & rayOrigin, XMVECTOR & rayDirection);

bool PointInTriangle(XMVECTOR & triV1, XMVECTOR & triV2, XMVECTOR & triV3, XMVECTOR & point);

// Spline Interpolation
void kochanekSplineandSquadInterpolation(const CameraViewPoint& point0, const CameraViewPoint& point1, const CameraViewPoint& point2, const CameraViewPoint& point4, float s);

XMVECTOR eye;
XMVECTOR at;
XMVECTOR up;

XMMATRIX g_View;
XMFLOAT4X4 m_View;
XMFLOAT4X4 g_Projection;
XMFLOAT4X4 g_World;

const int numberofSpheres = 3;
XMFLOAT4X4 s_World[numberofSpheres];

std::vector<Triangle> TrianglesWorld[numberofSpheres];
std::vector<Triangle*> TrianglesWorldPointers;
bool transVertices = true;
kdtree* tree;

XMVECTOR eyel;
XMVECTOR atl;
XMVECTOR upl;

XMMATRIX l_View;

XMVECTOR DefaultForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
XMVECTOR DefaultRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
XMVECTOR camForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
XMVECTOR camRight = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

XMVECTOR quaternion = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
XMVECTOR quaternionl = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

XMMATRIX camRotationMatrix;

float moveLeftRight = 0.0f;
float moveBackForward = 0.0f;

float moveLeftRightl = 0.0f;
float moveBackForwardl = 0.0f;

float camYaw = 0.0f;
float camPitch = 0.0f;
float camRoll = 0.0f;

float camYawl = 0.0f;
float camPitchl = 0.0f;
float camRolll = 0.0f;

int counter = 0;
bool driveActivated = false;

//CameraPoints
CameraViewPoint newPoint1;
CameraViewPoint newPoint2;
CameraViewPoint newPoint3;
CameraViewPoint newPoint4;
CameraViewPoint newPoint5;

// Input states for Keyboard.
bool m_forward = false;
bool m_back = false;
bool m_left = false;
bool m_right = false;
bool m_pitchp = false;
bool m_pitchn = false;
bool m_rollp = false;
bool m_rolln = false;
bool m_yawp = false;
bool m_yawn = false;

bool m_forwardl = false;
bool m_backl = false;
bool m_leftl = false;
bool m_rightl = false;

bool m_pitchpl = false;
bool m_pitchnl = false;
bool m_yawpl = false;
bool m_yawnl = false;
bool m_rollpl = false;
bool m_rollnl = false;

// Controls the filter mode
int		m_filterMode = 8;
// Controls the Normal Mode
bool	m_EarthNormalMap = true;
bool	m_MoonNormalMap = true;
bool	m_drawKDTree = false;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
ShadowSceneRenderer::ShadowSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
    m_loadingComplete(false),
    m_deviceResources(deviceResources),
    m_degreesPerSecond(22.5),
    m_indexCountCube(0),
    m_indexCountFloor(0),
    m_indexCountQuad(0), 
	m_indexCountSphere(0),
    m_useLinear(true),
    m_shadowMapDimension(2048.f)
{
	InitStdPoints();
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}

// Initialization.
void ShadowSceneRenderer::CreateDeviceDependentResources()
{
    DetermineShadowFeatureSupport();

    // Load shaders asynchronously.
    auto loadVSTask = DX::ReadDataAsync(L"VertexShader.cso");
    auto loadSimpleVSTask = DX::ReadDataAsync(L"SimpleVertexShader.cso");
	auto loadShadowPSPointTask = DX::ReadDataAsync(L"ShadowPixelShader.cso");
    auto loadTexturePSTask = DX::ReadDataAsync(L"TextureShader.cso");
    auto loadComparisonPSTask = DX::ReadDataAsync(L"ComparisonShader.cso");
	auto loadLineVSTask = DX::ReadDataAsync(L"VertexLineShader.cso");
	auto loadLinePSTask = DX::ReadDataAsync(L"LinePixelShader.cso");

	// load dds texture from file
	HRESULT hr = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"seafloor.dds",
		nullptr, m_textureResource.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// load dds texture from file
	hr = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"worldtexture.dds",
		nullptr, m_textureResourceCube1.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// load dds texture from file
	hr = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"worldnormals_crazy.dds",
		nullptr, m_textureworldnormals.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// load dds texture from file
	hr = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"moontexture.dds",
		nullptr, m_textureResourceCube2.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// load dds texture from file
	hr = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"moonnormals_crazy.dds",
		nullptr, m_texturemoonnormals.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// load dds texture from file
	hr = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"hothtexture.dds",
		nullptr, m_textureResourceCube3.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// load dds texture from file
	hr = CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), L"lighttexture.dds",
		nullptr, m_textureResourceLight.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Once the texture view is created, create a sampler.  This defines how the color
	// for a particular texture coordinate is determined using the relevant texture data.
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory(&samplerDesc, sizeof(samplerDesc));
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_9_2 ? 4 : 2;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;

	// allow use of all mip levels
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;   // ANISOTROPIC FILTER  D3D11_FILTER_ANISOTROPIC
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateSamplerState(
			&samplerDesc,
			&m_textureSampler
			)
		);

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_textureSampler_1));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_textureSampler_2));
	samplerDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_textureSampler_3));
	samplerDesc.Filter = D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_textureSampler_4));
	samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_textureSampler_5));
	samplerDesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_textureSampler_6));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_textureSampler_7));
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateSamplerState(&samplerDesc, &m_textureSampler_8));


	// create the normal map sampler
	ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_9_2 ? 4 : 2;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDesc.BorderColor[0] = 0.0f;
	samplerDesc.BorderColor[1] = 0.0f;
	samplerDesc.BorderColor[2] = 0.0f;
	samplerDesc.BorderColor[3] = 0.0f;
	// allow use of all mip levels
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateSamplerState(
			&samplerDesc,
			&m_normalSampler)
		);

    // After the vertex shader file is loaded, create the shader and input layout.
    auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
    {
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateVertexShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &m_vertexShader
                )
            );

        static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TANGENT",  1, DXGI_FORMAT_R32G32B32_FLOAT, 0, 52, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateInputLayout(
                vertexDesc,
                ARRAYSIZE(vertexDesc),
                &fileData[0],
                fileData.size(),
                &m_inputLayout
                )
            );

        CD3D11_BUFFER_DESC viewProjectionConstantBufferDesc(sizeof(ViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &viewProjectionConstantBufferDesc,
                nullptr,
                &m_cubeViewProjectionBuffer
                )
            );

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &viewProjectionConstantBufferDesc,
                nullptr,
                &m_lightViewProjectionBuffer
                )
            );

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &viewProjectionConstantBufferDesc,
                nullptr,
                &m_orthoViewProjectionBuffer
                )
            );

        CD3D11_BUFFER_DESC modelConstantBufferDesc(sizeof(ModelConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &modelConstantBufferDesc,
                nullptr,
                &m_staticModelBuffer
                )
            );
		// cube Buffer 1
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &modelConstantBufferDesc,
                nullptr,
                &m_rotatedModelBuffer1
                )
            );

		// cube Buffer 2
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&modelConstantBufferDesc,
				nullptr,
				&m_rotatedModelBuffer2
				)
			);

		// cube Buffer 3
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&modelConstantBufferDesc,
				nullptr,
				&m_rotatedModelBuffer3
				)
			);

		// cube light buffer
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateBuffer(
				&modelConstantBufferDesc,
				nullptr,
				&m_lightModelBuffer
				)
			);

        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &modelConstantBufferDesc,
                nullptr,
                &m_orthoTransformBuffer
                )
            );
    });

    auto createSimpleVSTask = loadSimpleVSTask.then([this](const std::vector<byte>& fileData)
    {
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateVertexShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &m_simpleVertexShader
                )
            );
    });

	auto createLineVSTask = loadLineVSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreateVertexShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_lineVertexShader
				)
			);
	});

    auto createShadowPSPointTask = loadShadowPSPointTask.then([this](const std::vector<byte>& fileData)
    {
        if (m_deviceSupportsD3D9Shadows)
        {
            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreatePixelShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &m_shadowPixelShader_point
                )
                );
        }
    });

    auto createTexturePSTask = loadTexturePSTask.then([this](const std::vector<byte>& fileData)
    {
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreatePixelShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &m_textureShader
                )
            );
    });

	auto createLinePSTask = loadLinePSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(
			m_deviceResources->GetD3DDevice()->CreatePixelShader(
				&fileData[0],
				fileData.size(),
				nullptr,
				&m_lineShader
				)
			);
	});

    auto createComparisonPSTask = loadComparisonPSTask.then([this](const std::vector<byte>& fileData)
    {
        if (m_deviceSupportsD3D9Shadows)
        {
            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreatePixelShader(
                &fileData[0],
                fileData.size(),
                nullptr,
                &m_comparisonShader
                )
                );
        }
    });

    // Once the shaders are loaded, create meshes.
    auto createQuadTask = (createVSTask && createSimpleVSTask && createLineVSTask).then([this]()
    {

        // Load mesh vertices. Each vertex has a position and a color.
        static const VertexPositionTexNormColor quadVertices [] =
        {
            {XMFLOAT3(0.f, 0.f, 0.0f), XMFLOAT2(0.f, 1.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(0.f, 1.f, 0.0f), XMFLOAT2(0.f, 0.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(1.f, 1.f, 0.0f), XMFLOAT2(1.f, 0.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(1.f, 0.f, 0.0f), XMFLOAT2(1.f, 1.f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
        };

        D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
        vertexBufferData.pSysMem = quadVertices;
        vertexBufferData.SysMemPitch = 0;
        vertexBufferData.SysMemSlicePitch = 0;
        CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(quadVertices), D3D11_BIND_VERTEX_BUFFER);
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &vertexBufferDesc,
                &vertexBufferData,
                &m_vertexBufferQuad
                )
            );

        // Load mesh indices. Each triple of indices represents
        // a triangle to be rendered on the screen.
        // For example, 0, 2, 1 means that the vertices with indexes
        // 0, 2 and 1 from the vertex buffer compose the
        // first triangle of this mesh.
        static const unsigned short quadIndices [] =
        {
            0, 1, 2,
            0, 2, 3,
        };

        m_indexCountQuad = ARRAYSIZE(quadIndices);

        D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
        indexBufferData.pSysMem = quadIndices;
        indexBufferData.SysMemPitch = 0;
        indexBufferData.SysMemSlicePitch = 0;
        CD3D11_BUFFER_DESC indexBufferDesc(sizeof(quadIndices), D3D11_BIND_INDEX_BUFFER);
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &indexBufferDesc,
                &indexBufferData,
                &m_indexBufferQuad
                )
            );
    });

    auto createFloorTask = createQuadTask.then([this]()
    {

        float w = 999.f; // Tile width.
		
        // Load a quad that appears to be an infinite plane.
        static const VertexPositionTexNormColor floorVertices [] =
        {
            {XMFLOAT3(-w, -10.f, -w), XMFLOAT2(0.f, 0.f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(-w, -10.f, w), XMFLOAT2(0.f, 0.f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(w, -10.f, w), XMFLOAT2(0.f, 0.f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(w, -10.f, -w), XMFLOAT2(0.f, 0.f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f)},
        };

        D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
        vertexBufferData.pSysMem = floorVertices;
        vertexBufferData.SysMemPitch = 0;
        vertexBufferData.SysMemSlicePitch = 0;
        CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(floorVertices), D3D11_BIND_VERTEX_BUFFER);
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &vertexBufferDesc,
                &vertexBufferData,
                &m_vertexBufferFloor
                )
            );

        // Load mesh indices. Each triple of indices represents
        // a triangle to be rendered on the screen.
        // For example, 0, 2, 1 means that the vertices with indexes
        // 0, 2 and 1 from the vertex buffer compose the
        // first triangle of this mesh.
        static const unsigned short floorIndices [] =
        {
            0, 2, 1,
            3, 2, 0,
        };

        m_indexCountFloor = ARRAYSIZE(floorIndices);

        D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
        indexBufferData.pSysMem = floorIndices;
        indexBufferData.SysMemPitch = 0;
        indexBufferData.SysMemSlicePitch = 0;
        CD3D11_BUFFER_DESC indexBufferDesc(sizeof(floorIndices), D3D11_BIND_INDEX_BUFFER);
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &indexBufferDesc,
                &indexBufferData,
                &m_indexBufferFloor
                )
            );
    });

	auto createSphereTask = createFloorTask.then([this]()
	{
		// create the vertex and index buffers for drawing the sphere
		BasicShapes^ shapes = ref new BasicShapes(m_deviceResources->GetD3DDevice());
		
		shapes->CreateTangentSphere(
			&m_vertexBufferSphere,
			&m_indexBufferSphere,
			nullptr,
			&m_indexCountSphere,
			m_sphereIndices,
			m_sphereVertices
			);
	});

	auto createCubeTask = createSphereTask.then([this]()
	{
		// create the vertex and index buffers for drawing the cubes
		BasicShapes^ shapes = ref new BasicShapes(m_deviceResources->GetD3DDevice());

		float cubesize = 10.f;

		shapes->CreateCube(
			cubesize,
			&m_vertexBuffer,
			&m_indexBuffer,
			nullptr,
			&m_indexCountCube,
			m_cubeIndices,
			m_cubeVertices
			);
	});

	// create cube
	/*
    auto createCubeTask = createSphereTask.then([this]()
    {
        // Load mesh vertices. Each vertex has a position and a color.
        float a = 10.f;

        VertexPositionTexNormColor cubeVertices [] =
        {
			{XMFLOAT3(a, a, -a), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 1.0f, 0.0f)}, // +y
            {XMFLOAT3(a, a, a), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(-a, a, a), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 1.0f, 1.0f)},
            {XMFLOAT3(-a, a, -a), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.f, 1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 1.0f, 0.0f)},

            {XMFLOAT3(-a, -a, -a), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.f, -1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 0.0f, 0.0f)}, // -y
            {XMFLOAT3(-a, -a, a), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.f, -1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 0.0f, 1.0f)},
            {XMFLOAT3(a, -a, a), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.f, -1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 0.0f, 1.0f)},
            {XMFLOAT3(a, -a, -a), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.f, -1.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 0.0f, 0.0f)},

            {XMFLOAT3(a, -a, -a), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(1.f, 0.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 0.0f, 0.0f)}, // +x
            {XMFLOAT3(a, -a, a), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(1.f, 0.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 0.0f, 1.0f)},
            {XMFLOAT3(a, a, a), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(1.f, 0.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(a, a, -a), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(1.f, 0.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 1.0f, 0.0f)},

            {XMFLOAT3(-a, a, -a), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(-1.f, 0.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 1.0f, 0.0f)}, // -x
            {XMFLOAT3(-a, a, a), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(-1.f, 0.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 1.0f, 1.0f)},
            {XMFLOAT3(-a, -a, a), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(-1.f, 0.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 0.0f, 1.0f)},
            {XMFLOAT3(-a, -a, -a), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(-1.f, 0.f, 0.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 0.0f, 0.0f)},

            {XMFLOAT3(-a, -a, a), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 0.0f, 1.0f)}, // +z
            {XMFLOAT3(-a, a, a), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 1.0f, 1.0f)},
            {XMFLOAT3(a, a, a), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 1.0f, 1.0f)},
            {XMFLOAT3(a, -a, a), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.f, 0.f, 1.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 0.0f, 1.0f)},

            {XMFLOAT3(a, -a, -a), XMFLOAT2(1.0f, 0.0f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 0.0f, 0.0f)}, // -z
            {XMFLOAT3(a, a, -a), XMFLOAT2(1.0f, 1.0f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(1.0f, 1.0f, 0.0f)},
            {XMFLOAT3(-a, a, -a), XMFLOAT2(0.0f, 1.0f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 1.0f, 0.0f)},
            {XMFLOAT3(-a, -a, -a), XMFLOAT2(0.0f, 0.0f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, //XMFLOAT3(0.0f, 0.0f, 0.0f)},
        };

        unsigned short cubeIndices [] =
        {
            0, 1, 2,    // +y
            0, 2, 3,

            4, 5, 6,    // -y
            4, 6, 7,

            8, 9, 10,    // +x
            8, 10, 11,

            12, 13, 14, // -x
            12, 14, 15,

            16, 17, 18, // +z
            16, 18, 19,

            20, 21, 22, // -z
            20, 22, 23,
        };

        m_indexCountCube = ARRAYSIZE(cubeIndices);

        D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
        vertexBufferData.pSysMem = cubeVertices;
        vertexBufferData.SysMemPitch = 0;
        vertexBufferData.SysMemSlicePitch = 0;
        CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &vertexBufferDesc,
                &vertexBufferData,
                &m_vertexBuffer
                )
            );

        D3D11_SUBRESOURCE_DATA indexBufferData = {0};
        indexBufferData.pSysMem = cubeIndices;
        indexBufferData.SysMemPitch = 0;
        indexBufferData.SysMemSlicePitch = 0;
        CD3D11_BUFFER_DESC indexBufferDesc(sizeof(cubeIndices), D3D11_BIND_INDEX_BUFFER);
        DX::ThrowIfFailed(
            m_deviceResources->GetD3DDevice()->CreateBuffer(
                &indexBufferDesc,
                &indexBufferData,
                &m_indexBuffer
                )
            );
    });*/

    auto createShadowBuffersTask = (createShadowPSPointTask).then([this]()
    {
        if (m_deviceSupportsD3D9Shadows)
        {
            // Init shadow map resources

            InitShadowMap();

            // Init samplers

            ID3D11Device1* pD3DDevice = m_deviceResources->GetD3DDevice();

            D3D11_SAMPLER_DESC comparisonSamplerDesc;
            ZeroMemory(&comparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
            comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
            comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
            comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
            comparisonSamplerDesc.BorderColor[0] = 1.0f;
            comparisonSamplerDesc.BorderColor[1] = 1.0f;
            comparisonSamplerDesc.BorderColor[2] = 1.0f;
            comparisonSamplerDesc.BorderColor[3] = 1.0f;
            comparisonSamplerDesc.MinLOD = 0.f;
            comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
            comparisonSamplerDesc.MipLODBias = 0.f;
            comparisonSamplerDesc.MaxAnisotropy = m_deviceResources->GetDeviceFeatureLevel() >= D3D_FEATURE_LEVEL_9_2 ? 4 : 2;
            comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
			/*
            // Point filtered shadow
			comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
            DX::ThrowIfFailed(
                pD3DDevice->CreateSamplerState(
                    &comparisonSamplerDesc,
                    &m_comparisonSampler_point
                    )
                );*/
			//Anisotropic filtered shadow
			comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
            DX::ThrowIfFailed(
                pD3DDevice->CreateSamplerState(
                    &comparisonSamplerDesc,
                    &m_comparisonSampler_linear
                    )
                );

            D3D11_SAMPLER_DESC linearSamplerDesc;
            ZeroMemory(&linearSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
            linearSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            linearSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            linearSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            linearSamplerDesc.MinLOD = 0;
            linearSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
            linearSamplerDesc.MipLODBias = 0.f;
            linearSamplerDesc.MaxAnisotropy = 0;
            linearSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            linearSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

            DX::ThrowIfFailed(
                pD3DDevice->CreateSamplerState(
                    &linearSamplerDesc,
                    &m_linearSampler
                    )
                );

            // Init render states for back/front face culling

            D3D11_RASTERIZER_DESC drawingRenderStateDesc;
            ZeroMemory(&drawingRenderStateDesc, sizeof(D3D11_RASTERIZER_DESC));
            drawingRenderStateDesc.CullMode = D3D11_CULL_BACK;
            drawingRenderStateDesc.FillMode = D3D11_FILL_SOLID;
            drawingRenderStateDesc.DepthClipEnable = false; // Feature level 9_1 requires DepthClipEnable == true
			drawingRenderStateDesc.AntialiasedLineEnable = true;
			drawingRenderStateDesc.MultisampleEnable = true;
            DX::ThrowIfFailed(
                pD3DDevice->CreateRasterizerState(
                    &drawingRenderStateDesc,
                    &m_drawingRenderState
                    )
                );

            D3D11_RASTERIZER_DESC shadowRenderStateDesc;
            ZeroMemory(&shadowRenderStateDesc, sizeof(D3D11_RASTERIZER_DESC));
            shadowRenderStateDesc.CullMode = D3D11_CULL_FRONT;
            shadowRenderStateDesc.FillMode = D3D11_FILL_SOLID;
            shadowRenderStateDesc.DepthClipEnable = true;
			shadowRenderStateDesc.AntialiasedLineEnable = true;
			shadowRenderStateDesc.MultisampleEnable = true;

            DX::ThrowIfFailed(
                pD3DDevice->CreateRasterizerState(
                    &shadowRenderStateDesc,
                    &m_shadowRenderState
                    )
                );
        }
    });

    // Once everything is loaded, we can let rendering happen.
    (createTexturePSTask && createComparisonPSTask && createCubeTask && createShadowBuffersTask).then([this]()
    {
        // Init static constant buffers
        UpdateAllConstantBuffers();

        m_loadingComplete = true;
    });
}

// Determine whether the driver supports depth comparison in the pixel shader.
void ShadowSceneRenderer::DetermineShadowFeatureSupport()
{
    ID3D11Device1* pD3DDevice = m_deviceResources->GetD3DDevice();

    D3D11_FEATURE_DATA_D3D9_SHADOW_SUPPORT isD3D9ShadowSupported;
    ZeroMemory(&isD3D9ShadowSupported, sizeof(isD3D9ShadowSupported));
    pD3DDevice->CheckFeatureSupport(
        D3D11_FEATURE_D3D9_SHADOW_SUPPORT,
        &isD3D9ShadowSupported,
        sizeof(D3D11_FEATURE_D3D9_SHADOW_SUPPORT)
        );

    if (isD3D9ShadowSupported.SupportsDepthAsTextureWithLessEqualComparisonFilter)
    {
        m_deviceSupportsD3D9Shadows = true;
    }
    else
    {
        m_deviceSupportsD3D9Shadows = false;
    }
}

// Initialize a new shadow buffer with size equal to m_shadowMapDimension.
void ShadowSceneRenderer::InitShadowMap()
{
	unsigned int sampleSize = m_deviceResources->GetSampleSize();

    ID3D11Device1* pD3DDevice = m_deviceResources->GetD3DDevice();
    CD3D11_TEXTURE2D_DESC shadowMapDesc;
    ZeroMemory(&shadowMapDesc, sizeof(D3D11_TEXTURE2D_DESC));
    shadowMapDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    shadowMapDesc.MipLevels = 1;
    shadowMapDesc.ArraySize = 1;
	shadowMapDesc.SampleDesc.Count = 1;//static_cast<UINT>(sampleSize);
	//shadowMapDesc.SampleDesc.Quality = static_cast<UINT>(m_deviceResources->GetQualityLevel());
    shadowMapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
    shadowMapDesc.Height = static_cast<UINT>(m_shadowMapDimension);
    shadowMapDesc.Width = static_cast<UINT>(m_shadowMapDimension);

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.Texture2D.MipSlice = 0;
	
    CD3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    shaderResourceViewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

	if (sampleSize > 1)
	{
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;  //D3D11_DSV_DIMENSION_TEXTURE2DMS
		depthStencilViewDesc.Texture2DMS.UnusedField_NothingToDefine = 0;

		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;  //D3D11_DSV_DIMENSION_TEXTURE2DMS
		shaderResourceViewDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
	}
	else
	{
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	}

    HRESULT hr = pD3DDevice->CreateTexture2D(
        &shadowMapDesc,
        nullptr,
        &m_shadowMap
        );

    hr = pD3DDevice->CreateDepthStencilView(
        m_shadowMap.Get(),
        &depthStencilViewDesc,
        &m_shadowDepthView
        );

    hr = pD3DDevice->CreateShaderResourceView(
        m_shadowMap.Get(),
        &shaderResourceViewDesc,
        &m_shadowResourceView
        );

    if (FAILED(hr))
    {
        shadowMapDesc.Format = DXGI_FORMAT_R16_TYPELESS;
        depthStencilViewDesc.Format = DXGI_FORMAT_D16_UNORM;
        shaderResourceViewDesc.Format = DXGI_FORMAT_R16_UNORM;

        DX::ThrowIfFailed(
            pD3DDevice->CreateTexture2D(
            &shadowMapDesc,
            nullptr,
            &m_shadowMap
            )
            );

        DX::ThrowIfFailed(
            pD3DDevice->CreateDepthStencilView(
            m_shadowMap.Get(),
            &depthStencilViewDesc,
            &m_shadowDepthView
            )
            );

        DX::ThrowIfFailed(
            pD3DDevice->CreateShaderResourceView(
            m_shadowMap.Get(),
            &shaderResourceViewDesc,
            &m_shadowResourceView
            )
            );
    }

    // Init viewport for shadow rendering.
    ZeroMemory(&m_shadowViewport, sizeof(D3D11_VIEWPORT));
    m_shadowViewport.Height = m_shadowMapDimension;
    m_shadowViewport.Width = m_shadowMapDimension;
    m_shadowViewport.MinDepth = 0.f;
    m_shadowViewport.MaxDepth = 1.f;
}

// Initializes view parameters when the window size changes.
void ShadowSceneRenderer::CreateWindowSizeDependentResources()
{
	Size windowSize = m_deviceResources->GetLogicalSize();
	aspectRatio = windowSize.Width / windowSize.Height;
    fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 1.5f;
    }

    // Note that the OrientationTransform3D matrix is post-multiplied here
    // in order to correctly orient the scene to match the display orientation.
    // This post-multiplication step is required for any draw calls that are
    // made to the swap chain render target. For draw calls to other targets,
    // this transform should not be applied.

    XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();
    XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

    // Set up the camera view.
    {
        XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovRH(
            fovAngleY,
            aspectRatio,
            8.f,
            600.0f
            );
		
        XMStoreFloat4x4(
            &m_cubeViewProjectionBufferData.projection,
            XMMatrixTranspose(perspectiveMatrix * orientationMatrix)
            );

		g_Projection = m_cubeViewProjectionBufferData.projection;

        // Eye is at (0, 14, 30), looking at point (0, -0.1, 0) with the up-vector along the y-axis.
		eye = { 30.0f, 14.0f, -84.0f, -8.0f };
        at = { 0.0f, -0.1f, 0.0f, 0.0f };
        up = { 0.0f, 1.0f, 0.0f, 0.0f };

		// create view matrix 
		g_View = XMMatrixLookAtRH(eye, at, up);

        // Store the eye position for lighting calculations.
        XMStoreFloat4(&m_cubeViewProjectionBufferData.pos, eye);

		XMStoreFloat4x4(&m_cubeViewProjectionBufferData.view, XMMatrixTranspose(g_View));
		//XMStoreFloat4x4(&m_cubeViewProjectionBufferData.view, (g_View));

		m_View = m_cubeViewProjectionBufferData.view;
    }

    // Initialize the model matrix for objects that aren't animated.
    XMStoreFloat4x4(&m_staticModelBufferData.model, XMMatrixIdentity());

    // Set up the light view.
    {
        XMMATRIX lightPerspectiveMatrix = XMMatrixPerspectiveFovRH(
            XM_PIDIV2,
            1.0f,
            1.0f,
            400.f   // change for light range!!!
            );

        XMStoreFloat4x4(
            &m_lightViewProjectionBufferData.projection,
            XMMatrixTranspose(lightPerspectiveMatrix)
            );

        // Point light at (20, 15, 20), pointed at the origin. POV up-vector is along the y-axis.
        eyel = { 20.0f, 30.0f, -60.0f, 0.0f };
        atl = { 0.0f, -0.1f, 0.0f, 0.0f };
        upl = { 0.0f, 1.0f, 0.0f, 0.0f };

        XMStoreFloat4x4(
            &m_lightViewProjectionBufferData.view,
            XMMatrixTranspose(XMMatrixLookAtRH(eyel, atl, upl))
            );

		// create view matrix 
		l_View = XMMatrixLookAtRH(eyel, atl, upl);

        // Store the light position to help calculate the shadow offset.
        XMStoreFloat4(&m_lightViewProjectionBufferData.pos, eyel);
    }

    // Set up the overlay view.
    {
        float orthoHeight = 3.f;
        float orthoWidth = orthoHeight * aspectRatio;

        XMStoreFloat4x4(
            &m_orthoViewProjectionBufferData.projection,
            XMMatrixTranspose(
                XMMatrixOrthographicRH(orthoWidth, orthoHeight, 0.01f, 10.f) * orientationMatrix
                )
            );

        // Overlay camera is at (0, 0, 1), pointed at origin, with the up-vector along the y-axis.
        static const XMVECTORF32 eye = { 0.0f, 0.0f, 1.0f, 0.0f };
        static const XMVECTORF32 at = { 0.0f, 0.0f, 0.0f, 0.0f };
        static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

        XMStoreFloat4x4(
            &m_orthoViewProjectionBufferData.view,
            XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up))
            );

        // Initialize the model matrix for the overlay quad used to display the shadow buffer contents.
        XMVECTORF32 overlayPlacement = { (orthoWidth / 2.f) - 1.2f, 0.2f, 0.f, 0.f };
        XMStoreFloat4x4(
            &m_orthoTransformBufferData.model,
            XMMatrixTranspose(XMMatrixTranslationFromVector(overlayPlacement))
            );
    }

    if (m_loadingComplete)
    {
        UpdateAllConstantBuffers();
    }
}

// Release all references to resources that depend on the graphics device.
// This method is invoked when the device is lost and resources are no longer usable.
void ShadowSceneRenderer::ReleaseDeviceDependentResources()
{
    m_loadingComplete = false;

    m_inputLayout.Reset();
    m_vertexBuffer.Reset();
    m_indexBuffer.Reset();
    m_vertexShader.Reset();
    m_simpleVertexShader.Reset();
    m_shadowPixelShader_point.Reset();
    m_shadowPixelShader_linear.Reset();
    m_comparisonShader.Reset();
    m_textureShader.Reset();

	m_textureResource.Reset();
	m_textureResourceCube1.Reset();
	m_textureworldnormals.Reset();
	m_texturemoonnormals.Reset();
	m_textureResourceCube2.Reset();
	m_textureResourceCube3.Reset();
	m_textureResourceLight.Reset();
	m_textureSampler.Reset();

    m_shadowMap.Reset();
    m_shadowDepthView.Reset();
    m_shadowResourceView.Reset();
    m_comparisonSampler_point.Reset();
    m_comparisonSampler_linear.Reset();
    m_linearSampler.Reset();

    m_cubeViewProjectionBuffer.Reset();
    m_lightViewProjectionBuffer.Reset();
    m_orthoViewProjectionBuffer.Reset();
    m_staticModelBuffer.Reset();
  
	m_lightModelBuffer.Reset();
	m_rotatedModelBuffer1.Reset();
	m_rotatedModelBuffer2.Reset();
	m_rotatedModelBuffer3.Reset();
    m_orthoTransformBuffer.Reset();

    m_shadowRenderState.Reset();
    m_drawingRenderState.Reset();

    m_vertexBufferQuad.Reset();
    m_indexBufferQuad.Reset();
    m_vertexBufferFloor.Reset();
    m_indexBufferFloor.Reset();
	m_vertexBufferSphere.Reset();
	m_indexBufferSphere.Reset();
}

void ShadowSceneRenderer::UpdateAllConstantBuffers()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    context->UpdateSubresource(m_cubeViewProjectionBuffer.Get(), 0, NULL, &m_cubeViewProjectionBufferData, 0, 0);

    context->UpdateSubresource(m_lightViewProjectionBuffer.Get(), 0, NULL, &m_lightViewProjectionBufferData, 0, 0);

    context->UpdateSubresource(m_orthoViewProjectionBuffer.Get(), 0, NULL, &m_orthoViewProjectionBufferData, 0, 0);

    context->UpdateSubresource(m_staticModelBuffer.Get(), 0, NULL, &m_staticModelBufferData, 0, 0);

    context->UpdateSubresource(m_orthoTransformBuffer.Get(), 0, NULL, &m_orthoTransformBufferData, 0, 0);

	context->UpdateSubresource(m_lightModelBuffer.Get(), 0, NULL, &m_lightModelBufferData, 0, 0);

	context->UpdateSubresource(m_rotatedModelBuffer1.Get(), 0, NULL, &m_rotatedModelBufferData1, 0, 0);

	context->UpdateSubresource(m_rotatedModelBuffer2.Get(), 0, NULL, &m_rotatedModelBufferData2, 0, 0);

	context->UpdateSubresource(m_rotatedModelBuffer3.Get(), 0, NULL, &m_rotatedModelBufferData3, 0, 0);
}

// Called once per frame, creates a Y rotation based on elapsed time.
void ShadowSceneRenderer::Update(DX::StepTimer const& timer)
{
	//get current fps value
	int fps = timer.GetFramesPerSecond();

	// activate cameradrive and update only every 0.33 sec if 60fps
	if (driveActivated == true & (RoundNum(fps) % 20 == 0 | fps <= 20))
	{
		CameraDrive();
	}

    // Convert degrees to radians, then convert seconds to rotation angle.
    float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
    double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
	float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

    // Uncomment the following line of code to oscillate the cube instead of
    // rotating it. Useful for testing different margin coefficients in the
    // pixel shader.

    totalRotation = 9.f + cos(totalRotation) *.2;

    float animRadians = (float)fmod(totalRotation, XM_2PI);

	//****************** sphere 1 ****************************************
	XMMATRIX mTranslate1 = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	XMMATRIX mScale1 = XMMatrixScaling(10.f, 10.f, 10.f);
    // Prepare to pass the view matrix, and updated model matrix, to the shader.
    //XMStoreFloat4x4(&m_rotatedModelBufferData1.model, XMMatrixTranspose(mScale1 * XMMatrixRotationY(animRadians)));
	XMStoreFloat4x4(&m_rotatedModelBufferData1.model, XMMatrixTranspose(mScale1 * mTranslate1)); // * XMMatrixRotationY(radians)

	// set normal map true for this sphere
	m_rotatedModelBufferData1.hasNormalMap = m_EarthNormalMap;

	s_World[0] = m_rotatedModelBufferData1.model;

	//****************** sphere 2 ****************************************
	// Prepare to pass the view matrix, and updated model matrix, to the shader.
	XMMATRIX mTranslate2 = XMMatrixTranslation(40.0f, 0.0f, 0.0f);
	XMMATRIX mScale2 = XMMatrixScaling(4.4f, 4.4f, 4.4f);
	XMMATRIX mOrbit = XMMatrixRotationY(-radians * 1.0f);

	// set normal map true for this sphere
	m_rotatedModelBufferData2.hasNormalMap = m_MoonNormalMap;

	XMStoreFloat4x4(&m_rotatedModelBufferData2.model, XMMatrixTranspose(mScale2 * mTranslate2));//  * mOrbit

	s_World[1] = m_rotatedModelBufferData2.model;

	//****************** sphere 3 ****************************************
	XMMATRIX mTranslate3 = XMMatrixTranslation(-40.0f, 5.0f, 30.0f);
	XMMATRIX mScale3 = XMMatrixScaling(5.2f, 5.2f, 5.2f);

	XMStoreFloat4x4(&m_rotatedModelBufferData3.model, XMMatrixTranspose(mScale3 * mTranslate3)); // * (XMMatrixRotationY(radians))
	
	s_World[2] = m_rotatedModelBufferData3.model;

	m_rotatedModelBufferData3.hasNormalMap = false;
	
	//******************* light source *********************************
	XMMATRIX mTranslate = XMMatrixTranslationFromVector(eyel);
	XMMATRIX mScale = XMMatrixScaling(0.05f, 0.05f, 0.05f);
	XMMATRIX mRotation = XMMatrixRotationQuaternion(quaternionl); 

	// first rotate then scale and translate, to rotate the cube around its own axis!!!
	XMStoreFloat4x4(&m_lightModelBufferData.model, XMMatrixTranspose(mRotation * mScale * mTranslate));
	m_lightModelBufferData.hasNormalMap = false;
	
	//****************** update light buffer to change light position
	XMStoreFloat4x4(&m_lightViewProjectionBufferData.view, XMMatrixTranspose(l_View));

	XMStoreFloat4(&m_lightViewProjectionBufferData.pos, eyel);

	// update with view matrix for new camera pos

	XMStoreFloat4x4(&m_cubeViewProjectionBufferData.view, XMMatrixTranspose(g_View));

	m_View = m_cubeViewProjectionBufferData.view;

	// Store the eye position for lighting calculations.
	XMStoreFloat4(&m_cubeViewProjectionBufferData.pos, eye);


	// store Triangles from all Objects in World space coordinates for intersection test and kd-tree
	if (transVertices == true && m_sphereIndices.size() != NULL)
	{
		// loop through each game object
		for (int i = 0; i < numberofSpheres; i++)
		{
			// fill TrianglesWorld with Triangles in World Space
			TrianglesWorld[i] = VerticesToTrianglesWorld(m_sphereVertices, m_sphereIndices, XMMatrixTranspose(XMLoadFloat4x4(&s_World[i])));

			// and pointer to triangles in world space
			for (int j = 0; j < TrianglesWorld[i].size(); j++)
			{
				TrianglesWorldPointers.push_back(&TrianglesWorld[i][j]);
			}
		}
		transVertices = false;

		// kd-tree ****************************************
		tree = new kdtree(&TrianglesWorldPointers);

		tree->prepareKDTreeForDrawing(tree->getRoot(), -15, 15, -15, 15, -15, 15);

		//tree->printTree(tree->getRoot(), 0);
		//OutputDebugStringA(std::to_string(tree->m_counter).c_str());

		// create the vertex and index buffer for drawing
		if (tree != NULL)
		{
			DrawKDTree(tree->vertexKD, Colors::LightBlue);
		}

	}

    // If the sample count has changed, recreate it.
    D3D11_TEXTURE2D_DESC desc = { 0 };
	unsigned int sampleSize = m_deviceResources->GetSampleSize();
    if (m_shadowMap != nullptr)
    {
        m_shadowMap->GetDesc(&desc);
        if (sampleSize != desc.SampleDesc.Count)
        {
            InitShadowMap();
        }
    }
}

std::vector<Triangle> VerticesToTrianglesWorld(std::vector<BasicVertex>& vertPosArray, std::vector<short>& indexPosArray, XMMATRIX& worldSpace)
{	
	std::vector<Triangle> triangle;
	Triangle tri;
	//Loop through each triangle in the object
	for (int i = 0; i < indexPosArray.size() / 3; i++)
	{
		//Triangle's vertices V1, V2, V3
		XMVECTOR tri1V1 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		XMVECTOR tri1V2 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
		XMVECTOR tri1V3 = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

		//Temporary 3d floats for each vertex
		float3 tV1, tV2, tV3;

		//Get triangle 
		tV1 = vertPosArray[indexPosArray[(i * 3) + 0]].pos;
		tV2 = vertPosArray[indexPosArray[(i * 3) + 1]].pos;
		tV3 = vertPosArray[indexPosArray[(i * 3) + 2]].pos;

		tri1V1 = XMVectorSet(tV1.x, tV1.y, tV1.z, 0.0f);
		tri1V2 = XMVectorSet(tV2.x, tV2.y, tV2.z, 0.0f);
		tri1V3 = XMVectorSet(tV3.x, tV3.y, tV3.z, 0.0f);

		//Transform the vertices to world space
		tri.tri1 = XMVector3TransformCoord(tri1V1, worldSpace);
		tri.tri2 = XMVector3TransformCoord(tri1V2, worldSpace);
		tri.tri3 = XMVector3TransformCoord(tri1V3, worldSpace);

		// calculate center and corners
		tri.center[0] = (XMVectorGetX(tri.tri1) + XMVectorGetX(tri.tri2) + XMVectorGetX(tri.tri3)) / 3;
		tri.center[1] = (XMVectorGetY(tri.tri1) + XMVectorGetY(tri.tri2) + XMVectorGetY(tri.tri3)) / 3;
		tri.center[2] = (XMVectorGetZ(tri.tri1) + XMVectorGetZ(tri.tri2) + XMVectorGetZ(tri.tri3)) / 3;
		
		tri.lower_corner[0] = std::fmin(XMVectorGetX(tri.tri1), std::fmin(XMVectorGetX(tri.tri2), XMVectorGetX(tri.tri3)));
		tri.lower_corner[1] = std::fmin(XMVectorGetY(tri.tri1), std::fmin(XMVectorGetY(tri.tri2), XMVectorGetY(tri.tri3)));
		tri.lower_corner[2] = std::fmin(XMVectorGetZ(tri.tri1), std::fmin(XMVectorGetZ(tri.tri2), XMVectorGetZ(tri.tri3)));
		tri.upper_corner[0] = std::fmax(XMVectorGetX(tri.tri1), std::fmax(XMVectorGetX(tri.tri2), XMVectorGetX(tri.tri3)));
		tri.upper_corner[1] = std::fmax(XMVectorGetY(tri.tri1), std::fmax(XMVectorGetY(tri.tri2), XMVectorGetY(tri.tri3)));
		tri.upper_corner[2] = std::fmax(XMVectorGetZ(tri.tri1), std::fmax(XMVectorGetZ(tri.tri2), XMVectorGetZ(tri.tri3)));

		tri.intersected = false;

		triangle.push_back(tri);

	}

	return triangle;
}

// Renders one frame using the vertex and pixel shaders.
void ShadowSceneRenderer::Render()
{
    // Loading is asynchronous. Only draw geometry after it's loaded.
    if (!m_loadingComplete)
    {
        return;
    }

    auto context = m_deviceResources->GetD3DDeviceContext();

    // For example, legacy devices that use WDDM 1.1 drivers do not support
    // sample comparison in D3D_FEATURE_LEVEL_9_x shaders.
    if (!m_deviceSupportsD3D9Shadows)
    {
        context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::Black);
        return;
    }

    // Clear the back buffer and depth stencil view.
    context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);
    context->ClearDepthStencilView(m_shadowDepthView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// clear for Multisampling
	if (m_deviceResources->GetOverlaySwapChain() != nullptr)
	{
		// Clear overlay.
		const DirectX::XMVECTORF32 clear = { 0.f, 0.f, 0.f, 0.f };
		context->ClearRenderTargetView(m_deviceResources->GetOverlayRenderTargetView(), clear);
	}

    // Update constant buffer data that has changed.
	// 1st Spere
    m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(m_rotatedModelBuffer1.Get(),0, NULL, &m_rotatedModelBufferData1,0, 0);
	// 2nd Sphere
	m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(m_rotatedModelBuffer2.Get(), 0, NULL, &m_rotatedModelBufferData2, 0, 0);
	// Cube
	m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(m_rotatedModelBuffer3.Get(), 0, NULL, &m_rotatedModelBufferData3, 0, 0);
	// light cube
	m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(m_lightModelBuffer.Get(), 0, NULL, &m_lightModelBufferData, 0, 0);
	// because the cube view matrix is changed in the camera update function
	m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(m_cubeViewProjectionBuffer.Get(), 0, NULL, &m_cubeViewProjectionBufferData, 0, 0);
	// because it is possible to change the position of the light -> update!
	m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(m_lightViewProjectionBuffer.Get(), 0, NULL, &m_lightViewProjectionBufferData, 0, 0);

    // Render the shadow buffer, then render the scene with shadows.
    RenderShadowMap();
    RenderSceneWithShadows();

    // display the shadow buffer as an overlay.
    //RenderQuad();
}

// Loads the shadow buffer with shadow information.
void ShadowSceneRenderer::RenderShadowMap()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Ensure the depth buffer is not bound for input before using it as a render target.
    // If you don't do this, the Direct3D runtime will have to force an unbind which 
    // causes warnings in the debug output.
    ID3D11ShaderResourceView* nullSrv = nullptr;
    context->PSSetShaderResources(0, 1, &nullSrv);

    // Render all the objects in the scene that can cast shadows onto themselves or onto other objects.

    // Only bind the ID3D11DepthStencilView for output.
    context->OMSetRenderTargets(0,nullptr,m_shadowDepthView.Get());

    // Set rendering state.
    context->RSSetState(m_shadowRenderState.Get());
    context->RSSetViewports(1, &m_shadowViewport);

    // Each vertex is one instance of the VertexPositionTexNormColor struct.
    UINT stride = sizeof(VertexPositionTexNormColor);
    UINT offset = 0;

	//************************** set sphere vertex buffer *************************
	context->IASetVertexBuffers(0, 1, m_vertexBufferSphere.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBufferSphere.Get(), DXGI_FORMAT_R16_UINT,0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());

    // Bind the vertex shader used to generate the depth map.
    context->VSSetShader( m_simpleVertexShader.Get(),nullptr,0);

    // Send the constant buffers to the Graphics device.
    context->VSSetConstantBuffers(0,1,m_lightViewProjectionBuffer.GetAddressOf());

    context->VSSetConstantBuffers(1,1,m_rotatedModelBuffer1.GetAddressOf());

    // Set the pixel shader to null to disable the pixel shader and 
    // output-merger stages.
    ID3D11PixelShader* nullPS = nullptr;
    context->PSSetShader(nullPS, nullptr,0);

    // Draw the objects.
    context->DrawIndexed(m_indexCountSphere,0,0);

	//******************** 2nd sphere ****************************************
	
	context->VSSetConstantBuffers(1, 1, m_rotatedModelBuffer2.GetAddressOf());
	
	// Draw the objects.
	context->DrawIndexed(m_indexCountSphere, 0, 0);
	
	//******************** set cube Vertex Buffers ***************************

	//context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

	//context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	
	//******************** 3rd cube ****************************************
	context->VSSetConstantBuffers(1, 1, m_rotatedModelBuffer3.GetAddressOf());

	// Draw the objects.
	context->DrawIndexed(m_indexCountSphere, 0, 0);
	/*
	//******************** light cube ****************************************
	context->VSSetConstantBuffers(1, 1, m_lightModelBuffer.GetAddressOf());

	// Draw the objects.
	context->DrawIndexed(m_indexCountCube, 0, 0);
	*/
}

// Render the objects in the scene that will have shadows cast onto them.
void ShadowSceneRenderer::RenderSceneWithShadows()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

    auto context = m_deviceResources->GetD3DDeviceContext();

    // Set render targets to the screen.
    ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
    context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

    // Set rendering state.
    context->RSSetState(m_drawingRenderState.Get());

    auto view = m_deviceResources->GetScreenViewport();
    context->RSSetViewports(1, &view);

    // Set up IA format for floor and cube.
    UINT stride = sizeof(VertexPositionTexNormColor);
    UINT offset = 0;
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());

    // First, draw the floor. **********************************************

    // Each vertex is one instance of the VertexPositionTexNormColor struct.
    context->IASetVertexBuffers(0,1, m_vertexBufferFloor.GetAddressOf(), &stride,&offset);
    context->IASetIndexBuffer(m_indexBufferFloor.Get(),DXGI_FORMAT_R16_UINT, 0);

    // Attach our vertex shader.
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

    // Send the constant buffers to the Graphics device.
    context->VSSetConstantBuffers(0, 1, m_cubeViewProjectionBuffer.GetAddressOf());

    context->VSSetConstantBuffers(1, 1, m_staticModelBuffer.GetAddressOf());

    context->VSSetConstantBuffers(2, 1, m_lightViewProjectionBuffer.GetAddressOf());

    ID3D11PixelShader* pixelShader;
    ID3D11SamplerState** comparisonSampler;

   pixelShader = m_shadowPixelShader_point.Get();
   comparisonSampler = m_comparisonSampler_linear.GetAddressOf();

    // Attach our pixel shader.
    context->PSSetShader(pixelShader, nullptr, 0);

	// shadow Texture Resources **************************************************
    context->PSSetSamplers(0, 1, comparisonSampler);
    context->PSSetShaderResources(0, 1, m_shadowResourceView.GetAddressOf());

	// set the loaded Texture from dds file for floor ****************************
	context->PSSetShaderResources(1, 1, m_textureResource.GetAddressOf());
	// Sampler

	ID3D11SamplerState* samplers[] = { m_textureSampler.Get(), m_normalSampler.Get() };
	//then put the two samplers into an array and check for filterMode -> set filter
	if		(m_filterMode == 0)	{ samplers[0] = m_textureSampler_1.Get(); }
	else if (m_filterMode == 1)	{ samplers[0] = m_textureSampler_2.Get(); }
	else if (m_filterMode == 2) { samplers[0] = m_textureSampler_3.Get(); }
	else if (m_filterMode == 3) { samplers[0] = m_textureSampler_4.Get(); }
	else if (m_filterMode == 4) { samplers[0] = m_textureSampler_5.Get(); }
	else if (m_filterMode == 5) { samplers[0] = m_textureSampler_6.Get(); }
	else if (m_filterMode == 6) { samplers[0] = m_textureSampler_7.Get(); }
	else if (m_filterMode == 7) { samplers[0] = m_textureSampler_8.Get(); }
	

	//context->PSSetSamplers(1, 1, m_textureSampler.GetAddressOf());
	// set samplers
	context->PSSetSamplers(1, 2, samplers);

	// update Pixel shader input for normal map -> check if has normal map
	context->PSSetConstantBuffers(0, 1, m_staticModelBuffer.GetAddressOf());

    // Draw the floor.
    context->DrawIndexed(m_indexCountFloor, 0, 0);

    // Then draw the sphere. We only need to change the pipeline state that's different.
    // Each vertex is one instance of the VertexPositionTexNormColor struct.
    context->IASetVertexBuffers(0, 1, m_vertexBufferSphere.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_indexBufferSphere.Get(), DXGI_FORMAT_R16_UINT, 0);

	//******************************** 1st Sphere ***********************************
	// set the loaded Texture from dds file ***************************************
	//context->PSSetShaderResources(1, 1, m_textureResourceCube1.GetAddressOf());
	// first put the two surfaces into an array
	ID3D11ShaderResourceView* shaderResourceViews[] = {
		m_textureResourceCube1.Get(),
		m_textureworldnormals.Get()
	};
	// set textures
	context->PSSetShaderResources(1, 2, shaderResourceViews);
    // Set a different model matrix for rotating the sphere.
    context->VSSetConstantBuffers(1,1, m_rotatedModelBuffer1.GetAddressOf());
	// update Pixel shader input for normal map -> check if has normal map
	context->PSSetConstantBuffers(0, 1, m_rotatedModelBuffer1.GetAddressOf());

    // Draw the sphere.
    context->DrawIndexed(m_indexCountSphere, 0, 0);
	
	//******************************** 2nd Sphere ***********************************
	// set the loaded Texture from dds file ***************************************
	shaderResourceViews[0] = m_textureResourceCube2.Get();
	shaderResourceViews[1] = m_texturemoonnormals.Get();
	// set textures
	context->PSSetShaderResources(1, 2, shaderResourceViews);
	// Set a different model matrix for rotating the cube.
	context->VSSetConstantBuffers(1, 1, m_rotatedModelBuffer2.GetAddressOf());
	// update Pixel shader input for normal map -> check if has normal map
	context->PSSetConstantBuffers(0, 1, m_rotatedModelBuffer2.GetAddressOf());

	// Draw the sphere.
	context->DrawIndexed(m_indexCountSphere, 0, 0);
	
	//******************************** change back to cube **************************
	//context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	//context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	
	//******************************** 3rd Cube ***********************************
	// set the loaded Texture from dds file ***************************************
	shaderResourceViews[0] = m_textureResourceCube3.Get();
	shaderResourceViews[1] = nullptr;
	// set textures
	context->PSSetShaderResources(1, 2, shaderResourceViews);
	//context->PSSetShaderResources(1, 1, m_textureResourceCube3.GetAddressOf());
	// Set a different model matrix for rotating the cube.
	context->VSSetConstantBuffers(1, 1, m_rotatedModelBuffer3.GetAddressOf());
	// update Pixel shader input for normal map->check if has normal map
	context->PSSetConstantBuffers(0, 1, m_rotatedModelBuffer3.GetAddressOf());

	// Draw the cube.
	context->DrawIndexed(m_indexCountSphere, 0, 0);

	//******************************** change back to cube **************************
	context->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	context->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	//******************************** Light Cube ***********************************
	// set the loaded Texture from dds file ***************************************
	shaderResourceViews[0] = m_textureResourceLight.Get();
	shaderResourceViews[1] = nullptr;
	// set textures
	context->PSSetShaderResources(1, 2, shaderResourceViews);
	//context->PSSetShaderResources(1, 1, m_textureResourceLight.GetAddressOf());
	// Set a different model matrix for rotating the cube.
	context->VSSetConstantBuffers(1, 1, m_lightModelBuffer.GetAddressOf());
	// update Pixel shader input for normal map -> check if has normal map
	context->PSSetConstantBuffers(0, 1, m_lightModelBuffer.GetAddressOf());

	// Draw the cube.
	context->DrawIndexed(m_indexCountCube, 0, 0);

	// draw the ray and the hit triangle
	if (mPickedTriangle != -1)
	{
		DrawRay(pointOrigin, pointOnNearest, Colors::Tomato);

		DrawHitTriangle(hitTriangle, Colors::LightBlue);

		// Attach our pixel shader.
		context->PSSetShader(m_lineShader.Get(), nullptr, 0);

		// Bind the vertex shader used to generate the depth map.
		context->VSSetShader(m_lineVertexShader.Get(), nullptr, 0);

		context->VSSetConstantBuffers(0, 1, m_cubeViewProjectionBuffer.GetAddressOf());

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		context->IASetInputLayout(m_inputLayout.Get());

		//******************************** change to line **************************
		context->IASetVertexBuffers(0, 1, m_lineVertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_lineIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		context->DrawIndexed(m_indexCountLine, 0, 0);

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		context->IASetInputLayout(m_inputLayout.Get());

		//******************************** change to triangle **************************
		context->IASetVertexBuffers(0, 1, m_triangleVertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_triangleIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		context->DrawIndexed(m_indexCountTriangle, 0, 0);
	}

	// draw the kdtree controlled by XAML checkbox
	if (m_drawKDTree)
	{
		// Attach our pixel shader.
		context->PSSetShader(m_lineShader.Get(), nullptr, 0);

		// Bind the vertex shader used to generate the depth map.
		context->VSSetShader(m_lineVertexShader.Get(), nullptr, 0);

		context->VSSetConstantBuffers(0, 1, m_cubeViewProjectionBuffer.GetAddressOf());

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
		context->IASetInputLayout(m_inputLayout.Get());

		//******************************** change to kdTree **************************
		context->IASetVertexBuffers(0, 1, m_kdTreeVertexBuffer.GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(m_kdTreeIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

		context->DrawIndexed(m_indexCountkdTree, 0, 0);
	}

}

// Uses an ortho projection to render the shadow map as an overlay.
void ShadowSceneRenderer::RenderQuad()
{
    auto context = m_deviceResources->GetD3DDeviceContext();

    // Set render targets to the screen.
    ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
    context->OMSetRenderTargets(1, targets, nullptr);
    context->RSSetState(m_drawingRenderState.Get());
    context->RSSetViewports(1, &m_deviceResources->GetScreenViewport());

    // Each vertex is one instance of the VertexPositionTexNormColor struct.
    UINT stride = sizeof(VertexPositionTexNormColor);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_vertexBufferQuad.GetAddressOf(), &stride, &offset);

    context->IASetIndexBuffer(
        m_indexBufferQuad.Get(),
        DXGI_FORMAT_R16_UINT, // Each index is one 16-bit unsigned integer (short).
        0
        );

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->IASetInputLayout(m_inputLayout.Get());

    // Attach our vertex shader.
    context->VSSetShader(m_vertexShader.Get(), nullptr, 0);

    // Send the constant buffer to the Graphics device.
    context->VSSetConstantBuffers(0, 1, m_orthoViewProjectionBuffer.GetAddressOf());

    context->VSSetConstantBuffers(1, 1, m_orthoTransformBuffer.GetAddressOf());

    context->VSSetConstantBuffers(2, 1, m_lightViewProjectionBuffer.GetAddressOf());

    ID3D11SamplerState** sampler;
    ID3D11PixelShader* shader;
    if (m_deviceResources->GetDeviceFeatureLevel() <= D3D_FEATURE_LEVEL_9_3)
    {
        // In feature level 9_1, we can't use a depth buffer with a non-comparison
        // sampler. So we display the stencil.
        // Note that 9_2 and 9_3 can work around that with a 16-bit stencil and a
        // texture copy - but this example doesn't do that.
        sampler = m_comparisonSampler_linear.GetAddressOf();
        shader = m_comparisonShader.Get();
    }
    else
    {
        // In feature levels 10_0 and above, we can use the shadow buffer with
        // a non-comparison sampler and display varying depth as shading.
        sampler = m_linearSampler.GetAddressOf();
        shader = m_textureShader.Get();
    }

    // Attach our pixel shader.
    context->PSSetShader(shader, nullptr, 0);

    context->PSSetSamplers(0, 1, sampler);
    context->PSSetShaderResources(0, 1, m_shadowResourceView.GetAddressOf());

    // Draw the quad.
    context->DrawIndexed(m_indexCountQuad, 0, 0);
}

void ShadowSceneRenderer::TestIntersection(float pointX, float pointY, float HEIGHT, float WIDTH)
{
	// DirectX Unproject Method
	/*
	// rayOrigin
	XMVECTOR vector1 = XMVector3Unproject(
		XMVectorSet(pointX, pointY, 0.0f, 1.0f),
		0.0f,
		0.0f,
		WIDTH,
		HEIGHT,
		0.0f,
		1.0f,
		XMLoadFloat4x4(&g_Projection),
		XMLoadFloat4x4(&m_View),
		XMLoadFloat4x4(&g_World)
		);

	// rayDirection = XMVector3Normalize(vector2 - vector1)
	XMVECTOR vector2 = XMVector3Unproject(
		XMVectorSet(pointX, pointY, 1.0f, 1.0f),
		0.0f,
		0.0f,
		WIDTH,
		HEIGHT,
		0.0f,
		1.0f,
		XMLoadFloat4x4(&g_Projection),
		XMLoadFloat4x4(&m_View),
		XMLoadFloat4x4(&g_World)
		);
		*/

	XMVECTOR pickRayInViewSpaceDir = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR pickRayInViewSpacePos = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	float PRVecX, PRVecY, PRVecZ;

	//Transform 2D pick position on screen space to 3D ray in View space
	PRVecX = (((2.0f * pointX) / WIDTH) - 1) / g_Projection._11;
	PRVecY = -(((2.0f * pointY) / HEIGHT) - 1) / g_Projection._22;
	PRVecZ = -1.0f;    //View space's Z direction ranges from 0 to 1, so we set 1 since the ray goes "into" the screen

	//std::string stroutput = "x-pos = " + std::to_string(PRVecX) + "; y-pos = " + std::to_string(PRVecY) + "\n";
	//OutputDebugStringA(stroutput.c_str());

	pickRayInViewSpaceDir = XMVectorSet(PRVecX, PRVecY, PRVecZ, 0.0f);

	// Transform 3D Ray from View space to 3D ray in World space
	XMMATRIX inverseMatrix;
	XMVECTOR matInvDeter;

	inverseMatrix = XMMatrixInverse(&matInvDeter, g_View);    //Inverse of View Space matrix is World space matrix

	rayOrigin = XMVector3TransformCoord(pickRayInViewSpacePos, inverseMatrix);  // camera position
	rayDirection = XMVector3TransformNormal(pickRayInViewSpaceDir, inverseMatrix);

	rayDirection = XMVector3Normalize(rayDirection);

	// kd-tree ************************
	// remove previous intersections
	for (unsigned int i = 0; i < TrianglesWorldPointers.size(); ++i) {
		if (TrianglesWorldPointers[i]->intersected) {
			TrianglesWorldPointers[i]->intersected = false;
		}
	}

	tree->m_closestDist = FLT_MAX;


	// check raycast against all Triangles vs kd-Tree and output time in debug console
	TimerFunc timer;

	timer.startTimer();
	tree->rayCastAll(tree->getRoot(), rayOrigin, rayDirection);
	OutputDebugStringA("Raycast all triangles [ms]: ");
	timer.stopTimer();
	timer.startTimer();
	tree->rayCast(tree->getRoot(), rayOrigin, rayDirection, 200.f, 0.f);
	OutputDebugStringA("Raycast triangles with kd-tree [ms]: ");
	timer.stopTimer();
	// ---------------------

	bool test = false;
	float tempDist = FLT_MAX;

	int counter = 0;
	// check found intersection
	for (unsigned int i = 0; i < TrianglesWorldPointers.size(); ++i) {
		if (TrianglesWorldPointers[i]->intersected && TrianglesWorldPointers[i]->distance < tempDist)
		{
			hitTriangle = *TrianglesWorldPointers[i];
			
			pointInPlane = hitTriangle.intersectionPoint;
			test = true;
			mPickedTriangle = 1;
			tempDist = hitTriangle.distance;
			/*
			counter++;
			OutputDebugStringA(std::to_string(counter).c_str());
			OutputDebugStringA("\n");
			OutputDebugStringA(std::to_string(tempDist).c_str());
			OutputDebugStringA("\n");
			*/
		}
	}
	
	// check measurement type and set according values 
	if (pointToCamera) // point-to-camera measurement
	{
		pointOrigin = rayOrigin;
		pointOnNearest = pointInPlane;

		closestDist = hitTriangle.distance;
	}
	else  // point-to-point measurement
	{
		// save previous found nearest point as origin
		pointOrigin = pointOnNearest;
		// next nearest point for point to point measure
		pointOnNearest = pointInPlane;

		// update dist between the points
		float xSqu = std::powf((XMVectorGetX(pointOrigin) - XMVectorGetX(pointOnNearest)), 2);
		float ySqu = std::powf((XMVectorGetY(pointOrigin) - XMVectorGetY(pointOnNearest)), 2);
		float zSqu = std::powf((XMVectorGetZ(pointOrigin) - XMVectorGetZ(pointOnNearest)), 2);

		closestDist = std::sqrtf(xSqu + ySqu + zSqu);
	}

	// ********************************
	/*
	// **** similiar to rayCastAll -> check all Triangles for intersection
	//rayOrigin = vector1;
	//rayDirection = XMVector3Normalize(vector2 - vector1);
	closestDist = FLT_MAX;
	float tempDist;
	bool test = false;
	for (int i = 0; i < numberofSpheres; i++)
	{
		//tempDist = pick(rayOrigin, rayDirection, m_sphereVertices, m_sphereIndices, XMMatrixTranspose(XMLoadFloat4x4(&s_World[i])));
		tempDist = pick(rayOrigin, rayDirection, TrianglesWorld[i]);
		if (tempDist < closestDist)
		{
			if (pointToCamera) // point-to-camera measurement
			{
				pointOrigin = rayOrigin;

				closestDist = tempDist;
				pointOnNearest = pointInPlane;
				test = true;
				mPickedTriangle = 1;

				hitTriangle = tempTriangle;
			}
			else  // point-to-point measurement
			{
				// save previous found nearest point as origin
				pointOrigin = pointOnNearest;
				// next nearest point for point to point measure
				pointOnNearest = pointInPlane;
				test = true;
				mPickedTriangle = 1;

				hitTriangle = tempTriangle;

				// update dist between the points
				float xSqu = std::powf((XMVectorGetX(pointOrigin) - XMVectorGetX(pointOnNearest)), 2);
				float ySqu = std::powf((XMVectorGetY(pointOrigin) - XMVectorGetY(pointOnNearest)), 2);
				float zSqu = std::powf((XMVectorGetZ(pointOrigin) - XMVectorGetZ(pointOnNearest)), 2);
				
				closestDist = std::sqrtf(xSqu + ySqu + zSqu);
			}
		}
	}
	*/
	
	//test = RaySphereIntersect(rayOrigin, rayDirection, 0.f);
	//std::string stroutput = "x = " + std::to_string(vx) + "; y = " + std::to_string(vy) + "\n";
	//OutputDebugStringA(stroutput.c_str());
	if (test == true)
	{
		OutputDebugStringA("Intersection = YES\n");
		mPickedTriangle = 1;
	}
	else
	{
		OutputDebugStringA("Intersection = NO\n");
		mPickedTriangle = -1;
	}
	
}


float pick(XMVECTOR& rayOrigin, XMVECTOR& rayDirection, std::vector<Triangle>& vertices)
{
	//Loop through each triangle in the object
	for each(Triangle triangle in vertices)
	{
		float dist = intersectRayTriangle(triangle, rayOrigin, rayDirection);
		if (dist != FLT_MAX)
		{
			tempTriangle = triangle;
			return dist;
		}
	}
	//return the max float value (near infinity) if an object was not picked
	return FLT_MAX;
}

float intersectRayTriangle(Triangle& tri, XMVECTOR& rayOrigin, XMVECTOR& rayDirection)
{
	//Triangle's vertices V1, V2, V3
	XMVECTOR tri1V1 = tri.tri1;
	XMVECTOR tri1V2 = tri.tri2;
	XMVECTOR tri1V3 = tri.tri3;

	//Find the normal using U, V coordinates (two edges)
	XMVECTOR U = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR V = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR faceNormal = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	U = tri1V2 - tri1V1;
	V = tri1V3 - tri1V1;

	//Compute face normal by crossing U, V
	faceNormal = XMVector3Cross(U, V);

	faceNormal = XMVector3Normalize(faceNormal);

	//Calculate a point on the triangle for the plane equation
	XMVECTOR triPoint = tri1V1;

	//Get plane equation ("Ax + By + Cz + D = 0") Variables
	float tri1A = XMVectorGetX(faceNormal);
	float tri1B = XMVectorGetY(faceNormal);
	float tri1C = XMVectorGetZ(faceNormal);
	float tri1D = (-tri1A*XMVectorGetX(triPoint) - tri1B*XMVectorGetY(triPoint) - tri1C*XMVectorGetZ(triPoint));

	//Now we find where (on the ray) the ray intersects with the triangles plane
	float ep1, ep2, t = 0.0f;
	float planeIntersectX, planeIntersectY, planeIntersectZ = 0.0f;
	pointInPlane = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);

	ep1 = (XMVectorGetX(rayOrigin) * tri1A) + (XMVectorGetY(rayOrigin) * tri1B) + (XMVectorGetZ(rayOrigin) * tri1C);
	ep2 = (XMVectorGetX(rayDirection) * tri1A) + (XMVectorGetY(rayDirection) * tri1B) + (XMVectorGetZ(rayDirection) * tri1C);

	//Make sure there are no divide-by-zeros
	if (ep2 != 0.0f)
		t = -(ep1 + tri1D) / (ep2);

	if (t > 0.0f)    //Make sure you don't pick objects behind the camera
	{
		//Get the point on the plane
		planeIntersectX = XMVectorGetX(rayOrigin) + XMVectorGetX(rayDirection) * t;
		planeIntersectY = XMVectorGetY(rayOrigin) + XMVectorGetY(rayDirection) * t;
		planeIntersectZ = XMVectorGetZ(rayOrigin) + XMVectorGetZ(rayDirection) * t;

		pointInPlane = XMVectorSet(planeIntersectX, planeIntersectY, planeIntersectZ, 0.0f);

		//Call function to check if point is in the triangle
		if (PointInTriangle(tri1V1, tri1V2, tri1V3, pointInPlane))
		{
			//Return the distance to the hit, so you can check all the other pickable objects in your scene
			//and choose whichever object is closest to the camera
			return t / 2.0f;
		}
	}

	return FLT_MAX;
}

bool PointInTriangle(XMVECTOR& triV1, XMVECTOR& triV2, XMVECTOR& triV3, XMVECTOR& point)
{
	//To find out if the point is inside the triangle, we will check to see if the point
	//is on the correct side of each of the triangles edges.

	XMVECTOR cp1 = XMVector3Cross((triV3 - triV2), (point - triV2));
	XMVECTOR cp2 = XMVector3Cross((triV3 - triV2), (triV1 - triV2));
	if (XMVectorGetX(XMVector3Dot(cp1, cp2)) >= 0)
	{
		cp1 = XMVector3Cross((triV3 - triV1), (point - triV1));
		cp2 = XMVector3Cross((triV3 - triV1), (triV2 - triV1));
		if (XMVectorGetX(XMVector3Dot(cp1, cp2)) >= 0)
		{
			cp1 = XMVector3Cross((triV2 - triV1), (point - triV1));
			cp2 = XMVector3Cross((triV2 - triV1), (triV3 - triV1));
			if (XMVectorGetX(XMVector3Dot(cp1, cp2)) >= 0)
			{
				return true;
			}
			else
				return false;
		}
		else
			return false;
	}
	return false;
}

//--------------------------------------------------------------------------------------
void ShadowSceneRenderer::DrawRay(FXMVECTOR Origin, FXMVECTOR Destination, FXMVECTOR color)
{
	BasicVertex verts[2];
	//VertexPositionColor verts[2];
	verts[0].pos = {XMVectorGetX(Origin), XMVectorGetY(Origin), XMVectorGetZ(Origin)};
	//XMStoreFloat3(&verts[0].pos, Origin);

	//XMVECTOR NormDirection = XMVector3Normalize(Direction);
	//XMVECTOR RayDirection = Direction * 1000.f;
	//RayDirection = XMVectorAdd(RayDirection, Origin);

	// instead of Direction with a certain length, draw the pointOnNearest of the object where it has been hit !!
	//verts[1].pos = { XMVectorGetX(RayDirection), XMVectorGetY(RayDirection), XMVectorGetZ(RayDirection) };
	verts[1].pos = { XMVectorGetX(Destination), XMVectorGetY(Destination), XMVectorGetZ(Destination) };

	XMFLOAT3 vertcolor;
	XMStoreFloat3(&vertcolor, color);
	verts[0].color.x = vertcolor.x; verts[0].color.y = vertcolor.y; verts[0].color.z = vertcolor.z;
	verts[1].color.x = vertcolor.x; verts[1].color.y = vertcolor.y; verts[1].color.z = vertcolor.z;

	unsigned short lineIndices[] =
	{
		0, 1,
	};

	m_indexCountLine = ARRAYSIZE(verts);

	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = verts;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(verts), D3D11_BIND_VERTEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			&m_lineVertexBuffer
			)
		);

	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	indexBufferData.pSysMem = lineIndices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(lineIndices), D3D11_BIND_INDEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			&m_lineIndexBuffer
			)
		);
}

//--------------------------------------------------------------------------------------
void ShadowSceneRenderer::DrawHitTriangle(Triangle& tri, FXMVECTOR color)
{
	XMVECTOR U = tri.tri2 - tri.tri1;
	XMVECTOR V = tri.tri3 - tri.tri1;

	//Compute face normal by crossing U, V
	XMVECTOR faceNormal = XMVector3Cross(U, V);

	// compute face normal to draw triangle a slightly over the original triangle!
	faceNormal = XMVector3Normalize(faceNormal) * -1.f / 100.f;

	BasicVertex verts[3];

	verts[0].pos = { XMVectorGetX(tri.tri1) + XMVectorGetX(faceNormal), XMVectorGetY(tri.tri1) + XMVectorGetY(faceNormal), XMVectorGetZ(tri.tri1) + XMVectorGetZ(faceNormal) };
	
	verts[1].pos = { XMVectorGetX(tri.tri2) + XMVectorGetX(faceNormal), XMVectorGetY(tri.tri2) + XMVectorGetY(faceNormal), XMVectorGetZ(tri.tri2) + XMVectorGetZ(faceNormal) };

	verts[2].pos = { XMVectorGetX(tri.tri3) + XMVectorGetX(faceNormal), XMVectorGetY(tri.tri3) + XMVectorGetY(faceNormal), XMVectorGetZ(tri.tri3) + XMVectorGetZ(faceNormal) };

	XMFLOAT3 vertcolor;
	XMStoreFloat3(&vertcolor, color);
	verts[0].color.x = vertcolor.x; verts[0].color.y = vertcolor.y; verts[0].color.z = vertcolor.z;
	verts[1].color.x = vertcolor.x; verts[1].color.y = vertcolor.y; verts[1].color.z = vertcolor.z;
	verts[2].color.x = vertcolor.x; verts[2].color.y = vertcolor.y; verts[2].color.z = vertcolor.z;

	unsigned short triangleIndices[] =
	{
		0, 1, 2,
	};

	m_indexCountTriangle = ARRAYSIZE(verts);

	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = verts;
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(verts), D3D11_BIND_VERTEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			&m_triangleVertexBuffer
			)
		);

	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	indexBufferData.pSysMem = triangleIndices;
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(triangleIndices), D3D11_BIND_INDEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			&m_triangleIndexBuffer
			)
		);
}

//--------------------------------------------------------------------------------------
void ShadowSceneRenderer::DrawKDTree(std::vector<BasicVertex> vertices, FXMVECTOR color)
{
	XMFLOAT3 vertcolor;
	XMStoreFloat3(&vertcolor, color);

	//BasicVertex *vertexArr = new BasicVertex[vertices.size()];

	std::vector<unsigned short> kdIndices;

	for (int i = 0; i < vertices.size(); i++)
	{
		vertices[i].color.x = vertcolor.x; vertices[i].color.y = vertcolor.y; vertices[i].color.z = vertcolor.z;
		
		// index of -1 indicates an explicit 'cut' or 'restart' of the current strip
		if (i % 5 == 0) { 
			kdIndices.push_back(-1); 
			kdIndices.push_back(i); }
		else { 
			kdIndices.push_back(i); }
		
	}

	/*
	unsigned int boxCount = vertices.size() / 8u;
	m_indexCountkdTree = boxCount * 24u;
	unsigned long* indices = new unsigned long[m_indexCountkdTree];

	for (unsigned int i = 0u; i < boxCount; ++i)
	{
		indices[i * 24u] = static_cast<unsigned long>(i * 8u);
		indices[i * 24u + 1u] = static_cast<unsigned long>(i * 8u + 1u);
		indices[i * 24u + 2u] = static_cast<unsigned long>(i * 8u + 1u);
		indices[i * 24u + 3u] = static_cast<unsigned long>(i * 8u + 2u);
		indices[i * 24u + 4u] = static_cast<unsigned long>(i * 8u + 2u);
		indices[i * 24u + 5u] = static_cast<unsigned long>(i * 8u + 3u);
		indices[i * 24u + 6u] = static_cast<unsigned long>(i * 8u + 3u);
		indices[i * 24u + 7u] = static_cast<unsigned long>(i * 8u);
		indices[i * 24u + 8u] = static_cast<unsigned long>(i * 8u + 4u);
		indices[i * 24u + 9u] = static_cast<unsigned long>(i * 8u + 5u);
		indices[i * 24u + 10u] = static_cast<unsigned long>(i * 8u + 5u);
		indices[i * 24u + 11u] = static_cast<unsigned long>(i * 8u + 6u);
		indices[i * 24u + 12u] = static_cast<unsigned long>(i * 8u + 6u);
		indices[i * 24u + 13u] = static_cast<unsigned long>(i * 8u + 7u);
		indices[i * 24u + 14u] = static_cast<unsigned long>(i * 8u + 7u);
		indices[i * 24u + 15u] = static_cast<unsigned long>(i * 8u + 4u);
		indices[i * 24u + 16u] = static_cast<unsigned long>(i * 8u);
		indices[i * 24u + 17u] = static_cast<unsigned long>(i * 8u + 4u);
		indices[i * 24u + 18u] = static_cast<unsigned long>(i * 8u + 1u);
		indices[i * 24u + 19u] = static_cast<unsigned long>(i * 8u + 5u);
		indices[i * 24u + 20u] = static_cast<unsigned long>(i * 8u + 2u);
		indices[i * 24u + 21u] = static_cast<unsigned long>(i * 8u + 6u);
		indices[i * 24u + 22u] = static_cast<unsigned long>(i * 8u + 3u);
		indices[i * 24u + 23u] = static_cast<unsigned long>(i * 8u + 7u);
	}
	*/

	m_indexCountkdTree = kdIndices.size();

	D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
	vertexBufferData.pSysMem = vertices.data();
	vertexBufferData.SysMemPitch = 0;
	vertexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(BasicVertex) * vertices.size(), D3D11_BIND_VERTEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&vertexBufferDesc,
			&vertexBufferData,
			&m_kdTreeVertexBuffer
			)
		);

	D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
	indexBufferData.pSysMem = kdIndices.data();
	indexBufferData.SysMemPitch = 0;
	indexBufferData.SysMemSlicePitch = 0;
	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned short) * kdIndices.size(), D3D11_BIND_INDEX_BUFFER);
	DX::ThrowIfFailed(
		m_deviceResources->GetD3DDevice()->CreateBuffer(
			&indexBufferDesc,
			&indexBufferData,
			&m_kdTreeIndexBuffer
			)
		);

	//delete[] indices, vertexArr;
}
// update filter mode
void ShadowSceneRenderer::SetFilterMode(Platform::String^ filtermode)
{
	if		(filtermode == "0")	{ m_filterMode = 0; }
	else if (filtermode == "1") { m_filterMode = 1; }
	else if (filtermode == "2") { m_filterMode = 2; }
	else if (filtermode == "3") { m_filterMode = 3; }
	else if (filtermode == "4") { m_filterMode = 4; }
	else if (filtermode == "5") { m_filterMode = 5; }
	else if (filtermode == "6") { m_filterMode = 6; }
	else if (filtermode == "7") { m_filterMode = 7; }
	else if (filtermode == "8") { m_filterMode = 8; }
}

// update earth normal mapping
void ShadowSceneRenderer::SetEarthNormalMap(bool normal)
{
	m_EarthNormalMap = normal;
}

// update earth normal mapping
void ShadowSceneRenderer::SetMoonNormalMap(bool normal)
{
	m_MoonNormalMap = normal;
}

void ShadowSceneRenderer::SetKDTree(bool draw)
{
	m_drawKDTree = draw;
}

// Updates the Camera's View Matrix, Translation and Rotation
void ShadowSceneRenderer::UpdateCamera()
{
	quaternion = XMQuaternionNormalize(quaternion);
	camRotationMatrix = XMMatrixRotationQuaternion(quaternion);

	at = XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	at = XMVector3Normalize(at);

	camRight = XMVector3TransformCoord(DefaultRight, camRotationMatrix);
	camForward = XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	up = XMVector3Cross(camForward, camRight);

	eye += moveLeftRight*camRight;
	eye += moveBackForward*camForward;

	moveLeftRight = 0.0f;
	moveBackForward = 0.0f;

	at = eye + at;

	g_View = XMMatrixLookAtRH(eye, at, up);
}

// Updates the Camera's View Matrix, Translation and Rotation
void ShadowSceneRenderer::UpdateLight()
{
	quaternionl = XMQuaternionNormalize(quaternionl);
	camRotationMatrix = XMMatrixRotationQuaternion(quaternionl);

	atl = XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	atl = XMVector3Normalize(atl);

	camRight = XMVector3TransformCoord(DefaultRight, camRotationMatrix);
	camForward = XMVector3TransformCoord(DefaultForward, camRotationMatrix);
	upl = XMVector3Cross(camForward, camRight);

	eyel += moveLeftRightl*camRight;
	eyel += moveBackForwardl*camForward;

	moveLeftRightl = 0.0f;
	moveBackForwardl = 0.0f;

	atl = eyel + atl;

	l_View = XMMatrixLookAtRH(eyel, atl, upl);
}

// Checks which Key was pressed and sets specific value
void ShadowSceneRenderer::KeyDownCheck(VirtualKey keystate)
{
	// Figure out the command from the keyboard.
	if (keystate == VirtualKey::W)
		m_forward = true;
	if (keystate == VirtualKey::S)
		m_back = true;
	if (keystate == VirtualKey::A)
		m_left = true;
	if (keystate == VirtualKey::D)
		m_right = true;

	if (keystate == VirtualKey::I)
		m_forwardl = true;
	if (keystate == VirtualKey::K)
		m_backl = true;
	if (keystate == VirtualKey::J)
		m_leftl = true;
	if (keystate == VirtualKey::L)
		m_rightl = true;

	if (keystate == VirtualKey::Down)
		m_pitchp = true;
	if (keystate == VirtualKey::Up)
		m_pitchn = true;
	if (keystate == VirtualKey::Left)
		m_yawp = true;
	if (keystate == VirtualKey::Right)
		m_yawn = true;
	if (keystate == VirtualKey::Q)
		m_rollp = true;
	if (keystate == VirtualKey::E)
		m_rolln = true;

	if (keystate == VirtualKey::H)
		m_pitchpl = true;
	if (keystate == VirtualKey::Z)
		m_pitchnl = true;
	if (keystate == VirtualKey::U)
		m_yawpl = true;
	if (keystate == VirtualKey::O)
		m_yawnl = true;
	if (keystate == VirtualKey::N)
		m_rollpl = true;
	if (keystate == VirtualKey::M)
		m_rollnl = true;

	if (keystate == VirtualKey::Number1)
	{
		newPoint1.Eye = eye;
		newPoint1.camPitch = camPitch;
		newPoint1.camRoll = camRoll;
		newPoint1.camYaw = camYaw;
		newPoint1.quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
		newPoint1.quaternion = XMQuaternionNormalize(newPoint1.quaternion);
	}
	if (keystate == VirtualKey::Number2)
	{
		newPoint2.Eye = eye;
		newPoint2.camPitch = camPitch;
		newPoint2.camRoll = camRoll;
		newPoint2.camYaw = camYaw;
		newPoint2.quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
		newPoint2.quaternion = XMQuaternionNormalize(newPoint2.quaternion);
	}
	if (keystate == VirtualKey::Number3)
	{
		newPoint3.Eye = eye;
		newPoint3.camPitch = camPitch;
		newPoint3.camRoll = camRoll;
		newPoint3.camYaw = camYaw;
		newPoint3.quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
		newPoint3.quaternion = XMQuaternionNormalize(newPoint3.quaternion);
	}
	if (keystate == VirtualKey::Number4)
	{
		newPoint4.Eye = eye;
		newPoint4.camPitch = camPitch;
		newPoint4.camRoll = camRoll;
		newPoint4.camYaw = camYaw;
		newPoint4.quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
		newPoint4.quaternion = XMQuaternionNormalize(newPoint4.quaternion);
	}
	if (keystate == VirtualKey::Number5)
	{
		newPoint5.Eye = eye;
		newPoint5.camPitch = camPitch;
		newPoint5.camRoll = camRoll;
		newPoint5.camYaw = camYaw;
		newPoint5.quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
		newPoint5.quaternion = XMQuaternionNormalize(newPoint5.quaternion);
	}

	if (keystate == VirtualKey::F1)
	{
		counter = 0;
		driveActivated = true;
	}

	if (keystate == VirtualKey::Enter)
	{
		//setpoint back
		eye = newPoint1.Eye;
		camPitch = newPoint1.camPitch;
		camYaw = newPoint1.camYaw;
		camRoll = newPoint1.camRoll;
		quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);

		counter = 0;
		driveActivated = false;
	}

	// camera ray to point distance or // point to point distance
	if (keystate == VirtualKey::Space)
	{
		pointToCamera = pointToCamera ? false : true;
	}
	
}

// Checks which Key was released and resets specific value
void ShadowSceneRenderer::KeyUpCheck(VirtualKey keystate)
{
	// Figure out the command from the keyboard.
	if (keystate == VirtualKey::W)
		m_forward = false;
	if (keystate == VirtualKey::S)
		m_back = false;
	if (keystate == VirtualKey::A)
		m_left = false;
	if (keystate == VirtualKey::D)
		m_right = false;

	if (keystate == VirtualKey::Down)
		m_pitchp = false;
	if (keystate == VirtualKey::Up)
		m_pitchn = false;
	if (keystate == VirtualKey::Left)
		m_yawp = false;
	if (keystate == VirtualKey::Right)
		m_yawn = false;
	if (keystate == VirtualKey::Q)
		m_rollp = false;
	if (keystate == VirtualKey::E)
		m_rolln = false;

	if (keystate == VirtualKey::I)
		m_forwardl = false;
	if (keystate == VirtualKey::K)
		m_backl = false;
	if (keystate == VirtualKey::J)
		m_leftl = false;
	if (keystate == VirtualKey::L)
		m_rightl = false;

	if (keystate == VirtualKey::H)
		m_pitchpl = false;
	if (keystate == VirtualKey::Z)
		m_pitchnl = false;
	if (keystate == VirtualKey::U)
		m_yawpl = false;
	if (keystate == VirtualKey::O)
		m_yawnl = false;
	if (keystate == VirtualKey::N)
		m_rollpl = false;
	if (keystate == VirtualKey::M)
		m_rollnl = false;
}


// update the Movevariables (Translation and Rotation) depending on timer and the keyStatus -> CameraView
void ShadowSceneRenderer::DirectInput(DX::StepTimer const& timer)
{
	float fps = timer.GetFramesPerSecond();
	if (fps < 1)
		fps = 60;
	// set movement speed dependent on fps
	float speed = 30.0f * (1 / fps);
	float frameTime = (1 / fps);

	// move Camera to desired position from KeyEvent
	if (m_pitchn == true)
	{
		camPitch -= frameTime;
		quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
	}
	if (m_pitchp == true)
	{
		camPitch += frameTime;
		quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
	}
	if (m_yawp == true)
	{
		camYaw += frameTime;
		quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
	}
	if (m_yawn == true)
	{
		camYaw -= frameTime;
		quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
	}
	if (m_rolln == true)
	{
		camRoll -= frameTime;
		quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
	}
	if (m_rollp == true)
	{
		camRoll += frameTime;
		quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
	}
	if (m_left == true)
	{
		moveLeftRight += speed;
	}
	if (m_right == true)
	{
		moveLeftRight -= speed;
	}
	if (m_back == true)
	{
		moveBackForward -= speed;
	}
	if (m_forward == true)
	{
		moveBackForward += speed;
	}

	if (m_leftl == true)
	{
		moveLeftRightl += speed;
	}
	if (m_rightl == true)
	{
		moveLeftRightl -= speed;
	}
	if (m_backl == true)
	{
		moveBackForwardl -= speed;
	}
	if (m_forwardl == true)
	{
		moveBackForwardl += speed;
	}

	if (m_pitchnl == true)
	{
		camPitchl -= frameTime;
		quaternionl = XMQuaternionRotationRollPitchYaw(camPitchl, camYawl, camRolll);
	}
	if (m_pitchpl == true)
	{
		camPitchl += frameTime;
		quaternionl = XMQuaternionRotationRollPitchYaw(camPitchl, camYawl, camRolll);
	}
	if (m_yawpl == true)
	{
		camYawl += frameTime;
		quaternionl = XMQuaternionRotationRollPitchYaw(camPitchl, camYawl, camRolll);
	}
	if (m_yawnl == true)
	{
		camYawl -= frameTime;
		quaternionl = XMQuaternionRotationRollPitchYaw(camPitchl, camYawl, camRolll);
	}
	if (m_rollnl == true)
	{
		camRolll -= frameTime;
		quaternionl = XMQuaternionRotationRollPitchYaw(camPitchl, camYawl, camRolll);
	}
	if (m_rollpl == true)
	{
		camRolll += frameTime;
		quaternionl = XMQuaternionRotationRollPitchYaw(camPitchl, camYawl, camRolll);
	}
}

// automated Camera drive to 5 positions (chosen by user or default values) with Spline Interpolation (100 values between points)
void ShadowSceneRenderer::CameraDrive()
{
	counter += 1;

	//set 1st Point to global variables (update camera...)
	if (counter == 1)
	{
		eye = newPoint1.Eye;
		camPitch = newPoint1.camPitch;
		camYaw = newPoint1.camYaw;
		camRoll = newPoint1.camRoll;
		quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
		quaternion = XMQuaternionNormalize(quaternion);
	}
	else if (counter >= 2 && counter <= 100)
	{
		camPitch = newPoint2.camPitch;
		camYaw = newPoint2.camYaw;
		camRoll = newPoint2.camRoll;

		// Squad Quaternion Interpolation for Rotation and Kochanek-Bartel Spline Interpolation for Translation
		kochanekSplineandSquadInterpolation(newPoint1, newPoint1, newPoint2, newPoint3, (float)counter / 100);
	}
	else if (counter > 100 && counter <= 200)
	{
		camPitch = newPoint3.camPitch;
		camYaw = newPoint3.camYaw;
		camRoll = newPoint3.camRoll;

		// Squad Quaternion Interpolation for Rotation and Kochanek-Bartel Spline Interpolation for Translation
		kochanekSplineandSquadInterpolation(newPoint1, newPoint2, newPoint3, newPoint4, ((float)counter - 100) / 100);
	}
	else if (counter > 200 && counter <= 300)
	{
		camPitch = newPoint4.camPitch;
		camYaw = newPoint4.camYaw;
		camRoll = newPoint4.camRoll;

		// Squad Quaternion Interpolation for Rotation and Kochanek-Bartel Spline Interpolation for Translation
		kochanekSplineandSquadInterpolation(newPoint2, newPoint3, newPoint4, newPoint5, ((float)counter - 200) / 100);
	}
	else if (counter > 300 && counter <= 400)
	{
		camPitch = newPoint5.camPitch;
		camYaw = newPoint5.camYaw;
		camRoll = newPoint5.camRoll;

		// Squad Quaternion Interpolation for Rotation and Kochanek-Bartel Spline Interpolation for Translation
		kochanekSplineandSquadInterpolation(newPoint3, newPoint4, newPoint5, newPoint5, ((float)counter - 300) / 100);
	}
	else
	{
		driveActivated = false;
	}
}


// Squad Quaternion Interpolation for Rotation and Kochanek-Bartel Spline Interpolation for Translation
void kochanekSplineandSquadInterpolation(const CameraViewPoint& point0, const CameraViewPoint& point1, const CameraViewPoint& point2, const CameraViewPoint& point3, float s)
{
	float tension = 0.0f;
	float bias = 0.0f;
	float cont = 0.0f;

	XMVECTOR starttang, endtang;

	XMVECTOR qRot1, qRot2, qRot3;

	// test to adapt speed
	XMVECTOR veclength = XMVector4Length((point2.Eye - point1.Eye));

	// Kochanek-Bartels spline tangents
	starttang = (((1 - tension)*(1 + bias)*(1 - cont)) / 2) * (point1.Eye - point0.Eye) + (((1 - tension)*(1 - bias)*(1 + cont)) / 2) * (point2.Eye - point1.Eye);
	endtang = (((1 - tension)*(1 + bias)*(1 + cont)) / 2) * (point2.Eye - point1.Eye) + (((1 - tension)*(1 - bias)*(1 - cont)) / 2) * (point3.Eye - point2.Eye);

	//eye = XMVectorHermite(point1.Eye, starttang, point2.Eye, endtang, s); //Spline Interpolation
	// Kochanek-Bartel Spline for Position interpolation
	eye = (2 * pow(s, 3) - 3 * pow(s, 2) + 1) * point1.Eye + (pow(s, 3) - 2 * pow(s, 2) + s) * starttang + (-2 * pow(s, 3) + 3 * pow(s, 2)) * point2.Eye + (pow(s, 3) - pow(s, 2)) * endtang;
	//Quaternion Squad Interpolation for Quaternion rotation to desired positions
	XMQuaternionSquadSetup(&qRot1, &qRot2, &qRot3, point0.quaternion, point1.quaternion, point2.quaternion, point3.quaternion);
	quaternion = XMQuaternionSquad(point1.quaternion, qRot1, qRot2, qRot3, s);
}

// Sets Standard Points for the camera drive operation
void ShadowSceneRenderer::InitStdPoints()
{
	camPitch = -0.066667f;
	camRoll = 0.000000f;
	camYaw = -0.295082f;
	quaternion = XMQuaternionRotationRollPitchYaw(camPitch, camYaw, camRoll);
	// init standard points
	newPoint1.Eye = XMVectorSet(30.53129f, 14.08332f, -84.70342f, -8.78142f);
	newPoint1.camPitch = -0.066667f;
	newPoint1.camRoll = 0.000000f;
	newPoint1.camYaw = -0.295082f;
	newPoint1.quaternion = XMQuaternionRotationRollPitchYaw(newPoint1.camPitch, newPoint1.camYaw, newPoint1.camRoll);
	newPoint1.quaternion = XMQuaternionNormalize(newPoint1.quaternion);

	newPoint2.Eye = XMVectorSet(58.18593f, 10.97303f, -30.58045f, -86.11476f);
	newPoint2.camPitch = -0.066667f;
	newPoint2.camRoll = 0.000000f;
	newPoint2.camYaw = -0.895082f;
	newPoint2.quaternion = XMQuaternionRotationRollPitchYaw(newPoint2.camPitch, newPoint2.camYaw, newPoint2.camRoll);
	newPoint2.quaternion = XMQuaternionNormalize(newPoint2.quaternion);

	newPoint3.Eye = XMVectorSet(12.13479f, 8.08554f, 30.66161f, -206.11477f);
	newPoint3.camPitch = -0.066667f;
	newPoint3.camRoll = 0.000000f;
	newPoint3.camYaw = -2.411747f;
	newPoint3.quaternion = XMQuaternionRotationRollPitchYaw(newPoint3.camPitch, newPoint3.camYaw, newPoint3.camRoll);
	newPoint3.quaternion = XMQuaternionNormalize(newPoint3.quaternion);

	newPoint4.Eye = XMVectorSet(-56.47461f, 8.75172f, 64.38279f, -258.78148f);
	newPoint4.camPitch = -0.066667f;
	newPoint4.camRoll = 0.000000f;
	newPoint4.camYaw = -3.561746f;
	newPoint4.quaternion = XMQuaternionRotationRollPitchYaw(newPoint4.camPitch, newPoint4.camYaw, newPoint4.camRoll);
	newPoint4.quaternion = XMQuaternionNormalize(newPoint4.quaternion);

	newPoint5.Eye = XMVectorSet(-56.69905f, 6.08628f, -72.73223f, -436.67962f);
	newPoint5.camPitch = 0.316667f;
	newPoint5.camRoll = 0.000000f;
	newPoint5.camYaw = -5.390569f;
	newPoint5.quaternion = XMQuaternionRotationRollPitchYaw(newPoint5.camPitch, newPoint5.camYaw, newPoint5.camRoll);
	newPoint5.quaternion = XMQuaternionNormalize(newPoint5.quaternion);

	camPitchl = -0.066667f;
	camRolll = 0.000000f;
	camYawl = -0.295082f;
	quaternionl = XMQuaternionRotationRollPitchYaw(camPitchl, camYawl, camRolll);
}

/*
// Ray Triangle Intersection
_Use_decl_annotations_
bool XM_CALLCONV ShadowSceneRenderer::Intersects(FXMVECTOR Origin, FXMVECTOR Direction, FXMVECTOR V0, GXMVECTOR V1, HXMVECTOR V2, float& Dist)
{
	//assert(DirectX::Internal::XMVector3IsUnit(Direction));

	XMVECTOR Zero = XMVectorZero();

	XMVECTOR e1 = V1 - V0;
	XMVECTOR e2 = V2 - V0;

	// p = Direction ^ e2;
	XMVECTOR p = XMVector3Cross(Direction, e2);

	// det = e1 * p;
	XMVECTOR det = XMVector3Dot(e1, p);

	XMVECTOR u, v, t;

	if (XMVector3GreaterOrEqual(det, g_RayEpsilon))
	{
		// Determinate is positive (front side of the triangle).
		XMVECTOR s = Origin - V0;

		// u = s * p;
		u = XMVector3Dot(s, p);

		XMVECTOR NoIntersection = XMVectorLess(u, Zero);
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(u, det));

		// q = s ^ e1;
		XMVECTOR q = XMVector3Cross(s, e1);

		// v = Direction * q;
		v = XMVector3Dot(Direction, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(v, Zero));
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(u + v, det));

		// t = e2 * q;
		t = XMVector3Dot(e2, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(t, Zero));

		if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
		{
			Dist = 0.f;
			return false;
		}
	}
	else if (XMVector3LessOrEqual(det, g_RayNegEpsilon))
	{
		// Determinate is negative (back side of the triangle).
		XMVECTOR s = Origin - V0;

		// u = s * p;
		u = XMVector3Dot(s, p);

		XMVECTOR NoIntersection = XMVectorGreater(u, Zero);
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(u, det));

		// q = s ^ e1;
		XMVECTOR q = XMVector3Cross(s, e1);

		// v = Direction * q;
		v = XMVector3Dot(Direction, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(v, Zero));
		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorLess(u + v, det));

		// t = e2 * q;
		t = XMVector3Dot(e2, q);

		NoIntersection = XMVectorOrInt(NoIntersection, XMVectorGreater(t, Zero));

		if (XMVector4EqualInt(NoIntersection, XMVectorTrueInt()))
		{
			Dist = 0.f;
			return false;
		}
	}
	else
	{
		// Parallel ray.
		Dist = 0.f;
		return false;
	}

	t = XMVectorDivide(t, det);

	// (u / det) and (v / dev) are the barycentric cooridinates of the intersection.

	// Store the x-component to *pDist
	XMStoreFloat(&Dist, t);

	return true;
}*/