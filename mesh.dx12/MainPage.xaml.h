﻿//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include <windows.ui.xaml.media.dxinterop.h>

namespace mesh_dx12
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	public:
		MainPage();
		Windows::UI::Xaml::Controls::SwapChainPanel^ get_swap_chain_native();
	};
}
