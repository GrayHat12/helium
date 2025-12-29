#pragma once
#include <algorithm>
#include <cassert>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

enum class TokenType {
    EXIT,
    LET,
    INT_LT,
    SEMICL,
    EQUALS,
    IDENT,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPERATOR,
    OPEN_CURLY,
    CLOSE_CURLY,
    IF,
    ELSE,
    MUTABLE,
    PRINT,
    STR_LIT,
    DINV_COMMA,
};

const std::vector<std::pair<std::string, std::string>> Comments = {
    { "//", "\n" },
    { "/*", "*/" },
};

const std::vector Operators {
    '+',
    '-',
    '/',
    '*',
    // '%',
};

const std::unordered_map<std::string, size_t> OperatorPrecedence {
    { "+", 0 },
    { "-", 0 },
    { "/", 1 },
    { "*", 1 },
};

template <typename T>
std::ostream& operator<<(std::enable_if_t<std::is_enum_v<T>, std::ostream>& stream, const T& e)
{
    return stream << static_cast<std::underlying_type_t<T>>(e);
}

struct Token {
    TokenType type;
    std::optional<std::string> value;
    std::pair<size_t, size_t> position;
    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        out << "Token{.type=" << type << ", .value=" << value.value_or("nil") << "}";
        return out;
    }
};

inline std::optional<size_t> bin_precedence(const Token& token)
{
    if (token.type == TokenType::OPERATOR) {
        if (OperatorPrecedence.contains(token.value.value())) {
            return OperatorPrecedence.at(token.value.value());
        }
        assert(false); // not supported;
    }
    return {};
}

class Tokenizer {
public:
    explicit Tokenizer(std::string src)
        : m_src(std::move(src))
    {
    }

