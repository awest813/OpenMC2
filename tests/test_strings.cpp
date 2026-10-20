#include "doctest/doctest.h"

#include "gameos.hpp"
#include "estring.h"
#include "echarstring.h"

#include <string.h>

TEST_CASE("EString: concatenation")
{
    EString a("Mech");
    EString b("Commander");

    CHECK(strcmp((const char*)(a + b), "MechCommander") == 0);
    CHECK(strcmp((const char*)(a + " 2"), "Mech 2") == 0);
    CHECK(strcmp((const char*)("Big " + a), "Big Mech") == 0);

    EString c = a;
    c += b;
    c += '!';
    CHECK(strcmp((const char*)c, "MechCommander!") == 0);
    CHECK(c.Length() == 14);
    // the source of the copy is unaffected (copy-on-write buffer)
    CHECK(strcmp((const char*)a, "Mech") == 0);
}

TEST_CASE("EString: appending and prepending a single character")
{
    EString a("Mech");
    CHECK(strcmp((const char*)(a + 'X'), "MechX") == 0);
    CHECK(strcmp((const char*)('X' + a), "XMech") == 0);
}

TEST_CASE("ECharString: appending and prepending a single character")
{
    ECharString a("Mech");
    CHECK(strcmp((a + 'X').Data(), "MechX") == 0);
    CHECK(strcmp(('X' + a).Data(), "XMech") == 0);
}

// EWCharString is not tested: on Linux it needs Win32 functions (GetStringTypeExW, _strinc)
// that the platform shims don't provide, so it can't be linked outside the game.
