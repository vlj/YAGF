#include "mesh.h"
#include <Windows.h>

extern "C"
{
	__declspec(dllexport) void* create_vulkan_mesh(HWND window, HINSTANCE hInstance);
}

void*  create_vulkan_mesh(HWND window, HINSTANCE)
{
	try
	{
#ifndef NDEBUG
		Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
		debugInterface->EnableDebugLayer();
#endif //  DEBUG

		ID3D12Device *dev;
		Microsoft::WRL::ComPtr<IDXGIFactory4> fact;
		CHECK_HRESULT(CreateDXGIFactory1(IID_PPV_ARGS(fact.GetAddressOf())));
		Microsoft::WRL::ComPtr<IDXGIAdapter> adaptater;
		CHECK_HRESULT(fact->EnumAdapters(0, adaptater.GetAddressOf()));
		CHECK_HRESULT(D3D12CreateDevice(adaptater.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&dev)));

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

		CHECK_HRESULT(fact->CreateSwapChainForHwnd(queue, window, &swapChain, nullptr, nullptr, (IDXGISwapChain1**)&chain));

		uint32_t w, h;
		CHECK_HRESULT(chain->GetSourceSize(&w, &h));
		return new MeshSample(std::make_unique<device_t>(dev), std::make_unique<swap_chain_t>(chain), std::make_unique<command_queue_t>(queue), 1024, 1024, irr::video::ECF_R8G8B8A8_UNORM);
	}
	catch (...)
	{
		return nullptr;
	}
}
