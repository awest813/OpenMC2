#include "gameos.hpp"

#include <SDL2/SDL.h> // can avoid inluding this here if move MouseInfo struct to separate file
#include "gos_input.h"
#include "utils/vec.h"

#include<string.h>

//
// The mouse position and buttons are read once a frame.
//
// Position returns values converted into position on window. Range is 0.0 to 1.0  (0 to 639 on a 640 wide screen)
// When running in a window this may not match the sum of the deltas, as the windows mouse cursor can have acceleration
//
//
// Deltas returns values as a percentage of screen size. Range is -1.0 to 1.0
//
// ie: 0.5 means half a screen to the right or down
//
// The wheel position is only available as a delta value, Range is -1.0 to 1.0 (999.0 means no wheel)
//
//
// ButtonsPressed are bitflags, 0=Left Button, 2=Right, 1=Middle, 3=X1, 4=X2 (X1 and X2 are on 5 button mice)
//
// If you want to make sure you do not miss mouse clicks, check for the mouse buttons with
//
// You may pass zero as any parameter you do not wish to query
//
////////////////////////////////////////////////////////////////////////////////
void __stdcall gos_GetMouseInfo( float* pXPosition, float* pYPosition, int* pXDelta, int* pYDelta, int* pWheelDelta, DWORD* pButtonsPressed )
{
    //const float w = (float)Environment.screenWidth;
    //const float h = (float)Environment.screenHeight;

    const float w = (float)Environment.drawableWidth;
    const float h = (float)Environment.drawableHeight;

    const input::MouseInfo* mi = input::getMouseInfo();

    if(pXPosition)
        *pXPosition = mi->x_ / w;
    if(pYPosition)
        *pYPosition = mi->y_ / h;

	// as percentage of screen size
    
    // sebi: I do not like this percentage style and it does not work good, so
    /*
    if(pXDelta)
        *pXDelta = (int)(100.0f * clamp(mi->rel_x_ / w, -1.0f, 1.0f));
    if(pYDelta)
        *pYDelta = (int)(100.0f * clamp(mi->rel_y_ / h, -1.0f, 1.0f));
        */
    if(pXDelta)
        *pXDelta = (int)(clamp(mi->rel_x_, -w, w));
    if(pYDelta)
        *pYDelta = (int)(clamp(mi->rel_y_, -h, h));

    //if(pWheelDelta)
    //  *pWheelDelta = (int)(100.0f * (mi->wheel_vert_ / h));
    if(pWheelDelta)
        *pWheelDelta = (int)(mi->wheel_vert_);

    if(pButtonsPressed) {
        DWORD bs = 0;
        for(int i=0; i<input::MouseInfo::NUM_BUTTONS;++i)
        {
            bool down = mi->button_state_[i] == input::KS_PRESSED;
            down |= mi->button_state_[i] == input::KS_HELD;
            bs |= down ? (1<<i) : 0;
        }
        *pButtonsPressed = bs;
    }
}

////////////////////////////////////////////////////////////////////////////////
extern SDL_Window* g_sdl_window;
void __stdcall gos_SetMousePosition( float XPosition, float YPosition )
{
    if(g_sdl_window) {
        SDL_WarpMouseInWindow(g_sdl_window, (int)XPosition, (int)YPosition);
    }
}

// loks like KEY_* enum contains everything, from scancodes to localized keys, e.g. it has GRAVE and APOSTROPHE but those are usually on the same physical button and valuse depends on localization
// let's keep it like this for now and see later
//
// One table for both directions, so they can't get out of sync. Digits and letters are handled
// separately. Some KEY_* values are shared (KEY_LCONTROL == KEY_RCONTROL, KEY_LSHIFT == KEY_RSHIFT,
// KEY_LMENU == KEY_RMENU (Alt), KEY_RETURN == KEY_NUMPADENTER, KEY_PAUSE == KEY_STOP): lookups take
// the first matching entry.
struct KeyMapping {
    gosEnum_KeyIndex key;
    SDL_Scancode scancode;
};

