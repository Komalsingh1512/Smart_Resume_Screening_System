#include "screening_system.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

using namespace std;

namespace {
ScreeningSystem systemData;

string jsonEscape(const string& value) {
    string result;
    for (char c : value) {
        if (c == '"') result += "\\\"";
        else if (c == '\\') result += "\\\\";
        else if (c == '\n') result += "\\n";
        else if (c == '\r') result += "\\r";
        else if (c == '\t') result += "\\t";
        else result += c;
    }
    return result;
}

string urlDecode(const string& value) {
    string result;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') result += ' ';
        else if (value[i] == '%' && i + 2 < value.size()) {
            try { result += static_cast<char>(stoi(value.substr(i + 1, 2), nullptr, 16)); i += 2; }
            catch (...) { result += value[i]; }
        } else result += value[i];
    }
    return result;
}

map<string, string> parseForm(const string& text) {
    map<string, string> fields;
    stringstream input(text);
    string pair;
    while (getline(input, pair, '&')) {
        size_t equals = pair.find('=');
        fields[urlDecode(pair.substr(0, equals))] =
            equals == string::npos ? "" : urlDecode(pair.substr(equals + 1));
    }
    return fields;
}

vector<string> splitComma(const string& value) {
    vector<string> result;
    stringstream input(value);
    string item;
    while (getline(input, item, ',')) if (!item.empty()) result.push_back(item);
    return result;
}

string resumesJson() {
    const auto resumes = systemData.allResumes();
    ostringstream out;
    out << "{\"count\":" << resumes.size() << ",\"resumes\":[";
    for (size_t i = 0; i < resumes.size(); ++i) {
        const Resume& r = resumes[i];
        if (i) out << ',';
        out << "{\"id\":" << r.id << ",\"name\":\"" << jsonEscape(r.name)
            << "\",\"email\":\"" << jsonEscape(r.email) << "\",\"experience\":"
            << r.experienceYears << ",\"education\":\"" << jsonEscape(r.education)
            << "\",\"skills\":[";
        for (size_t j = 0; j < r.skills.size(); ++j) {
            if (j) out << ',';
            out << '"' << jsonEscape(r.skills[j]) << '"';
        }
        out << "]}";
    }
    out << "]}";
    return out.str();
}

string rankingJson(const Job& job) {
    const auto results = systemData.rankCandidates(job);
    ostringstream out;
    out << "{\"job\":\"" << jsonEscape(job.title) << "\",\"results\":[";
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        if (i) out << ',';
        out << "{\"rank\":" << i + 1 << ",\"id\":" << r.candidate->id
            << ",\"name\":\"" << jsonEscape(r.candidate->name) << "\",\"email\":\""
            << jsonEscape(r.candidate->email) << "\",\"experience\":"
            << r.candidate->experienceYears << ",\"score\":" << r.score << ",\"matched\":[";
        for (size_t j = 0; j < r.matchedSkills.size(); ++j) {
            if (j) out << ',';
            out << '"' << jsonEscape(r.matchedSkills[j]) << '"';
        }
        out << "],\"missing\":[";
        for (size_t j = 0; j < r.missingSkills.size(); ++j) {
            if (j) out << ',';
            out << '"' << jsonEscape(r.missingSkills[j]) << '"';
        }
        out << "]}";
    }
    out << "]}";
    return out.str();
}

string readFile(const string& path) {
    ifstream input(path, ios::binary);
    if (!input) return {};
    return string((istreambuf_iterator<char>(input)), istreambuf_iterator<char>());
}

