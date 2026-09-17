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
#include <SDL2/SDL_syswm.h>
#if defined(SDL_VIDEO_DRIVER_COCOA)
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
                case SDL_QUIT: {
                    running = false;
                    break;
                }
                case SDL_KEYDOWN: {
                    needDraw |= app->keydown(canvas, static_cast<Key>(event.key.keysym.sym));
                    break;
                }
                case SDL_KEYUP: {
                    needDraw |= app->keyup(canvas, static_cast<Key>(event.key.keysym.sym));
                    break;
                }
                case SDL_MOUSEBUTTONDOWN: {
                    needDraw |= app->clickdown(canvas, event.button.x, event.button.y);
                    break;
                }
                case SDL_MOUSEBUTTONUP: {
                    needDraw |= app->clickup(canvas, event.button.x, event.button.y);
                    break;
                }
                case SDL_MOUSEMOTION: {
                    needDraw |= app->motion(canvas, event.button.x, event.button.y);
                    break;
                }
                case SDL_WINDOWEVENT: {
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                        size = {(uint32_t)event.window.data1, (uint32_t)event.window.data2};
                        needResize = true;
                        needDraw = true;
                    }
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

    window = SDL_CreateWindow(app->name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size.w, size.h, SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);

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
    window = SDL_CreateWindow(app->name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size.w, size.h, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
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

    SDL_GL_DeleteContext(context);
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

    window = SDL_CreateWindow(app->name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size.w, size.h, SDL_WINDOW_HIDDEN);

    // here we create our WebGPU surface from the window!
    SDL_SysWMinfo windowWMInfo;
    SDL_VERSION(&windowWMInfo.version);
    if (!SDL_GetWindowWMInfo(window, &windowWMInfo)) return;

    // init WebGPU
    WGPUInstanceDescriptor desc{};
    instance = wgpuCreateInstance(&desc);
    if (!instance) return;

    // windowWMInfo.subsystem tells which member of the windowWMInfo.info union is valid.
    union {
        WGPUSurfaceSourceMetalLayer cocoa;
        WGPUSurfaceSourceXlibWindow x11;
        WGPUSurfaceSourceWaylandSurface wl;
        WGPUSurfaceSourceWindowsHWND win;
    } surfaceNativeDesc{};

    switch (windowWMInfo.subsystem) {
#if defined(SDL_VIDEO_DRIVER_COCOA)
        case SDL_SYSWM_COCOA: {
            [windowWMInfo.info.cocoa.window.contentView setWantsLayer:YES];
            auto layer = [CAMetalLayer layer];
            [windowWMInfo.info.cocoa.window.contentView setLayer:layer];

            surfaceNativeDesc.cocoa = {
                .chain = {nullptr, WGPUSType_SurfaceSourceMetalLayer},
                .layer = layer};
            break;
        }
#endif
#if defined(SDL_VIDEO_DRIVER_X11)
        case SDL_SYSWM_X11:
            surfaceNativeDesc.x11 = {
                .chain = {nullptr, WGPUSType_SurfaceSourceXlibWindow},
                .display = windowWMInfo.info.x11.display,
                .window = windowWMInfo.info.x11.window};
            break;
#endif
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
        case SDL_SYSWM_WAYLAND:
            surfaceNativeDesc.wl = {
                .chain = {nullptr, WGPUSType_SurfaceSourceWaylandSurface},
                .display = windowWMInfo.info.wl.display,
                .surface = windowWMInfo.info.wl.surface};
            break;
#endif
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
        case SDL_SYSWM_WINDOWS:
            surfaceNativeDesc.win = {
                .chain = {nullptr, WGPUSType_SurfaceSourceWindowsHWND},
                .hinstance = GetModuleHandle(nullptr),
                .hwnd = windowWMInfo.info.win.window};
            break;
#endif
        default: return;
    }

    // create surface
    WGPUSurfaceDescriptor surfaceDesc{};
    surfaceDesc.nextInChain = (WGPUChainedStruct*)&surfaceNativeDesc;
    surfaceDesc.label.data = "The surface";
    surfaceDesc.label.length = WGPU_STRLEN;
    surface = wgpuInstanceCreateSurface(instance, &surfaceDesc);
    if (!surface) return;

    // request adapter
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, WGPU_NULLABLE void* userdata1, WGPU_NULLABLE void* userdata2) {
        *((WGPUAdapter*)userdata1) = adapter;
    };
    const WGPURequestAdapterOptions requestAdapterOptions{.featureLevel = WGPUFeatureLevel_Compatibility, .powerPreference = WGPUPowerPreference_HighPerformance, .compatibleSurface = surface};
    const WGPURequestAdapterCallbackInfo requestAdapterCallback{.mode = WGPUCallbackMode_WaitAnyOnly, .callback = onAdapterRequestEnded, .userdata1 = &adapter};
    wgpuInstanceRequestAdapter(instance, &requestAdapterOptions, requestAdapterCallback);
    if (!adapter) return;

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
    if (!device) return;

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

    if (device) wgpuDeviceRelease(device);
    if (adapter) wgpuAdapterRelease(adapter);
    if (surface) wgpuSurfaceRelease(surface);
    if (instance) wgpuInstanceRelease(instance);
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
