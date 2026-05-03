#pragma once
#include <vector>
#include <cstddef>

struct Record {
    std::vector<char> data;

    inline size_t size() const { return data.size(); }
};
