//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "DirectXPage.xaml.h"
#include "BasicMath.h"
#include <string>     // std::string, std::to_string

using namespace ShadowMapping;
using namespace DX;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Graphics::Display;
using namespace Windows::System::Threading;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

DirectXPage::DirectXPage():
    m_windowVisible(true)
{
    InitializeComponent();

    // Register event handlers for page lifecycle.
    CoreWindow^ window = Window::Current->CoreWindow;

	// KeyDown and KeyUp Event Handling for User Input
	window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &DirectXPage::KeyDown);

	window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &DirectXPage::KeyUp);

	// opt in to recieve touch/mouse events
	window->PointerPressed +=
		ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &DirectXPage::OnPointerPressed);

    window->SizeChanged +=
        ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &DirectXPage::OnWindowSizeChanged);

    window->VisibilityChanged +=
        ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &DirectXPage::OnVisibilityChanged);

    DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

    currentDisplayInformation->DpiChanged +=
        ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDpiChanged);

    currentDisplayInformation->OrientationChanged +=
        ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnOrientationChanged);

    DisplayInformation::DisplayContentsInvalidated +=
        ref new TypedEventHandler<DisplayInformation^, Object^>(this, &DirectXPage::OnDisplayContentsInvalidated);

	swapChainPanel->CompositionScaleChanged +=
		ref new TypedEventHandler<SwapChainPanel^, Object^>(this, &DirectXPage::OnCompositionScaleChanged);

	swapChainPanel->SizeChanged +=
		ref new SizeChangedEventHandler(this, &DirectXPage::OnSwapChainPanelSizeChanged);

    // Register the rendering event, called every time XAML renders the screen.
    m_eventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &DirectXPage::OnRendering));

    // Whenever this screen is not being used anymore, you can unregister this event with the following line:
    // CompositionTarget::Rendering::remove(m_eventToken);

    // Disable all pointer visual feedback for better performance when touching.
    auto pointerVisualizationSettings = PointerVisualizationSettings::GetForCurrentView();
    pointerVisualizationSettings->IsContactFeedbackEnabled = false;
    pointerVisualizationSettings->IsBarrelButtonFeedbackEnabled = false;

    // At this point we have access to the device.
    // We can create the device-dependent resources.
    m_deviceResources = std::make_shared<DX::DeviceResources>();
    m_deviceResources->SetWindow(Window::Current->CoreWindow, swapChainPanel);

	// Adjust slider to reflect multisampling support of the device
	SetupSampleSizeSlider();

	m_main = std::unique_ptr<ShadowMapSampleMain>(new ShadowMapSampleMain(m_deviceResources));
}

// Sets up an array to translate linear XAML slider values into a non-linear 
// list of sample sizes.
void DirectXPage::SetupSampleSizeSlider()
{
	MultisamplingSupportInfo^ samplingInfo = m_deviceResources->GetMultisamplingSupportInfo();
	unsigned int maxArraySize = samplingInfo->GetLargestSampleSize();
	unsigned int currentSampleSize = m_deviceResources->GetSampleSize();

	ZeroMemory(&m_sampleLookupArray, sizeof(m_sampleLookupArray));
	unsigned int maxSampleSize = 0;
	unsigned int numberOfSampleSizesSupported = 0;

	unsigned int format = samplingInfo->GetFormat();

	for (int i = 0; i < 129; i++)
	{
		if (samplingInfo->GetSampleSize(format, i) == 1)
		{
			maxSampleSize = i;
			m_sampleLookupArray[numberOfSampleSizesSupported] = i;

			if (i == (currentSampleSize))
			{
				SampleCountSlider->Value = numberOfSampleSizesSupported;
				CurrentSampleCount->Text = i.ToString();
			}

			numberOfSampleSizesSupported++;
		}
	}

	SampleCountSlider->Minimum = 0;
	SampleCountSlider->Maximum = numberOfSampleSizesSupported - 1;
}
/*
// Saves the current state of the app for suspend and terminate events.
void DirectXPage::SaveInternalState(IPropertySet^ state)
{
    // DirectX apps call Trim() to reduce memory footprint before suspend.
    m_deviceResources->Trim();

    // Delegate save operations to the main class.
    m_main->SaveInternalState(state);
}*/

// Called every time XAML decides to render a frame.
void DirectXPage::OnRendering(Object^ sender, Object^ args)
{
    if (m_windowVisible)
    {
        m_main->Update();

        if (m_main->Render())
        {
            m_deviceResources->Present();
        }
    }
}


// Window event handlers.
void DirectXPage::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
    m_deviceResources->SetLogicalSize(Size(sender->Bounds.Width, sender->Bounds.Height));
    m_main->UpdateForWindowSizeChange();
}

void DirectXPage::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
    m_windowVisible = args->Visible;
}

// DisplayInformation event handlers.

void DirectXPage::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
    m_deviceResources->SetDpi(sender->LogicalDpi);
    //m_main->UpdateForWindowSizeChange();
}

