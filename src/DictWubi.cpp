//
// Created by zhangfuwen on 2022/1/22.
//

#include "DictWubi.h"
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <pulse/simple.h>
#include <string>
#include <thread>
#include <vector>

volatile bool g_wubi86_table = false;
volatile bool g_wubi98_table = false;
Wubi::Wubi(std::string tablePath) {
    m_trieRoot = NewNode();
    std::string s1;
    s1.reserve(256);
    bool has_began = false;

    for (std::ifstream f2(tablePath); getline(f2, s1);) {
        if (s1 == "BEGIN_TABLE") {
            has_began = true;
            continue;
        }
        if (!has_began) {
            continue;
        }
        if (s1 == "END_TABLE") {
            continue;
        }
        auto first_space = s1.find_first_of(" \t");
        std::string key = s1.substr(0, first_space);
        s1 = s1.substr(first_space + 1);
        auto second_space = s1.find_first_of('\t');
        std::string value = s1.substr(0, second_space);
        std::string freq_str = s1.substr(second_space + 1);
        uint64_t freq = stoll(freq_str);
        TrieInsert(m_trieRoot, key, value, freq);
        if (m_searchCode) {
            m_wubiCodeSearcher.insert({value, key});
        }
    }
}

TrieNode *Wubi::Search(const std::string &key) { return TrieSearch(m_trieRoot, key); }
void Wubi::TrieTraversal(std::map<uint64_t, std::string> &m) { SubTreeTraversal(m, m_trieRoot); }

std::string Wubi::CodeSearch(const std::string &text) {
    if (m_wubiCodeSearcher.count(text)) {
        return m_wubiCodeSearcher[text];
    }

    return "";
}

Wubi::~Wubi() {
    TrieTraverse(m_trieRoot, [](struct TrieNode *node) { delete node; });
    m_wubiCodeSearcher.clear();
}
struct TrieNode *NewNode() {
    auto pNode = new TrieNode;

    pNode->isEndOfWord = false;

    for (auto &i : pNode->children)
        i = nullptr;

    return pNode;
} // If not present, inserts key into trie
// If the key is prefix of trie node, just
// marks leaf node
void TrieInsert(struct TrieNode *root, const std::string &key, const std::string &value, uint64_t freq) {
    struct TrieNode *pCrawl = root;

    for (char i : key) {
        int index = i - 'a';
        if (!pCrawl->children[index])
            pCrawl->children[index] = NewNode();

        pCrawl = pCrawl->children[index];
    }

    // mark last node as leaf
    pCrawl->isEndOfWord = true;
    pCrawl->word = key;
    pCrawl->values.insert({freq, value});
}
void TrieTraverse(struct TrieNode *root, void (*fn)(struct TrieNode *node)) {
    if (root == nullptr) {
        return;
    }
    for (auto &node : root->children) {
        if (node != nullptr) {
            TrieTraverse(node, fn);
        }
    }
    fn(root);
}
TrieNode *TrieSearch(struct TrieNode *root, const std::string &key) {
    if (root == nullptr) {
        return nullptr;
    }
    struct TrieNode *pCrawl = root;

    for (char i : key) {
        int index = i - 'a';
        if (!pCrawl->children[index])
            return nullptr;

        pCrawl = pCrawl->children[index];
    }

    return pCrawl;
}
void SubTreeTraversal(std::map<uint64_t, std::string> &m, struct TrieNode *root) {
    if (root == nullptr) {
        return;
    }
    for (auto &index : root->children) {
        if (index && index->isEndOfWord) {
            for (const auto &pair : index->values) {
                auto freq = pair.first;
                auto value = pair.second;
                m.insert({freq, value});
            }
        }
        SubTreeTraversal(m, index);
    }
}
