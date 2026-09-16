/*
 * Copyright (c) 2026 ThorVG project. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <cassert>
#include "tvgWindow.h"
#include <SDL3/SDL_main.h>
#if defined(SDL_PLATFORM_MACOS)
    #include <Cocoa/Cocoa.h>
    #include <QuartzCore/CAMetalLayer.h>
#endif

// fix the sdl macro conflict
#ifdef Success
    #undef Success
#endif

namespace tvg::toolkit
{

static bool CHECK(Result result)
{
    assert(result == Result::Success);
    return result == Result::Success;
}

/************************************************************************/
/* Window                                                               */
/************************************************************************/

Window::Window(App* app, const App::Size size) : app(app), size(size), initialized(true)
{
    SDL_SetMainReady();

#if defined(SDL_PLATFORM_LINUX) || defined(SDL_PLATFORM_FREEBSD)
    // Under X11 or XWayland, bypassing the window compositor (SDL default)
    // could cause side effects on other applications.
    // https://github.com/yshui/picom/issues/998 and
    // https://github.com/libsdl-org/SDL/issues/3802
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0");
#endif
    SDL_Init(SDL_INIT_VIDEO);
}

Window::~Window()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool Window::draw()
{
    if (CHECK(canvas->draw(app->clear))) {
        return CHECK(canvas->sync());
    }

    return false;
}

bool Window::ready()
{
    if (!canvas) return false;

    if (!app->content(canvas, size)) return false;

    // initiate the first rendering before window pop-up.
    if (!CHECK(canvas->draw())) return false;
    if (!CHECK(canvas->sync())) return false;

    return true;
}

void Window::show()
{
    SDL_ShowWindow(window);
    refresh();

    // Mainloop
    SDL_Event event;
    running = true;

    auto ptime = SDL_GetTicks();

    while (running) {
        // SDL Event handling
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
                case SDL_EVENT_KEY_DOWN: {
                    needDraw |= app->keydown(canvas, static_cast<Key>(event.key.key));
                    break;
                }
                case SDL_EVENT_KEY_UP: {
                    needDraw |= app->keyup(canvas, static_cast<Key>(event.key.key));
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                    needDraw |= app->clickdown(canvas, event.button.x, event.button.y);
                    break;
                }
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    needDraw |= app->clickup(canvas, event.button.x, event.button.y);
                    break;
                }
                case SDL_EVENT_MOUSE_MOTION: {
                    needDraw |= app->motion(canvas, event.button.x, event.button.y);
                    break;
                }
                case SDL_EVENT_WINDOW_RESIZED: {
                    size = {(uint32_t)event.window.data1, (uint32_t)event.window.data2};
                    needResize = true;
                    needDraw = true;
                }
            }
        }

        if (needResize) {
            resize();
            needResize = false;
        }

        if (frameNo > 0) {
            needDraw |= app->update(canvas, elapsed);
        }

        if (needDraw) {
            if (draw()) refresh();
            needDraw = false;
        }

        auto ctime = SDL_GetTicks();
        elapsed += (ctime - ptime);
        ptime = ctime;
        ++frameNo;
    }
}


/************************************************************************/
/* SwCanvas                                                             */
/************************************************************************/

SwWindow::SwWindow(App* app, const App::Size& size) : Window(app, size)
{
    if (!initialized) return;

    window = SDL_CreateWindow(app->name.c_str(), size.w, size.h, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);

    canvas = tvg::SwCanvas::gen();
    if (!canvas) {
        std::cout << "SwCanvas is not supported. Did you enable the SwEngine?" << std::endl;
        return;
    }

    resize();
}

SwWindow::~SwWindow()
{
    delete (app);
    delete (canvas);
}

void SwWindow::resize()
{
    auto surface = SDL_GetWindowSurface(window);
    if (!surface) return;

    CHECK(static_cast<tvg::SwCanvas*>(canvas)->target((uint32_t*)surface->pixels, surface->pitch / 4, surface->w, surface->h, tvg::ColorSpace::ARGB8888));
}

void SwWindow::refresh()
{
    SDL_UpdateWindowSurface(window);
}

/************************************************************************/
/* GlCanvas                                                             */
/************************************************************************/

