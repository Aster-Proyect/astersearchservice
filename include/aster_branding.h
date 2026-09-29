#pragma once

#include <string>

namespace AsterSearch {

constexpr const char* PROJECT_NAME = "AsterSearchService";
constexpr const char* DISPLAY_NAME = "Aster Search";
constexpr const char* DESCRIPTION = "A cooperative system-wide file indexing and search service for AsterOS";
constexpr const char* VERSION = "1.0.0";
constexpr const char* VERSION_STRING = "AsterSearchService 1.0.0";
constexpr const char* DAEMON_NAME = "aster-searchd";
constexpr const char* CLI_NAME = "aster-search";
constexpr const char* DB_FILENAME = ".aster_search_db";
constexpr const char* DEFAULT_SOCKET_PATH = "/tmp/aster-search.sock";

inline std::string get_version_string() {
    return std::string(VERSION_STRING) + " - AsterOS System Component";
}

}  // namespace AsterSearch