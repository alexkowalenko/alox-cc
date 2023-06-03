//
// ALOX-CC
//

#include <iostream>
#include <map>

#include <fmt/core.h>
#include <string>
#include <unicode/uchar.h>

#include "scanner.hh"
#include "utf8/checked.h"

namespace alox {

inline constexpr auto debug_scan{false};
template <typename S, typename... Args>
static void debug(const S &format, const Args &...msg) {
    if constexpr (debug_scan) {
        std::cout << "scanner: " << fmt::format(fmt::runtime(format), msg...) << '\n';
    }
}

Scanner::Scanner(const std::string &s) : source(s), current(begin(source)) {
    debug(source);
}

constexpr bool is_emoji(Char c) {
    return (0x1f600 <= c && c <= 0x1f64f) || // Emoticons NOLINT
           (0x1F300 <= c && c <= 0x1F5FF) || // Misc Symbols and Pictographs NOLINT
           (0x1F680 <= c && c <= 0x1F6FF) || // Transport and Map NOLINT
           (0x1F1E6 <= c && c <= 0x1F1FF) || // Regional country flags NOLINT
           (0x2600 <= c && c <= 0x26FF) ||   // Misc symbols NOLINT
           (0x2700 <= c && c <= 0x27BF) ||   // Dingbats NOLINT
           (0xE0020 <= c && c <= 0xE007F) || // Tags NOLINT
           (0xFE00 <= c && c <= 0xFE0F) ||   // Variation Selectors NOLINT
           (0x1F900 <= c &&
            c <= 0x1F9FF) || // Supplemental Symbols and Pictographs NOLINT
           (0x1F018 <= c && c <= 0x1F270) || // Various asian characters NOLINT
           (0x238C <= c && c <= 0x2454) ||   // Misc items NOLINT
           (0x20D0 <= c && c <= 0x20FF);     // NOLINT
}

const std::map<Char, TokenType> single_tokens = {
    {'+', TokenType::PLUS},       {'-', TokenType::MINUS},
    {'*', TokenType::ASTÉRIX},    {'/', TokenType::SLASH},
    {'(', TokenType::LEFT_PAREN}, {')', TokenType::RIGHT_PAREN},
    {'{', TokenType::LEFT_BRACE}, {'}', TokenType::RIGHT_BRACE},
    {';', TokenType::SEMICOLON},  {',', TokenType::COMMA},
    {'.', TokenType::DOT}};

const std::map<std::string, TokenType> keyword_map = {
    {"and", TokenType::AND},     {"break", TokenType::BREAK},
    {"class", TokenType::CLASS}, {"continue", TokenType::CONTINUE},
    {"else", TokenType::ELSE},   {"false", TokenType::FALSE},
    {"for", TokenType::FOR},     {"fun", TokenType::FUN},
    {"if", TokenType::IF},       {"nil", TokenType::NIL},
    {"or", TokenType::OR},       {"return", TokenType::RETURN},
    {"print", TokenType::PRINT}, {"super", TokenType::SUPER},
    {"this", TokenType::THIS},   {"true", TokenType::TRUE},
    {"var", TokenType::VAR},     {"while", TokenType::WHILE},
};

Token Scanner::error_token(const char *message) const {
    Token token{};
    token.type = TokenType::ERROR;
    token.text = {message, strlen(message)};
    token.line = line;
    return token;
}

Char Scanner::get_char() {
    do {
        auto c = scan();
        if (c == 0) {
            return c;
        }
        if (c == '\n') {
            line++;
            continue;
        }
        if (u_isspace(c)) {
            continue;
        }
        if (c == '/' && peek() == '/') {
            do {
                c = scan();
            } while (c != '\n' && c != 0);
            if (c == '\n') {
                line++;
            }
            continue;
        }
        return c;
    } while (true);
};

// expecting strings with the " character - might comeback and take these off.
auto Scanner::get_string() -> Token {
    std::string buf;
    while (peek() != 0) {
        auto c = scan();
        if (c == '"') {
            return {TokenType::STRING, buf, line};
        }
        utf8::append(char32_t(c), buf);
    }
    return error_token("Unterminated string.");
}

auto Scanner::get_number(Char c) -> Token {
    std::string buf;
    utf8::append(char32_t(c), buf);
    auto p = peek();
    auto seen_dot{false};
    while (u_isdigit(p) || p == '.') {
        if (p == '.') {
            if (seen_dot) {
                break;
            }
            seen_dot = true;
        }
        utf8::append(char32_t(scan()), buf);
        p = peek();
    }
    if (buf.ends_with(".")) {
        prior();
        return {TokenType::NUMBER, buf.substr(0, buf.size() - 1), line};
    }
    return {TokenType::NUMBER, buf, line};
}

auto Scanner::get_identifier(Char c) -> Token {
    std::string buf;
    utf8::append(char32_t(c), buf);
    auto p = peek();
    while (u_isalnum(p) || is_emoji(p) || p == '_') {
        utf8::append(char32_t(scan()), buf);
        p = peek();
    }
    if (keyword_map.contains(buf)) {
        return {keyword_map.at(buf), buf, line};
    }
    return {TokenType::IDENTIFIER, buf, line};
}

Token Scanner::scanToken() {
    auto c = get_char();
    // debug("get_token :{0}: ", char(c));
    if (c == 0) {
        return {TokenType::EOFS, "", line};
    }
    if (single_tokens.contains(c)) {
        return {single_tokens.at(c), {static_cast<char>(c)}, line};
    }

    auto next = peek();
    switch (c) {
    case '!': {
        if (next == '=') {
            get_char();
            return {TokenType::BANG_EQUAL, "!=", line};
        }
        return {TokenType::BANG, "!", line};
    }
    case '=': {
        if (next == '=') {
            get_char();
            return {TokenType::EQUAL_EQUAL, "==", line};
        }
        return {TokenType::EQUAL, "=", line};
    }
    case '>': {
        if (next == '=') {
            get_char();
            return {TokenType::GREATER_EQUAL, ">=", line};
        }
        return {TokenType::GREATER, ">", line};
    }
    case '<': {
        if (next == '=') {
            get_char();
            return {TokenType::LESS_EQUAL, "<=", line};
        }
        return {TokenType::LESS, "<", line};
    }
    }

    if (c == '"') {
        return get_string();
    }
    if (u_isdigit(c)) {
        return get_number(c);
    }
    if (u_isalpha(c) || is_emoji(c) || c == '_') {
        return get_identifier(c);
    }

    return error_token("Unexpected character.");
}

} // namespace lox