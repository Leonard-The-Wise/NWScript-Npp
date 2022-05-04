/** @file D2DImageFactory.cpp
 * A Direct2D renderer and image processing class
 *
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"

#include "PluginDarkMode.h"
#include "D2DImageFactory.h"


using namespace D2DWrapper;

// Main Direct3D and Direct2D objects (singleton objects)
Microsoft::WRL::ComPtr<ID2D1Factory7> _pD2DFactory;
Microsoft::WRL::ComPtr<ID3D11Device> _pD3DDevice;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> _pD3DDeviceContext;
Microsoft::WRL::ComPtr<IDXGIDevice1> _pDXGIDevice;
Microsoft::WRL::ComPtr<ID2D1Device6> _pD2DDevice;
Microsoft::WRL::ComPtr<ID2D1DeviceContext6> _pD2DDeviceContext;

// Offscreen buffer - it often gets resized to the largest control in display
Microsoft::WRL::ComPtr<ID2D1Bitmap1> _pD2DBitmap;
Microsoft::WRL::ComPtr<ID2D1GdiInteropRenderTarget> _pGDIInteropRenderer; // For interoperability with GDI

// Image factory to manipulate bitmaps
Microsoft::WRL::ComPtr<IWICImagingFactory2> _pWICImagingFactory;
D3D_FEATURE_LEVEL _d3dFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;

// Public Access

bool D2DImageFactory::beginDrawFrame(HWND hWndControl, HDC hdc)
{
	// BeginDraw cannot be called twice without a endDrawFrame in between
	assert(bRendering == false);

	if (_pD2DFactory == nullptr)
		initialize();

	// Update pointers
	hWndCurrentControl = hWndControl;
	if (hdc != nullptr)
	{
		currentDC = hdc;
		dcNeedsRelease = false;
	}
	else
	{
		currentDC = GetDC(hWndCurrentControl);
		if (currentDC)
			dcNeedsRelease = true;
		else
			return false;
	}

	// Recreate objects if needed
	bool success = refreshResources();

	if (success)
	{
		_pD2DDeviceContext->BeginDraw();
		_pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

		bRendering = true;
	}

	return success;
}

bool D2DImageFactory::endDrawFrame()
{
	// Cannot call endDrawFrame if beginDraw wasn't successful
	assert(bRendering == true);
	bRendering = false;

	assert(_pD2DFactory != nullptr);

	HDC sourceDC;
	HRESULT hr = _pGDIInteropRenderer->GetDC(D2D1_DC_INITIALIZE_MODE_COPY, &sourceDC);
	if (SUCCEEDED(hr))
	{
		BitBlt(currentDC, 0, 0, rcCurrentControl.right, rcCurrentControl.bottom, sourceDC, 0, 0, SRCCOPY);
		_pGDIInteropRenderer->ReleaseDC(NULL);
	}
	
	hr = _pD2DDeviceContext->EndDraw();
	if (hr == D2DERR_RECREATE_TARGET)
	{
		discardDeviceResources();
		if (dcNeedsRelease)
		{
			ReleaseDC(hWndCurrentControl, currentDC);
			dcNeedsRelease = false;
		}
		InvalidateRect(hWndCurrentControl, NULL, false);

		return false;
	}

	if (dcNeedsRelease)
	{
		ReleaseDC(hWndCurrentControl, currentDC);
		dcNeedsRelease = false;
	}

	return true;
}

void D2DImageFactory::disposeOffScreen()
{
	_pGDIInteropRenderer.Reset();
	_pD2DBitmap.Reset();
}

void D2DImageFactory::dispose()
{
	discardDeviceResources();
	_pWICImagingFactory.Reset();
	_pD2DFactory.Reset();
}

void D2DImageFactory::createBrush(COLORREF color, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>& brush)
{
	float r = GetRValue(color) / 255.0f;
	float g = GetGValue(color) / 255.0f;
	float b = GetBValue(color) / 255.0f;
	_pD2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(r, g, b), &brush);
}

bool D2DImageFactory::refreshResources()
{
	if (!_pD2DFactory)
		return false;

	return (createDeviceResources() == S_OK);
}

bool D2DImageFactory::initialize()
{
	if (_pD2DFactory)
		return true;

	return (createDeviceIndependentResources() == S_OK);
}

ID2D1DeviceContext& D2DImageFactory::getRenderer() {
	return *_pD2DDeviceContext.Get();
}

void D2DImageFactory::getRendererTarget(ID2D1Image** Target)
{
	if (_pD2DDeviceContext)
		_pD2DDeviceContext->GetTarget(Target);
	else
		*Target = nullptr;
}

bool D2DImageFactory::isInitialized() {
	return (_pD2DFactory != nullptr && _pD2DDeviceContext != nullptr);
}

HBITMAP D2DImageFactory::loadSVGToHBITMAP(HMODULE module, int idResource, bool , UINT32 width, UINT32 height)
{
	// Cannot load SVG to HBITMAP while rendering
	assert(!bRendering);
	if (bRendering)
		return NULL;

	if (!refreshResources())
		return NULL;

	HBITMAP destBitmap = NULL;

	auto hResource = FindResourceW(module, MAKEINTRESOURCE(idResource), L"SVG");
	size_t _size = SizeofResource(module, hResource);
	auto hMemory = LoadResource(module, hResource);
	LPVOID ptr = LockResource(hMemory);

	// Copy SVG bytes into a real hglobal memory handle
	hMemory = ::GlobalAlloc(GHND, _size);
	if (hMemory)
	{
		void* pBuffer = ::GlobalLock(hMemory);
		memcpy(pBuffer, ptr, _size);
	}

	// Create stream
	IStream* pStream = nullptr;
	HRESULT hr = CreateStreamOnHGlobal(hMemory, TRUE, &pStream);
	if (SUCCEEDED(hr))
	{
		// Create WIC renderer to retrieve device context
		Microsoft::WRL::ComPtr<IWICBitmap> wicBitmap;
		Microsoft::WRL::ComPtr<ID2D1SvgDocument> svgDocument;
		Microsoft::WRL::ComPtr<ID2D1SvgElement> svgRoot;
		Microsoft::WRL::ComPtr<ID2D1RenderTarget> wicRenderer;
		Microsoft::WRL::ComPtr<ID2D1DeviceContext5> wicDc;
		hr = _pWICImagingFactory->CreateBitmap(width, height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, &wicBitmap);
		D2D1_RENDER_TARGET_PROPERTIES targetProps = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_SOFTWARE);
		targetProps.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
		hr = _pD2DFactory->CreateWicBitmapRenderTarget(wicBitmap.Get(), targetProps, &wicRenderer);
		wicRenderer->QueryInterface(IID_PPV_ARGS(&wicDc));

		// Create SVG document
		D2D1_SIZE_F initSize = D2D1::SizeF(32.0f, 32.0f);  // Just the initial viewport...
		hr = wicDc->CreateSvgDocument(pStream, initSize, &svgDocument);

		if (SUCCEEDED(hr))
		{
			// Get SVG document size
			svgDocument->GetRoot(&svgRoot);
			D2D1_SVG_LENGTH svgWidth = {}, svgHeight = {};

			if (svgRoot->IsAttributeSpecified(L"viewBox"))
			{
				D2D1_SVG_VIEWBOX viewBox;

				hr = svgRoot->GetAttributeValue(L"viewBox", D2D1_SVG_ATTRIBUTE_POD_TYPE_VIEWBOX, &viewBox, sizeof(viewBox));
				if (SUCCEEDED(hr))
				{
					svgWidth.value = viewBox.width;
					svgHeight.value = viewBox.height;
					svgDocument->SetViewportSize(D2D1::SizeF(viewBox.width, viewBox.height));
				}
			}
			else
			{
				if (svgRoot->IsAttributeSpecified(L"width"))
					svgRoot->GetAttributeValue(L"width", &svgWidth);
				if (svgRoot->IsAttributeSpecified(L"height"))
					svgRoot->GetAttributeValue(L"height", &svgHeight);
			}

			// Error: size not found
			if (svgWidth.value == 0.0f || svgHeight.value == 0.0f)
				return NULL;

			// Render svgDocument to WICBitmap
			if (SUCCEEDED(hr))
			{
				const float scale = std::min(static_cast<float>(width) / svgWidth.value, static_cast<float>(height) / svgHeight.value);
				const D2D1_POINT_2F fCenter = D2D1::Point2F(static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f);
				const D2D1_SIZE_F fTranslation = D2D1::SizeF((static_cast<float>(width) - svgWidth.value) / 2.0f, 
					(static_cast<float>(height) - svgHeight.value) / 2.0f);

				D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Identity();
				transform = transform * D2D1::Matrix3x2F::Translation(fTranslation);
				transform = transform * D2D1::Matrix3x2F::Scale(scale, scale, fCenter);

				wicDc->BeginDraw();
				wicDc->SetTransform(transform);
				wicDc->DrawSvgDocument(svgDocument.Get());
				hr = wicDc->EndDraw();
			}

			// Get access to WIC bitmap pixel data
			Microsoft::WRL::ComPtr<IWICBitmapLock> pBuffer;
			UINT pBufferSize = 0;
			BYTE* pv = nullptr;
			hr = wicBitmap->Lock(NULL, WICBitmapLockRead, &pBuffer);
			hr = pBuffer->GetDataPointer(&pBufferSize, &pv);

			// Create HBITMAP as a DIB Section from pixel data
			BITMAPINFOHEADER bmih = {};
			bmih.biSize = sizeof(BITMAPINFOHEADER);
			bmih.biWidth = width;
			bmih.biHeight = -static_cast<int>(height);  // Height is inverted
			bmih.biPlanes = 1;
			bmih.biBitCount = 32;
			bmih.biCompression = BI_RGB;
			BITMAPINFO dbmi = {};
			dbmi.bmiHeader = bmih;
			HDC screenDC = GetDC(NULL);
			void* pvDest = nullptr;
			destBitmap = CreateDIBSection(screenDC, &dbmi, DIB_RGB_COLORS, &pvDest, NULL, 0);
			ReleaseDC(NULL, screenDC);

			// Copy contents to DIB section.
			if (pv && pvDest)
				memcpy(pvDest, pv, pBufferSize);
		}
		pStream->Release();
	}

	if (hMemory)
		GlobalFree(hMemory);

	return destBitmap;
}

HRESULT D2DImageFactory::createDeviceIndependentResources()
{
	HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&_pD2DFactory));
	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_pWICImagingFactory));
	}
	return hr;
}

HRESULT D2DImageFactory::createDeviceResources()
{
	HRESULT hr = S_OK;

	// Check sizes to see if we need to refresh bitmap size later
	GetClientRect(hWndCurrentControl, &rcCurrentControl);
	SIZE szControl = { rcCurrentControl.right - rcCurrentControl.left, rcCurrentControl.bottom - rcCurrentControl.top };
	SIZE szCurrentRect = { _rcBitmapRenderer.right - _rcBitmapRenderer.left, _rcBitmapRenderer.bottom - _rcBitmapRenderer.top };

	if (!_pD2DDeviceContext)
	{
		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};

		// Create main objects
		D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, &_pD3DDevice, &_d3dFeatureLevel, &_pD3DDeviceContext);
		_pD3DDevice.As(&_pDXGIDevice);
		_pD2DFactory->CreateDevice(_pDXGIDevice.Get(), &_pD2DDevice);
		_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &_pD2DDeviceContext);

		// Create main back-buffer bitmap for draws
		D2D1_BITMAP_PROPERTIES1 bitmapProperties =
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			);
		_pD2DDeviceContext->CreateBitmap(D2D1::SizeU(szControl.cx, szControl.cy), NULL, 0, bitmapProperties, &_pD2DBitmap);
		_pD2DDeviceContext->SetTarget(_pD2DBitmap.Get());

		_pD2DDeviceContext->QueryInterface(IID_PPV_ARGS(&_pGDIInteropRenderer));
		_rcBitmapRenderer = rcCurrentControl;
		return S_OK;

		if (dcNeedsRelease)
			ReleaseDC(hWndCurrentControl, currentDC);

		return E_FAIL;
	}

	if (szControl.cx > szCurrentRect.cx || szControl.cy > szCurrentRect.cy || !_pD2DBitmap)
	{
		SIZE szNewSize = { std::max(szControl.cx, szCurrentRect.cx), std::max(szControl.cy, szCurrentRect.cy) };
		_rcBitmapRenderer = { 0, 0, szNewSize.cx, szNewSize.cy };
		D2D1_SIZE_F bmpSize = D2D1::SizeF(static_cast<float>(szNewSize.cx), static_cast<float>(szNewSize.cy));
		_pGDIInteropRenderer.Reset();
		_pD2DBitmap.Reset();

		D2D1_BITMAP_PROPERTIES1 bitmapProperties =
			D2D1::BitmapProperties1(
				D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
			);
		hr = _pD2DDeviceContext->CreateBitmap(D2D1::SizeU(szNewSize.cx, szNewSize.cy), NULL, 0, bitmapProperties, &_pD2DBitmap);
		_pD2DDeviceContext->SetTarget(_pD2DBitmap.Get());
		if (_pD2DBitmap)
			_pD2DDeviceContext->QueryInterface(IID_PPV_ARGS(&_pGDIInteropRenderer));

		if (!SUCCEEDED(hr) && dcNeedsRelease)
			ReleaseDC(hWndCurrentControl, currentDC);
	}

	return hr;
}

void D2DImageFactory::discardDeviceResources()
{
	_pGDIInteropRenderer.Reset();
	_pD2DBitmap.Reset();
	_pD2DDeviceContext.Reset();
	_pD2DDevice.Reset();
	_pDXGIDevice.Reset();
	_pD3DDeviceContext.Reset();
	_pD3DDevice.Reset();
}


// Dark Mode interface


bool D2DDarkModeRenderer::beginDrawFrame(HWND hWndControl, HDC hdc)
{
	if (!D2DImageFactory::beginDrawFrame(hWndControl, hdc))
		return false;

	// Refresh brushes if they changed
	if (bBrushesDirty)
		refreshBrushes();

	D2D1_RECT_F fillRect = D2D1::RectF(static_cast<float>(rcCurrentControl.left), static_cast<float>(rcCurrentControl.top),
		static_cast<float>(rcCurrentControl.right), static_cast<float>(rcCurrentControl.bottom));
	_pD2DDeviceContext->FillRectangle(fillRect, getDarkBrush(D2DDarkBrushes::background));

	return true;
}

ID2D1SolidColorBrush* D2DDarkModeRenderer::getDarkBrush(D2DDarkBrushes brush)
{
	// Cannot access brushes if not initialized
	assert(isInitialized());

	switch (brush)
	{
	case D2DDarkBrushes::background:
		return d2dBackground.Get();
	case D2DDarkBrushes::darkerBackground:
		return d2dPureBackground.Get();
	case D2DDarkBrushes::softerBackground:
		return d2dSofterBackground.Get();
	case D2DDarkBrushes::hotBackground:
		return d2dHotBackground.Get();
	case D2DDarkBrushes::errorBackground:
		return d2dErrorBackground.Get();
	case D2DDarkBrushes::hardlightBackground:
		return d2dHardlightBackground.Get();
	case D2DDarkBrushes::softlightBackground:
		return d2dSoftlightBackground.Get();
	case D2DDarkBrushes::textColor:
		return d2dTextColorBrush.Get();
	case D2DDarkBrushes::darkerTextColor:
		return d2dDarkerTextColorBrush.Get();
	case D2DDarkBrushes::disabledTextColor:
		return d2dDisabledTextColorBrush.Get();
	case D2DDarkBrushes::linkTextColor:
		return d2dLinkTextColorBrush.Get();
	case D2DDarkBrushes::edgeColor:
		return d2dEdgeBackground.Get();
	case D2DDarkBrushes::lightEdgeColor:
		return d2dHotEdgeBackground.Get();
	case D2DDarkBrushes::darkEdgeColor:
		return d2dDarkEdgeBackground.Get();
	}

	return NULL;
}

void D2DDarkModeRenderer::refreshBrushes()
{
	// Brushes may not be refreshed instantly because _pBitmapRenderer may not yet be created.
	// When beginDrawFrame is called, this is checked.
	bBrushesDirty = true;

	if (!isInitialized())
		return;

	discardBrushes();
	reCreateBrushes();

	bBrushesDirty = false;
}

void D2DDarkModeRenderer::discardBrushes()
{
	d2dBackground.Reset();
	d2dSofterBackground.Reset();
	d2dHotBackground.Reset();
	d2dPureBackground.Reset();
	d2dErrorBackground.Reset();
	d2dHardlightBackground.Reset();
	d2dSoftlightBackground.Reset();
	d2dTextColorBrush.Reset();
	d2dDarkerTextColorBrush.Reset();
	d2dDisabledTextColorBrush.Reset();
	d2dLinkTextColorBrush.Reset();
	d2dEdgeBackground.Reset();
	d2dHotEdgeBackground.Reset();
	d2dDarkEdgeBackground.Reset();
}

void D2DDarkModeRenderer::reCreateBrushes()
{
	// Backgrounds
	createBrush(PluginDarkMode::getTheme()._colors.background, d2dBackground);
	createBrush(PluginDarkMode::getTheme()._colors.softerBackground, d2dSofterBackground);
	createBrush(PluginDarkMode::getTheme()._colors.hotBackground, d2dHotBackground);
	createBrush(PluginDarkMode::getTheme()._colors.pureBackground, d2dPureBackground);
	createBrush(PluginDarkMode::getTheme()._colors.errorBackground, d2dErrorBackground);
	createBrush(PluginDarkMode::lightColor(PluginDarkMode::getTheme()._colors.background, BKLUMINANCE_BRIGHTER), d2dHardlightBackground);
	createBrush(PluginDarkMode::lightColor(PluginDarkMode::getTheme()._colors.background, BKLUMINANCE_SOFTER), d2dSoftlightBackground);

	// Text
	createBrush(PluginDarkMode::getTheme()._colors.text, d2dTextColorBrush);
	createBrush(PluginDarkMode::getTheme()._colors.darkerText, d2dDarkerTextColorBrush);
	createBrush(PluginDarkMode::getTheme()._colors.disabledText, d2dDisabledTextColorBrush);
	createBrush(PluginDarkMode::getTheme()._colors.linkText, d2dLinkTextColorBrush);

	// Shape edges
	createBrush(PluginDarkMode::getTheme()._colors.edge, d2dEdgeBackground);
	createBrush(PluginDarkMode::lightColor(PluginDarkMode::getTheme()._colors.edge, EDGELUMINANCE_BRIGHTER), d2dHotEdgeBackground);
	createBrush(PluginDarkMode::lightColor(PluginDarkMode::getTheme()._colors.edge, EDGELUMINANCE_DARKER), d2dDarkEdgeBackground);
}

bool D2DDarkModeRenderer::refreshResources()
{
	bool wasInitialized = isInitialized();

	if (!D2DImageFactory::refreshResources())
		return false;

	if (!wasInitialized)  // recreate resources if was not initialized before calling refreshResources
		refreshBrushes();

	return true;
}

void D2DDarkModeRenderer::discardDeviceResources()
{
	discardBrushes();
	D2DImageFactory::discardDeviceResources();
}