GlWindow::GlWindow(App* app, const App::Size& size) : Window(app, size)
{
    if (!initialized) return;

#ifdef TVG_GLES_SUPPORTED
    SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    window = SDL_CreateWindow(app->name.c_str(), size.w, size.h, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    context = SDL_GL_CreateContext(window);

    SDL_GL_SetSwapInterval(0);  // disable fps limit

    // create a Canvas
    canvas = tvg::GlCanvas::gen();
    if (!canvas) {
        std::cout << "GlCanvas is not supported. Did you enable the GlEngine?" << std::endl;
        return;
    }

    resize();
}

GlWindow::~GlWindow()
{
    delete (app);
    delete (canvas);

    SDL_GL_DestroyContext(context);
}

void GlWindow::resize()
{
    // set the canvas target and draw on it.
    // TODO: When using SDL3, EGLDisplay and EGLSurface may need to be passed as arguments to target().
    CHECK(static_cast<tvg::GlCanvas*>(canvas)->target(nullptr, nullptr, context, 0, size.w, size.h, tvg::ColorSpace::ABGR8888S));
}

void GlWindow::refresh()
{
    SDL_GL_SwapWindow(window);
}

/************************************************************************/
/* WgCanvas                                                             */
/************************************************************************/

#ifdef TVG_WGPU_SUPPORTED

WgWindow::WgWindow(App* app, const App::Size& size) : Window(app, size)
{
    if (!initialized) return;

    window = SDL_CreateWindow(app->name.c_str(), size.w, size.h, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);

    // init WebGPU
    WGPUInstanceDescriptor desc{};
    instance = wgpuCreateInstance(&desc);

    // windowWMInfo.subsystem tells which member of the windowWMInfo.info union is valid.
    union {
        WGPUSurfaceSourceMetalLayer cocoa;
        WGPUSurfaceSourceXlibWindow x11;
        WGPUSurfaceSourceWaylandSurface wl;
        WGPUSurfaceSourceWindowsHWND win;
    } surfaceNativeDesc{};

#if defined(SDL_PLATFORM_MACOS)
    auto nswindow = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    if (nswindow) {
            [windowWMInfo.info.cocoa.window.contentView setWantsLayer:YES];
            auto layer = [CAMetalLayer layer];
            [windowWMInfo.info.cocoa.window.contentView setLayer:layer];

            surfaceNativeDesc.cocoa = {
                .chain = {nullptr, WGPUSType_SurfaceSourceMetalLayer},
                .layer = layer};
    }
#elif defined(SDL_PLATFORM_LINUX) || defined(SDL_PLATFORM_FREEBSD)
        if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
            auto xdisplay = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
            auto xwindow = SDL_GetNumberProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
            if (xdisplay && xwindow) {
                surfaceNativeDesc.x11 = {
                    .chain = {nullptr, WGPUSType_SurfaceSourceXlibWindow},
                    .display = xdisplay,
                    .window = (uint64_t) xwindow};
            }
        } else if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
            auto display = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, NULL);
            auto surface = SDL_GetPointerProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, NULL);
            if (display && surface) {
                surfaceNativeDesc.wl = {
                    .chain = {nullptr, WGPUSType_SurfaceSourceWaylandSurface},
                    .display = display,
                    .surface = surface};
            }
        } else {
            std::cout << "Unknown linux platform!" << std::endl;
            std::exit(1);
        }
#elif defined(SDL_PLATFORM_WIN32)
        auto hwnd = SDL_GetProperty(SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
        if (hwnd) {
            surfaceNativeDesc.win = {
                .chain = {nullptr, WGPUSType_SurfaceSourceWindowsHWND},
                .hinstance = GetModuleHandle(nullptr),
                .hwnd = hwnd};
        }
#endif

    // create surface
    WGPUSurfaceDescriptor surfaceDesc{};
    surfaceDesc.nextInChain = (WGPUChainedStruct*)&surfaceNativeDesc;
    surfaceDesc.label.data = "The surface";
    surfaceDesc.label.length = WGPU_STRLEN;
    surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);

    // request adapter
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
        *((WGPUAdapter*)userdata1) = adapter;
    };
    const WGPURequestAdapterOptions requestAdapterOptions{.featureLevel = WGPUFeatureLevel_Compatibility, .powerPreference = WGPUPowerPreference_HighPerformance, .compatibleSurface = surface};
    const WGPURequestAdapterCallbackInfo requestAdapterCallback{.mode = WGPUCallbackMode_WaitAnyOnly, .callback = onAdapterRequestEnded, .userdata1 = &adapter};
    wgpuInstanceRequestAdapter(instance, &requestAdapterOptions, requestAdapterCallback);

    // request device
    auto onDeviceError = [](WGPUDevice const* device, WGPUErrorType type, WGPUStringView message, void* userdata1, void* userdata2) {
        std::cout << message.data << std::endl;
    };
    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata1, void* userdata2) {
        *((WGPUDevice*)userdata1) = device;
    };
    const WGPUDeviceDescriptor deviceDesc{.label = {"The device", WGPU_STRLEN}, .uncapturedErrorCallbackInfo = {.callback = onDeviceError}};
    const WGPURequestDeviceCallbackInfo requestDeviceCallback{.callback = onDeviceRequestEnded, .userdata1 = &device};
    wgpuAdapterRequestDevice(this->adapter, &deviceDesc, requestDeviceCallback);

    // create a Canvas
    canvas = tvg::WgCanvas::gen();
    if (!canvas) {
        std::cout << "WgCanvas is not supported. Did you enable the WgEngine?" << std::endl;
        return;
    }

    resize();
}

WgWindow::~WgWindow()
{
    delete (app);
    delete (canvas);

    wgpuDeviceRelease(device);
    wgpuAdapterRelease(adapter);
    wgpuSurfaceRelease(surface);
    wgpuInstanceRelease(instance);
}

void WgWindow::resize()
{
    // set the canvas target and draw on it.
    CHECK(static_cast<tvg::WgCanvas*>(canvas)->target({instance, adapter, device}, surface, size.w, size.h, tvg::ColorSpace::ABGR8888));
}

void WgWindow::refresh()
{
    wgpuSurfacePresent(surface);
}

#endif

}  // namespace tvg::toolkit
