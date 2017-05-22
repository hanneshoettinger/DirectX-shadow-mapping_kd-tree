//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

namespace ShadowMapping
{
    // Constant buffer used to send view and projection matrices and camera (eye)
    // position to the vertex shader.
    struct ViewProjectionConstantBuffer
    {
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
        DirectX::XMFLOAT4   pos;
    };

    // Constant buffer used to send model matrix to the vertex shader.
    struct ModelConstantBuffer
    {
        DirectX::XMFLOAT4X4 model;
		BOOL				hasNormalMap = false;
		DirectX::XMFLOAT3	padding;
    };

    // Used to send per-vertex data to the vertex shader.
    struct VertexPositionTexNormColor
    {
        DirectX::XMFLOAT3 pos;
        DirectX::XMFLOAT2 tex;
        DirectX::XMFLOAT3 norm;
        DirectX::XMFLOAT3 color;
		DirectX::XMFLOAT3 uTan; // texture coordinate u-tangent vector
		DirectX::XMFLOAT3 vTan; // texture coordinate v-tangent vector
    };
}