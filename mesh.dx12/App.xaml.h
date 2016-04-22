//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"
#include "mesh.h"

namespace mesh_dx12
{
	/// <summary>
	/// Provides application-specific behavior to supplement the default Application class.
	/// </summary>
	ref class App sealed
	{
	protected:
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;

	internal:
		App();

	private:
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
		void OnNavigationFailed(Platform::Object ^sender, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs ^e);

		std::unique_ptr<MeshSample> sample;
		Windows::Foundation::IAsyncAction^ render_loop;
	};
}
