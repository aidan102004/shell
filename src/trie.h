#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

constexpr int NUM_CHAR = 256;

typedef struct trienode {
    struct trienode *children[NUM_CHAR]; //an array of chidlren
    bool terminal; //whether this is the last char
} trienode;

class Trie {
    public:
        Trie();
        void insert();
        void search();
    private:
        trienode* root;
};