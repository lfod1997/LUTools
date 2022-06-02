// Created: 2022-05-27

#ifndef _PATHUTILS_HPP_
#define _PATHUTILS_HPP_

#include <cctype>
#include <fstream>
#include <string>
#include <algorithm>

/// \brief Returns the directory component of \c path
inline std::string getDirectory(const std::string& path) {
    const auto where_slash = path.find_last_of("/\\");
    return where_slash != std::string::npos ? path.substr(0, where_slash) : "";
}

/// \brief Returns the leaf component of \c path
inline std::string getFileName(const std::string& path) {
    const auto where_slash = path.find_last_of("/\\");
    return path.substr(where_slash + 1);
}

/// \brief Returns the leaf component of \c path with its extension name removed
inline std::string getBaseName(const std::string& path) {
    const auto where_slash = path.find_last_of("/\\");
    const auto where_dot = path.find_last_of('.');

    // This is trouble-free
    return path.substr(where_slash + 1, where_dot - where_slash - 1);
}

/// \brief Returns entire \c path with its trailing extension name removed
inline std::string getExtensionNameRemoved(const std::string& path) {
    const auto where_dot = path.find_last_of('.');
    return where_dot != std::string::npos ? path.substr(0, where_dot) : path;
}

/// \brief Returns the extension name of \c path
/// \param path The path
/// \param to_lower Whether to return an all-lower string
inline std::string getExtensionName(const std::string& path, bool to_lower = true) {
    const auto where_dot = path.find_last_of('.');
    std::string result = where_dot != std::string::npos ? path.substr(where_dot + 1) : "";
    if (!result.empty() && to_lower) {
        std::transform(result.begin(), result.end(), result.begin(), std::tolower);
    }
    return result;
}

/// \brief Returns the secondary extension name of \c path, e.g. \c getSecondaryExtensionName("a.b.c") returns \c "b"
/// \param path The path
/// \param to_lower Whether to return an all-lower string
inline std::string getSecondaryExtensionName(const std::string& path, bool to_lower = true) {
    return getExtensionName(getExtensionNameRemoved(path), to_lower);
}

/// \brief Checks if a file exists and is available for the current thread
inline bool isFileAvailable(const std::string& path) {
    return std::ifstream { path }.good();
}

#endif // _PATHUTILS_HPP_
