#pragma once

#include <string>
#include <vector>

using namespace std;

struct Resume {
    int id{};
    string name;
    string email;
    int experienceYears{};
    string education;
    string content;
    vector<string> skills;
};

struct Job {
    string title;
    int minimumExperience{};
    vector<string> requiredSkills;
};

struct MatchResult {
    const Resume* candidate{};
    double score{};
    vector<string> matchedSkills;
    vector<string> missingSkills;
};
