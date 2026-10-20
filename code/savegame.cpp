//---------------------------------------------------------------------------
// Reading save game data structures, including saves written by older 64-bit Linux builds.
// See savegame.h.
//---------------------------------------------------------------------------

#include "savegame.h"

#include "mclib.h"
#include "packet.h"

#include "objmgr.h"
#include "weaponbolt.h"
#include "mover.h"
#include "mech.h"
#include "gvehicl.h"
#include "team.h"
#include "comndr.h"
#include "gate.h"

#include <string.h>
#include <vector>

// the generated file checks offsetof() of members of structures that embed classes with virtual
// functions; that is fine for the compilers we support
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
#include "savelayout_v1.inc"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

static_assert(sizeof(kSaveLayoutsV0) / sizeof(kSaveLayoutsV0[0]) == NUM_SAVE_DATA_TYPES,
              "SaveDataType and the generated layout table are out of sync");

const SaveLayout& getLegacySaveLayout(SaveDataType type)
{
    gosASSERT(type >= 0 && type < NUM_SAVE_DATA_TYPES);
    return kSaveLayoutsV0[type];
}

void convertLegacySaveData(const SaveLayout& layout, const void* oldData, void* newData)
{
    const unsigned char* src = (const unsigned char*)oldData;
    unsigned char* dst = (unsigned char*)newData;
    for (uint32_t i = 0; i < layout.numOps; ++i)
    {
        const SaveLayoutOp& op = layout.ops[i];
        if (op.narrow)
        {
            int64_t value;
            memcpy(&value, src + op.oldOffset, sizeof(value));
            const int32_t narrowed = (int32_t)value;
            memcpy(dst + op.newOffset, &narrowed, sizeof(narrowed));
        }
        else
        {
            memcpy(dst + op.newOffset, src + op.oldOffset, op.size);
        }
    }
}

bool isSaveDataSize(SaveDataType type, size_t packetSize)
{
    const SaveLayout& layout = getLegacySaveLayout(type);
    return packetSize == layout.newSize || packetSize == layout.oldSize;
}

bool readSaveData(PacketFile* file, long packet, SaveDataType type, void* data, size_t dataSize)
{
    const SaveLayout& layout = getLegacySaveLayout(type);
    gosASSERT(dataSize == layout.newSize);

    if (file->seekPacket(packet) != NO_ERR)
    {
        STOP(("Save game: cannot find packet %ld (%s)", packet, layout.name));
        return false;
    }

    const size_t packetSize = (size_t)file->getPacketSize();
    if (packetSize == layout.newSize)
    {
        file->readPacket(packet, (MemoryPtr)data);
        return true;
    }
    if (packetSize == layout.oldSize)
    {
        std::vector<unsigned char> oldData(layout.oldSize);
        file->readPacket(packet, oldData.data());
        convertLegacySaveData(layout, oldData.data(), data);
        return true;
    }

    STOP(("Save game: packet %ld has %zu bytes, expected %u for %s", packet, packetSize, layout.newSize, layout.name));
    return false;
}
