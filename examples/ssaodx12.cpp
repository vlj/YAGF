#include <D3DAPI/Misc.h>


int WINAPI WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,
  int nCmdShow)
{
  HWND hwnd = WindowUtil::Create(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
  // this struct holds Windows event messages
  MSG msg;

  // wait for the next message in the queue, store the result in 'msg'
  while (GetMessage(&msg, NULL, 0, 0))
  {
    // translate keystroke messages into the right format
    TranslateMessage(&msg);

    // send the message to the WindowProc function
    DispatchMessage(&msg);
  }

  // return this part of the WM_QUIT message to Windows
  return (int)msg.wParam;
}

