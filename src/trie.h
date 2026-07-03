#ifndef TRIE_H
#define TRIE_H
#include <string>
#include <memory>
#include <array>


class Trie {
    public:
        Trie() = default;
        void insert(const std::string &text);
        std::string complete(const std::string &prefix) const;
    private:
        typedef struct trienode {
        std::array<std::unique_ptr<trienode>, 256> children{};
        bool terminal = false; //whether this is the last char
        } trienode;
        std::unique_ptr<trienode> root;
};

#endif