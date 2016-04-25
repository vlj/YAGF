#include "mesh.h"
#include <Windows.h>

namespace
{
	// this is the main message handler for the program
	LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		// sort through and find what code to run for the message given
		switch (message)
		{
			// this message is read when the window is closed
		case WM_DESTROY:
		{
			// close the application entirely
			PostQuitMessage(0);
			return 0;
		} break;
		}

		// Handle any messages the switch statement didn't
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	HWND Create(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nCmdShow)
	{
		// the handle for the window, filled by a function
		HWND hWnd;
		// this struct holds information for the window class
		WNDCLASSEX wc = {};

		// fill in the struct with the needed information
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = hInstance;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wc.lpszClassName = L"WindowClass1";

		// register the window class
		RegisterClassEx(&wc);

		// create the window and use the result as the handle
		hWnd = CreateWindowEx(NULL,
			L"WindowClass1",    // name of the window class
			L"MeshDX12",   // title of the window
			WS_OVERLAPPEDWINDOW,    // window style
			300,    // x-position of the window
			300,    // y-position of the window
			1024,    // width of the window
			1024,    // height of the window
			NULL,    // we have no parent window, NULL
			NULL,    // we aren't using menus, NULL
			hInstance,    // application handle
			NULL);    // used with multiple windows, NULL
		ShowWindow(hWnd, nCmdShow);
		return hWnd;
	}
}

int WINAPI WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow)
{
	HWND window = Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	device_t dev;
	std::unique_ptr<swap_chain_t> chain;
	command_queue_t cmd_queue;
	uint32_t width;
	uint32_t height;
	irr::video::ECOLOR_FORMAT format;
	std::tie(dev, chain, cmd_queue, width, height, format) = create_device_swapchain_and_graphic_presentable_queue(hInstance, window);
	MeshSample app(dev, std::move(chain), cmd_queue, width, height, format);

	// this struct holds Windows event messages
	MSG msg = {};

	// Loop from https://msdn.microsoft.com/en-us/library/windows/apps/dn166880.aspx
	while (WM_QUIT != msg.message)
	{
		bool bGotMsg = (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE) != 0);

		if (bGotMsg)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		app.Draw();
	}

	// return this part of the WM_QUIT message to Windows
	return (int)msg.wParam;
}
