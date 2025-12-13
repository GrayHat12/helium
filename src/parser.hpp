#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <optional>
#include <variant>

#include "./arena.hpp"
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
        struct Expression;
        struct Operation
        {
            Expression *left_hand;
            Token oprator;
            Expression *right_hand;
        };
        struct Term
        {
            std::variant<IntLiteral *, Identifier *> term;
        };
        struct Expression
        {
            std::variant<Term *, Operation *> expression;
        };
    };
    namespace Statement
    {
        struct Exit
        {
            Expression::Expression *expression;
        };
        struct Let
        {
            Token identifier;
            Expression::Expression *expression;
        };
        struct Statement
        {
            std::variant<Exit *, Let *> statement;
        };
    };
    struct Program
    {
        std::vector<Statement::Statement *> stmts;
    };
}

class Parser
{
public:
    inline explicit Parser(const std::vector<Token> &tokens) : m_tokens(std::move(tokens)), m_allocator(1024 * 1024 * 4)
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
    std::optional<Node::Expression::Term *> parse_term()
    {
        if (peek().has_value() && peek().value().type == TokenType::INT_LT)
        {
            auto node_expression_int_lit = m_allocator.alloc<Node::Expression::IntLiteral>();
            node_expression_int_lit->int_lit = consume().value();
            auto node_expression = m_allocator.alloc<Node::Expression::Term>();
            node_expression->term = node_expression_int_lit;
            return node_expression;
        }
        else if (peek().has_value() && peek().value().type == TokenType::IDENT)
        {
            auto node_expression_identifier = m_allocator.alloc<Node::Expression::Identifier>();
            node_expression_identifier->ident = consume().value();
            auto node_expression = m_allocator.alloc<Node::Expression::Term>();
            node_expression->term = node_expression_identifier;
            return node_expression;
        }
        else
        {
            return {};
        }
    }
    std::optional<Node::Expression::Expression *> parse_expression()
    {
        if (auto term = parse_term())
        {
            auto node_expression = m_allocator.alloc<Node::Expression::Expression>();
            if (peek().has_value() && peek().value().type == TokenType::OPERATOR)
            {
                auto left_hand = m_allocator.alloc<Node::Expression::Expression>();
                left_hand->expression = term.value();
                auto operation_expression = m_allocator.alloc<Node::Expression::Operation>();
                auto oprator = consume();
                if (auto rhs = parse_expression())
                {
                    operation_expression->left_hand = left_hand;
                    operation_expression->oprator = oprator.value();
                    operation_expression->right_hand = rhs.value();

                    node_expression->expression = operation_expression;
                }
                else
                {
                    std::cerr << "did ya lose some nuts ya weeb" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                node_expression->expression = term.value();
            }
            return node_expression;
        }
        else
        {
            return {};
        }
    }

    std::optional<Node::Statement::Exit *> parse_exit()
    {
        std::optional<Node::Statement::Exit *> op_exit_node;
        // std::cout << peek().value().type << " : " << peek().value().value.value_or("") << std::endl;
        if (peek().value().type == TokenType::EXIT && peek(1).has_value() && peek(1).value().type == TokenType::OPEN_PAREN)
        {
            consume();
            consume();
            if (auto node_expr = parse_expression())
            {
                // exit_node = Node::Statement::Exit{.expression = node_expr.value()};
                auto exit_node = m_allocator.alloc<Node::Statement::Exit>();
                exit_node->expression = node_expr.value();
                op_exit_node = exit_node;
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

        return op_exit_node;
    }

    std::optional<Node::Expression::Operation *> parse_operation()
    {
        // std::optional<Node::Expression::Operation *> op_operation_node = {};
        // std::cout << peek().value().type << " : " << peek().value().value.value_or("") << std::endl;
        if (auto lhs = parse_expression())
        {
            auto operation_expression = m_allocator.alloc<Node::Expression::Operation>();
            if (peek().has_value() && peek().value().type == TokenType::OPERATOR)
            {
                auto oprator = consume();
                if (auto rhs = parse_expression())
                {
                    operation_expression->left_hand = lhs.value();
                    operation_expression->oprator = oprator.value();
                    operation_expression->right_hand = rhs.value();
                    return operation_expression;
                }
                else
                {
                    std::cerr << "did ya lose some nuts ya weeb" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                std::cerr << "ya wat is this expression ya dankey" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            return {};
        }
    }

    std::optional<Node::Statement::Let *> parse_let()
    {
        std::optional<Node::Statement::Let *> op_let_node = {};
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
                auto let_node = m_allocator.alloc<Node::Statement::Let>();
                let_node->identifier = ident;
                let_node->expression = node_expr.value();
                op_let_node = let_node;
                // Node::Statement::Let{.identifier = ident, .expression = node_expr.value()};
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

        return op_let_node;
    }

    std::optional<Node::Statement::Statement *> parse_statement()
    {
        if (auto exit_node = parse_exit())
        {
            auto node_statement = m_allocator.alloc<Node::Statement::Statement>();
            node_statement->statement = exit_node.value();
            return node_statement;
        }
        else if (auto let_node = parse_let())
        {
            auto node_statement = m_allocator.alloc<Node::Statement::Statement>();
            node_statement->statement = let_node.value();
            return node_statement;
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
    ArenaAllocator m_allocator;
};