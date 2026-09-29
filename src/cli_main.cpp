#include "../include/search_engine.h"
#include "../include/aster_version.h"
#include "../include/aster_branding.h"
#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <filesystem>

int main(int argc, char* argv[]) {
    namespace fs = std::filesystem;
    std::string dir_path;

    if (argc >= 2 && (std::string(argv[1]) == "--version" || std::string(argv[1]) == "-v")) {
        std::cout << AsterSearch::get_version_string() << std::endl;
        return 0;
    }

    if(argc >= 2) {
        if(fs::is_directory(argv[1])) {
            dir_path = fs::weakly_canonical(argv[1]).string();
        } else {
            std::cerr << "Invalid directory \"" << argv[1] << "\" fallback to defaults\n";
        }
    }
    if(dir_path.empty()) dir_path = fs::weakly_canonical("data").string();

    SearchEngine engine;
    engine.build(dir_path);

    while(true) {
        std::string query;
        std::cout << "Enter text to search: ";
        std::getline(std::cin, query);

        if(query.empty()) continue;
        if(query == ":q" || query == ":Q") {
            engine.save();
            break;
        }
        if(query == ":help") {
            std::cout << "\nCommands:\n";
            std::cout << "  :q, :Q    - Quit (saves index)\n";
            std::cout << "  :reindex  - Delete and rebuild index\n";
            std::cout << "  :help     - Show help\n";
            std::cout << "  <query>   - Search for files\n\n";
            continue;
        }
        if(query == ":reindex") {
            fs::remove(dir_path + "/" + AsterSearch::DB_FILENAME);
            std::cout << "Rebuilding index...\n";
            engine.build(dir_path);
            continue;
        }

        auto results = engine.search(query);
        engine.display(results);
    }

    return 0;
}
