#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <optional>
#include "pageHeader.h"
#include "slot.h"
#include "record.h"

class Page {
public:
    static constexpr size_t PAGE_SIZE = 4096;
    static constexpr size_t MAX_SLOTS = 255;
    Page();

    bool canFit(size_t recordSize) const;

    std::optional<uint8_t> insertRecord(const Record& record);
    std::optional<Record> readRecord(uint8_t slotId) const;

    bool updateRecord(uint8_t slotId, const Record& record);
    bool deleteLogical(uint8_t slotId);
    bool deletePhysical(uint8_t slotId);

    void compact();

    char* raw();
    const char* raw() const;

    bool getSlot(uint8_t slotId, Slot& out) const;
    bool markMoved(uint8_t slotId, uint32_t newPageId, uint8_t newSlotId);

    ~Page();

private:
    std::array<char, PAGE_SIZE> buffer;

    PageHeader& header();
    const PageHeader& header() const;

    Slot& slot(uint8_t slotId);
    const Slot& slot(uint8_t slotId) const;
};

