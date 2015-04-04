// Copyright (C) 2015 Vincent Lejeune
// For conditions of distribution and use, see copyright notice in License.txt

#pragma once
#include <windows.h>
#include <windowsx.h>

class WindowUtil
{
private:
  // this is the main message handler for the program
  static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

public:
  static HWND Create(HINSTANCE hInstance,
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
    wc.lpszClassName = "WindowClass1";

    // register the window class
    RegisterClassEx(&wc);

    // create the window and use the result as the handle
    hWnd = CreateWindowEx(NULL,
      "WindowClass1",    // name of the window class
      "MeshDX12",   // title of the window
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
};