void DirectXPage::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
	m_deviceResources->UpdateForWindowSizeChange();
    //m_deviceResources->SetCurrentOrientation(sender->CurrentOrientation);
    m_main->UpdateForWindowSizeChange();
}

void DirectXPage::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
    m_deviceResources->ValidateDevice();
}

/*
void DirectXPage::OnEnableLinearFiltering(Object^ sender, RoutedEventArgs^ args)
{
    if (m_main != nullptr)
    {
        m_main->SetFiltering(true);
    }
}

void DirectXPage::OnDisableLinearFiltering(Object^ sender, RoutedEventArgs^ args)
{
    if (m_main != nullptr)
    {
        m_main->SetFiltering(false);
    }
}

void DirectXPage::OnShadowBufferSizeChanged(Object^ sender, RangeBaseValueChangedEventArgs^ e)
{
    if (m_main != nullptr)
    {
        float val = static_cast<float>(e->NewValue);
        m_main->SetShadowSize(val);   
        SetShadowSizeText(val);
    }
}*/

// Called when user presses key down
void DirectXPage::KeyDown(CoreWindow^ sender, KeyEventArgs^ e)
{
	m_3Drenderer->KeyDownCheck(e->VirtualKey);
}

// Called when user releases key
void DirectXPage::KeyUp(CoreWindow^ sender, KeyEventArgs^ e)
{
	m_3Drenderer->KeyUpCheck(e->VirtualKey);
}

void ShadowMapping::DirectXPage::OnPointerPressed(Windows::UI::Core::CoreWindow ^ sender, Windows::UI::Core::PointerEventArgs ^ args)
{
	if (args->CurrentPoint->Properties->IsRightButtonPressed)
	{
		Point m_currentPoint = args->CurrentPoint->Position;
		Point pt = m_currentPoint; //m_deviceResources->TransformToOrientation(m_currentPoint, true);  // something wrong here *****
		
		// get the current pointer position
		//uint32 pointerID = args->CurrentPoint->PointerId;
		//float2 position = float2(args->CurrentPoint->Position.X, args->CurrentPoint->Position.Y);
		
		float HEIGHT = sender->Bounds.Height;
		float WIDTH = sender->Bounds.Width;

		// Move the mouse cursor coordinates into the -1 to +1 range.
		float pointX = pt.X; float pointY = pt.Y;
		OutputDebugStringA("Mouse Coordinates: ");
		std::string stroutput = "x-pos = " + std::to_string(pointX) + "; y-pos = " + std::to_string(pointY) + "\n";
		OutputDebugStringA(stroutput.c_str());
		if (m_main != nullptr)
		{
			m_3Drenderer->TestIntersection(pointX,pointY,HEIGHT,WIDTH);
			float dist = m_3Drenderer->GetDistance();
			if (dist != FLT_MAX)
				Distance->Text = dist.ToString();
			else
				Distance->Text = "Distance";
		}
	}
}

void DirectXPage::comboBox_SelectionChanged_1(Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	if (m_main != nullptr)
	{
		m_3Drenderer->SetFilterMode(comboBox->SelectedIndex.ToString());
	}
}

void ShadowMapping::DirectXPage::Earth_Normal_Checkbox_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (m_main != nullptr)
	{
		m_3Drenderer->SetEarthNormalMap(true);
	}
}


void ShadowMapping::DirectXPage::Earth_Normal_Checkbox_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (m_main != nullptr)
	{
		m_3Drenderer->SetEarthNormalMap(false);
	}
}


void ShadowMapping::DirectXPage::Moon_Normal_Checkbox_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (m_main != nullptr)
	{
		m_3Drenderer->SetMoonNormalMap(true);
	}
}


void ShadowMapping::DirectXPage::Moon_Normal_Checkbox_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (m_main != nullptr)
	{
		m_3Drenderer->SetMoonNormalMap(false);
	}
}

void DirectXPage::OnCompositionScaleChanged(SwapChainPanel^ sender, Object^ args)
{
	m_deviceResources->CreateWindowSizeDependentResources();
	m_main->UpdateForWindowSizeChange();
}

void DirectXPage::OnSwapChainPanelSizeChanged(Object^ sender, SizeChangedEventArgs^ e)
{
	m_deviceResources->UpdateForWindowSizeChange();
	m_main->UpdateForWindowSizeChange();
}

void ShadowMapping::DirectXPage::SampleCountSlider_ValueChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^ e)
{
	if (m_deviceResources == nullptr) return;

	unsigned int newValue = static_cast<unsigned int>(e->NewValue);

	unsigned int newSampleSize = m_sampleLookupArray[newValue];

	// set the Multisampling Level and create correct pipeline
	m_deviceResources->SetMultisampling(newSampleSize, D3D11_STANDARD_MULTISAMPLE_PATTERN);
	
	CurrentSampleCount->Text = newSampleSize.ToString();

}



void ShadowMapping::DirectXPage::KDtree_Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (m_main != nullptr)
	{
		m_3Drenderer->SetKDTree(true);
	}
}


void ShadowMapping::DirectXPage::KDtree_Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (m_main != nullptr)
	{
		m_3Drenderer->SetKDTree(false);
	}
}
