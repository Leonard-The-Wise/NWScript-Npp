// This file is part of Notepad++ project
// Copyright (C) 2022 Leonardo Silva

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "pch.h"

#include "PluginDarkMode.h"
#include "D2DRenderFactory.h"


using namespace PluginDarkMode;

// Main Direct3D and Direct2D objects (singleton objects)
static Microsoft::WRL::ComPtr<ID2D1Factory7> _pD2DFactory;
static Microsoft::WRL::ComPtr<ID3D11Device> _pD3DDevice;
static Microsoft::WRL::ComPtr<ID3D11DeviceContext> _pD3DDeviceContext;
static Microsoft::WRL::ComPtr<IDXGIDevice1> _pDXGIDevice;
static Microsoft::WRL::ComPtr<ID2D1Device6> _pD2DDevice;
static Microsoft::WRL::ComPtr<ID2D1DeviceContext6> _pD2DDeviceContext;
static D3D_FEATURE_LEVEL _d3dFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;

// Public Access

bool D2DRenderFactory::beginDrawFrame(HWND hWndControl, HDC hdc)
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

bool D2DRenderFactory::endDrawFrame()
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

void D2DRenderFactory::disposeOffScreen()
{
	_pGDIInteropRenderer.Reset();
	_pD2DBitmap.Reset();
}

void D2DRenderFactory::dispose()
{
	discardDeviceResources();
	_pD2DFactory.Reset();
}

void D2DRenderFactory::createBrush(COLORREF color, Microsoft::WRL::ComPtr<ID2D1SolidColorBrush>& brush)
{
	float r = GetRValue(color) / 255.0f;
	float g = GetGValue(color) / 255.0f;
	float b = GetBValue(color) / 255.0f;
	_pD2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(r, g, b), &brush);
}

bool D2DRenderFactory::refreshResources()
{
	if (!_pD2DFactory)
		return false;

	return (createDeviceResources() == S_OK);
}

bool D2DRenderFactory::initialize()
{
	if (_pD2DFactory)
		return true;

	return (createDeviceIndependentResources() == S_OK);
}

ID2D1DeviceContext& D2DRenderFactory::getRenderer() {
	return *_pD2DDeviceContext.Get();
}

void D2DRenderFactory::getRendererTarget(ID2D1Image** Target)
{
	if (_pD2DDeviceContext)
		_pD2DDeviceContext->GetTarget(Target);
	else
		*Target = nullptr;
}

bool D2DRenderFactory::isInitialized() {
	return (_pD2DFactory != nullptr && _pD2DDeviceContext != nullptr);
}


HRESULT D2DRenderFactory::createDeviceIndependentResources()
{
	return D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, IID_PPV_ARGS(&_pD2DFactory));
}

HRESULT D2DRenderFactory::createDeviceResources()
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

void D2DRenderFactory::discardDeviceResources()
{
	_pGDIInteropRenderer.Reset();
	_pD2DBitmap.Reset();
	_pD2DDeviceContext.Reset();
	_pD2DDevice.Reset();
	_pDXGIDevice.Reset();
	_pD3DDeviceContext.Reset();
	_pD3DDevice.Reset();
}


bool DarkModeD2DRenderFactory::beginDrawFrame(HWND hWndControl, HDC hdc)
{
	if (!D2DRenderFactory::beginDrawFrame(hWndControl, hdc))
		return false;

	// Refresh brushes if they changed
	if (bBrushesDirty)
		refreshBrushes();

	D2D1_RECT_F fillRect = D2D1::RectF(static_cast<float>(rcCurrentControl.left), static_cast<float>(rcCurrentControl.top),
		static_cast<float>(rcCurrentControl.right), static_cast<float>(rcCurrentControl.bottom));
	_pD2DDeviceContext->FillRectangle(fillRect, getDarkBrush(D2DDarkBrushes::background));

	return true;
}

ID2D1SolidColorBrush* DarkModeD2DRenderFactory::getDarkBrush(D2DDarkBrushes brush)
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
		return d2dLightEdgeBackground.Get();
	case D2DDarkBrushes::darkEdgeColor:
		return d2dDarkEdgeBackground.Get();
	}

	return NULL;
}

void DarkModeD2DRenderFactory::refreshBrushes()
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

void DarkModeD2DRenderFactory::discardBrushes()
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
	d2dLightEdgeBackground.Reset();
	d2dDarkEdgeBackground.Reset();
}

void DarkModeD2DRenderFactory::reCreateBrushes()
{
	// Backgrounds
	createBrush(getTheme()._colors.background, d2dBackground);
	createBrush(getTheme()._colors.softerBackground, d2dSofterBackground);
	createBrush(getTheme()._colors.hotBackground, d2dHotBackground);
	createBrush(getTheme()._colors.pureBackground, d2dPureBackground);
	createBrush(getTheme()._colors.errorBackground, d2dErrorBackground);
	createBrush(lightColor(getTheme()._colors.background, BKLUMINANCE_BRIGHTER), d2dHardlightBackground);
	createBrush(lightColor(getTheme()._colors.background, BKLUMINANCE_SOFTER), d2dSoftlightBackground);

	// Text
	createBrush(getTheme()._colors.text, d2dTextColorBrush);
	createBrush(getTheme()._colors.darkerText, d2dDarkerTextColorBrush);
	createBrush(getTheme()._colors.disabledText, d2dDisabledTextColorBrush);
	createBrush(getTheme()._colors.linkText, d2dLinkTextColorBrush);

	// Shape edges
	createBrush(getTheme()._colors.edge, d2dEdgeBackground);
	createBrush(lightColor(getTheme()._colors.edge, EDGELUMINANCE_BRIGHTER), d2dLightEdgeBackground);
	createBrush(lightColor(getTheme()._colors.edge, EDGELUMINANCE_DARKER), d2dDarkEdgeBackground);
}

bool DarkModeD2DRenderFactory::refreshResources()
{
	bool wasInitialized = isInitialized();

	if (!D2DRenderFactory::refreshResources())
		return false;

	if (!wasInitialized)  // recreate resources if was not initialized before calling refreshResources
		refreshBrushes();

	return true;
}

void DarkModeD2DRenderFactory::discardDeviceResources()
{
	discardBrushes();
	D2DRenderFactory::discardDeviceResources();
}

void DarkModeD2DRenderFactory::dispose()
{
	discardBrushes();
	D2DRenderFactory::dispose();
}

