# AsterSearchService

A central, cooperative file indexing and search service for AsterOS.

**AsterSearchService** maintains one authoritative file index that all system applications query through D-Bus. No app needs to create its own index or recursively scan the filesystem.

---

## Why AsterSearchService?

In a typical desktop environment, each app that needs search:
- Scans the filesystem independently
- Maintains its own index
- Wastes disk space and CPU

**AsterSearchService** solves this:
```
One Index
    ↓
Multiple Apps (Aster Files, Aster Menu, Aster Photos, etc.)
    ↓
All consume the same central index via D-Bus
```

**Benefits:**
- ⚡ No repeated filesystem scans
- 💾 Single index on disk
- 🔄 Real-time updates via inotify
- 🚀 Fast, responsive search across the entire system
- 🔒 Central permission and access control

---

## Architecture

```
┌─────────────────────────────────────────┐
│   AsterSearchService (aster-searchd)    │
│                                         │
│  ┌────────────────────────────────────┐ │
│  │  Central File Index                │ │
│  │  - FileRegistry                    │ │
│  │  - PrefixTree (filenames)          │ │
│  │  - InvertedIndex (content)         │ │
│  └────────────────────────────────────┘ │
│                  ↑                       │
│         ┌────────┼────────┐             │
│         │        │        │             │
│      FsHook   D-Bus    Search           │
│     (inotify) Query    Engine           │
└─────────────────────────────────────────┘
         ↑         ↑         ↑
         │         │         │
    ┌────┴────┬────┴────┬────┴────┐
    │          │         │         │
Aster Files  Aster    Aster     Aster
             Menu     Photos    Music
```

**How it works:**

1. **Indexing**: AsterSearchService recursively scans configured directories on startup
2. **Persistence**: Index is stored in `.aster_search_db` (binary format)
3. **Live Updates**: inotify watches directories for file changes
4. **Search**: Apps call the service via D-Bus
5. **Scope Filtering**: Results filtered by USER, FILESYSTEM, CURRENT_DIRECTORY, or APPLICATIONS scope
6. **Ranking**: Results ranked by TF-IDF + filename relevance

---

## Features

- ✅ Recursive directory indexing
- ✅ Trie-based filename search (exact and prefix matching)
- ✅ Full-text content search via Inverted Index
- ✅ TF-IDF based ranking
- ✅ Binary persistence (`.aster_search_db`)
- ✅ Incremental indexing on startup
- ✅ Real-time filesystem monitoring via `inotify`
- ✅ Automatic index updates while daemon runs
- ✅ Thread-safe access (`std::shared_mutex`)
- ✅ D-Bus IPC for system-wide integration
- ✅ Multiple search scopes (USER, FILESYSTEM, CURRENT_DIRECTORY, APPLICATIONS)
- ✅ Pagination (limit/offset)
- ✅ Multiple sort options (relevance, name, modified, size)
- ✅ Interactive CLI for testing
- ✅ Benchmark suite

---

## Installation

### Prerequisites

- Linux (Ubuntu 20.04+, Fedora 33+, or equivalent)
- C++20 compiler (g++11+, clang+13)
- libdbus-1-dev (D-Bus development headers)
- make

### Install dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install -y \
    build-essential \
    libdbus-1-dev \
    pkg-config
```

**Fedora/RHEL:**
```bash
sudo dnf install -y \
    gcc-c++ \
    dbus-devel \
    pkg-config
```

**Arch:**
```bash
sudo pacman -S \
    base-devel \
    dbus \
    pkg-config
```

### Build from source

```bash
git clone https://github.com/Aster-Proyect/astersearchservice.git
cd astersearchservice

# Build the CLI and daemon
make all daemon

# Install to system
sudo make install
```

Or manually:
```bash
# Install daemon
sudo install -Dm755 bin/aster-searchd /usr/local/bin/aster-searchd
sudo install -Dm755 bin/aster-search /usr/local/bin/aster-search

# Install D-Bus service file
sudo install -Dm644 data/com.astersearch.Service.service \
    /usr/share/dbus-1/services/com.astersearch.Service.service

# Install D-Bus interface definition
sudo install -Dm644 data/com.astersearch.Service.xml \
    /usr/share/dbus-1/interfaces/com.astersearch.Service.xml
```

### Start the service

**As a user service (recommended):**
```bash
# Enable the service to start on login
systemctl --user enable aster-search.service
systemctl --user start aster-search.service