    std::vector<Token> tokenize()
    {
        m_index = 0;
        std::string buffer;
        std::vector<Token> tokens;
        while (peek().has_value()) {
            // std::cout << "At " << m_index << " Char " << peek().value() << std::endl;
            if (std::isalpha(peek().value())) {
                buffer.push_back(consume().value());
                while (peek().has_value() && std::isalnum(peek().value())) {
                    buffer.push_back(consume().value());
                }
                if (buffer == "exit") {
                    tokens.push_back({ .type = TokenType::EXIT, .position = { m_lineno, m_colno } });
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                if (buffer == "let") {
                    tokens.push_back({ .type = TokenType::LET, .position = { m_lineno, m_colno } });
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                if (buffer == "mut") {
                    tokens.push_back({ .type = TokenType::MUTABLE, .position = { m_lineno, m_colno } });
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                if (buffer == "print") {
                    tokens.push_back({ .type = TokenType::PRINT, .position = { m_lineno, m_colno } });
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                if (buffer == "if") {
                    tokens.push_back({ .type = TokenType::IF, .position = { m_lineno, m_colno } });
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                if (buffer == "else") {
                    tokens.push_back({ .type = TokenType::ELSE, .position = { m_lineno, m_colno } });
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                tokens.push_back({ .type = TokenType::IDENT, .value = buffer, .position = { m_lineno, m_colno } });
                buffer.clear();
                continue;
                // std::cerr << "ya messed up bitches" << std::endl;
                // exit(EXIT_FAILURE);
            }
            {
                auto foundComments = false;
                for (const auto& comment : Comments) {
                    bool hasStart = true;
                    for (size_t i = 0; i < comment.first.length(); i++) {
                        if (peek(i).has_value() && peek(i).value() != comment.first.at(i)) {
                            hasStart = false;
                        }
                    }
                    if (hasStart) {
                        foundComments = true;
                        // comment started

                        // consume comment starting item
                        // ReSharper disable once CppDFAUnreadVariable
                        for (auto _ : comment.first) {
                            consume();
                        }
                        while (true) {
                            bool isComment = false;
                            for (size_t i = 0; i < comment.second.length(); i++) {
                                if (peek(i).has_value() && peek(i).value() != comment.second.at(i)) {
                                    isComment = true;
                                }
                            }
                            if (isComment) {
                                consume();
                                if (!peek().has_value()) {
                                    // std::cerr << "close ya comment ya bitch " << current_position().str() <<
                                    // std::endl; exit(EXIT_FAILURE);
                                    break;
                                }
                            }
                            else {
                                // ReSharper disable once CppDFAUnreadVariable
                                for (auto _ : comment.second) {
                                    consume();
                                }
                                break;
                            }
                        }
                    }
                }
                if (foundComments) {
                    continue;
                }
            }
            if (std::isdigit(peek().value())) {
                buffer.push_back(consume().value());
                while (peek().has_value() && std::isdigit(peek().value())) {
                    buffer.push_back(consume().value());
                }
                tokens.push_back({ .type = TokenType::INT_LT, .value = buffer, .position = { m_lineno, m_colno } });
                // std::cout << "Got INT_LT " << buffer << std::endl;
                buffer.clear();
                continue;
            }
            if (peek().value() == '(') {
                consume();
                tokens.push_back({ .type = TokenType::OPEN_PAREN, .position = { m_lineno, m_colno } });
                continue;
            }
            if (peek().value() == ')') {
                consume();
                tokens.push_back({ .type = TokenType::CLOSE_PAREN, .position = { m_lineno, m_colno } });
                continue;
            }
            if (peek().value() == '{') {
                consume();
                tokens.push_back({ .type = TokenType::OPEN_CURLY, .position = { m_lineno, m_colno } });
                continue;
            }
            if (peek().value() == '}') {
                consume();
                tokens.push_back({ .type = TokenType::CLOSE_CURLY, .position = { m_lineno, m_colno } });
                continue;
            }
            if (peek().value() == '"') {
                // std::cout << "inside string" << std::endl;
                auto prevChar = consume();
                tokens.push_back({ .type = TokenType::DINV_COMMA, .position = { m_lineno, m_colno } });
                while (peek().has_value()) {
                    auto currChar = consume().value();
                    if (currChar == '"' && prevChar == '\\') {
                        buffer.push_back(currChar);
                    }
                    else if (currChar == '"') {
                        break;
                    }
                    else {
                        buffer.push_back(currChar);
                        prevChar = currChar;
                    }
                }
                tokens.push_back({ .type = TokenType::STR_LIT, .value = buffer, .position = { m_lineno, m_colno } });
                // std::cout << "consumed string " << buffer << " peek=" << peek().value_or('-') << std::endl;
                buffer.clear();
                tokens.push_back({ .type = TokenType::DINV_COMMA, .position = { m_lineno, m_colno } });
                continue;
            }
            if (peek().value() == '=') {
                consume();
                // std::cout << "Got SEMICL " << buffer << std::endl;
                tokens.push_back({ .type = TokenType::EQUALS, .position = { m_lineno, m_colno } });
                continue;
            }
            if (std::ranges::find(Operators, peek().value()) != Operators.end()) {
                tokens.push_back(
                    {
                        .type = TokenType::OPERATOR,
                        .value = std::string { consume().value() },
                        .position = { m_lineno, m_colno },
                    });
                continue;
            }
            if (peek().value() == ';') {
                consume();
                // std::cout << "Got SEMICL " << buffer << std::endl;
                tokens.push_back({ .type = TokenType::SEMICL, .position = { m_lineno, m_colno } });
                continue;
            }
            if (std::isspace(peek().value())) {
                consume();
                continue;
            }
            std::cerr << "ye or me messed up ya savagez" << current_position().str() << std::endl;
            exit(EXIT_FAILURE);
        }
        return tokens;
    }

private:
    [[nodiscard]] std::optional<char> peek(const size_t ahead = 0) const
    {
        if (m_index + ahead >= m_src.length()) {
            return {};
        }
        return m_src.at(m_index + ahead);
    }

    std::stringstream current_position(std::string prefix = "error at ")
    {
        std::stringstream out;
        out << prefix << m_lineno << ":" << m_colno;
        return out;
    }

    std::optional<char> consume()
    {
        if (m_index >= m_src.length()) {
            return {};
        }
        auto c = m_src.at(m_index++);
        if (c == '\n') {
            m_lineno += 1;
            m_colno = 1;
        }
        else {
            m_colno += 1;
        }
        return c;
    }

    const std::string m_src;
    size_t m_index = 0;
    size_t m_lineno = 1;
    size_t m_colno = 1;
};