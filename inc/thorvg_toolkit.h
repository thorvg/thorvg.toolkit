#ifndef _THORVG_TOOLKIT_H_
#define _THORVG_TOOLKIT_H_

#include <string>
#include <thorvg.h>

#define TVG_TOOLKIT_VERSION 0

namespace tvg::toolkit
{

/**
 * @brief Rendering engine used by the application.
 *
 * @note If a GPU engine fails, the application falls back to the CPU engine.
 */
enum struct RenderEngine : uint8_t
{
    CPU = 0, /**< CPU-based software rendering engine. */
    GL,      /**< OpenGL-based GPU rendering engine. */
    WEBGPU   /**< WebGPU-based GPU rendering engine. */
};

/**
 * @brief Keyboard key identifiers.
 *
 * Includes navigation keys, control keys, modifiers, and function keys.
 * Letters and digits use their ASCII values without named identifiers.
 * Values match SDL2 key codes. Keys without a named identifier retain their
 * numeric key code. @c Unknown represents an unknown key.
 */
enum struct Key : int32_t
{
    Unknown = 0,

    Left = (1 << 30) | 80,
    Right = (1 << 30) | 79,
    Up = (1 << 30) | 82,
    Down = (1 << 30) | 81,

    Space = ' ',
    Enter = '\r',
    Escape = 27,
    Tab = '\t',
    Backspace = '\b',
    Insert = (1 << 30) | 73,
    Delete = 127,
    Home = (1 << 30) | 74,
    End = (1 << 30) | 77,
    PageUp = (1 << 30) | 75,
    PageDown = (1 << 30) | 78,

    LeftShift = (1 << 30) | 225,
    RightShift = (1 << 30) | 229,
    LeftControl = (1 << 30) | 224,
    RightControl = (1 << 30) | 228,
    LeftAlt = (1 << 30) | 226,
    RightAlt = (1 << 30) | 230,
    LeftSuper = (1 << 30) | 227,
    RightSuper = (1 << 30) | 231,

    F1 = (1 << 30) | 58, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
};

/**
 * @brief Base class for a ThorVG application with rendering and input callbacks.
 *
 * Derive from this class and implement content() to initialize the canvas.
 * Override the optional update and input callbacks to animate content and handle
 * user interaction, then pass the application to tvg::toolkit::run().
 *
 * @note The caller must initialize ThorVG with tvg::init() before creating the
 * application and call tvg::term() after the application and its resources have
 * been destroyed.
 */
struct TVG_API App
{
    /**
     * @brief Two-dimensional size in pixels.
     */
    struct Size
    {
        uint32_t w;  /**< Width in pixels. */
        uint32_t h;  /**< Height in pixels. */
    };

    /**
     * @brief Creates an application with the specified name and buffer clearing behavior.
     *
     * @param name Application name used as the window title.
     * @param size Initial window width and height in pixels.
     * @param clear Whether to clear the rendering buffer before drawing.
     */
    App(const std::string& name, const Size& size, bool clear = false);

    virtual ~App() = default;

    /**
     * @brief Requests termination of the application's window event loop.
     */
    Result quit();

    /**
     * @brief Populates the canvas with the application's initial content.
     *
     * @param canvas Canvas to populate with drawable content.
     * @param size Canvas size.
     *
     * @return @c true if the content was initialized successfully, @c false otherwise.
     */
    virtual bool content(tvg::Canvas* canvas, const Size& size) = 0;

    /**
     * @brief Updates the application's canvas content.
     *
     * @param canvas Canvas containing the content to update.
     * @param elapsed Elapsed time in milliseconds since the application started.
     *
     * @return @c true if the canvas needs to be redrawn, @c false otherwise.
     *
     * @note The default implementation does nothing and returns @c false.
     */
    virtual bool update(tvg::Canvas* canvas, size_t elapsed) { return false; }

