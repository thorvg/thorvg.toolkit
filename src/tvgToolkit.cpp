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
#include "tvgWindow.h"

namespace tvg::toolkit
{

/************************************************************************/
/* App                                                                  */
/************************************************************************/

struct App::Impl
{
    Impl(const App::Size& size) : size(size) {}
    App::Size size;
    Window* window = nullptr;
    size_t prevFrames = 0;
    size_t prevElapsed = 0;
    uint32_t lastFps = 0;

    uint32_t fps() noexcept
    {
        if (!window) return 0;

        auto dt = window->elapsed - prevElapsed;
        if (dt == 0) return lastFps;

        lastFps = static_cast<uint32_t>((window->frameNo - prevFrames) * 1000.0 / dt + 0.5);
        prevFrames = window->frameNo;
        prevElapsed = window->elapsed;
        return lastFps;
    }
};

App::App(const std::string& name, const Size& size, bool clear) :
    name(name), clear(clear), pImpl(new App::Impl(size)) {}

const App::Size& App::size() noexcept
{
    return pImpl->size;
}

uint32_t App::fps() noexcept
{
    return pImpl->fps();
}

Result App::quit() noexcept
{
    if (!pImpl->window) return Result::InsufficientCondition;
    pImpl->window->running = false;
    return Result::Success;
}

/************************************************************************/
/* Toolkit                                                              */
/************************************************************************/

Result run(App* app, RenderEngine engine) noexcept
{
    Window* window;

    switch (engine) {
        case RenderEngine::GL:
            window = new GlWindow(app, app->size());
            break;
        case RenderEngine::WEBGPU:
            window = new WgWindow(app, app->size());
            break;
        default:
            window = new SwWindow(app, app->size());
            break;
    }

    auto ready = window->ready();

    // fallback to cpu engine
    if (!ready && engine != RenderEngine::CPU) {
        window->app = nullptr;
        delete (window);
        window = new SwWindow(app, app->size());
        ready = window->ready();
    }

    // succeed
    if (ready) {
        app->pImpl->window = window;
        window->show();
        delete (window);
        return Result::Success;
    }

    delete (window);

    return Result::InsufficientCondition;
}

float progress(size_t elapsed, float duration, bool rewind) noexcept
{
    auto durInMS = size_t(duration * 1000.0f);
    if (durInMS == 0) return 0.0f;
    auto value = float(elapsed % durInMS) / float(durInMS);
    if (rewind && (elapsed / durInMS) % 2 != 0) return 1.0f - value;
    return value;
}

}  // namespace tvg::toolkit
