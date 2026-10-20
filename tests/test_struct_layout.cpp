// Compile-time size checks for structures that are written to or read from files as raw bytes:
// data file formats and save games. A change of size (a member added, or a type whose size
// differs between platforms, e.g. long or pointers) breaks existing files, and different sizes on
// different platforms make files non-portable. CI builds this on Linux x64, Linux arm64 and
// Windows x64, so all of them are checked against the same numbers.
//
// If you change one of these structures on purpose, update the size here and think about file
// compatibility (version the format or convert old files).

#include "doctest/doctest.h"

#include "gameos.hpp"
#include "mclib.h"
#include "ffile.h"
#include "tgainfo.h"
#include "move.h"
#include "soundsys.h"

#include "objmgr.h"
#include "weaponbolt.h"
#include "turret.h"
#include "terrobj.h"
#include "warrior.h"
#include "mover.h"
#include "mech.h"
#include "gvehicl.h"
#include "carnage.h"
#include "bldng.h"
#include "artlry.h"
#include "gameobj.h"

#define CHECK_SIZE(type, size) static_assert(sizeof(type) == size, "size of " #type " changed, see the comment at the top of test_struct_layout.cpp")

// data files (fast files, textures, sounds, mission movement data packets)
CHECK_SIZE(FILEENTRY, 266);
CHECK_SIZE(TGAFileHeader, 18);
CHECK_SIZE(MC2_WAVEFORMATEX, 14);
CHECK_SIZE(MissionMapCellInfo, 14);
CHECK_SIZE(MapCell, 4);
CHECK_SIZE(PreservedCell, 8);
CHECK_SIZE(DoorLink, 11);
CHECK_SIZE(GlobalMapDoor, 57);
CHECK_SIZE(DoorInfo, 3);
CHECK_SIZE(GlobalMapArea, 25);
CHECK_SIZE(GameObjectFootPrint, 264);
CHECK_SIZE(ObjData, 15);
CHECK_SIZE(MiscObjectData, 16);
CHECK_SIZE(TeamObjectInfo, 12);

// save games
CHECK_SIZE(GameObjectData, 372);
CHECK_SIZE(MoverData, 3128);
CHECK_SIZE(MechData, 3928);
CHECK_SIZE(GroundVehicleData, 3264);
CHECK_SIZE(TurretData, 820);
CHECK_SIZE(BuildingData, 756);
CHECK_SIZE(TerrainObjectData, 720);
CHECK_SIZE(ArtilleryData, 432);
CHECK_SIZE(CarnageData, 396);
CHECK_SIZE(WeaponBoltData, 600);
CHECK_SIZE(StaticMechWarriorData, 40);
CHECK_SIZE(ObjectManagerData, 60);

// Not checked yet: TeamStaticData, TeamData, StaticCommanderData, CommanderData and GateData are
// also saved as raw bytes but contain 'long' members (8 bytes on Linux, 4 on Windows) and GateData
// a pointer, so their save game layout differs between platforms. Fixing that changes the save
// format; see MODERNIZATION.md.

TEST_CASE("Struct layout: file format structures have their expected sizes")
{
    // the checks above are static_asserts; this only makes the test show up in the test list
    CHECK(sizeof(FILEENTRY) == 266);
}
