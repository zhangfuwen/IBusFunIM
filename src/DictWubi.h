//
// Created by zhangfuwen on 2022/1/22.
//

#ifndef AUDIO_IME_DICTWUBI_H
#define AUDIO_IME_DICTWUBI_H
#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include "common.h"

extern volatile bool g_wubi86_table;
extern volatile bool g_wubi98_table;
const int ALPHABET_SIZE = 26; // trie node
struct TrieNode {
    struct TrieNode *children[ALPHABET_SIZE] = {};

    // isEndOfWord is true if the node represents
    // end of a word
    bool isEndOfWord = false;
    std::string word;
    std::map<uint64_t, std::string> values = {};
};

class Wubi {
  public:
    Wubi(std::string tablePath);
    TrieNode *Search(const std::string &key);
    void TrieTraversal(std::map<uint64_t, std::string> &m);
    std::string CodeSearch(const std::string &text);
    ~Wubi();

  private:
    TrieNode *m_trieRoot = nullptr;
    std::unordered_map<std::string, std::string> m_wubiCodeSearcher; // chinese to zigen table
    bool m_searchCode = true;
    std::vector<CandidateAttr> m_searchResult;
}; // Returns new trie node (initialized to nullptrs)

struct TrieNode *NewNode();
void TrieInsert(struct TrieNode *root, const std::string &key, const std::string &value, uint64_t freq);
void TrieTraverse(struct TrieNode *root, void (*fn)(struct TrieNode *node));
TrieNode *TrieSearch(struct TrieNode *root, const std::string &key);
void SubTreeTraversal(std::map<uint64_t, std::string> &m, struct TrieNode *root);

#endif // AUDIO_IME_DICTWUBI_H
