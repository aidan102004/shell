#pragma once
#include "trie.h"

class Populater {
public:
    static void populate_from_path(Trie* builtin_trie);
    static void populate_files(Trie* filename_trie);
};