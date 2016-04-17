#pragma once
#include <Windows.h>

struct Sample
{
	Sample(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
	int run();
private:
	HWND window;
	HINSTANCE hinstance;
protected:
	virtual void Init(HINSTANCE hInstance, HWND window) = 0;
	virtual void Draw() = 0;
};