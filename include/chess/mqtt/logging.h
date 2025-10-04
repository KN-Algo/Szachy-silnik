// logging.hpp
#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

// ENV: LOG_PAYLOAD_PREVIEW (domyślnie 256) | LOG_LEVEL (0=ERROR,1=WARN,2=INFO,3=DEBUG)
inline size_t log_preview_limit() {
    const char* v = std::getenv("LOG_PAYLOAD_PREVIEW");
if (!v) return 256;
try { return static_cast<size_t>(std::stoul(v)); }
catch (...) { return 256; }
}
inline int log_level() {
    const char* v = std::getenv("LOG_LEVEL");
if (!v) return 3; // INFO
try { return std::stoi(v); }
catch (...) { return 3; }
}

inline std::string preview(const std::string& s, size_t maxlen = log_preview_limit())
{
    if (s.size() <= maxlen) return s;
    std::ostringstream oss;
    oss << s.substr(0, maxlen) << "...(+" << (s.size() - maxlen) << "B)";
    return oss.str();
}

#define LOG_E(msg) do { if (log_level() >= 0) std::cerr << "[ERROR]" << msg << "\n"; } while(0)
#define LOG_W(msg) do { if (log_level() >= 1) std::cerr << "[WARN ]" << msg << "\n"; } while(0)
#define LOG_I(msg) do { if (log_level() >= 2) std::cout << "[INFO ]" << msg << "\n"; } while(0)
#define LOG_D(msg) do { if (log_level() >= 3) std::cout << "[DEBUG]" << msg << "\n"; } while(0)

inline std::string yesno(bool b) { return b ? "true" : "false"; }
