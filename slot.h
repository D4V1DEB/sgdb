#pragma once
#include <cstdint>

struct ForwardPointer {
    uint32_t pageId;
    uint8_t slotId;
};

struct Slot {
    uint16_t offset;
    uint16_t length;
    uint8_t flags;
};

enum SlotFlags : uint8_t {
    SLOT_EMPTY   = 0,
    SLOT_USED    = 1,
    SLOT_DELETED = 2,
    SLOT_MOVED   = 4
};