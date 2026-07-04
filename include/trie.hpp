#pragma once

#include <array>
#include <cctype>
#include <memory>
#include <string>
#include <vector>

using namespace std;

class SkillTrie {
    struct Node {
        array<unique_ptr<Node>, 128> children{};
        bool terminal{false};
        string skill;
    };
    Node root_;

    static string lower(string value) {
        for (char& c : value) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        return value;
    }

public:
    void insert(const string& skill) {
        Node* current = &root_;
        for (unsigned char c : lower(skill)) {
            if (c >= 128) continue;
            if (!current->children[c]) current->children[c] = make_unique<Node>();
            current = current->children[c].get();
        }
        current->terminal = true;
        current->skill = lower(skill);
    }

    vector<string> extract(const string& text) const {
        const string input = lower(text);
        vector<string> found;
        for (size_t start = 0; start < input.size(); ++start) {
            if (start > 0 && isalnum(static_cast<unsigned char>(input[start - 1]))) continue;
            const Node* current = &root_;
            string longestMatch;
            for (size_t i = start; i < input.size(); ++i) {
                const unsigned char c = static_cast<unsigned char>(input[i]);
                if (c >= 128 || !current->children[c]) break;
                current = current->children[c].get();
                if (current->terminal) {
                    const bool boundary = i + 1 == input.size() ||
                        !isalnum(static_cast<unsigned char>(input[i + 1]));
                    if (boundary) longestMatch = current->skill;
                }
            }
            if (!longestMatch.empty()) found.push_back(longestMatch);
        }
        return found;
    }
};
