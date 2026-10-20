#include "doctest/doctest.h"

#include "gameos.hpp"

#include <SDL2/SDL.h>
#include "gos_input.h"

#include <string.h>

// defined in GameOS/gameos/gameos_input.cpp
gosEnum_KeyIndex remap_sdl_to_gos(const SDL_Scancode scancode);
SDL_Scancode remap_gos_to_sdl(const gosEnum_KeyIndex key_index);

namespace {

SDL_Event key_event(Uint32 type, SDL_Scancode sc)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.key.type = type;
    e.key.state = type == SDL_KEYDOWN ? SDL_PRESSED : SDL_RELEASED;
    e.key.keysym.scancode = sc;
    e.key.keysym.sym = SDL_GetKeyFromScancode(sc);
    return e;
}

SDL_Event mouse_button_event(Uint32 type, Uint8 button)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.button.type = type;
    e.button.button = button;
    e.button.state = type == SDL_MOUSEBUTTONDOWN ? SDL_PRESSED : SDL_RELEASED;
    return e;
}

// no real keyboard/mouse is attached, so SDL reports everything as up and only the events count
void update_input()
{
    input::updateMouseState();
    input::updateKeyboardState();
}

} // namespace

TEST_CASE("Input: SDL scancodes map to gos keys")
{
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_A) == KEY_A);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_0) == KEY_0);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_RETURN) == KEY_RETURN);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_SPACE) == KEY_SPACE);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_TAB) == KEY_TAB);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_F1) == KEY_F1);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_F12) == KEY_F12);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_UP) == KEY_UP);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_HOME) == KEY_HOME);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_KP_5) == KEY_NUMPAD5);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_RCTRL) == KEY_RCONTROL);
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_LALT) == KEY_LALT);
    // keys gos doesn't know must not turn into Escape
    CHECK(remap_sdl_to_gos(SDL_SCANCODE_MUTE) == (gosEnum_KeyIndex)0);
}

TEST_CASE("Input: gos keys map back to SDL scancodes")
{
    const SDL_Scancode scancodes[] = { SDL_SCANCODE_A, SDL_SCANCODE_7, SDL_SCANCODE_ESCAPE, SDL_SCANCODE_RETURN,
        SDL_SCANCODE_SPACE, SDL_SCANCODE_F5, SDL_SCANCODE_KP_0, SDL_SCANCODE_LEFT, SDL_SCANCODE_DELETE,
        SDL_SCANCODE_LCTRL, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_LALT, SDL_SCANCODE_RALT };
    for (SDL_Scancode sc : scancodes) {
        CAPTURE(sc);
        CHECK(remap_gos_to_sdl(remap_sdl_to_gos(sc)) == sc);
    }
    // KEY_LMENU is the Windows virtual key for Alt, not the context menu key
    CHECK(remap_gos_to_sdl(KEY_LMENU) == SDL_SCANCODE_LALT);
}

TEST_CASE("Input: a key tapped between two updates is reported as pressed, then released")
{
    update_input();
    REQUIRE(gos_GetKeyStatus(KEY_SPACE) == KEY_FREE);

    SDL_Event down = key_event(SDL_KEYDOWN, SDL_SCANCODE_SPACE);
    SDL_Event up = key_event(SDL_KEYUP, SDL_SCANCODE_SPACE);
    input::handleKeyEvent(&down);
    input::handleKeyEvent(&up);

    update_input();
    CHECK(gos_GetKeyStatus(KEY_SPACE) == KEY_PRESSED);
    update_input();
    CHECK(gos_GetKeyStatus(KEY_SPACE) == KEY_RELEASED);
    update_input();
    CHECK(gos_GetKeyStatus(KEY_SPACE) == KEY_FREE);
}

TEST_CASE("Input: right Ctrl/Shift/Alt count for the combined modifier keys")
{
    const struct { SDL_Scancode sc; gosEnum_KeyIndex key; } cases[] = {
        { SDL_SCANCODE_RCTRL, KEY_LCONTROL },
        { SDL_SCANCODE_RSHIFT, KEY_LSHIFT },
        { SDL_SCANCODE_RALT, KEY_LMENU },
        { SDL_SCANCODE_LALT, KEY_LMENU },
    };
    for (const auto& c : cases) {
        CAPTURE(c.sc);
        update_input();
        SDL_Event down = key_event(SDL_KEYDOWN, c.sc);
        input::handleKeyEvent(&down);
        update_input();
        CHECK(gos_GetKeyStatus(c.key) == KEY_PRESSED);
        update_input();
        update_input();
        CHECK(gos_GetKeyStatus(c.key) == KEY_FREE);
    }
}

TEST_CASE("Input: a mouse click between two updates is not lost")
{
    update_input();
    REQUIRE(gos_GetKeyStatus(KEY_LMOUSE) == KEY_FREE);

    SDL_Event down = mouse_button_event(SDL_MOUSEBUTTONDOWN, SDL_BUTTON_LEFT);
    SDL_Event up = mouse_button_event(SDL_MOUSEBUTTONUP, SDL_BUTTON_LEFT);
    input::handleMouseButton(&down);
    input::handleMouseButton(&up);

    update_input();
    CHECK(gos_GetKeyStatus(KEY_LMOUSE) == KEY_PRESSED);
    DWORD buttons = 0;
    gos_GetMouseInfo(0, 0, 0, 0, 0, &buttons);
    CHECK((buttons & 1) == 1);

    update_input();
    CHECK(gos_GetKeyStatus(KEY_LMOUSE) == KEY_RELEASED);
    update_input();
    CHECK(gos_GetKeyStatus(KEY_LMOUSE) == KEY_FREE);
}

TEST_CASE("Input: mouse motion and wheel accumulate over all events of a frame")
{
    Environment.drawableWidth = 800;
    Environment.drawableHeight = 600;

    input::beginUpdateMouseState();
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEMOTION;
    e.motion.x = 100; e.motion.y = 50; e.motion.xrel = 3; e.motion.yrel = -2;
    input::handleMouseMotion(&e);
    e.motion.x = 104; e.motion.y = 49; e.motion.xrel = 4; e.motion.yrel = -1;
    input::handleMouseMotion(&e);

    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEWHEEL;
    e.wheel.y = 1;
    input::handleMouseWheel(&e);
    input::handleMouseWheel(&e);

    int dx = 0, dy = 0, wheel = 0;
    float x = 0.0f, y = 0.0f;
    gos_GetMouseInfo(&x, &y, &dx, &dy, &wheel, 0);
    CHECK(dx == 7);
    CHECK(dy == -3);
    CHECK(wheel == 2);
    CHECK(x == doctest::Approx(104.0f / 800.0f));
    CHECK(y == doctest::Approx(49.0f / 600.0f));

    input::beginUpdateMouseState();
    gos_GetMouseInfo(0, 0, &dx, &dy, &wheel, 0);
    CHECK(dx == 0);
    CHECK(wheel == 0);
}
