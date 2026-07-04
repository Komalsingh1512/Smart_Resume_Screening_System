#pragma once

#include "models.hpp"
#include "trie.hpp"

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class ScreeningSystem {
    unordered_map<int, Resume> resumes_;
    SkillTrie skillTrie_;
    int nextId_{1};

    static string trim(const string& value);
    static vector<string> split(const string& value, char delimiter);

public:
    ScreeningSystem();
    int addResume(string name, string email, int experience,
                  string education, string content);
    bool removeResume(int id);
    const Resume* findResume(int id) const;
    vector<Resume> allResumes() const;
    vector<MatchResult> rankCandidates(const Job& job) const;
    bool loadResumes(const string& filename, string& error);
    bool saveResumes(const string& filename, string& error) const;
    bool writeReport(const Job& job, const vector<MatchResult>& results,
                     const string& filename, size_t limit, string& error) const;
};
