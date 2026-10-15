#include "SongInfo.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <cmath>

namespace BeatUp {
namespace {
std::string Trim(const std::string& s) {
    auto a = s.find_first_not_of(" \t\r\n");
    return a == std::string::npos ? "" : s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}
std::wstring FromUtf8(const std::string& s) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (n <= 0) return {};
    std::wstring out(n, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.size()), out.data(), n);
    return out;
}
}
SongInfo ReadSongInfo(const std::wstring& chartPath, double fallbackBpm) {
    namespace fs = std::filesystem;
    fs::path chart(chartPath), sidecar(chart);
    sidecar.replace_extension(L".song.ini");
    SongInfo info;
    info.title = chart.stem().wstring();
    info.bpm = fallbackBpm;
    std::ifstream file(sidecar, std::ios::binary);
    std::string line;
    while (std::getline(file, line)) {
        if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        auto key = Trim(line.substr(0, eq)), value = Trim(line.substr(eq + 1));
        if (key == "title" || key == "artist" || key == "audio") {
            auto wide = FromUtf8(value);
            if (wide.empty()) continue;
            if (key == "title") info.title = wide;
            if (key == "artist") info.artist = wide;
            if (key == "audio") info.audioPath = (chart.parent_path() / fs::path(wide)).wstring();
        } else if (key == "bpm" || key == "offset_ms" || key == "audition_offset") {
            try {
                size_t used = 0;
                double number = std::stod(value, &used);
                if (used != value.size() || !std::isfinite(number)) continue;
                if (key == "bpm" && number >= 30.0 && number <= 400.0) info.bpm = number;
                if (key == "offset_ms") info.offsetMs = number;
                if (key == "audition_offset") { info.auditionOffset=number; info.hasAuditionOffset=true; }
            } catch (...) { /* Keep defaults for malformed optional fields. */ }
        }
    }
    return info;
}
}
