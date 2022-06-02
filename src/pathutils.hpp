// Created: 2022-05-27

#ifndef _PATHUTILS_HPP_
#define _PATHUTILS_HPP_

#include <cctype>
#include <fstream>
#include <string>
#include <algorithm>

inline std::string getDirectory(const std::string& path) {
    const auto where_slash = path.find_last_of("/\\");
    return where_slash != std::string::npos ? path.substr(0, where_slash) : "";
}

inline std::string getFileName(const std::string& path) {
    const auto where_slash = path.find_last_of("/\\");
    return path.substr(where_slash + 1);
}

inline std::string getBaseName(const std::string& path) {
    const auto where_slash = path.find_last_of("/\\");
    const auto where_dot = path.find_last_of('.');

    // This is trouble-free
    return path.substr(where_slash + 1, where_dot - where_slash - 1);
}

inline std::string getExtensionNameRemoved(const std::string& path) {
    const auto where_dot = path.find_last_of('.');
    return where_dot != std::string::npos ? path.substr(0, where_dot) : path;
}

inline std::string getExtensionName(const std::string& path, bool to_lower = true) {
    const auto where_dot = path.find_last_of('.');
    std::string result = where_dot != std::string::npos ? path.substr(where_dot + 1) : "";
    if (!result.empty() && to_lower) {
        std::transform(result.begin(), result.end(), result.begin(), std::tolower);
    }
    return result;
}

inline std::string getSecondaryExtensionName(const std::string& path, bool to_lower = true) {
    return getExtensionName(getExtensionNameRemoved(path), to_lower);
}

inline bool isFileAvailable(const std::string& path) {
    return std::ifstream { path }.good();
}

#endif // _PATHUTILS_HPP_
