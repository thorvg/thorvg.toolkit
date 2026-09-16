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

#ifndef _TVG_WINDOW_H_
#define _TVG_WINDOW_H_

#include "thorvg_toolkit.h"
#include <iostream>
#include <SDL3/SDL.h>

#ifdef TVG_WGPU_SUPPORTED
    #include <webgpu/webgpu.h>
#endif

namespace tvg::toolkit
{

struct Window
{
    SDL_Window* window = nullptr;
    App* app = nullptr;
    Canvas* canvas = nullptr;
    App::Size size;
    size_t elapsed = 0;
    size_t frameNo = 0;

    bool needResize = false;
    bool needDraw = false;
    bool initialized = false;
    bool running = false;

    Window(App* app, const App::Size size);
    virtual ~Window();
    virtual void resize() {}
    virtual void refresh() {}

    bool draw();
    bool ready();
    void show();
};

struct SwWindow : Window
{
    SwWindow(App* app, const App::Size& size);
    ~SwWindow();
    void resize() override;
    void refresh() override;
};

struct GlWindow : Window
{
    SDL_GLContext context;

    GlWindow(App* app, const App::Size& size);
    ~GlWindow();
    void resize() override;
    void refresh() override;
};

#ifdef TVG_WGPU_SUPPORTED
    struct WgWindow : Window
    {
        WGPUInstance instance;
        WGPUSurface surface;
        WGPUAdapter adapter;
        WGPUDevice device;

        WgWindow(App* app, const App::Size& size);
        ~WgWindow();
        void resize() override;
        void refresh() override;
    };
#else
    struct WgWindow : Window
    {
        WgWindow(App* app, const App::Size& size) : Window(app, size)
        {
            std::cout << "webgpu driver is not detected!" << std::endl;
        }
    };
#endif

}  // namespace tvg::toolkit

#endif  //_TVG_WINDOW_H_