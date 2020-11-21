#pragma once
#include <algorithm>
#include <random>
#include <d3d11_2.h>
#include <d2d1_3helper.h>
#include <dwrite_3.h>
#include <dcomp.h>
#include <wrl.h>

#include "cli.hpp"
#include "core.h"
#include "xorstr.hpp"

#pragma comment( lib, "dxgi" )
#pragma comment( lib, "d2d1" )
#pragma comment( lib, "d3d11" )
#pragma comment( lib, "dcomp" )
#pragma comment( lib, "dwrite" )

#define RET_CHK(x) if ( x != S_OK ) return
#define RET_CHK2(x) if ( x != S_OK ) { core::set_thread( remote_window, remote_thread ); return; }

class d2d_window_t
{
public:
	d2d_window_t()
	{
		_name.reserve(16u);
		std::generate_n(std::back_inserter(_name), 16u, []
			{
				thread_local std::mt19937_64 mersenne_engine(std::random_device{}());
				const std::uniform_int_distribution<> distribution(97, 122); // 'a', 'z'
				return static_cast<uint8_t>(distribution(mersenne_engine));
			});

		WNDCLASSA window_class
		{
			0,
			[](const HWND window, const UINT message, const WPARAM wparam, const LPARAM lparam) -> LRESULT
			{
				return DefWindowProcA(window, message, wparam, lparam);
			},
			0,
			0,
			GetModuleHandleW(nullptr),
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			"_DWMTestCom"
		};

		RegisterClassA(&window_class);

		_handle = CreateWindowExA(0, /*_name.c_str()*/"_DWMTestCom", "", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, window_class.hInstance, nullptr);
	}

	~d2d_window_t()
	{

		DestroyWindow(_handle);
		UnregisterClassA(_name.c_str(), GetModuleHandleW(nullptr));
	}

	HWND _handle;
	std::string _name;
};

