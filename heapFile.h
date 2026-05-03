#pragma once
#include <string>
#include <optional>
#include "fileManager.h"
#include "page.h"
#include "rid.h"

using namespace std;


class HeapFile {
public:
    explicit HeapFile(const std::string& path);

    RID insert(const Record& record);
    std::optional<Record> read(const RID& rid);

    RID update(const RID& rid, const Record& record);
    bool deleteLogical(const RID& rid);
    bool deletePhysical(const RID& rid);

    uint32_t pageCount();
    Page readPage(uint32_t pageId);

private:
    FileManager fileManager;

    std::optional<uint32_t> findPageWithSpace(size_t recordSize);
};
