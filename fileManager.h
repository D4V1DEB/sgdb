#pragma once
#include <string>
#include <fstream>
#include "page.h"

using namespace std;


class FileManager {
public:
    explicit FileManager(const string& path);
    ~FileManager();

    Page readPage(uint32_t pageId);
    void writePage(uint32_t pageId, const Page& page);
    uint32_t appendPage(const Page& page);

    uint32_t pageCount();

private:
    fstream file;
    string path;
};
