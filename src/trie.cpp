#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "trie.h"

void Trie::insert(const std::string &text) {
    if (!root) root = std::make_unique<trienode>();

    trienode *node = root.get();
    for (unsigned char c : text) {
        if (!node->children[c]) {
            node->children[c] = std::make_unique<trienode>();
        }
        node = node->children[c].get();
    }
    node->terminal = true;
}

std::string Trie::complete(const std::string &prefix) const {
    trienode *node = root.get();
    if (!node) return "";

    for (unsigned char c : prefix) {
        if (!node->children[c]) return "";
        node = node->children[c].get();
    }

    std::string suffix;
    while (!node->terminal) {
        int child_count = 0;
        int next_char = -1;
        for (int i = 0; i < 256; i++) {
            if (node->children[i]) {
                child_count++;
                next_char = i;
            }
        }

        if (child_count != 1) return "";
        suffix += static_cast<char>(next_char);
        node = node->children[next_char].get();
    }

    return prefix + suffix;
}