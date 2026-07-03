#ifndef TRIE_H
#define TRIE_H
#include <string>
#include <memory>
#include <array>

constexpr int MAX_CHAR = 256;

class Trie {
    public:
        Trie() = default; //generate default constructor
        void insert(const std::string &text); //insertion method
        std::string complete(const std::string &prefix) const; //completion checking
    private:
        typedef struct trienode { //trienode struct
        std::array<std::unique_ptr<trienode>, MAX_CHAR> children{}; //initialised an empty arry of points to trienodes
        bool terminal = false; //whether this is the last char
        } trienode;
        std::unique_ptr<trienode> root; //root node
};

#endif