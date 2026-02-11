#include "options.h"
#include <iostream>
#include <sstream>

// Helper to escape JSON strings
static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    out += "\"";
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (static_cast<unsigned char>(c) < 0x20) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\\u%04x", c);
            out += buf;
        }
        else out += c;
    }
    out += "\"";
    return out;
}

// Helper to consume whitespace and expected chars
static void skip_ws(std::istream& is) {
    while (isspace(is.peek())) is.get();
}

static bool consume(std::istream& is, char expected) {
    skip_ws(is);
    if (is.peek() == expected) {
        is.get();
        return true;
    }
    return false;
}

static std::string parse_string(std::istream& is) {
    std::string out;
    skip_ws(is);
    if (is.peek() != '"') return "";
    is.get(); // skip opening quote
    while (is.good()) {
        char c = is.get();
        if (c == '"') break;
        if (c == '\\') {
            c = is.get();
            if (c == '"') out += '"';
            else if (c == '\\') out += '\\';
            else if (c == 'b') out += '\b';
            else if (c == 'f') out += '\f';
            else if (c == 'n') out += '\n';
            else if (c == 'r') out += '\r';
            else if (c == 't') out += '\t';
            else out += c; // loose fallthrough
        } else {
            out += c;
        }
    }
    return out;
}

static int parse_int(std::istream& is) {
    skip_ws(is);
    int sign = 1;
    if (is.peek() == '-') { sign = -1; is.get(); }
    else if (is.peek() == '+') { is.get(); }
    int v = 0;
    while (isdigit(is.peek())) {
        v = v * 10 + (is.get() - '0');
    }
    return sign * v;
}

static int64_t parse_int64(std::istream& is) {
    skip_ws(is);
    int64_t sign = 1;
    if (is.peek() == '-') { sign = -1; is.get(); }
    else if (is.peek() == '+') { is.get(); }
    int64_t v = 0;
    while (isdigit(is.peek())) {
        v = v * 10 + (is.get() - '0');
    }
    return sign * v;
}

static bool parse_bool(std::istream& is) {
    std::string s;
    skip_ws(is);
    while (isalpha(is.peek())) s += (char)is.get();
    return s == "true";
}

// --- Options Implementation ---

void Options::to_json(std::ostream& os) const {
    os << "{\n";
    os << "  \"games\": " << games << ",\n";
    os << "  \"rounds\": " << rounds << ",\n";
    os << "  \"concurrency\": " << concurrency << ",\n";
    os << "  \"boardSize\": " << boardSize << ",\n";
    os << "  \"openings\": " << json_escape(openings) << ",\n";
    os << "  \"gameRule\": " << (int)gameRule << ",\n";
    os << "  \"openingType\": " << (int)openingType << ",\n";
    os << "  \"drawCount\": " << drawCount << ",\n";
    os << "  \"drawScore\": " << drawScore << ",\n";
    os << "  \"resignCount\": " << resignCount << ",\n";
    os << "  \"resignScore\": " << resignScore << ",\n";
    os << "  \"forceDrawAfter\": " << forceDrawAfter << ",\n";
    os << "  \"random\": " << (random ? "true" : "false") << ",\n";
    os << "  \"useTURN\": " << (useTURN ? "true" : "false") << ",\n";
    os << "  \"pgn\": " << json_escape(pgn) << ",\n";
    os << "  \"sgf\": " << json_escape(sgf) << ",\n";
    os << "  \"msg\": " << json_escape(msg) << ",\n";
    os << "  \"fatalError\": " << (fatalError ? "true" : "false") << "\n";
    os << "}";
}

