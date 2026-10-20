#ifndef SAVEGAME_H
#define SAVEGAME_H
//---------------------------------------------------------------------------
// Reading save game data structures, including saves written by older 64-bit Linux builds.
//
// These structures are written to save files as raw bytes. They used to contain 'long' members,
// which are 8 bytes on 64-bit Linux and 4 on Windows (and in the original game), so the layout
// differed between platforms. They now use fixed size types; packets in the old 64-bit Linux
// layout are recognised by their size and converted when loading.
//---------------------------------------------------------------------------

#include <stddef.h>
#include <stdint.h>

class PacketFile;

struct SaveLayoutOp {
    uint32_t oldOffset;
    uint32_t newOffset;
    uint32_t size;      // bytes written to the new layout
    uint32_t narrow;    // 0: copy 'size' bytes, 1: 8 byte integer -> 4 byte integer
};

struct SaveLayout {
    const char* name;
    uint32_t oldSize;   // size in the old 64-bit Linux layout
    uint32_t newSize;   // size now (sizeof)
    const SaveLayoutOp* ops;
    uint32_t numOps;
};

// order of the layouts in the generated table (tools/save_layout/gen_save_layout.py)
enum SaveDataType {
    SAVE_MoverData,
    SAVE_MechData,
    SAVE_GroundVehicleData,
    SAVE_WeaponBoltData,
    SAVE_TeamData,
    SAVE_TeamStaticData,
    SAVE_StaticCommanderData,
    SAVE_CommanderData,
    SAVE_GateData,
    NUM_SAVE_DATA_TYPES
};

const SaveLayout& getLegacySaveLayout(SaveDataType type);

// converts one structure from the old layout; only the fields are written, padding and anything
// else in 'newData' (e.g. vtable pointers of embedded classes) are left alone
void convertLegacySaveData(const SaveLayout& layout, const void* oldData, void* newData);

// Reads packet 'packet' into data (sizeof == layout's newSize), converting it if it was written
// in the old layout. Returns false (after a STOP) for packets of any other size.
bool readSaveData(PacketFile* file, long packet, SaveDataType type, void* data, size_t dataSize);

// whether a packet of this size holds this type (in either layout)
bool isSaveDataSize(SaveDataType type, size_t packetSize);

#endif // SAVEGAME_H
