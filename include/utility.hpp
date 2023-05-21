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