static const KeyMapping g_key_mappings[] = {
    { KEY_ESCAPE, SDL_SCANCODE_ESCAPE },
    { KEY_MINUS, SDL_SCANCODE_MINUS },
    { KEY_EQUALS, SDL_SCANCODE_EQUALS },
    { KEY_BACK, SDL_SCANCODE_BACKSPACE },
    { KEY_TAB, SDL_SCANCODE_TAB },
    { KEY_LBRACKET, SDL_SCANCODE_LEFTBRACKET },
    { KEY_RBRACKET, SDL_SCANCODE_RIGHTBRACKET },
    { KEY_SEMICOLON, SDL_SCANCODE_SEMICOLON },
    { KEY_GRAVE, SDL_SCANCODE_GRAVE },
    { KEY_BACKSLASH, SDL_SCANCODE_BACKSLASH },
    { KEY_COMMA, SDL_SCANCODE_COMMA },
    { KEY_PERIOD, SDL_SCANCODE_PERIOD },
    { KEY_SLASH, SDL_SCANCODE_SLASH },
    { KEY_MULTIPLY, SDL_SCANCODE_KP_MULTIPLY },
    { KEY_SPACE, SDL_SCANCODE_SPACE },
    { KEY_CAPITAL, SDL_SCANCODE_CAPSLOCK },
    { KEY_F1, SDL_SCANCODE_F1 },
    { KEY_F2, SDL_SCANCODE_F2 },
    { KEY_F3, SDL_SCANCODE_F3 },
    { KEY_F4, SDL_SCANCODE_F4 },
    { KEY_F5, SDL_SCANCODE_F5 },
    { KEY_F6, SDL_SCANCODE_F6 },
    { KEY_F7, SDL_SCANCODE_F7 },
    { KEY_F8, SDL_SCANCODE_F8 },
    { KEY_F9, SDL_SCANCODE_F9 },
    { KEY_F10, SDL_SCANCODE_F10 },
    { KEY_F11, SDL_SCANCODE_F11 },
    { KEY_F12, SDL_SCANCODE_F12 },
    { KEY_F13, SDL_SCANCODE_F13 },
    { KEY_F14, SDL_SCANCODE_F14 },
    { KEY_F15, SDL_SCANCODE_F15 },
    { KEY_NUMPAD0, SDL_SCANCODE_KP_0 },
    { KEY_NUMPAD1, SDL_SCANCODE_KP_1 },
    { KEY_NUMPAD2, SDL_SCANCODE_KP_2 },
    { KEY_NUMPAD3, SDL_SCANCODE_KP_3 },
    { KEY_NUMPAD4, SDL_SCANCODE_KP_4 },
    { KEY_NUMPAD5, SDL_SCANCODE_KP_5 },
    { KEY_NUMPAD6, SDL_SCANCODE_KP_6 },
    { KEY_NUMPAD7, SDL_SCANCODE_KP_7 },
    { KEY_NUMPAD8, SDL_SCANCODE_KP_8 },
    { KEY_NUMPAD9, SDL_SCANCODE_KP_9 },
    { KEY_SUBTRACT, SDL_SCANCODE_KP_MINUS },
    { KEY_ADD, SDL_SCANCODE_KP_PLUS },
    { KEY_DECIMAL, SDL_SCANCODE_KP_PERIOD },
    { KEY_KANA, SDL_SCANCODE_LANG3 },
    { KEY_NUMPADEQUALS, SDL_SCANCODE_KP_EQUALS },
    { KEY_KANJI, SDL_SCANCODE_LANG2 }, // is it correct?
    { KEY_RETURN, SDL_SCANCODE_RETURN },
    { KEY_NUMPADENTER, SDL_SCANCODE_KP_ENTER },
    { KEY_NUMPADCOMMA, SDL_SCANCODE_KP_COMMA },
    { KEY_DIVIDE, SDL_SCANCODE_KP_DIVIDE },
    { KEY_SYSRQ, SDL_SCANCODE_SYSREQ },
    { KEY_HOME, SDL_SCANCODE_HOME },
    { KEY_UP, SDL_SCANCODE_UP },
    { KEY_PRIOR, SDL_SCANCODE_PAGEUP },
    { KEY_LEFT, SDL_SCANCODE_LEFT },
    { KEY_RIGHT, SDL_SCANCODE_RIGHT },
    { KEY_END, SDL_SCANCODE_END },
    { KEY_DOWN, SDL_SCANCODE_DOWN },
    { KEY_NEXT, SDL_SCANCODE_PAGEDOWN },
    { KEY_INSERT, SDL_SCANCODE_INSERT },
    { KEY_DELETE, SDL_SCANCODE_DELETE },
    { KEY_LWIN, SDL_SCANCODE_LGUI },
    { KEY_RWIN, SDL_SCANCODE_RGUI },
    { KEY_APPS, SDL_SCANCODE_APPLICATION },
    { KEY_NUMLOCK, SDL_SCANCODE_NUMLOCKCLEAR },
    { KEY_PAUSE, SDL_SCANCODE_PAUSE },
    // gos does not differentiate left and right control/shift
    { KEY_LCONTROL, SDL_SCANCODE_LCTRL },
    { KEY_RCONTROL, SDL_SCANCODE_RCTRL },
    { KEY_LSHIFT, SDL_SCANCODE_LSHIFT },
    { KEY_RSHIFT, SDL_SCANCODE_RSHIFT },
    // Alt keys are reported as KEY_LALT/KEY_RALT, KEY_LMENU (both Alt keys) is accepted as well
    { KEY_LALT, SDL_SCANCODE_LALT },
    { KEY_RALT, SDL_SCANCODE_RALT },
    { KEY_LMENU, SDL_SCANCODE_LALT },
};

