﻿//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

#include "DirectXPage.g.h"

#include "DeviceResources.h"
#include "ShadowMappingMain.h"

namespace ShadowMapping
{
    /// <summary>
    /// A page that hosts a DirectX SwapChainBackgroundPanel.
    /// This page must be the root of the Window content (it cannot be hosted on a Frame).
    /// </summary>
    public ref class DirectXPage sealed
    {
    public:
        DirectXPage();

        //void SaveInternalState(Windows::Foundation::Collections::IPropertySet^ state);
        //void LoadInternalState(Windows::Foundation::Collections::IPropertySet^ state);

    private:

		// For multisampling selector
		void SetupSampleSizeSlider();
		unsigned int m_sampleLookupArray[16];

        // XAML low-level rendering event handler.
        void OnRendering(Platform::Object^ sender, Platform::Object^ args);

		// Key Event Handler
		void KeyDown(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ e);
		void KeyUp(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::KeyEventArgs^ e);

		// Pointer Pressed Event
		void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args);

		// Other event handlers. -> MSAA
		void OnCompositionScaleChanged(Windows::UI::Xaml::Controls::SwapChainPanel^ sender, Object^ args);
		void OnSwapChainPanelSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e);

        // Window event handlers.
        void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
        void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);

        // DisplayInformation event handlers.
        void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
        void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
        void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

        // Other event handlers.
        //void OnEnableLinearFiltering(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);
        //void OnDisableLinearFiltering(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ args);

        // Resource used to keep track of the rendering event registration.
        Windows::Foundation::EventRegistrationToken m_eventToken;

        // Resources used to render the DirectX content in the XAML page background.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;
        std::unique_ptr<ShadowMapSampleMain> m_main;
        bool m_windowVisible;

		std::unique_ptr<ShadowSceneRenderer> m_3Drenderer;

		void SampleCountSlider_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);

        // Resources for changing the shadow buffer size.
        //void OnShadowBufferSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e);

        // Convenience method to synchronize the state of the UI controls with the state of the app.
        //void UpdateUI();

        // Convenience method for updating UI text.
        //void SetShadowSizeText(float val);
		void comboBox_SelectionChanged_1(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
		void Earth_Normal_Checkbox_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Earth_Normal_Checkbox_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Moon_Normal_Checkbox_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void Moon_Normal_Checkbox_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void KDtree_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void KDtree_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
	};
}
