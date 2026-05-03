#include "fileManager.h"
#include <fstream>
#include <iostream>

FileManager::FileManager(const std::string& p) : path(p) {
    file.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        // create the file then reopen
        std::ofstream create(path, std::ios::binary);
        create.close();
        file.open(path, std::ios::in | std::ios::out | std::ios::binary);
    }
}

FileManager::~FileManager() {
    if (file.is_open()) file.close();
}

Page FileManager::readPage(uint32_t pageId) {
    Page page;
    file.seekg(static_cast<std::streamoff>(pageId) * static_cast<std::streamoff>(Page::PAGE_SIZE), std::ios::beg);
    file.read(page.raw(), static_cast<std::streamsize>(Page::PAGE_SIZE));
    if (!file) {
        // if read failed (e.g., beyond EOF), clear flags so file can be reused
        file.clear();
    }
    return page;
}

void FileManager::writePage(uint32_t pageId, const Page& page) {
    file.seekp(static_cast<std::streamoff>(pageId) * static_cast<std::streamoff>(Page::PAGE_SIZE), std::ios::beg);
    file.write(page.raw(), static_cast<std::streamsize>(Page::PAGE_SIZE));
    file.flush();
}

uint32_t FileManager::appendPage(const Page& page) {
    uint32_t id = pageCount();
    file.seekp(0, std::ios::end);
    file.write(page.raw(), static_cast<std::streamsize>(Page::PAGE_SIZE));
    file.flush();
    return id;
}

uint32_t FileManager::pageCount() {
    file.seekg(0, std::ios::end);
    std::streampos sz = file.tellg();
    if (sz == static_cast<std::streampos>(-1)) return 0;
    return static_cast<uint32_t>(sz / static_cast<std::streampos>(Page::PAGE_SIZE));
}