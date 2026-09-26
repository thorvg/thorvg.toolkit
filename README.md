[![CodeFactor](https://www.codefactor.io/repository/github/thorvg/thorvg.toolkit/badge)](https://www.codefactor.io/repository/github/thorvg/thorvg.toolkit)
[![License](https://img.shields.io/badge/licence-MIT-green.svg?style=flat)](LICENSE)
[![Wikipedia](https://img.shields.io/badge/Wikipedia-000000?style=flat&logo=wikipedia&logoColor=white)](https://en.wikipedia.org/wiki/Thor_Vector_Graphics)
[![Discord](https://img.shields.io/badge/Community-5865f2?style=flat&logo=discord&logoColor=white)](https://discord.gg/n25xj6J6HM)
[![OpenCollective](https://img.shields.io/badge/OpenCollective-84B5FC?style=flat&logo=opencollective&logoColor=white)](https://opencollective.com/thorvg)

# ThorVG Toolkit

<p align="center">
  <img width="550" height="auto" src="https://github.com/thorvg/thorvg.site/blob/main/readme/logo/animated_brand.svg">
</p>

ThorVG Toolkit is a C++ library for building applications with [ThorVG](https://github.com/thorvg/thorvg). It handles window creation, rendering, and the event loop through [SDL2](https://www.libsdl.org/), so you can focus on drawing content and responding to user input.

The toolkit provides:

- Seamless integration and binding between a native window and a ThorVG canvas.
- An `App` base class with callbacks for canvas content, animation updates, and keyboard and mouse input.
- Essential UI components (planned).

## Contents

- [Build and Install](#build-and-install)
- [Create a Project](#create-a-project)
- [Basic Usage](#basic-usage)
  - [Rendering Engines](#rendering-engines)
  - [Input and Update](#input-and-update)
- [Examples](#examples)
- [Communication](#communication)

## Build and Install

Install the following dependencies before building:

- A C++17 or higher compiler.
- [ThorVG](https://github.com/thorvg/thorvg) 1.0.0 or later.
- [SDL2](https://www.libsdl.org/).
- [Meson](https://mesonbuild.com/), [Ninja](https://ninja-build.org/), and [pkg-config](https://www.freedesktop.org/wiki/Software/pkgconfig/).

From the project root, run:

```sh
meson setup builddir
ninja -C builddir install
```
ThorVG Toolkit installs a pkg-config file `thorvg-toolkit.pc` for integration with build systems such as Meson and CMake.

[Back to contents](#contents)
<br/>
<br/>

## Create a Project

After installing ThorVG Toolkit, generate a minimal C++17 application:

```sh
./tvg-toolkit.sh MyApp
cd MyApp
meson setup builddir
meson compile -C builddir
./builddir/MyApp
```

The script creates a directory in the current working directory containing
`meson.build` and `main.cpp`. The project name is used for the executable and
window title. The template opens an 800 x 600 window and draws a blue rectangle
using the default CPU renderer.

Project names must start with a letter or digit and contain only ASCII letters,
digits, underscores, or hyphens. Existing paths are never overwritten. The script
can also be invoked by its absolute path from another directory.

[Back to contents](#contents)
<br/>
<br/>

## Basic Usage

Derive from `tvg::toolkit::App`, implement `content()`, and pass a new application instance to `tvg::toolkit::run()`:

```cpp
#include <thorvg_toolkit.h>

struct MyApp : tvg::toolkit::App
{
    // Set the window title and initial size (800×600).
    MyApp() : App("ThorVG Toolkit", {800, 600}) {}

    // Create and add your drawing content to the canvas.
    // Returns whether content initialization succeeded.
    bool content(tvg::Canvas* canvas, const App::Size& size) override
    {
        auto shape = tvg::Shape::gen();
        shape->appendRect(0, 0, size.w, size.h);
        shape->fill(30, 120, 240);
        canvas->add(shape);

        return true;
    }
};

int main()
{
    // Initialize the ThorVG engine.
    if (tvg::Initializer::init() != tvg::Result::Success) return 1;
    // Run the application.
    if (tvg::toolkit::run(new MyApp) != tvg::Result::Success) return 1;
    // Shut down the ThorVG engine.
    if (tvg::Initializer::term() != tvg::Result::Success) return 1;
}
```

Initialize ThorVG before creating the application and terminate it after `run()` returns. The current implementation takes ownership of the application and deletes it when `run()` finishes; pass a heap-allocated instance. The event loop runs until the window is closed or Escape is pressed.

### Rendering Engines

`run()` uses the CPU engine by default. Select a GPU engine with its second argument:

```cpp
auto result = tvg::toolkit::run(new MyApp, tvg::toolkit::RenderEngine::GL /* or tvg::toolkit::RenderEngine::WEBGPU */);
```

GPU rendering requires the corresponding ThorVG backend and platform support. The toolkit falls back to CPU rendering if the requested GPU engine is unavailable or fails to initialize.

### Input and Update

Override the `App` optional callbacks to update content or handle interaction:

| Callback | Purpose |
| --- | --- |
| `update(canvas, elapsed)` | Update content using elapsed time in milliseconds since the event loop started. |
| `clickdown(canvas, x, y)` | Handle a mouse button press. |
| `clickup(canvas, x, y)` | Handle a mouse button release. |
| `keydown(canvas, key)` | Handle a key press using a key code. |
| `keyup(canvas, key)` | Handle a key release using a key code. |
| `motion(canvas, x, y)` | Handle mouse movement in window coordinates. |
| `wheel(canvas, x, y)` | Handle horizontal and vertical scrolling in wheel steps; positive values mean right and up. Flipped scrolling is normalized. |

Return `true` from these callbacks when the canvas needs to be redrawn. Their default implementations return `false`.

See [thorvg_toolkit.h](inc/thorvg_toolkit.h) for the public API documentation.

[Back to contents](#contents)
<br/>
<br/>
## Examples
A wide range of native sample codes is available in the [thorvg.example](https://github.com/thorvg/thorvg.example) repository to help you understand and work with the ThorVG Toolkit usage.

[Back to contents](#contents)
<br/>
<br/>

## Communication
For real-time conversations and discussions, please join us on [Discord](https://discord.gg/n25xj6J6HM)
