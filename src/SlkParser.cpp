#include "SlkParser.h"
#include <fstream>
#include <map>
#include <algorithm>
#include <cmath>

namespace BeatUp {
namespace {
int Integer(const std::string& s) {
    size_t used = 0;
    int value = std::stoi(s, &used);
    if (used != s.size()) throw std::invalid_argument("Invalid chart integer");
    return value;
}
bool Lane(int lane) { return lane == 7 || lane == 4 || lane == 1 || lane == 9 || lane == 6 || lane == 3 || lane == 5; }
}
bool SlkParser::LoadChart(const std::wstring& filePath, std::vector<ChartNote>& outNotes, double bpm, bool allowEmpty) {
    if (!std::isfinite(bpm) || bpm <= 0) return false;
    std::ifstream file(filePath, std::ios::binary);
    if (!file) return false;
    std::map<int, std::map<int, std::string>> table;
    std::vector<ChartNote> notes;
    int x = 1, y = 1;
    std::string line;
    try {
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.compare(0, 2, "C;") != 0) continue;
            // Delimiters inside quoted values are literal; double quotes escape a quote.
            std::vector<std::string> fields;
            std::string field;
            bool quoted = false;
            for (size_t i = 2; i <= line.size(); ++i) {
                char ch = i == line.size() ? ';' : line[i];
                if (ch == '"') {
                    if (quoted && i+1 < line.size() && line[i+1] == '"') { field += ch; field += ch; ++i; continue; }
                    quoted = !quoted;
                }
                if (ch == ';' && !quoted) { fields.push_back(field); field.clear(); }
                else field += ch;
            }
            if (quoted) return false;
            for (const auto& f : fields) {
                if (f.size() > 1 && f[0] == 'X') x = Integer(f.substr(1));
                if (f.size() > 1 && f[0] == 'Y') y = Integer(f.substr(1));
            }
            if (x < 1 || y < 1) return false;
            for (const auto& f : fields) {
                if (f.empty() || f[0] != 'K') continue;
                std::string value = f.substr(1);
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size()-2);
                    for (size_t i = 0; (i = value.find("\"\"", i)) != std::string::npos; ++i) value.erase(i, 1);
                }
                table[y][x] = value;
            }
        }
        bool milliseconds = table[1][1] == "TIME" && table[1][2] == "LANE";
        // Official charts also use Korean labels; the typed schema is stable.
        bool audition = table[1][6] == "Key" &&
            ((table[1][4] == "Location" && table[1][5] == "Type") ||
             (table[2][4] == "int" && table[2][5] == "enum(n,s,f)" && table[2][6] == "string"));
        if (!milliseconds && !audition) return false;
        for (const auto& [row, cols] : table) {
            if (row < (milliseconds ? 2 : 3)) continue;
            int timeColumn = milliseconds ? 1 : 4;
            auto time = cols.find(timeColumn);
            if (time == cols.end()) continue;
            ChartNote note;
            int location = Integer(time->second);
            if (location < 0) return false;
            note.timeMs = milliseconds ? location : location * (60000.0 / (bpm * 4.0));
            note.tick = milliseconds ? -1.0 : location;
            auto type = cols.find(milliseconds ? 3 : 5);
            if (milliseconds) note.type = type == cols.end() ? 0 : Integer(type->second);
            else {
                const std::string kind = type == cols.end() ? "n" : type->second;
                if (kind.empty() || kind.size() % 2 == 0) return false;
                for (size_t i = 0; i < kind.size(); ++i) {
                    if (i % 2 == 1) { if (kind[i] != ',') return false; }
                    else if (kind[i] != 'n' && kind[i] != 's' && kind[i] != 'f') return false;
                }
                note.finish = kind.find('f') != std::string::npos;
                if (kind.find('n') != std::string::npos) {
                    auto key = cols.find(6);
                    if (key == cols.end() || key->second.empty()) return false;
                    note.lane = Integer(key->second);
                    if (!Lane(note.lane) || note.lane == 5) return false;
                    notes.push_back(note);
                }
                if (kind.find('s') != std::string::npos) {
                    note.lane = 5;
                    note.type = 1;
                    notes.push_back(note);
                } else if (kind == "f") {
                    note.lane = 0; // Timeline event, not a key to judge.
                    note.type = 2;
                    notes.push_back(note);
                }
                continue;
            }
            auto key = cols.find(milliseconds ? 2 : 6);
            note.lane = key == cols.end() || key->second.empty() ? 5 : Integer(key->second);
            if (!Lane(note.lane) || note.type < 0 || note.type > 2) return false;
            notes.push_back(note);
        }
    } catch (const std::exception&) { return false; }
    if (notes.empty() && !allowEmpty) return false;
    std::stable_sort(notes.begin(), notes.end(), [](const ChartNote& a, const ChartNote& b) { return a.timeMs < b.timeMs; });
    outNotes = std::move(notes);
    return true;
}
} // namespace BeatUp
