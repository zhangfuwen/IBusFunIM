//
// Created by zhangfuwen on 2022/1/17.
//

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <map>

const int ALPHABET_SIZE = 26;

// trie node
struct TrieNode
{
    struct TrieNode *children[ALPHABET_SIZE];

    // isEndOfWord is true if the node represents
    // end of a word
    bool isEndOfWord;
    std::string word;
    std::string value;
    int freq;
};

// Returns new trie node (initialized to NULLs)
struct TrieNode *NewNode(void)
{
    struct TrieNode *pNode =  new TrieNode;

    pNode->isEndOfWord = false;

    for (int i = 0; i < ALPHABET_SIZE; i++)
        pNode->children[i] = NULL;

    return pNode;
}

// If not present, inserts key into trie
// If the key is prefix of trie node, just
// marks leaf node
void insert(struct TrieNode *root, std::string key, std::string value, int freq)
{
    struct TrieNode *pCrawl = root;

    for (int i = 0; i < key.length(); i++)
    {
        int index = key[i] - 'a';
        if (!pCrawl->children[index])
            pCrawl->children[index] = NewNode();

        pCrawl = pCrawl->children[index];
    }

    // mark last node as leaf
    pCrawl->isEndOfWord = true;
    pCrawl->word = key;
    pCrawl->value = value;
    pCrawl->freq = freq;

}

// Returns true if key presents in trie, else
// false
TrieNode* search(struct TrieNode *root, std::string key)
{
    struct TrieNode *pCrawl = root;

    for (int i = 0; i < key.length(); i++)
    {
        int index = key[i] - 'a';
        if (!pCrawl->children[index])
            return NULL;

        pCrawl = pCrawl->children[index];
    }

    return pCrawl;
}

void traversal(std::map<int, TrieNode*> &m, struct TrieNode *root) {
    if(root == NULL) {
        return;
    }
    if(root->isEndOfWord) {
        m.insert({root->freq, root});
    }
    for(int index = 0; index < ALPHABET_SIZE; index++) {
        traversal(m, root->children[index]);
    }
}


int main()
{
    TrieNode * root = NewNode();

    std::string s1;
    s1.reserve(256);
    bool has_began = false;

    for(std::ifstream f2("../wubi86.txt"); getline(f2,s1); ) {
        if(s1 == "BEGIN_TABLE") {
            has_began = true;
            continue;
        }
        if(!has_began) {
           continue;
        }
        if(s1 == "END_TABLE") {
            continue;
        }
        int first_space = s1.find_first_of(" \t");
        std::string key = s1.substr(0, first_space);
        s1 = s1.substr(first_space + 1);
        int second_space = s1.find_first_of("\t");
        std::string value = s1.substr(0, second_space);
        std::string freq_str = s1.substr(second_space+1);
        int freq = std::atoi(freq_str.c_str());
//        std::cout << key << value << freq<< std::endl;
        insert(root, key, value, freq);
    }

    // search and get candidates
    TrieNode * x = search(root, "wg");
    std::map<int, TrieNode *> m;
    traversal(m, x);
    for(auto it = m.rbegin(); it != m.rend(); it++) {
        auto node = it->second;
        std::cout << node->word << " " << node->value << " " << node->freq << std::endl;
    }
    std::cout << m.size() << std::endl;
    return 0;
}

