#include "screening_system.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

using namespace std;

namespace {
string join(const vector<string>& values, const string& separator) {
    ostringstream output;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) output << separator;
        output << values[i];
    }
    return output.str();
}
}

ScreeningSystem::ScreeningSystem() {
    const vector<string> skills = {
        "c", "c++", "c#", "java", "python", "javascript", "typescript", "html", "css",
        "react", "angular", "node.js", "sql", "mysql", "mongodb", "git", "docker",
        "kubernetes", "aws", "azure", "linux", "data structures", "algorithms",
        "machine learning", "communication", "leadership", "problem solving"
    };
    for (const auto& skill : skills) skillTrie_.insert(skill);
}

string ScreeningSystem::trim(const string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

vector<string> ScreeningSystem::split(const string& value, char delimiter) {
    vector<string> parts;
    stringstream stream(value);
    string part;
    while (getline(stream, part, delimiter)) parts.push_back(trim(part));
    return parts;
}

int ScreeningSystem::addResume(string name, string email, int experience,
                               string education, string content) {
    Resume resume{nextId_++, trim(name), trim(email), max(0, experience),
                  trim(education), trim(content), {}};
    auto extracted = skillTrie_.extract(resume.content);
    set<string> unique(extracted.begin(), extracted.end());
    resume.skills.assign(unique.begin(), unique.end());
    const int id = resume.id;
    resumes_[id] = move(resume);
    return id;
}

bool ScreeningSystem::removeResume(int id) { return resumes_.erase(id) > 0; }

const Resume* ScreeningSystem::findResume(int id) const {
    const auto it = resumes_.find(id);
    return it == resumes_.end() ? nullptr : &it->second;
}

vector<Resume> ScreeningSystem::allResumes() const {
    vector<Resume> result;
    for (const auto& entry : resumes_) result.push_back(entry.second);
    sort(result.begin(), result.end(), [](const Resume& a, const Resume& b) { return a.id < b.id; });
    return result;
}

vector<MatchResult> ScreeningSystem::rankCandidates(const Job& job) const {
    vector<MatchResult> results;
    set<string> required;
    for (const auto& item : job.requiredSkills) {
        auto parsed = skillTrie_.extract(item);
        if (parsed.empty()) required.insert(trim(item));
        else required.insert(parsed.begin(), parsed.end());
    }

    for (const auto& entry : resumes_) {
        const Resume& resume = entry.second;
        set<string> candidateSkills(resume.skills.begin(), resume.skills.end());
        MatchResult result;
        result.candidate = &resume;
        for (const auto& skill : required) {
            if (candidateSkills.count(skill)) result.matchedSkills.push_back(skill);
            else result.missingSkills.push_back(skill);
        }
        const double skillScore = required.empty() ? 100.0 :
            100.0 * static_cast<double>(result.matchedSkills.size()) / required.size();
        const double experienceScore = job.minimumExperience <= 0 ? 100.0 :
            min(100.0, 100.0 * resume.experienceYears / job.minimumExperience);
        result.score = round((0.8 * skillScore + 0.2 * experienceScore) * 100.0) / 100.0;
        results.push_back(move(result));
    }
    sort(results.begin(), results.end(), [](const MatchResult& a, const MatchResult& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.candidate->experienceYears != b.candidate->experienceYears)
            return a.candidate->experienceYears > b.candidate->experienceYears;
        return a.candidate->name < b.candidate->name;
    });
    return results;
}

bool ScreeningSystem::loadResumes(const string& filename, string& error) {
    ifstream input(filename);
    if (!input) { error = "Could not open " + filename; return false; }
    string line;
    int lineNumber = 0;
    while (getline(input, line)) {
        ++lineNumber;
        if (trim(line).empty() || line[0] == '#') continue;
        auto fields = split(line, '|');
        if (fields.size() != 5) {
            error = "Invalid format on line " + to_string(lineNumber) + " (expected 5 fields).";
            return false;
        }
        try {
            addResume(fields[0], fields[1], stoi(fields[2]), fields[3], fields[4]);
        } catch (...) {
            error = "Invalid experience value on line " + to_string(lineNumber) + ".";
            return false;
        }
    }
    return true;
}

bool ScreeningSystem::saveResumes(const string& filename, string& error) const {
    ofstream output(filename);
    if (!output) { error = "Could not write " + filename; return false; }
    output << "# name|email|experience years|education|resume text\n";
    for (const auto& resume : allResumes())
        output << resume.name << '|' << resume.email << '|' << resume.experienceYears << '|'
               << resume.education << '|' << resume.content << '\n';
    return true;
}

bool ScreeningSystem::writeReport(const Job& job, const vector<MatchResult>& results,
                                  const string& filename, size_t limit, string& error) const {
    ofstream output(filename);
    if (!output) { error = "Could not write " + filename; return false; }
    output << "SMART RESUME SCREENING - SHORTLIST REPORT\n"
           << "Job: " << job.title << "\nRequired skills: " << join(job.requiredSkills, ", ")
           << "\nMinimum experience: " << job.minimumExperience << " years\n\n";
    limit = min(limit, results.size());
    for (size_t i = 0; i < limit; ++i) {
        const auto& result = results[i];
        output << i + 1 << ". " << result.candidate->name << " (ID " << result.candidate->id << ")\n"
               << "   Score: " << fixed << setprecision(2) << result.score << "%\n"
               << "   Email: " << result.candidate->email << "\n"
               << "   Experience: " << result.candidate->experienceYears << " years\n"
               << "   Matched: " << (result.matchedSkills.empty() ? "None" : join(result.matchedSkills, ", ")) << "\n"
               << "   Missing: " << (result.missingSkills.empty() ? "None" : join(result.missingSkills, ", ")) << "\n\n";
    }
    return true;
}
