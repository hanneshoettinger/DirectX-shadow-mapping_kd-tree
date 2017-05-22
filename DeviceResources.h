//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#define MAX_SAMPLES_CHECK 128

#define DXGI_FORMAT_MAX 116


namespace DX
{
    // Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
    interface IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;
    };
	// This helper class stores multisampling capability info. It's a ref 
	// class so that we can use it to communicate with the XAML page, which 
	// displays some of this info.
	// This helper class stores multisampling capability info. It's a ref 
	// class so that we can use it to communicate with the XAML page, which 
	// displays some of this info.
	private ref class MultisamplingSupportInfo sealed
	{
	internal:
		MultisamplingSupportInfo()
		{
			ZeroMemory(&sampleSizeData, sizeof(sampleSizeData));
			ZeroMemory(&qualityFlagData, sizeof(qualityFlagData));
		};

	public:
		unsigned int GetSampleSize(int i, int j) { return sampleSizeData[i][j]; };
		void         SetSampleSize(int i, int j, unsigned int value) { sampleSizeData[i][j] = value; };

		unsigned int GetQualityFlagsAt(int i, int j) { return qualityFlagData[i][j]; };
		void         SetQualityFlagsAt(int i, int j, unsigned int value) { qualityFlagData[i][j] = value; };

		bool         GetFormatSupport(int format) { return formatSupport[format]; };
		void         SetFormatSupport(int format, bool value) { formatSupport[format] = value; };

		unsigned int GetFormat() { return m_format; };
		void         SetFormat(unsigned int i) { m_format = (DXGI_FORMAT)i; };

		unsigned int GetLargestSampleSize() { return m_largestSampleSize; };
		unsigned int GetSmallestSampleSize() { return m_smallestSampleSize; };

		void SetLargestSampleSize(unsigned int value) { m_largestSampleSize = value; };
		void SetSmallestSampleSize(unsigned int value) { m_smallestSampleSize = value; };

	private:
		unsigned int sampleSizeData[DXGI_FORMAT_MAX][MAX_SAMPLES_CHECK];
		unsigned int qualityFlagData[DXGI_FORMAT_MAX][MAX_SAMPLES_CHECK];

		bool formatSupport[DXGI_FORMAT_MAX];

		unsigned int m_largestSampleSize;
		unsigned int m_smallestSampleSize;

		DXGI_FORMAT m_format;
	};
    // Controls all the DirectX device resources.
    class DeviceResources
    {
    public:
        DeviceResources();
        void SetWindow(Windows::UI::Core::CoreWindow^ window);
        void SetWindow(Windows::UI::Core::CoreWindow^ window, Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
        void SetLogicalSize(Windows::Foundation::Size logicalSize);
        void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
        void SetDpi(float dpi);
        void ValidateDevice();
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify);
        void Trim();
        void Present();

		void CreateWindowSizeDependentResources();
		Windows::Foundation::Point TransformToOrientation(Windows::Foundation::Point point, bool dipsToPixels);
		void UpdateForWindowSizeChange();

		void SetMultisampling(unsigned int sampleSize, unsigned int flags);

		// Multisampling info accessors.
		MultisamplingSupportInfo^ GetMultisamplingSupportInfo()		{ return m_supportInfo; }
		unsigned int            GetSampleSize()						{ return m_sampleSize; }
		unsigned int            GetQualityLevel()					{ return m_qualityFlags; }
		float                   GetScalingFactor()					{ return m_scalingFactor; }
		IDXGISwapChain1*		GetOverlaySwapChain() const			{ return m_overlaySwapChain.Get(); }
		ID3D11RenderTargetView*	GetOverlayRenderTargetView() const	{ return m_overlayRenderTargetView.Get(); }

		// Device Accessors.
		Windows::Foundation::Size GetOutputBounds() const { return m_outputSize; }
        // Device Accessors.
        Windows::Foundation::Size GetOutputSize() const                 { return m_outputSize; }
        Windows::Foundation::Size GetLogicalSize() const                { return m_logicalSize; }
        Windows::UI::Core::CoreWindow^ GetWindow() const                { return m_window.Get(); }

        // D3D Accessors.
        ID3D11Device2*          GetD3DDevice() const                    { return m_d3dDevice.Get(); }
        ID3D11DeviceContext2*   GetD3DDeviceContext() const             { return m_d3dContext.Get(); }
        IDXGISwapChain1*        GetSwapChain() const                    { return m_swapChain.Get(); }
        D3D_FEATURE_LEVEL       GetDeviceFeatureLevel() const           { return m_d3dFeatureLevel; }
        ID3D11RenderTargetView* GetBackBufferRenderTargetView() const   { return m_d3dRenderTargetView.Get(); }
        ID3D11DepthStencilView* GetDepthStencilView() const             { return m_d3dDepthStencilView.Get(); }
        D3D11_VIEWPORT          GetScreenViewport() const               { return m_screenViewport; }
        DirectX::XMFLOAT4X4     GetOrientationTransform3D() const       { return m_orientationTransform3D; }

        // D2D Accessors.
        ID2D1Factory2*          GetD2DFactory() const                   { return m_d2dFactory.Get(); }
        ID2D1Device1*           GetD2DDevice() const                    { return m_d2dDevice.Get(); }
        ID2D1DeviceContext1*    GetD2DDeviceContext() const             { return m_d2dContext.Get(); }
        ID2D1Bitmap1*           GetD2DTargetBitmap() const              { return m_d2dTargetBitmap.Get(); }
        IDWriteFactory2*        GetDWriteFactory() const                { return m_dwriteFactory.Get(); }
        IWICImagingFactory2*    GetWicImagingFactory() const            { return m_wicFactory.Get(); }
        D2D1::Matrix3x2F        GetOrientationTransform2D() const       { return m_orientationTransform2D; }

    private:
        void CreateDeviceIndependentResources();
        void CreateDeviceResources();

		void CreateMultisampledRenderTarget();
		void CheckMultisamplingCapabilities();

        DXGI_MODE_ROTATION ComputeDisplayRotation();
		
		void CreateDirectRenderTarget();
		void CreateD2DRenderTarget();

		// Objects added for multisampling.
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_backBuffer;
		Microsoft::WRL::ComPtr<ID3D11Texture2D> m_offScreenSurface;

		MultisamplingSupportInfo^ m_supportInfo;

		float        m_scalingFactor;
		unsigned int m_sampleSize;
		unsigned int m_qualityFlags;

		Microsoft::WRL::ComPtr<IDXGISwapChain2>         m_overlaySwapChain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_overlayRenderTargetView;

        // Direct3D objects.
        Microsoft::WRL::ComPtr<ID3D11Device2>           m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D11DeviceContext2>    m_d3dContext;
        Microsoft::WRL::ComPtr<IDXGISwapChain2>         m_swapChain;

        // Direct3D rendering objects. Required for 3D.
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_d3dRenderTargetView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_d3dDepthStencilView;
        D3D11_VIEWPORT                                  m_screenViewport;

        // Direct2D drawing components.
        Microsoft::WRL::ComPtr<ID2D1Factory2>       m_d2dFactory;
        Microsoft::WRL::ComPtr<ID2D1Device1>        m_d2dDevice;
        Microsoft::WRL::ComPtr<ID2D1DeviceContext1> m_d2dContext;
        Microsoft::WRL::ComPtr<ID2D1Bitmap1>        m_d2dTargetBitmap;

        // DirectWrite drawing components.
        Microsoft::WRL::ComPtr<IDWriteFactory2>     m_dwriteFactory;
        Microsoft::WRL::ComPtr<IWICImagingFactory2> m_wicFactory;

        // Cached reference to the Window.
        Platform::Agile<Windows::UI::Core::CoreWindow>  m_window;

        // Cached reference to the XAML panel (optional).
        Windows::UI::Xaml::Controls::SwapChainPanel^ m_panel;

        // Cached device properties.
        D3D_FEATURE_LEVEL                               m_d3dFeatureLevel;
        Windows::Foundation::Size                       m_d3dRenderTargetSize;
        Windows::Foundation::Size                       m_outputSize;
        Windows::Foundation::Size                       m_logicalSize;
        Windows::Graphics::Display::DisplayOrientations m_nativeOrientation;
        Windows::Graphics::Display::DisplayOrientations m_currentOrientation;
        float                                           m_dpi;

        // Transforms used for display orientation.
        D2D1::Matrix3x2F    m_orientationTransform2D;
        DirectX::XMFLOAT4X4 m_orientationTransform3D;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify* m_deviceNotify;
    };
}