# Check status
systemctl --user status aster-search.service
```

**Manually:**
```bash
aster-searchd &
```

---

## D-Bus API

AsterSearchService exposes a D-Bus interface for all applications to query.

### Service Information

| Property | Value |
|----------|-------|
| **Service Name** | `com.astersearch.Service` |
| **Object Path** | `/com/astersearch/Service` |
| **Interface** | `com.astersearch.Service.Search` |

### Method: Search

**Signature:**
```text
Search(s query, s scope, s path, i limit, i offset, s sort) -> s response_json
```

**Parameters:**

| Name | Type | Default | Description |
|------|------|---------|-------------|
| `query` | string | (required) | Search query |
| `scope` | string | `USER` | Search scope: `USER`, `FILESYSTEM`, `CURRENT_DIRECTORY`, `APPLICATIONS` |
| `path` | string | `$HOME` | Directory path (for `CURRENT_DIRECTORY` scope) |
| `limit` | int32 | `50` | Max results to return (capped at 1000) |
| `offset` | int32 | `0` | Pagination offset |
| `sort` | string | `relevance` | Sort order: `relevance`, `name`, `modified`, `size` |

**Returns:**

JSON string with the following structure:

```json
{
  "status": "ok",
  "results": [
    {
      "id": 0,
      "name": "document.pdf",
      "path": "/home/user/Documents/document.pdf",
      "type": "file",
      "size": 102400,
      "modified": 1693468800,
      "score": 45
    }
  ],
  "total": 1,
  "query_time_ms": 12,
  "index_state": "ready",
  "indexed_files": 4521
}
```

**Field Descriptions:**

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | `ok` or `error` |
| `results` | array | Array of matching files |
| `results[].id` | int | Unique file ID in the index |
| `results[].name` | string | Filename only |
| `results[].path` | string | Full absolute path |
| `results[].type` | string | `file` or `directory` |
| `results[].size` | uint64 | File size in bytes |
| `results[].modified` | int64 | Last modification time (unix timestamp) |
| `results[].score` | int | Relevance score (higher = better match) |
| `total` | int | Total matches (before pagination) |
| `query_time_ms` | int | Time to execute search in milliseconds |
| `index_state` | string | Index status: `ready`, `indexing`, `error` |
| `indexed_files` | int | Total number of files in the index |

### Method: GetStatus

**Signature:**
```text
GetStatus() -> s status_json
```

Returns current service status as JSON:
```json
{
  "status": "ready",
  "indexed_files": 4521,
  "index_state": "ready",
  "last_update": 1693468800
}
```

### Method: GetIndexStats

**Signature:**
```text
GetIndexStats() -> i indexed_files
```

Returns the total number of indexed files.

---

## Using AsterSearchService in Your App

### Example 1: Aster Menu (Application Launcher)

The Aster Menu is a great example of how to integrate with AsterSearchService.

**Scenario:** User opens the Start Menu and types "document"

**Flow:**
```
User types "document"
    ↓
Aster Menu queries D-Bus:
  - query: "document"
  - scope: "USER"  (search home directory only)
  - limit: 20      (show top 20 matches)
    ↓
AsterSearchService searches the central index
    ↓
Response (< 50ms):
  [
    "My Document.pdf",
    "document_draft.docx",
    "document_review.txt"
  ]
    ↓
Aster Menu renders results in the UI
    ↓
User clicks "My Document.pdf"
    ↓
Aster Menu opens the file
```

**C++ Implementation (using GLib/GObject):**

```cpp
#include <gio/gio.h>
#include <json-glib/json-glib.h>

