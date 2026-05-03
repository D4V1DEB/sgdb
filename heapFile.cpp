#include "heapFile.h"
#include <stdexcept>
#include <cstring>

HeapFile::HeapFile(const std::string& path) : fileManager(path) {}

std::optional<uint32_t> HeapFile::findPageWithSpace(size_t recordSize) {
    uint32_t count = fileManager.pageCount();
    for (uint32_t i = 0; i < count; ++i) {
        Page p = fileManager.readPage(i);
        if (p.canFit(recordSize)) return i;
    }
    return std::nullopt;
}

RID HeapFile::insert(const Record& record) {
    auto opt = findPageWithSpace(record.size());
    uint32_t pid;
    Page page;
    if (!opt.has_value()) {
        page = Page();
        pid = fileManager.appendPage(page);
    } else {
        pid = *opt;
        page = fileManager.readPage(pid);
    }

    auto slotOpt = page.insertRecord(record);
    if (!slotOpt) {
        // try appending a new page
        Page newPage;
        pid = fileManager.appendPage(newPage);
        auto s2 = newPage.insertRecord(record);
        if (!s2) throw std::runtime_error("Insert failed even on new page");
        fileManager.writePage(pid, newPage);
        return RID{pid, *s2};
    }

    fileManager.writePage(pid, page);
    return RID{pid, *slotOpt};
}

std::optional<Record> HeapFile::read(const RID& rid) {
    Page page = fileManager.readPage(rid.pageId);
    Slot s;
    if (!page.getSlot(rid.slotId, s)) return std::nullopt;

    if (s.flags == SLOT_USED) {
        return page.readRecord(rid.slotId);
    } else if (s.flags == SLOT_MOVED) {
        // read forward pointer and follow
        const char* raw = page.raw();
        uint32_t newPageId;
        memcpy(&newPageId, raw + s.offset, sizeof(uint32_t));
        uint8_t newSlotId = *(uint8_t*)(raw + s.offset + sizeof(uint32_t));
        return read(RID{newPageId, newSlotId});
    }
    return std::nullopt;
}

RID HeapFile::update(const RID& rid, const Record& record) {
    Page page = fileManager.readPage(rid.pageId);
    Slot s;
    if (!page.getSlot(rid.slotId, s)) throw std::runtime_error("Invalid RID for update");

    if (s.flags == SLOT_USED) {
        if (page.updateRecord(rid.slotId, record)) {
            fileManager.writePage(rid.pageId, page);
            return rid;
        } else {
            // needs relocation
            RID newRid = insert(record);
            // mark old slot as moved and write forward pointer
            page.markMoved(rid.slotId, newRid.pageId, newRid.slotId);
            fileManager.writePage(rid.pageId, page);
            return newRid;
        }
    } else if (s.flags == SLOT_MOVED) {
        // follow pointer and update at destination
        const char* raw = page.raw();
        uint32_t newPageId;
        memcpy(&newPageId, raw + s.offset, sizeof(uint32_t));
        uint8_t newSlotId = *(uint8_t*)(raw + s.offset + sizeof(uint32_t));
        return update(RID{newPageId, newSlotId}, record);
    }

    throw std::runtime_error("Cannot update: invalid slot state");
}

bool HeapFile::deleteLogical(const RID& rid) {
    Page page = fileManager.readPage(rid.pageId);
    Slot s;
    if (!page.getSlot(rid.slotId, s)) return false;

    if (s.flags == SLOT_MOVED) {
        const char* raw = page.raw();
        uint32_t newPageId;
        memcpy(&newPageId, raw + s.offset, sizeof(uint32_t));
        uint8_t newSlotId = *(uint8_t*)(raw + s.offset + sizeof(uint32_t));
        bool res = deleteLogical(RID{newPageId, newSlotId});
        // also mark this slot deleted
        page.deleteLogical(rid.slotId);
        fileManager.writePage(rid.pageId, page);
        return res;
    }

    bool ok = page.deleteLogical(rid.slotId);
    fileManager.writePage(rid.pageId, page);
    return ok;
}

bool HeapFile::deletePhysical(const RID& rid) {
    Page page = fileManager.readPage(rid.pageId);
    Slot s;
    if (!page.getSlot(rid.slotId, s)) return false;

    if (s.flags == SLOT_MOVED) {
        const char* raw = page.raw();
        uint32_t newPageId;
        memcpy(&newPageId, raw + s.offset, sizeof(uint32_t));
        uint8_t newSlotId = *(uint8_t*)(raw + s.offset + sizeof(uint32_t));
        bool res = deletePhysical(RID{newPageId, newSlotId});
        // remove this slot physically as well
        page.deletePhysical(rid.slotId);
        fileManager.writePage(rid.pageId, page);
        return res;
    }

    bool ok = page.deletePhysical(rid.slotId);
    fileManager.writePage(rid.pageId, page);
    return ok;
}

uint32_t HeapFile::pageCount() {
    return fileManager.pageCount();
}

Page HeapFile::readPage(uint32_t pageId) {
    return fileManager.readPage(pageId);
}
