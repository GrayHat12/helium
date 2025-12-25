#pragma once
#include <cassert>
#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <optional>
#include <unordered_map>

enum class TokenType
{
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
};

const std::vector Operators{
    '+',
    '-',
    '/',
    '*',
    // '%',
};

const std::unordered_map<std::string, size_t> OperatorPrecedence{
    {"+", 0},
    {"-", 0},
    {"/", 1},
    {"*", 1},
};

template <typename T>
std::ostream &operator<<(std::enable_if_t<std::is_enum_v<T>, std::ostream> &stream, const T &e)
{
    return stream << static_cast<std::underlying_type_t<T>>(e);
}

struct Token
{
    TokenType type;
    std::optional<std::string> value;
    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        out << "Token{.type=" << type << ", .value=" << value.value_or("nil") << "}";
        return out;
    }
};

inline std::optional<size_t> bin_precedence(const Token &token)
{
    if (token.type == TokenType::OPERATOR)
    {
        if (OperatorPrecedence.contains(token.value.value()))
        {
            return OperatorPrecedence.at(token.value.value());
        }
        assert(false); // not supported;
    }
    return {};
}

class Tokenizer
{
public:
    explicit Tokenizer(std::string src) : m_src(std::move(src))
    {
    }

    std::vector<Token> tokenize()
    {
        m_index = 0;
        std::string buffer;
        std::vector<Token> tokens;
        while (peek().has_value())
        {
            // std::cout << "At " << m_index << " Char " << peek().value() << std::endl;
            if (std::isalpha(peek().value()))
            {
                buffer.push_back(consume().value());
                while (peek().has_value() && std::isalnum(peek().value()))
                {
                    buffer.push_back(consume().value());
                }
                if (buffer == "exit")
                {
                    tokens.push_back({.type = TokenType::EXIT});
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                if (buffer == "let")
                {
                    tokens.push_back({.type = TokenType::LET});
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                if (buffer == "if")
                {
                    tokens.push_back({.type = TokenType::IF});
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                tokens.push_back({.type = TokenType::IDENT, .value = buffer});
                buffer.clear();
                continue;
                // std::cerr << "ya messed up bitches" << std::endl;
                // exit(EXIT_FAILURE);
            }
            if (std::isdigit(peek().value()))
            {
                buffer.push_back(consume().value());
                while (peek().has_value() && std::isdigit(peek().value()))
                {
                    buffer.push_back(consume().value());
                }
                tokens.push_back({.type = TokenType::INT_LT, .value = buffer});
                // std::cout << "Got INT_LT " << buffer << std::endl;
                buffer.clear();
                continue;
            }
            if (peek().value() == '(')
            {
                consume();
                tokens.push_back({.type = TokenType::OPEN_PAREN});
                continue;
            }
            if (peek().value() == ')')
            {
                consume();
                tokens.push_back({.type = TokenType::CLOSE_PAREN});
                continue;
            }
            if (peek().value() == '{')
            {
                consume();
                tokens.push_back({.type = TokenType::OPEN_CURLY});
                continue;
            }
            if (peek().value() == '}')
            {
                consume();
                tokens.push_back({.type = TokenType::CLOSE_CURLY});
                continue;
            }
            if (peek().value() == '=')
            {
                consume();
                // std::cout << "Got SEMICL " << buffer << std::endl;
                tokens.push_back({.type = TokenType::EQUALS});
                continue;
            }
            if (std::ranges::find(Operators, peek().value()) != Operators.end())
            {
                tokens.push_back({.type = TokenType::OPERATOR, .value = std::string{consume().value()}});
                continue;
            }
            if (peek().value() == ';')
            {
                consume();
                // std::cout << "Got SEMICL " << buffer << std::endl;
                tokens.push_back({.type = TokenType::SEMICL});
                continue;
            }
            if (std::isspace(peek().value()))
            {
                consume();
                continue;
            }
            std::cerr << "ye or me messed up ya savagez" << std::endl;
            exit(EXIT_FAILURE);
        }
        return tokens;
    }

private:
    [[nodiscard]] std::optional<char> peek(const size_t ahead = 0) const
    {
        if (m_index + ahead >= m_src.length())
        {
            return {};
        }
        return m_src.at(m_index + ahead);
    }

    std::optional<char> consume()
    {
        if (m_index >= m_src.length())
        {
            return {};
        }
        return m_src.at(m_index++);
    }

    const std::string m_src;
    size_t m_index = 0;
};