void search_files(const gchar* query) {
    GError* error = nullptr;
    GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    
    if (!connection) {
        g_printerr("Failed to connect to D-Bus: %s\n", error->message);
        g_error_free(error);
        return;
    }

    GVariant* result = g_dbus_connection_call_sync(
        connection,
        "com.astersearch.Service",
        "/com/astersearch/Service",
        "com.astersearch.Service.Search",
        "Search",
        g_variant_new("(ssssiss)",
            query,              // query
            "USER",             // scope
            "",                 // path (empty = use default)
            20,                 // limit
            0,                  // offset
            "relevance"         // sort
        ),
        G_VARIANT_TYPE("(s)"),  // return type: single string (JSON)
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (!result) {
        g_printerr("D-Bus call failed: %s\n", error->message);
        g_error_free(error);
        g_object_unref(connection);
        return;
    }

    // Extract the JSON response
    const gchar* json_response;
    g_variant_get(result, "(&s)", &json_response);

    // Parse JSON (using json-glib)
    JsonParser* parser = json_parser_new();
    json_parser_load_from_data(parser, json_response, -1, &error);

    if (error) {
        g_printerr("JSON parse error: %s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        g_variant_unref(result);
        g_object_unref(connection);
        return;
    }

    JsonNode* root = json_parser_get_root(parser);
    JsonObject* obj = json_node_get_object(root);

    // Get results array
    JsonArray* results = json_object_get_array_member(obj, "results");
    guint len = json_array_get_length(results);

    g_print("Found %u results:\n", len);

    for (guint i = 0; i < len; i++) {
        JsonObject* result_obj = json_array_get_object_element(results, i);
        const gchar* name = json_object_get_string_member(result_obj, "name");
        const gchar* path = json_object_get_string_member(result_obj, "path");
        gint score = json_object_get_int_member(result_obj, "score");

        g_print("  [%d] %s (%s) - score: %d\n", i, name, path, score);
    }

    // Cleanup
    g_object_unref(parser);
    g_variant_unref(result);
    g_object_unref(connection);
}
```

### Example 2: Aster Files (File Manager)

**Scenario:** User opens Aster Files, navigates to `/home/user/Documents`, and searches for "pdf"

**Flow:**
```
User types "pdf" in the search box
    ↓
Aster Files queries D-Bus:
  - query: "pdf"
  - scope: "CURRENT_DIRECTORY"  (search current folder only)
  - path: "/home/user/Documents"
  - limit: 50
    ↓
AsterSearchService filters index results to /home/user/Documents
    ↓
Response:
  [
    "report.pdf",
    "invoice_2024.pdf",
    "notes.pdf"
  ]
    ↓
Aster Files shows results inline in the folder view
```

### Example 3: Aster Photos (Photo Viewer)

**Scenario:** User searches for photos by filename

```cpp
// Search in APPLICATIONS scope for image files
// query: "vacation"
// scope: "FILESYSTEM"
// Results will include all files matching "vacation" across the system
```

---

## Command Line Usage

### Interactive CLI Search

```bash
aster-search /path/to/directory

Enter text to search: documents
# Shows results in the directory

Enter text to search: :help
  :q, :Q    - Quit (saves index)
  :reindex  - Delete and rebuild index
  :help     - Show this message
  <query>   - Search for files
```

### Test D-Bus Service

**Check if service is running:**
```bash
dbus-send --session --print-reply \
  --dest=com.astersearch.Service \
  /com/astersearch/Service \
  com.astersearch.Service.Search.GetStatus
```

**Query the service:**
```bash
dbus-send --session --print-reply \
  --dest=com.astersearch.Service \
  /com/astersearch/Service \
  com.astersearch.Service.Search.Search \
  string:"document" \
  string:"USER" \
  string:"" \
  int32:20 \
  int32:0 \
  string:"relevance"
```

**Get index statistics:**
```bash
dbus-send --session --print-reply \
  --dest=com.astersearch.Service \
  /com/astersearch/Service \
  com.astersearch.Service.Search.GetIndexStats
```

### Benchmarks

```bash
make benchmark
make run-bench ARGS="./benchmark_data"
```

Generates performance reports in `benchmark_results/`.

---

## Configuration

By default, AsterSearchService indexes:
- `$HOME` (user home directory)

Future versions will support:
- Custom indexed directories
- Excluded patterns
- Content indexing options
- Service configuration files

---

## Search Ranking

Scores are accumulated per file across all index sources:

| Match Type | Score |
|---|---|
| Exact filename token | +10 |
| Prefix filename token | +5 |
| Content match (TF-IDF weighted) | variable |

Results are sorted by descending score, so best matches appear first.

---

## How It Works

### File Registration

Each file is registered with metadata:

```cpp
struct FileMetaData {
    int id;
    std::string name;
    std::string path;
    uintmax_t file_size;
    int64_t last_modified_ticks;
    bool index_content;  // true for .txt, .md, .csv, .log
};
```

### Filename Index (Trie)

Filenames are tokenized and stored in a Trie structure.

Example: `math_notes.txt` → tokens: `["math", "notes", "txt"]`

Supports:
- Exact matching
- Prefix matching
- O(n) search where n = length of query

### Content Index (Inverted Index)

Text files (`.txt`, `.md`, `.csv`, `.log`) have their contents indexed.

Example structure:
```
token: "matrix"
├── file 3 → frequency 5
└── file 8 → frequency 2
```

### TF-IDF Scoring

Content relevance uses TF-IDF (Term Frequency - Inverse Document Frequency):

```
TF-IDF = (word frequency in file) × log(total files / files containing word)
```

This penalizes common words and rewards rare, specific matches.

### Persistence

Index is stored in `.aster_search_db` using a custom binary format:

```
[METADATA] [REGISTRY] [PREFIX__] [INVERTED]
```

On startup:
1. Load index from disk
2. Scan filesystem
3. Re-index only changed files
4. Start watching for file changes

### Live Updates

inotify watches the indexed directory for:
- File creation
- File modification
- File deletion

Changes are queued and processed efficiently.

---

## Project Structure

```
astersearchservice/
├── include/
│   ├── search_engine.h       # Core search logic
│   ├── search_types.h        # D-Bus request/response types
│   ├── aster_branding.h      # Project branding & constants
│   ├── dbus_service.h        # D-Bus interface
│   ├── file_registery.h      # File metadata storage
│   ├── prefix_tree.h         # Trie-based filename index
│   ├── inverted_index.h      # Content index
│   ├── fs_hook.h             # inotify wrapper
│   └── tokenizer.h           # Text tokenization
├── src/
│   ├── cli_main.cpp          # Interactive CLI
│   ├── daemon_main.cpp       # D-Bus daemon
│   ├── dbus_service.cpp      # D-Bus implementation
│   ├── search_engine.cpp
│   ├── file_registery.cpp
│   ├── prefix_tree.cpp
│   ├── inverted_index.cpp
│   ├── fs_hook.cpp
│   ├── tokenizer.cpp
│   └── benchmark.cpp         # Performance testing
├── data/
│   └── com.astersearch.Service.xml  # D-Bus interface definition
├── Makefile
└── README.md
```

---

## Design Decisions

| Decision | Reason |
|----------|--------|
| Trie for filename indexing | Efficient exact and prefix search |
| Inverted index for content | O(1) token lookup |
| TF-IDF scoring | Balances common/rare words |
| Single binary database | Fast startup, simpler persistence |
| Incremental indexing | Avoids full rebuilds |
| inotify for monitoring | Real-time updates without polling |
| D-Bus for IPC | Native system integration |
| Separate daemon process | Decouples indexing from clients |
| `shared_mutex` | Concurrent reads + exclusive writes |
| C++20 | Modern, efficient language |

---

## Requirements

- **OS:** Linux (Ubuntu 20.04+, Fedora 33+, Arch, etc.)
- **Compiler:** g++11+ or clang++13+
- **Libraries:** libdbus-1-dev, pkg-config
- **Build:** make
- **Language:** C++20

---

## Troubleshooting

### Service won't start

**Check D-Bus connection:**
```bash
echo $DBUS_SESSION_BUS_ADDRESS
dbus-send --session --print-reply /org/freedesktop/DBus /org/freedesktop/DBus org.freedesktop.DBus.ListNames
```

**Check logs:**
```bash
journalctl --user -u aster-search.service -n 50
```

### Index not updating

**Force rebuild:**
```bash
rm ~/.local/share/aster-search/.aster_search_db
systemctl --user restart aster-search.service
```

### D-Bus method not found

**Verify service is registered:**
```bash
dbus-send --session --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.ListNames | grep astersearch
```

**Check interface file:**
```bash
cat /usr/share/dbus-1/interfaces/com.astersearch.Service.xml
```

---

## Performance Characteristics

| Operation | Time (typical) |
|-----------|---|
| Cold startup (first index) | 2-5 seconds per 1000 files |
| Warm startup (load from disk) | < 100ms |
| Single-word search | 1-5 ms |
| Prefix search | 5-20 ms |
| Large query (10+ words) | 20-100 ms |
| Index update (1 file) | < 10 ms |
| Memory usage (1000 files) | ~10-20 MB |

---

## Based on / Upstream Attribution

AsterSearchService is derived from and inspired by:

**[vexselene/file-search-daemon](https://github.com/vexselene/file-search-daemon)**

The upstream project provided the technical foundation including:
- Recursive filesystem indexing
- Filename and content indexing via Trie and Inverted Index
- Binary persistence format
- Incremental indexing logic
- inotify filesystem monitoring
- Thread-safe search access

AsterSearchService extends this foundation by:
- Rebranding for AsterOS integration
- Adding D-Bus IPC instead of Unix sockets
- Implementing search scopes (USER, FILESYSTEM, CURRENT_DIRECTORY, APPLICATIONS)
- Adding structured request/response types
- Improving daemon lifecycle management
- Centralizing the service for system-wide consumption
- Documenting integration patterns for AsterOS applications

**Original License:** MIT  
**AsterSearchService License:** MIT

Both projects remain under the MIT License. The original copyright notice from vexselene is preserved.

---

## License

MIT License

See LICENSE file for details.

---

## Contributing

Contributions are welcome! Please submit issues and pull requests to:  
https://github.com/Aster-Proyect/astersearchservice

---

## Support

For issues, questions, or suggestions:

- **Issue Tracker:** https://github.com/Aster-Proyect/astersearchservice/issues
- **Email:** theasterproyect@gmail.com

---

**AsterSearchService — The search engine for AsterOS**
