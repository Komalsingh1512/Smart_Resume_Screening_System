#include "screening_system.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

using namespace std;

namespace {
string readLine(const string& prompt) {
    cout << prompt;
    string value;
    getline(cin, value);
    return value;
}

int readInt(const string& prompt, int minimum = 0) {
    while (true) {
        string value = readLine(prompt);
        try {
            size_t used = 0;
            int number = stoi(value, &used);
            if (used == value.size() && number >= minimum) return number;
        } catch (...) {}
        cout << "Please enter a number of at least " << minimum << ".\n";
    }
}

vector<string> commaSeparated(const string& text) {
    vector<string> result;
    stringstream input(text);
    string item;
    while (getline(input, item, ',')) {
        const auto first = item.find_first_not_of(" \t");
        const auto last = item.find_last_not_of(" \t");
        if (first != string::npos) result.push_back(item.substr(first, last - first + 1));
    }
    return result;
}

void showResume(const Resume& resume) {
    cout << "\nID: " << resume.id << " | " << resume.name << " | " << resume.email
              << "\nExperience: " << resume.experienceYears << " years | Education: " << resume.education
              << "\nSkills: ";
    if (resume.skills.empty()) cout << "No known skills detected";
    else for (size_t i = 0; i < resume.skills.size(); ++i)
        cout << (i ? ", " : "") << resume.skills[i];
    cout << "\n";
}

Job readJob() {
    Job job;
    job.title = readLine("Job title: ");
    job.minimumExperience = readInt("Minimum experience (years): ");
    job.requiredSkills = commaSeparated(readLine("Required skills (comma-separated): "));
    return job;
}

void displayRanking(const vector<MatchResult>& results) {
    if (results.empty()) { cout << "No resumes available.\n"; return; }
    cout << "\nRank  ID    Candidate                 Exp     Score\n"
              << "------------------------------------------------------\n";
    for (size_t i = 0; i < results.size(); ++i)
        cout << left << setw(6) << i + 1 << setw(6) << results[i].candidate->id
                  << setw(26) << results[i].candidate->name.substr(0, 24)
                  << setw(8) << results[i].candidate->experienceYears
                  << fixed << setprecision(2) << results[i].score << "%\n";
}
}

int main() {
    ScreeningSystem system;
    cout << "=======================================\n"
              << "   SMART RESUME SCREENING SYSTEM\n"
              << "   Hashing + Trie + Ranking\n"
              << "=======================================\n";

    while (true) {
        cout << "\n1. Add resume\n2. View all resumes\n3. Find resume by ID\n"
                     "4. Remove resume\n5. Load resumes from file\n6. Save resumes to file\n"
                     "7. Match and rank candidates\n8. Generate shortlist report\n0. Exit\n";
        const int choice = readInt("Choose an option: ");
        if (choice == 0) break;

        if (choice == 1) {
            const string name = readLine("Candidate name: ");
            const string email = readLine("Email: ");
            const int years = readInt("Experience (years): ");
            const string education = readLine("Education: ");
            const string content = readLine("Resume text: ");
            cout << "Resume stored with ID "
                      << system.addResume(name, email, years, education, content) << ".\n";
        } else if (choice == 2) {
            const auto resumes = system.allResumes();
            if (resumes.empty()) cout << "No resumes available.\n";
            for (const auto& resume : resumes) showResume(resume);
        } else if (choice == 3) {
            const Resume* resume = system.findResume(readInt("Resume ID: ", 1));
            if (resume) showResume(*resume); else cout << "Resume not found.\n";
        } else if (choice == 4) {
            cout << (system.removeResume(readInt("Resume ID: ", 1)) ? "Resume removed.\n" : "Resume not found.\n");
        } else if (choice == 5 || choice == 6) {
            const string filename = readLine("File path: ");
            string error;
            const bool ok = choice == 5 ? system.loadResumes(filename, error) : system.saveResumes(filename, error);
            cout << (ok ? "Operation completed.\n" : "Error: " + error + "\n");
        } else if (choice == 7) {
            displayRanking(system.rankCandidates(readJob()));
        } else if (choice == 8) {
            Job job = readJob();
            auto results = system.rankCandidates(job);
            displayRanking(results);
            const size_t limit = static_cast<size_t>(readInt("Number of candidates to shortlist: ", 1));
            const string filename = readLine("Report filename: ");
            string error;
            cout << (system.writeReport(job, results, filename, limit, error)
                          ? "Shortlist report generated.\n" : "Error: " + error + "\n");
        } else {
            cout << "Unknown option.\n";
        }
    }
    cout << "Goodbye!\n";
}
