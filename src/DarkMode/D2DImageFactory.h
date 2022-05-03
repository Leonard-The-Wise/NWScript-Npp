/** @file D2DImageFactory.h
 * A Direct2D renderer and image processing class
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

//#include <Windows.h>
//#include <d2d1_3.h>
//#include <d2d1effects.h>
//#include <d2d1svg.h>
//#include <d3d11.h>
//#include <wrl/client.h>
//#include <wincodec.h> 
//#include <assert.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")

namespace D2DWrapper
{

	// A class to work with Direct2D rendering and WIC image compositioning
	class D2DImageFactory
	{
	public:

		D2DImageFactory() 
		{
			initialize();
		}

		// Begins drawing a frame to offscreen of the control.
		// If the function succeeds, you must call endDrawFrame later to flush the data into the Device Context
		// of the window. It only fails if a driver has a problem or something like that.
		// You may only call beginDrawFrame once before calling endDrawFrame.
		virtual bool beginDrawFrame(HWND hWndControl, HDC hdc = nullptr);

		// Finishes drawing the off-screen image and flush contents to window (or control) HDC.
		// Must call beginDrawFrame first to setup the routines.
		virtual bool endDrawFrame();

		// Access the Direct2D renderer.
		ID2D1DeviceContext& getRenderer();

		// Release the current off-screen image, so it will be rebuilt (useful when window is closed to free resources)
		virtual void disposeOffScreen();

		// Release all objects
		virtual void dispose();

		// Retrieves the back-buffer of the current renderer
		virtual void getRendererTarget(ID2D1Image** Target);

		void createBrush(COLORREF color, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>& brush);

		HBITMAP loadSVGToHBITMAP(HMODULE module, int idResource, bool invertLuminosity = false, UINT32 width = 0, UINT32 height = 0);

	protected:
		bool bRendering = false;
		RECT rcCurrentControl = {};

		bool isInitialized(); // non-derived class members shouldn't be concerned if initialized or not
		virtual bool refreshResources();
		virtual HRESULT createDeviceResources();
		virtual void discardDeviceResources();

	private:
		// Current control being processed
		HWND hWndCurrentControl = {};
		HDC currentDC = nullptr;
		bool dcNeedsRelease = false;

		RECT _rcBitmapRenderer = {};

		// Direct2D resource management
		bool initialize();
		HRESULT createDeviceIndependentResources();
	};

	// Selection of brushes for Dark Mode
	enum class D2DDarkBrushes
	{
		background, darkerBackground, softerBackground, hotBackground, errorBackground,
		hardlightBackground, softlightBackground,
		textColor, darkerTextColor, disabledTextColor, linkTextColor,
		edgeColor, lightEdgeColor, darkEdgeColor
	};

	// Direct2D Dark Mode renderer for hardware accelerated anti-aliased and double-buffered drawings
	class D2DDarkModeRenderer : public D2DImageFactory
	{
	public:

		// Begins drawing a frame to offscreen of the control.
		// If the function succeeds, you must call endDrawFrame later to flush the data into the Device Context
		// of the window. It only fails if a driver has a problem or something like that.
		// You may only call beginDrawFrame once before calling endDrawFrame.
		virtual bool beginDrawFrame(HWND hWndControl, HDC hdc = nullptr) override;

		virtual void dispose() override;

		// Get a drawing brush (in Direct2D, the concept of PEN do not exist. It is all brushes).
		ID2D1SolidColorBrush* getDarkBrush(D2DDarkBrushes brush);

		// Refresh drawing brushes to the current selected theme's
		void refreshBrushes();

	protected:
		virtual bool refreshResources() override;
		virtual void discardDeviceResources() override;

	private:

		// Current states
		bool bBrushesDirty = false;

		// D2DBrushes (D2D don't have the notions of pens - it's all brushes)
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dBackground;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dSofterBackground;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dHotBackground;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dPureBackground;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dErrorBackground;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dHardlightBackground;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dSoftlightBackground;

		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dTextColorBrush;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dDarkerTextColorBrush;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dDisabledTextColorBrush;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dLinkTextColorBrush;

		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dEdgeBackground;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dLightEdgeBackground;
		Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> d2dDarkEdgeBackground;

		void discardBrushes();
		void reCreateBrushes();
	};
}

