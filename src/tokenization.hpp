#pragma once
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <optional>

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
};

const std::vector<char> Operators{
    '+',
    '-',
    '/',
    '*',
    '%',
};

template <typename T>
std::ostream &operator<<(typename std::enable_if<std::is_enum<T>::value, std::ostream>::type &stream, const T &e)
{
    return stream << static_cast<typename std::underlying_type<T>::type>(e);
}

struct Token
{
    TokenType type;
    std::optional<std::string> value;
    std::stringstream to_string() const
    {
        std::stringstream out;
        out << "Token{.type=" << type << ", .value=" << value.value_or("nil") << "}";
        return out;
    }
};

class Tokenizer
{
public:
    inline explicit Tokenizer(const std::string &src) : m_src(std::move(src))
    {
    }

    inline std::vector<Token> tokenize()
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
                else if (buffer == "let")
                {
                    tokens.push_back({.type = TokenType::LET});
                    // std::cout << "Got Exit " << buffer << std::endl;
                    buffer.clear();
                    continue;
                }
                else
                {
                    tokens.push_back({.type = TokenType::IDENT, .value = buffer});
                    buffer.clear();
                    continue;
                    // std::cerr << "ya messed up bitches" << std::endl;
                    // exit(EXIT_FAILURE);
                }
            }
            else if (std::isdigit(peek().value()))
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
            else if (peek().value() == '(')
            {
                consume();
                tokens.push_back({.type = TokenType::OPEN_PAREN});
                continue;
            }
            else if (peek().value() == ')')
            {
                consume();
                tokens.push_back({.type = TokenType::CLOSE_PAREN});
                continue;
            }
            else if (peek().value() == '=')
            {
                consume();
                // std::cout << "Got SEMICL " << buffer << std::endl;
                tokens.push_back({.type = TokenType::EQUALS});
                continue;
            }
            else if (std::find(Operators.begin(), Operators.end(), peek().value()) != Operators.end())
            {
                tokens.push_back({.type = TokenType::OPERATOR, .value = std::string{consume().value()}});
                continue;
            }
            else if (peek().value() == ';')
            {
                consume();
                // std::cout << "Got SEMICL " << buffer << std::endl;
                tokens.push_back({.type = TokenType::SEMICL});
                continue;
            }
            else if (std::isspace(peek().value()))
            {
                consume();
                continue;
            }
            else
            {
                std::cerr << "ye or me messed up ya savagez" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return tokens;
    }

private:
    [[nodiscard]] std::optional<char> peek(int ahead = 0) const
    {
        if (m_index + ahead >= m_src.length())
        {
            return {};
        }
        else
        {
            return m_src.at(m_index + ahead);
        }
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