// returns 0 (no key) for scancodes gos has no key for
gosEnum_KeyIndex remap_sdl_to_gos(const SDL_Scancode scancode) {

    if(scancode >= SDL_SCANCODE_1 && scancode <= SDL_SCANCODE_9)
        return (gosEnum_KeyIndex)(scancode - SDL_SCANCODE_1 + KEY_1);
    if(scancode == SDL_SCANCODE_0) return KEY_0;

    if(scancode >= SDL_SCANCODE_A && scancode <= SDL_SCANCODE_Z)
        return (gosEnum_KeyIndex)(scancode - SDL_SCANCODE_A + KEY_A);

    for(const KeyMapping& m : g_key_mappings) {
        if(m.scancode == scancode)
            return m.key;
    }

    return (gosEnum_KeyIndex)0;
}

SDL_Scancode remap_gos_to_sdl(const gosEnum_KeyIndex key_index) {

    if(key_index >= KEY_1 && key_index <= KEY_9)
        return (SDL_Scancode)(key_index - KEY_1 + SDL_SCANCODE_1);
    if(key_index == KEY_0) return SDL_SCANCODE_0;

    if(key_index >= KEY_A && key_index <= KEY_Z)
        return (SDL_Scancode)(key_index - KEY_A + SDL_SCANCODE_A);

    for(const KeyMapping& m : g_key_mappings) {
        if(m.key == key_index)
            return m.scancode;
    }

    return SDL_SCANCODE_UNKNOWN;
}

// the second scancode for KEY_* values that stand for both the left and the right key
static SDL_Scancode right_hand_scancode(const gosEnum_KeyIndex key_index) {
    switch(key_index) {
        case KEY_LCONTROL: return SDL_SCANCODE_RCTRL;
        case KEY_LSHIFT: return SDL_SCANCODE_RSHIFT;
        case KEY_LMENU: return SDL_SCANCODE_RALT;
        default: return SDL_SCANCODE_UNKNOWN;
    }
}

