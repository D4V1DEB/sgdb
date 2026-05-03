#include "page.h"
#include "pageHeader.h"
#include "slot.h"

#include <cstring>
#include <vector>
#include <algorithm>

Page::Page() {
    header().freeSpaceOffset = sizeof(PageHeader);
    header().slotDirectoryOffset = static_cast<uint16_t>(PAGE_SIZE);
    header().slotCount = 0;
    header().deletedSlotCount = 0;
    header().freeSpace = static_cast<uint16_t>(PAGE_SIZE - sizeof(PageHeader));
    header().nextPage = 0;
}

Page::~Page() = default;

PageHeader& Page::header() {
    return *reinterpret_cast<PageHeader*>(buffer.data());
}

const PageHeader& Page::header() const {
    return *reinterpret_cast<const PageHeader*>(buffer.data());
}

Slot& Page::slot(uint8_t slotId) {
    size_t slotSize = sizeof(Slot);
    size_t offset = PAGE_SIZE - slotSize * (static_cast<size_t>(slotId) + 1);
    return *reinterpret_cast<Slot*>(buffer.data() + offset);
}

const Slot& Page::slot(uint8_t slotId) const {
    size_t slotSize = sizeof(Slot);
    size_t offset = PAGE_SIZE - slotSize * (static_cast<size_t>(slotId) + 1);
    return *reinterpret_cast<const Slot*>(buffer.data() + offset);
}

char* Page::raw() { return buffer.data(); }
const char* Page::raw() const { return buffer.data(); }

bool Page::getSlot(uint8_t slotId, Slot& out) const {
    if (slotId >= header().slotCount) return false;
    out = slot(slotId);
    return true;
}

bool Page::canFit(size_t recordSize) const {
    size_t usedSlotBytes = static_cast<size_t>(header().slotCount) * sizeof(Slot);
    size_t freeSpace = PAGE_SIZE - header().freeSpaceOffset - usedSlotBytes;
    return recordSize + sizeof(Slot) <= freeSpace || recordSize <= freeSpace; // allow reuse of deleted slots externally
}

std::optional<uint8_t> Page::insertRecord(const Record& record) {
    // try to reuse deleted slot
    for (uint8_t i = 0; i < header().slotCount; ++i) {
        Slot& s = slot(i);
        if (s.flags == SLOT_DELETED && s.length >= record.size()) {
            memcpy(buffer.data() + s.offset, record.data.data(), record.size());
            s.length = static_cast<uint16_t>(record.size());
            s.flags = SLOT_USED;
            if (header().deletedSlotCount > 0) header().deletedSlotCount--;
            header().freeSpace = static_cast<uint16_t>(PAGE_SIZE - header().freeSpaceOffset - (header().slotCount * sizeof(Slot)));
            return i;
        }
    }

    // ensure there is room for record + new slot
    size_t usedSlotBytes = static_cast<size_t>(header().slotCount) * sizeof(Slot);
    size_t freeSpace = PAGE_SIZE - header().freeSpaceOffset - usedSlotBytes;
    if (record.size() + sizeof(Slot) > freeSpace) return std::nullopt;

    uint8_t newSlotId = header().slotCount;
    Slot& s = slot(newSlotId);

    s.offset = header().freeSpaceOffset;
    s.length = static_cast<uint16_t>(record.size());
    s.flags = SLOT_USED;

    memcpy(buffer.data() + s.offset, record.data.data(), record.size());

    header().slotCount++;
    header().slotDirectoryOffset = static_cast<uint16_t>(PAGE_SIZE - header().slotCount * sizeof(Slot));
    header().freeSpaceOffset = static_cast<uint16_t>(header().freeSpaceOffset + record.size());
    header().freeSpace = static_cast<uint16_t>(header().slotDirectoryOffset - header().freeSpaceOffset);

    return newSlotId;
}

std::optional<Record> Page::readRecord(uint8_t slotId) const {
    if (slotId >= header().slotCount) return std::nullopt;

    const Slot& s = slot(slotId);
    if (s.flags != SLOT_USED) return std::nullopt;

    Record r;
    r.data.resize(s.length);
    memcpy(r.data.data(), buffer.data() + s.offset, s.length);
    return r;
}

bool Page::updateRecord(uint8_t slotId, const Record& record) {
    if (slotId >= header().slotCount) return false;
    Slot& s = slot(slotId);
    if (s.flags != SLOT_USED) return false;

    if (record.size() <= s.length) {
        memcpy(buffer.data() + s.offset, record.data.data(), record.size());
        s.length = static_cast<uint16_t>(record.size());
        return true;
    }

    // does not fit in place
    return false;
}

bool Page::deleteLogical(uint8_t slotId) {
    if (slotId >= header().slotCount) return false;
    Slot& s = slot(slotId);
    if (s.flags != SLOT_USED && s.flags != SLOT_MOVED) return false;
    s.flags = SLOT_DELETED;
    header().deletedSlotCount++;
    return true;
}

bool Page::deletePhysical(uint8_t slotId) {
    if (slotId >= header().slotCount) return false;
    Slot& s = slot(slotId);
    if (s.flags == SLOT_EMPTY) return false;

    if (s.flags == SLOT_DELETED && header().deletedSlotCount > 0) header().deletedSlotCount--;
    s.flags = SLOT_EMPTY;
    s.length = 0;

    compact();
    return true;
}

bool Page::markMoved(uint8_t slotId, uint32_t newPageId, uint8_t newSlotId) {
    if (slotId >= header().slotCount) return false;
    Slot& s = slot(slotId);
    uint32_t pid = newPageId;
    // write forward pointer (pageId + slotId) at the record's offset
    memcpy(buffer.data() + s.offset, &pid, sizeof(uint32_t));
    memcpy(buffer.data() + s.offset + sizeof(uint32_t), &newSlotId, sizeof(uint8_t));
    s.length = static_cast<uint16_t>(sizeof(uint32_t) + sizeof(uint8_t));
    s.flags = SLOT_MOVED;
    return true;
}

void Page::compact() {
    struct LiveRecord {
        uint8_t slotId;
        std::vector<char> bytes;
    };

    std::vector<LiveRecord> liveRecords;

    for (uint8_t i = 0; i < header().slotCount; ++i) {
        const Slot& s = slot(i);
        if (s.flags == SLOT_USED || s.flags == SLOT_MOVED) {
            std::vector<char> bytes(s.length);
            memcpy(bytes.data(), buffer.data() + s.offset, s.length);
            liveRecords.push_back({i, std::move(bytes)});
        }
    }

    uint16_t writeOffset = sizeof(PageHeader);

    for (auto& record : liveRecords) {
        Slot& s = slot(record.slotId);
        memcpy(buffer.data() + writeOffset, record.bytes.data(), record.bytes.size());
        s.offset = writeOffset;
        s.length = static_cast<uint16_t>(record.bytes.size());
        writeOffset += s.length;
    }

    header().freeSpaceOffset = writeOffset;
    header().slotDirectoryOffset = static_cast<uint16_t>(PAGE_SIZE - header().slotCount * sizeof(Slot));
    header().freeSpace = static_cast<uint16_t>(header().slotDirectoryOffset - header().freeSpaceOffset);
}