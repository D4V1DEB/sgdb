#pragma once
#include <cstdint>

struct PageHeader {
    uint16_t freeSpaceOffset;
    uint16_t slotDirectoryOffset;
    uint8_t slotCount;
    uint8_t deletedSlotCount;
    uint16_t freeSpace;
    uint32_t nextPage;
};