    /**
     * @brief Handles a mouse button press.
     *
     * @param canvas Canvas containing the application's content.
     * @param x Horizontal mouse position in window pixels.
     * @param y Vertical mouse position in window pixels.
     *
     * @return @c true if the canvas needs to be redrawn, @c false otherwise.
     *
     * @note The default implementation does nothing and returns @c false.
     */
    virtual bool clickdown(tvg::Canvas* canvas, int32_t x, int32_t y) { return false; }

    /**
     * @brief Handles a mouse button release.
     *
     * @param canvas Canvas containing the application's content.
     * @param x Horizontal mouse position in window pixels.
     * @param y Vertical mouse position in window pixels.
     *
     * @return @c true if the canvas needs to be redrawn, @c false otherwise.
     *
     * @note The default implementation does nothing and returns @c false.
     */
    virtual bool clickup(tvg::Canvas* canvas, int32_t x, int32_t y) { return false; }

    /**
     * @brief Handles a key press.
     *
     * @param canvas Canvas containing the application's content.
     * @param key Key code of the pressed key.
     *
     * @return @c true if the canvas needs to be redrawn, @c false otherwise.
     *
     * @note The default implementation does nothing and returns @c false.
     */
    virtual bool keydown(tvg::Canvas* canvas, Key key) { return false; }

    /**
     * @brief Handles a key release.
     *
     * @param canvas Canvas containing the application's content.
     * @param key Key code of the released key.
     *
     * @return @c true if the canvas needs to be redrawn, @c false otherwise.
     *
     * @note The default implementation does nothing and returns @c false.
     */
    virtual bool keyup(tvg::Canvas* canvas, Key key) { return false; }

    /**
     * @brief Handles mouse movement.
     *
     * @param canvas Canvas containing the application's content.
     * @param x Horizontal mouse position in window pixels.
     * @param y Vertical mouse position in window pixels.
     *
     * @return @c true if the canvas needs to be redrawn, @c false otherwise.
     *
     * @note The default implementation does nothing and returns @c false.
     */
    virtual bool motion(tvg::Canvas* canvas, int32_t x, int32_t y) { return false; }

    /**
     * @brief Returns the application's current size in pixels.
     *
     * @return Application width and height in pixels.
     */
    const Size& size();

    /**
     * @brief Calculates average main loop FPS since the previous sample.
     *
     * Counts main loop iterations, including those without a redraw.
     * The first sample covers the time since the main loop started.
     *
     * @return FPS rounded to the nearest integer, or 0 before the main loop.
     *
     * @note Calculation occurs only when this method is called.
     */
    uint32_t fps();

    /**
     * @brief Application name used as the window title.
     */
    const std::string name;

    /**
     * @brief Controls whether the rendering buffer is cleared before drawing.
     * @note Defaults to @c false.
     */
    bool clear;

    _TVG_DECLARE_PRIVATE_BASE(App);
};

/**
 * @brief Runs the application with the specified window dimensions and rendering engine.
 *
 * @param app Application to run.
 * @param engine Rendering engine to use. Defaults to RenderEngine::CPU.
 *
 * @return @c Result::Success on success, or an error result on failure.
 */
TVG_API Result run(App* app, RenderEngine engine = RenderEngine::CPU);

/**
 * @brief Calculates normalized animation progress from elapsed time.
 *
 * @param elapsed Elapsed time in milliseconds.
 * @param duration Duration of one animation traversal in seconds.
 * @param rewind If @c true, alternates forward and backward traversals.
 *               Defaults to @c false for repeated forward traversals.
 *
 * @return Progress in the range [0.0, 1.0], or 0.0 if the duration in whole
 *         milliseconds is zero or cannot be represented as a uint32_t.
 *
 * @note Forward playback restarts at 0.0 at each duration boundary.
 *       With rewind enabled, successive boundaries alternate between 1.0 and 0.0.
 */
TVG_API float progress(size_t elapsed, float duration, bool rewind = false);

}  // namespace tvg::toolkit

#endif  //_THORVG_TOOLKIT_H_
