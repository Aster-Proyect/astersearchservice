#pragma once

#include <string>
#include <vector>

namespace AsterSearch {

enum class SearchScope {
    USER,
    FILESYSTEM,
    CURRENT_DIRECTORY,
    APPLICATIONS
};

enum class SortBy {
    RELEVANCE,
    NAME,
    MODIFIED,
    SIZE
};

struct SearchRequest {
    std::string query;
    SearchScope scope = SearchScope::USER;
    std::string path;
    int limit = 50;
    int offset = 0;
    SortBy sort = SortBy::RELEVANCE;

    void normalize() {
        if (limit <= 0) limit = 50;
        if (limit > 1000) limit = 1000;
        if (offset < 0) offset = 0;
    }
};

struct SearchResultEntry {
    int id = 0;
    std::string name;
    std::string path;
    std::string type = "file";
    std::uint64_t size = 0;
    std::int64_t modified = 0;
    int score = 0;
};

struct SearchResponse {
    std::string status = "ok";
    std::string error = "";
    std::vector<SearchResultEntry> results;
    int total = 0;
    int query_time_ms = 0;
    std::string index_state = "ready";
    int indexed_files = 0;
};

}  // namespace AsterSearch