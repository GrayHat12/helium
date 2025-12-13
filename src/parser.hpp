#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <variant>

#include "./tokenization.hpp"

namespace Node
{
    namespace Expression
    {
        struct IntLiteral
        {
            Token int_lit;
        };
        struct Identifier
        {
            Token ident;
        };
        struct Expression
        {
            std::variant<IntLiteral, Identifier> expression;
        };
    };
    namespace Statement
    {
        struct Exit
        {
            Expression::Expression expression;
        };
        struct Let
        {
            Token identifier;
            Expression::Expression expression;
        };
        struct Statement
        {
            std::variant<Exit, Let> statement;
        };
    };
    struct Program
    {
        std::vector<Statement::Statement> stmts;
    };
}

class Parser
{
public:
    inline explicit Parser(const std::vector<Token> &tokens) : m_tokens(std::move(tokens))
    {
    }

    Node::Program parse()
    {
        m_index = 0;
        Node::Program program_node;
        while (peek().has_value())
        {
            if (auto statement = parse_statement())
            {
                program_node.stmts.push_back(statement.value());
            }
            else
            {
                std::cerr << "wat di statement ya twat" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return program_node;
    }

private:
    std::optional<Node::Expression::Expression> parse_expression()
    {
        if (peek().has_value() && peek().value().type == TokenType::INT_LT)
        {
            return Node::Expression::Expression{.expression = Node::Expression::IntLiteral{.int_lit = consume().value()}};
        }
        else if (peek().has_value() && peek().value().type == TokenType::IDENT)
        {
            return Node::Expression::Expression{.expression = Node::Expression::Identifier{.ident = consume().value()}};
        }
        else
        {
            return {};
        }
    }

    std::optional<Node::Statement::Exit> parse_exit()
    {
        std::optional<Node::Statement::Exit> exit_node = {};
        // std::cout << peek().value().type << " : " << peek().value().value.value_or("") << std::endl;
        if (peek().value().type == TokenType::EXIT && peek(1).has_value() && peek(1).value().type == TokenType::OPEN_PAREN)
        {
            consume();
            consume();
            if (auto node_expr = parse_expression())
            {
                exit_node = Node::Statement::Exit{.expression = node_expr.value()};
            }
            else
            {
                std::cerr << "ya messed up bitches" << std::endl;
                exit(EXIT_FAILURE);
            }
            // consume close paren
            if (!peek().has_value() || peek().value().type != TokenType::CLOSE_PAREN)
            {
                std::cerr << "ya messed up ya parenthesis twat" << std::endl;
                exit(EXIT_FAILURE);
            }
            else
            {
                consume();
            }

            // consume semicolon
            if (!peek().has_value() || peek().value().type != TokenType::SEMICL)
            {
                std::cerr << "ya messed up ya semicolon twat" << std::endl;
                exit(EXIT_FAILURE);
            }
            else
            {
                consume();
            }
        }

        return exit_node;
    }

    std::optional<Node::Statement::Let> parse_let()
    {
        std::optional<Node::Statement::Let> let_node = {};
        // std::cout << peek().value().type << " : " << peek().value().value.value_or("") << std::endl;
        if (peek().value().type == TokenType::LET &&
            peek(1).has_value() &&
            peek(1).value().type == TokenType::IDENT &&
            peek(2).has_value() &&
            peek(2).value().type == TokenType::EQUALS)
        {
            consume();
            Token ident = consume().value();
            consume();
            if (auto node_expr = parse_expression())
            {
                let_node = Node::Statement::Let{.identifier = ident, .expression = node_expr.value()};
            }
            else
            {
                std::cerr << "ya messed up bitches" << std::endl;
                exit(EXIT_FAILURE);
            }
            // consume semicolon
            if (!peek().has_value() || peek().value().type != TokenType::SEMICL)
            {
                std::cerr << "ya messed up ya semicolon twat" << std::endl;
                exit(EXIT_FAILURE);
            }
            else
            {
                consume();
            }
        }

        return let_node;
    }

    std::optional<Node::Statement::Statement> parse_statement()
    {
        if (auto exit_node = parse_exit())
        {
            return Node::Statement::Statement{.statement = exit_node.value()};
        }
        else if (auto let_node = parse_let())
        {
            return Node::Statement::Statement{.statement = let_node.value()};
        }
        else
        {
            return {};
            // std::cerr << "ya messed up wat ts shit" << std::endl;
            // exit(EXIT_FAILURE);
        }
    }

    [[nodiscard]] std::optional<Token> peek(int ahead = 0) const
    {
        if (m_index + ahead >= m_tokens.size())
        {
            return {};
        }
        else
        {
            return m_tokens.at(m_index + ahead);
        }
    }

    std::optional<Token> consume()
    {
        if (m_index >= m_tokens.size())
        {
            return {};
        }
        return m_tokens.at(m_index++);
    }
    const std::vector<Token> m_tokens;
    size_t m_index = 0;
};