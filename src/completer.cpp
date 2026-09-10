#include <fstream>
#include "./headers/completer.h"
#include "./headers/util.h"

namespace fs = std::__fs::filesystem;
std::unordered_map<std::string, fs::directory_entry> Completer::complete_paths;

std::string Completer::completion(Trie& trie, std::string cur_input, const std::string& full_line, int tab_count) {
    std::vector<std::string> matches = trie.get_children(cur_input);
    return Util::matches_helper(matches, cur_input, full_line, tab_count);
}

std::string Completer::path_completion(const std::string& s, const std::string& full_line, size_t s_pos, int tab_count)  
{
    std::string dir_path = s.substr(0, s_pos + 1); //returns the directory path before the last /
    std::string prefix = s.substr(s_pos + 1); //whatever is after the slash
    std::vector<std::string> matches;
    try {
        for (const auto& entry : fs::directory_iterator(dir_path)) {
            std::string name = entry.path().filename().string(); //every file in that directory
            if (name.rfind(prefix, 0) == 0) matches.push_back(name); //if the prefix exists in name add to matches
        }

    } catch(...) {}
    
    return Util::matches_helper(matches, prefix, full_line, tab_count, dir_path);
}

bool Completer::find_path(const std::string& query) {
    return complete_paths.find(query) != complete_paths.end(); //return bool if found
}

std::filesystem::path Completer::get_path(const std::string& key) {
    auto it = complete_paths.find(key);
    if (it != complete_paths.end()) {
        return complete_paths[key].path();
    }
    return std::filesystem::path(); //return empty path
}