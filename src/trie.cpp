#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "trie.h"

void Trie::insert(const std::string &text) {
    if (!root) root = std::make_unique<trienode>(); //if the true is empty create the root 

    trienode *node = root.get(); //creates a raw ptr node to walk through the trie 
    for (unsigned char c : text) { 
        if (!node->children[c]) {
            node->children[c] = std::make_unique<trienode>(); //if a child node doesnt exist create one
        }
        node = node->children[c].get(); //moves the ptr node
    }
    node->terminal = true; //sets this as we are pointing to last node in word
}

std::string Trie::complete(const std::string &prefix) const { //readonly method
    trienode *node = root.get(); //gets a raw ptr to traverse
    if (!node) return ""; 

    for (unsigned char c : prefix) {
        if (!node->children[c]) return ""; //traverse trie using param chars, if they dont exist return
        node = node->children[c].get(); //node will point to every char we find
    }

    std::string suffix; //will hold chars we need to append
    while (!node->terminal) { //until we reach the end
        int child_count = 0;
        int next_char = -1;
        for (int i = 0; i < 256; i++) { //count how many child nodes we have
            if (node->children[i]) {
                child_count++;
                next_char = i;
            }
        }

        if (child_count != 1) return ""; //if its 0 or > 1 it means there is nothing to add or its ambigous 
        suffix += static_cast<char>(next_char); //append to the suffix
        node = node->children[next_char].get(); //point to child
    }

    return prefix + suffix; //return full word, could return only suffix but we compute in main
}