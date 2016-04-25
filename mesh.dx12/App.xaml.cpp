//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "mesh.h"

using namespace mesh_dx12;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Core;
using namespace Windows::System::Threading;
using namespace Microsoft::WRL;

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
    InitializeComponent();
    Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used such as when the application is launched to open a specific file.
/// </summary>
/// <param name="e">Details about the launch request and process.</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e)
{
#if _DEBUG
    // Show graphics profiling information while debugging.
    if (IsDebuggerPresent())
    {
        // Display the current frame rate counters
         DebugSettings->EnableFrameRateCounter = true;
    }
#endif
    auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

    // Do not repeat app initialization when the Window already has content,
    // just ensure that the window is active
    if (rootFrame == nullptr)
    {
        // Create a Frame to act as the navigation context and associate it with
        // a SuspensionManager key
        rootFrame = ref new Frame();

        rootFrame->NavigationFailed += ref new Windows::UI::Xaml::Navigation::NavigationFailedEventHandler(this, &App::OnNavigationFailed);

        if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
        {
            // TODO: Restore the saved session state only when appropriate, scheduling the
            // final launch steps after the restore is complete

        }

        if (e->PrelaunchActivated == false)
        {
            if (rootFrame->Content == nullptr)
            {
                // When the navigation stack isn't restored navigate to the first page,
                // configuring the new page by passing required information as a navigation
                // parameter
                rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);
            }
            // Place the frame in the current Window
            Window::Current->Content = rootFrame;
            // Ensure the current window is active
            Window::Current->Activate();
        }
    }
    else
    {
        if (e->PrelaunchActivated == false)
        {
            if (rootFrame->Content == nullptr)
            {
                // When the navigation stack isn't restored navigate to the first page,
                // configuring the new page by passing required information as a navigation
                // parameter
                rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);
            }
            // Ensure the current window is active
            Window::Current->Activate();
        }
    }

	// Create device swapchain and command queue
	auto main_page = ref new MainPage();
	Window::Current->Content = main_page;
	Window::Current->Activate();

#ifndef NDEBUG
	Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
	D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	debugInterface->EnableDebugLayer();
#endif //  DEBUG

	device_t dev;
	Microsoft::WRL::ComPtr<IDXGIFactory4> fact;
	CHECK_HRESULT(CreateDXGIFactory1(IID_PPV_ARGS(fact.GetAddressOf())));
	Microsoft::WRL::ComPtr<IDXGIAdapter> adaptater;
	CHECK_HRESULT(fact->EnumAdapters(0, adaptater.GetAddressOf()));
	CHECK_HRESULT(D3D12CreateDevice(adaptater.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(dev.GetAddressOf())));

	ID3D12CommandQueue *queue;
	D3D12_COMMAND_QUEUE_DESC cmddesc = { D3D12_COMMAND_LIST_TYPE_DIRECT };
	CHECK_HRESULT(dev->CreateCommandQueue(&cmddesc, IID_PPV_ARGS(&queue)));

	IDXGISwapChain3 *chain;
	DXGI_SWAP_CHAIN_DESC1 swapChain = {};
	swapChain.BufferCount = 2;
	swapChain.Scaling = DXGI_SCALING_STRETCH;
	swapChain.Width = 1024;
	swapChain.Height = 1024;
	swapChain.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChain.SampleDesc.Count = 1;
	swapChain.Flags = 0;
	swapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

	CHECK_HRESULT(fact->CreateSwapChainForComposition(queue, &swapChain, nullptr, (IDXGISwapChain1**)&chain));

	uint32_t w, h;
	CHECK_HRESULT(chain->GetSourceSize(&w, &h));
//	swap_chain_t chain = std::make_tuple(dev, chain, queue, w, h, irr::video::ECOLOR_FORMAT::ECF_R8G8B8A8_UNORM);
	auto swap_chain_panel = main_page->get_swap_chain_native();

	swap_chain_panel->Dispatcher->RunAsync(CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
	{
		// Get backing native interface for SwapChainPanel
		ComPtr<ISwapChainPanelNative> panelNative;
		reinterpret_cast<IUnknown*>(swap_chain_panel)->QueryInterface(IID_PPV_ARGS(&panelNative));
		panelNative->SetSwapChain(chain);
	}, CallbackContext::Any));

	sample = std::make_unique<MeshSample>(dev, std::make_unique<swap_chain_t>(chain), std::make_unique<command_queue_t>(queue), 1024, 1024, irr::video::ECF_R8G8B8A8_UNORM);

	// Create a task that will be run on a background thread.
	auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction ^ action)
	{
		// Calculate the updated frame and render once per vertical blanking interval.
		while (action->Status == AsyncStatus::Started)
		{
			sample->Draw();
		}
	});

	// Run task on a dedicated high priority background thread.
	render_loop = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
}

/// <summary>
/// Invoked when application execution is being suspended.  Application state is saved
/// without knowing whether the application will be terminated or resumed with the contents
/// of memory still intact.
/// </summary>
/// <param name="sender">The source of the suspend request.</param>
/// <param name="e">Details about the suspend request.</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
    (void) sender;  // Unused parameter
    (void) e;   // Unused parameter

    //TODO: Save application state and stop any background activity
}

/// <summary>
/// Invoked when Navigation to a certain page fails
/// </summary>
/// <param name="sender">The Frame which failed navigation</param>
/// <param name="e">Details about the navigation failure</param>
void App::OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e)
{
    throw ref new FailureException("Failed to load Page " + e->SourcePageType.Name);
}