template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class d2d_renderer_t
{
public:
	d2d_renderer_t(const HWND local_window_handle, const HWND process_window_handle) : remote_window(process_window_handle), is_destroyed(false)
	{
		ComPtr<ID3D11Device> d3d_device;
		RET_CHK(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &d3d_device, nullptr, nullptr));

		ComPtr<IDXGIDevice> dxgi_device;
		RET_CHK(d3d_device.As(&dxgi_device));

		ComPtr<IDXGIFactory2> dxgi_factory;
		RET_CHK(CreateDXGIFactory2(0, __uuidof(IDXGIFactory2), reinterpret_cast<void**>(dxgi_factory.GetAddressOf())));

		DXGI_SWAP_CHAIN_DESC1 description = { 0 };
		description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		description.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		description.BufferCount = 2;
		description.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
		description.SampleDesc.Count = 1;

		RECT rect;
		while (!GetClientRect(remote_window, &rect));

		// fucking compiler warnings
		description.Width = static_cast<UINT>(rect.right - rect.left);
		description.Height = static_cast<UINT>(rect.bottom - rect.top);

		wnd_hjk::screen_resolution.first = static_cast<float>(description.Width);
		wnd_hjk::screen_resolution.second = static_cast<float>(description.Height);

		RET_CHK(dxgi_factory->CreateSwapChainForComposition(dxgi_device.Get(), &description, nullptr, dxgi_chain.GetAddressOf()));

		const D2D1_FACTORY_OPTIONS factory_options = { D2D1_DEBUG_LEVEL_NONE };

		ComPtr<ID2D1Factory2> d2d_factory;
		RET_CHK(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factory_options, d2d_factory.GetAddressOf()));
		
		ComPtr<ID2D1Device1> d2d_device;
		RET_CHK(d2d_factory->CreateDevice(dxgi_device.Get(), d2d_device.GetAddressOf()));

		RET_CHK(d2d_device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, d2d_context.GetAddressOf()));
		
		ComPtr<IDXGISurface2> dxgi_surface;
		RET_CHK(dxgi_chain->GetBuffer(0, __uuidof(IDXGISurface2), reinterpret_cast<void**>(dxgi_surface.GetAddressOf())));;

		D2D1_BITMAP_PROPERTIES1 bitmap_properties = { };
		bitmap_properties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
		bitmap_properties.pixelFormat.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bitmap_properties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

		ComPtr<ID2D1Bitmap1> d2d_bitmap;
		RET_CHK(d2d_context->CreateBitmapFromDxgiSurface(dxgi_surface.Get(), bitmap_properties, d2d_bitmap.GetAddressOf()));

		d2d_context->SetTarget(d2d_bitmap.Get());

		RET_CHK(d2d_context->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), d2d_brush.GetAddressOf()));;

		core::get_thread(local_window_handle, &local_thread);
		core::get_thread(remote_window, &remote_thread);

		core::set_thread(remote_window, local_thread);

		RET_CHK2(DCompositionCreateDevice(dxgi_device.Get(), __uuidof(IDCompositionDevice), reinterpret_cast<void**>(composition_device.GetAddressOf())))
			RET_CHK2(composition_device->CreateTargetForHwnd(remote_window, TRUE, composition_target.GetAddressOf()))
			RET_CHK2(composition_device->CreateVisual(composition_visual.GetAddressOf()))
			RET_CHK2(composition_visual->SetContent(dxgi_chain.Get()))
			RET_CHK2(composition_target->SetRoot(composition_visual.Get()))
			RET_CHK2(composition_device->Commit())
			RET_CHK2(composition_device->WaitForCommitCompletion())

		core::set_thread(remote_window, remote_thread);
		printf("[+] successfully setup...\n");
	}

	void begin_scene()
	{
		d2d_context->BeginDraw();
		d2d_context->Clear();
	}

	void end_scene()
	{
		d2d_context->EndDraw();
		dxgi_chain->Present(0, 0);
	}

	void draw_line(float x1, float y1, float x2, float y2, float thickness, D2D1::ColorF color) {
		d2d_brush->SetColor(D2D1::ColorF(color));
		d2d_context->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), d2d_brush.Get(), thickness);
	}

	void draw_box(float x, float y, float width, float height, float thickness, D2D1::ColorF color, bool filled) {
		d2d_brush->SetColor(D2D1::ColorF(color));
		if (filled)  d2d_context->FillRectangle(D2D1::RectF(x, y, x + width, y + height), d2d_brush.Get());
		else d2d_context->DrawRectangle(D2D1::RectF(x, y, x + width, y + height), d2d_brush.Get(), thickness);
	}

	void draw_corner_box(float x, float y, float w, float h, float thickness, D2D1::ColorF color) {
		float lineW = (w / 5);
		float lineH = (h / 6);
		float lineT = 1;

		//draw inline
		draw_line(x, y, x, y + lineH, thickness, color);
		draw_line(x, y, x + lineW, y, thickness, color);
		draw_line(x + w - lineW, y, x + w, y, thickness, color);
		draw_line(x + w, y, x + w, y + lineH, thickness, color);

		draw_line(x, y + h - lineH, x, y + h, thickness, color);
		draw_line(x, y + h, x + lineW, y + h, thickness, color);
		draw_line(x + w - lineW, y + h, x + w, y + h, thickness, color);
		draw_line(x + w, y + h - lineH, x + w, y + h, thickness, color);
	}

	void draw_health_bar(float health, float x, float y, float height, D2D1::ColorF color) {
		float flBoxes = std::ceil(health / 10.f);

		float flX = x - 7 - height / 4.f; float flY = y - 1;
		float flHeight = height / 10.f;
		float flMultiplier = 12 / 360.f; flMultiplier *= flBoxes - 1;

		draw_box(flX, flY, 4, height + 2, 1, D2D1::ColorF::Black, false);

		draw_box(flX + 1, flY, 2, flHeight * 10.f + 1, 1, D2D1::ColorF::DarkRed, false);

		if (flBoxes > 1)
			draw_box(flX + 1, flY, 2, flHeight * flBoxes + 1, 1, color, false);

		if (height > 25) {
			for (int i = 0; i < 10; i++)
				draw_line(flX, flY + i * flHeight, flX + 4, flY + i * flHeight, 1, D2D1::ColorF::Black);
		}
	}

	void draw_circle(float x, float y, float radius, float thickness, D2D1::ColorF color, bool filled)
	{
		d2d_brush->SetColor(D2D1::ColorF(color));
		if (filled) d2d_context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), d2d_brush.Get());
		else d2d_context->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), d2d_brush.Get(), thickness);
	}

	void draw_crosshair(float x, float y, D2D1::ColorF color, float size) {
		d2d_brush->SetColor(D2D1::ColorF(color));
		d2d_context->DrawLine(D2D1::Point2F(x - size / 2, y), D2D1::Point2F(x + size / 2, y), d2d_brush.Get(), 1);
		d2d_context->DrawLine(D2D1::Point2F(x, y - size / 2), D2D1::Point2F(x, y + size / 2), d2d_brush.Get(), 1);
	}

	void draw_rectangle(const wnd_hjk::vec2_t & pos, const wnd_hjk::vec2_t & size)
	{
		const auto rectangle = D2D1::RectF(pos.first, pos.second, pos.first + size.second, pos.second + size.second);
		d2d_context->FillRectangle(rectangle, d2d_brush.Get());
	}

	void manual_destruct()
	{
		if (is_destroyed)
			return;

		is_destroyed = true;

		core::set_thread(remote_window, local_thread);

		composition_visual->SetContent(nullptr);
		composition_visual->Release();

		composition_target->SetRoot(nullptr);
		composition_target->Release();

		composition_device->Release();

		core::set_thread(remote_window, remote_thread);
	}

	~d2d_renderer_t()
	{
		if (is_destroyed)
			return;

		/*this->manual_destruct();*/
	}

private:
	ComPtr<IDXGISwapChain1> dxgi_chain;
	ComPtr<ID2D1DeviceContext> d2d_context;

	ComPtr<IDCompositionDevice> composition_device;
	ComPtr<IDCompositionTarget> composition_target;
	ComPtr<IDCompositionVisual> composition_visual;
	ComPtr<ID2D1SolidColorBrush> d2d_brush;

	uint64_t local_thread;
	uint64_t remote_thread;

	HWND remote_window;

	bool is_destroyed;
};