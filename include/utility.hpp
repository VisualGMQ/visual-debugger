#pragma once

#include <fstream>
#include <optional>
#include <string>
#include "log.hpp"

//! @brief read whole file content
//! @param filename 
//! @return file content. Read file failed will return `std::nullopt`
inline std::optional<std::string> ReadWholeFile(const std::string& filename) {
    std::ifstream file(filename);
    if (file.fail()) {
        LOGE("file %s open failed", filename);
        return std::nullopt;
    }
    std::string content(std::istreambuf_iterator<char>(file), (std::istreambuf_iterator<char>()));
    return content;
}

inline const uint8_t* FindSubStr(const uint8_t* str, size_t size1, const uint8_t* c, size_t size2) {
    const uint8_t* ptr = str;
    const uint8_t* cptr = c;

    while (ptr < str + size1) {
        const uint8_t* oldPtr = ptr;
        while (ptr < str + size1 && cptr < c + size2 && *cptr == *ptr) {
            ptr ++;
            cptr ++;
        }
        if (cptr - c == size2) {
            return oldPtr;
        } else {
            ptr = oldPtr + (cptr - c == 0 ? 1 : cptr - c);
            cptr = c;
        }
    }
    return nullptr;
}
