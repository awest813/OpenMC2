#ifndef PLATFORM_WINDOWS

#include<stdio.h>
#include<wctype.h>
#include<ctype.h> // tolower...

#include"platform_windows.h"
#include"platform_str.h"

#define _Inout_

int GetSystemMetrics(int)
{
    // TODO: implememt
    return 0;
}

SHORT GetAsyncKeyState(int vk_key)
{
    // TODO: implememt
    return 0;
}


/*
LPTSTR WINAPI CharLower(
          _Inout_ LPTSTR lpsz
        )
{
    return strlwr(lpsz);
}
*/

// There is no dialog, so answer with the choice that does not wait for anything: Cancel/No when the
// box offers it, OK otherwise. (Returning 0, which is no valid answer, made "insert the CD / retry"
// loops that only stop on IDCANCEL spin forever.)
static int default_message_box_answer(UINT type)
{
    switch(type & MB_TYPEMASK) {
        case MB_OKCANCEL:
        case MB_RETRYCANCEL:
        case MB_YESNOCANCEL:
            return IDCANCEL;
        case MB_YESNO:
            return IDNO;
        case MB_ABORTRETRYIGNORE:
            return IDABORT;
        default:
            return IDOK;
    }
}

int MessageBoxA(HWND,LPCSTR header, LPCSTR message, UINT type)
{
    fprintf(stderr, "MSGBOX: %s : %s\n", header, message);
    return default_message_box_answer(type);
}

int MessageBoxW(HWND,LPCWSTR header, LPCWSTR message, UINT type)
{
    fprintf(stderr, "MSGBOX: %S : %S\n", header, message);
    return default_message_box_answer(type);
}

LPSTR CharLowerA(LPSTR s)
{
    char* p = s;
    while ((*p = tolower( *p ))) p++;
    return s;
}

LPSTR CharUpperA(LPSTR s)
{
    char* p = s;
    while ((*p = toupper( *p ))) p++;
    return s;
}

LPWSTR CharLowerW(LPWSTR s)
{
    wchar_t* p = s;
    while ((*p = towlower( *p ))) p++;
    return s;
}
LPWSTR CharUpperW(LPWSTR s)
{
    wchar_t* p = s;
    while ((*p = towupper( *p ))) p++;
    return s;

}

#endif // PLATFORM_WINDOWS