void Options::from_json(std::istream& is) {
    skip_ws(is);
    if (is.get() != '{') return;
    
    while (is.good()) {
        skip_ws(is);
        if (is.peek() == '}') { is.get(); break; }
        
        std::string key = parse_string(is);
        consume(is, ':');

        if (key == "games") games = parse_int(is);
        else if (key == "rounds") rounds = parse_int(is);
        else if (key == "concurrency") concurrency = parse_int(is);
        else if (key == "boardSize") boardSize = parse_int(is);
        else if (key == "openings") openings = parse_string(is);
        else if (key == "gameRule") { int v = parse_int(is); gameRule = (GameRule)v; }
        else if (key == "openingType") { int v = parse_int(is); openingType = (OpeningType)v; }
        else if (key == "drawCount") drawCount = parse_int(is);
        else if (key == "drawScore") drawScore = parse_int(is);
        else if (key == "resignCount") resignCount = parse_int(is);
        else if (key == "resignScore") resignScore = parse_int(is);
        else if (key == "forceDrawAfter") forceDrawAfter = parse_int(is);
        else if (key == "random") random = parse_bool(is);
        else if (key == "useTURN") useTURN = parse_bool(is);
        else if (key == "pgn") pgn = parse_string(is);
        else if (key == "sgf") sgf = parse_string(is);
        else if (key == "msg") msg = parse_string(is);
        else if (key == "fatalError") fatalError = parse_bool(is);
        else {
            Options::skip_json_value(is);
        }
        consume(is, ',');
    }
}

// --- EngineOptions Implementation ---

void EngineOptions::to_json(std::ostream& os) const {
    os << "{\n";
    os << "  \"name\": " << json_escape(name) << ",\n";
    os << "  \"cmd\": " << json_escape(cmd) << ",\n";
    os << "  \"timeoutTurn\": " << timeoutTurn << ",\n";
    os << "  \"timeoutMatch\": " << timeoutMatch << ",\n";
    os << "  \"increment\": " << increment << ",\n";
    os << "  \"nodes\": " << nodes << ",\n";
    os << "  \"depth\": " << depth << ",\n";
    os << "  \"numThreads\": " << numThreads << ",\n";
    os << "  \"maxMemory\": " << maxMemory << ",\n";
    os << "  \"tolerance\": " << tolerance << ",\n";
    
    os << "  \"options\": [";
    for (size_t i = 0; i < options.size(); i++) {
        os << json_escape(options[i]);
        if (i < options.size() - 1) os << ", ";
    }
    os << "]\n";
    os << "}";
}

void EngineOptions::from_json(std::istream& is) {
    skip_ws(is);
    if (is.get() != '{') return;
    
    while (is.good()) {
        skip_ws(is);
        if (is.peek() == '}') { is.get(); break; }
        
        std::string key = parse_string(is);
        consume(is, ':');
        
        if (key == "name") name = parse_string(is);
        else if (key == "cmd") cmd = parse_string(is);
        else if (key == "timeoutTurn") timeoutTurn = parse_int64(is);
        else if (key == "timeoutMatch") timeoutMatch = parse_int64(is);
        else if (key == "increment") increment = parse_int64(is);
        else if (key == "nodes") nodes = parse_int64(is);
        else if (key == "depth") depth = parse_int(is);
        else if (key == "numThreads") numThreads = parse_int(is);
        else if (key == "maxMemory") maxMemory = parse_int64(is);
        else if (key == "tolerance") tolerance = parse_int64(is);
        else if (key == "options") {
            options.clear();
            skip_ws(is);
            if (is.get() == '[') {
                while (is.good()) {
                    skip_ws(is);
                    if (is.peek() == ']') { is.get(); break; }
                    options.push_back(parse_string(is));
                    consume(is, ',');
                }
            }
        }
        else {
             Options::skip_json_value(is);
        }
        consume(is, ',');
    }
}

void Options::skip_json_value(std::istream& is) {
    skip_ws(is);
    char c = is.peek();
    if (c == '"') {
        parse_string(is);
    } else if (c == '{') {
        is.get();
        while (is.good()) {
            skip_ws(is);
            if (is.peek() == '}') { is.get(); break; }
            parse_string(is); // key
            consume(is, ':');
            Options::skip_json_value(is); // value
            consume(is, ',');
        }
    } else if (c == '[') {
        is.get();
        while (is.good()) {
            skip_ws(is);
            if (is.peek() == ']') { is.get(); break; }
            Options::skip_json_value(is);
            consume(is, ',');
        }
    } else {
        // Primitive: number, bool, null
        // Consume valid chars until delimiter
        while (is.good()) {
            char next = is.peek();
            if (isspace(next) || next == ',' || next == '}' || next == ']') break;
            is.get();
        }
    }
}