bool hasSuffix(const string& value, const string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void closeSocket(SOCKET socketValue) {
#ifdef _WIN32
    closesocket(socketValue);
#else
    close(socketValue);
#endif
}

void respond(SOCKET client, int status, const string& type, const string& body) {
    const string label = status == 200 ? "OK" : status == 201 ? "Created" :
                         status == 404 ? "Not Found" : "Bad Request";
    ostringstream response;
    response << "HTTP/1.1 " << status << ' ' << label << "\r\nContent-Type: " << type
             << "\r\nContent-Length: " << body.size()
             << "\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n" << body;
    const string data = response.str();
    send(client, data.c_str(), static_cast<int>(data.size()), 0);
}

void handleClient(SOCKET client) {
    string request;
    char buffer[4096];
    int received;
    size_t requiredSize = string::npos;
    while ((received = recv(client, buffer, sizeof(buffer), 0)) > 0) {
        request.append(buffer, received);
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != string::npos && requiredSize == string::npos) {
            size_t contentLength = 0;
            const string marker = "Content-Length:";
            size_t position = request.find(marker);
            if (position != string::npos) contentLength = stoul(request.substr(position + marker.size()));
            requiredSize = headerEnd + 4 + contentLength;
        }
        if (requiredSize != string::npos && request.size() >= requiredSize) break;
    }

    size_t firstSpace = request.find(' ');
    size_t secondSpace = request.find(' ', firstSpace + 1);
    if (firstSpace == string::npos || secondSpace == string::npos) {
        respond(client, 400, "application/json", "{\"error\":\"Invalid request\"}"); return;
    }
    const string method = request.substr(0, firstSpace);
    string path = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
    const size_t headerEnd = request.find("\r\n\r\n");
    const string body = headerEnd == string::npos ? "" : request.substr(headerEnd + 4);
    const size_t queryAt = path.find('?');
    const map<string, string> query = parseForm(queryAt == string::npos ? "" : path.substr(queryAt + 1));
    if (queryAt != string::npos) path = path.substr(0, queryAt);

    if (method == "GET" && path == "/api/resumes") {
        respond(client, 200, "application/json", resumesJson());
    } else if (method == "POST" && path == "/api/resumes") {
        const auto f = parseForm(body);
        try {
            int id = systemData.addResume(f.at("name"), f.at("email"), stoi(f.at("experience")),
                                          f.at("education"), f.at("content"));
            respond(client, 201, "application/json", "{\"success\":true,\"id\":" + to_string(id) + "}");
        } catch (...) { respond(client, 400, "application/json", "{\"error\":\"Please complete every field\"}"); }
    } else if (method == "DELETE" && path == "/api/resumes") {
        try {
            bool removed = systemData.removeResume(stoi(query.at("id")));
            respond(client, removed ? 200 : 404, "application/json", removed ? "{\"success\":true}" : "{\"error\":\"Not found\"}");
        } catch (...) { respond(client, 400, "application/json", "{\"error\":\"Invalid ID\"}"); }
    } else if (method == "POST" && path == "/api/rank") {
        const auto f = parseForm(body);
        try {
            Job job{f.at("title"), stoi(f.at("experience")), splitComma(f.at("skills"))};
            respond(client, 200, "application/json", rankingJson(job));
        } catch (...) { respond(client, 400, "application/json", "{\"error\":\"Invalid job details\"}"); }
    } else {
        string filePath = path == "/" ? "web/index.html" : "web" + path;
        string content = readFile(filePath);
        if (content.empty()) respond(client, 404, "text/plain", "Not found");
        else {
            string type = hasSuffix(path, ".css") ? "text/css" :
                          hasSuffix(path, ".js") ? "application/javascript" : "text/html";
            respond(client, 200, type, content);
        }
    }
}
}

int main() {
#ifdef _WIN32
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) { cerr << "Could not start networking.\n"; return 1; }
#endif
    string error;
    systemData.loadResumes("data/sample_resumes.txt", error);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(8080);
    if (serverSocket == INVALID_SOCKET || bind(serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR ||
        listen(serverSocket, 10) == SOCKET_ERROR) {
        cerr << "Could not start server. Port 8080 may already be in use.\n";
        return 1;
    }
    cout << "Smart Resume Screening web app is running.\n"
         << "Open http://localhost:8080 in your browser.\n"
         << "Press Ctrl+C to stop the server.\n";
    while (true) {
        SOCKET client = accept(serverSocket, nullptr, nullptr);
        if (client != INVALID_SOCKET) { handleClient(client); closeSocket(client); }
    }
}