////////////////////////////////////////////////////////////////////////////////
DWORD __stdcall gos_GetKey()
{
    const input::KeyboardInfo* ki = input::getKeyboardInfo();

    if(ki->first_pressed_!=-1) {
        SDL_Scancode sc = (SDL_Scancode)ki->first_pressed_;
        SDL_Keycode kc = SDL_GetKeyFromScancode(sc);
        unsigned char c = 0;
        // check if it represents a character:
        if(!(kc & SDLK_SCANCODE_MASK) && isascii(kc)) {
            c = (unsigned char)kc;

            // this is all hardcodede crap, mayeb will fix it later

			// FIXME: it is incorrect to do so because caps lock is not held constantly but either turned on or off
            //const bool caps_pressed = !!ki->last_state_[SDL_SCANCODE_CAPSLOCK];

            const bool shift_pressed = !!(ki->last_state_[SDL_SCANCODE_LSHIFT] || ki->last_state_[SDL_SCANCODE_RSHIFT]);
            const bool register_mod = /*caps_pressed ^ */shift_pressed; 
            if(c >= SDLK_a && c<=SDLK_z && register_mod)
            {
                c-=32;
            }
            else if(register_mod)
            {
                switch(c)
                {
                    case SDLK_0:        c = SDLK_RIGHTPAREN; break;
                    case SDLK_1:        c = SDLK_EXCLAIM; break;
                    case SDLK_2:        c = SDLK_AT; break;
                    case SDLK_3:        c = SDLK_HASH; break;
                    case SDLK_4:        c = SDLK_DOLLAR; break;
                    case SDLK_5:        c = 37; break; // %
                    case SDLK_6:        c = SDLK_COLON; break;
                    case SDLK_7:        c = SDLK_AMPERSAND; break;
                    case SDLK_8:        c = SDLK_ASTERISK; break;
                    case SDLK_9:        c = SDLK_LEFTPAREN; break;
                    case SDLK_EQUALS:   c = SDLK_PLUS; break;
                    case SDLK_MINUS:    c = SDLK_UNDERSCORE; break;
                    case SDLK_COMMA:    c = SDLK_LESS; break;
                    case SDLK_PERIOD:   c = SDLK_GREATER; break;
                    case SDLK_SLASH:    c = SDLK_QUESTION; break;
                    case SDLK_BACKSLASH:c = '|'; break;
                    default: break;
                }
            }
        }

        gosEnum_KeyIndex ki = remap_sdl_to_gos(sc);

        return (ki<<8) | c;
    }

    if(Environment.ButtonsAsKeys) {
        // TODO: check buttons
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
const char* __stdcall gos_DescribeKey( DWORD Key )
{
    const DWORD k = (Key>>8)&0xFF;
    DWORD ascii = Key&0xFF; (void)ascii; // may be 0 if extended key
    bool is_extended = (Key&0x100)!=0; (void)is_extended;

    SDL_Scancode scancode = remap_gos_to_sdl((gosEnum_KeyIndex)k);
    return SDL_GetScancodeName(scancode);
}

////////////////////////////////////////////////////////////////////////////////
void __stdcall gos_KeyboardFlush()
{
    input::resetKeypress();
}

////////////////////////////////////////////////////////////////////////////////
gosEnum_KeyStatus __stdcall gos_GetKeyStatus(gosEnum_KeyIndex index)
{
    const input::MouseInfo* mi = input::getMouseInfo();
    const input::KeyboardInfo* ki = input::getKeyboardInfo();

    if(index >= KEY_LMOUSE && index <= KEY_MOUSEX2) {
        gosASSERT(index > 0 && index <= input::MouseInfo::NUM_BUTTONS);
        return (gosEnum_KeyStatus)mi->button_state_[index-1];
    }

    // if keyboard
    if((index & 0x01000000) == 0) {
        if(index > 255)
        {
            int asfa=0;
            (void)asfa;
        }
        SDL_Scancode sc = remap_gos_to_sdl((gosEnum_KeyIndex)(index & 0xFF));
        gosASSERT(sc>=0 && sc<sizeof(ki->last_state_)/sizeof(ki->last_state_[0]));
        uint8_t status = ki->last_state_[sc];

        // KEY_LCONTROL/KEY_LSHIFT/KEY_LMENU mean either the left or the right key: report the most
        // active of the two (held > pressed > released > free)
        SDL_Scancode sc_right = right_hand_scancode((gosEnum_KeyIndex)(index & 0xFF));
        if(sc_right != SDL_SCANCODE_UNKNOWN) {
            static const int rank[] = { 0 /*FREE*/, 2 /*PRESSED*/, 3 /*HELD*/, 1 /*RELEASED*/ };
            const uint8_t status_right = ki->last_state_[sc_right];
            if(rank[status_right] > rank[status])
                status = status_right;
        }

        return (gosEnum_KeyStatus)status;
    }

    return KEY_FREE;
}

