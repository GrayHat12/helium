#pragma once
#include <iostream>
#include <variant>
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
            std::stringstream to_string()
            {
                std::stringstream out;
                out << "IntLiteral{.int_lit=" << int_lit.to_string().str() << "}";
                return out;
            }
        };
        struct Identifier
        {
            Token ident;
            std::stringstream to_string()
            {
                std::stringstream out;
                out << "Identifier{.int_lit=" << ident.to_string().str() << "}";
                return out;
            }
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
            std::stringstream to_string()
            {
                std::stringstream out;
                if (std::holds_alternative<IntLiteral *>(term))
                {
                    out << "Term{.expression=" << std::get<IntLiteral *>(term)->to_string().str() << "}";
                }
                else if (std::holds_alternative<Identifier *>(term))
                {
                    out << "Term{.expression=" << std::get<Identifier *>(term)->to_string().str() << "}";
                }
                return out;
            }
        };
        struct Expression
        {
            std::variant<Term *, Operation *> expression;
            inline std::stringstream to_string() const
            {
                std::stringstream out;
                if (std::holds_alternative<Operation *>(expression))
                {
                    Operation *operation = std::get<Operation *>(expression);
                    std::stringstream operationout;
                    operationout << "Operation{.left=" << operation->left_hand->to_string().str() << ", .operator=" << operation->oprator.to_string().str() << ", .right=" << operation->right_hand->to_string().str() << "}";
                    out << "Expression{.expression=" << operationout.str() << "}";
                    // out << "Expression{.expression=" << operation_to_string(std::get<Operation *>(expression)).str() << "}";
                }
                else if (std::holds_alternative<Term *>(expression))
                {
                    out << "Expression{.expression=" << std::get<Term *>(expression)->to_string().str() << "}";
                }
                return out;
            }
        };

        std::stringstream operation_to_string(Operation *operation)
        {
            std::stringstream out;
            out << "Operation{.left=" << operation->left_hand->to_string().str() << ", .operator=" << operation->oprator.to_string().str() << ", .right=" << operation->right_hand->to_string().str() << "}";
            return out;
        }
    };
    namespace Statement
    {
        struct Exit
        {
            Expression::Expression *expression;
            std::stringstream to_string()
            {
                std::stringstream out;
                out << "Exit{.expression=" << expression->to_string().str() << "}";
                return out;
            }
        };
        struct Let
        {
            Token identifier;
            Expression::Expression *expression;
            std::stringstream to_string()
            {
                std::stringstream out;
                out << "Let{.identifier=" << identifier.to_string().str() << ", .expression=" << expression->to_string().str() << "}";
                return out;
            }
        };
        struct Statement
        {
            std::variant<Exit *, Let *> statement;
            std::stringstream to_string()
            {
                std::stringstream out;
                if (std::holds_alternative<Exit *>(statement))
                {
                    out << "Statement{.statement=" << std::get<Exit *>(statement)->to_string().str() << "}";
                }
                else if (std::holds_alternative<Let *>(statement))
                {
                    out << "Statement{.statement=" << std::get<Let *>(statement)->to_string().str() << "}";
                }
                return out;
            }
        };
    };
    struct Program
    {
        std::vector<Statement::Statement *> stmts;
        std::stringstream to_string()
        {
            std::stringstream out;
            out << "Program{.stmts=[";
            for (Statement::Statement *statmt : stmts)
            {
                out << statmt->to_string().str() << ", ";
            }
            out << "]}";
            return out;
        }
    };
}

Node::Expression::Operation *compute_precedence_tree(ArenaAllocator *allocator, const Node::Expression::Operation *operation)
{
    Node::Expression::Operation *balanced_operation = NULL;
    if (auto lhs = std::get_if<Node::Expression::Operation *>(&operation->left_hand->expression))
    {
        // std::cout << "got operation with lhs operation" << std::endl;

        auto balanced_lhs = compute_precedence_tree(allocator, *lhs);

        auto current_precedence = bin_precedence(operation->oprator).value();
        auto leftop_precedence = bin_precedence(balanced_lhs->oprator).value();

        if (leftop_precedence <= current_precedence)
        {
            // std::cout << "flipping tree lhs" << std::endl;
            auto newrootop = allocator->alloc<Node::Expression::Operation>();
            newrootop->left_hand = balanced_lhs->left_hand;
            newrootop->oprator = balanced_lhs->oprator;
            auto newright = allocator->alloc<Node::Expression::Operation>();
            newright->left_hand = balanced_lhs->right_hand;
            newright->oprator = operation->oprator;
            newright->right_hand = operation->right_hand;
            newrootop->right_hand = allocator->alloc<Node::Expression::Expression>();
            newrootop->right_hand->expression = newright;
            balanced_operation = newrootop;
            // balanced_lhs->left_hand = balanced_lhs->right_hand;
            // newrootop->right_hand->expression = balanced_lhs;

            // operation->left_hand->expression = (*lhs)->right_hand->expression;
            // (*lhs)->right_hand->expression = operation;
            // operation = (*lhs);
        }
        else
        {
            if (balanced_operation == NULL)
            {
                balanced_operation = allocator->alloc<Node::Expression::Operation>();
                balanced_operation->oprator = operation->oprator;
            }
            balanced_operation->left_hand = allocator->alloc<Node::Expression::Expression>();
            balanced_operation->left_hand->expression = balanced_lhs;
        }
    }
    else
    {
        if (balanced_operation == NULL)
        {
            balanced_operation = allocator->alloc<Node::Expression::Operation>();
            balanced_operation->oprator = operation->oprator;
        }
        balanced_operation->left_hand = allocator->alloc<Node::Expression::Expression>();
        balanced_operation->left_hand = operation->left_hand;
    }
    if (auto rhs = std::get_if<Node::Expression::Operation *>(&operation->right_hand->expression))
    {
        // std::cout << "got operation with rhs operation" << std::endl;
        auto balanced_rhs = compute_precedence_tree(allocator, *rhs);

        // std::cout << "balancedrhs=" << Node::Expression::operation_to_string(balanced_rhs).str() << std::endl;

        auto current_precedence = bin_precedence(operation->oprator).value();
        auto rightop_precedence = bin_precedence(balanced_rhs->oprator).value();
        if (rightop_precedence <= current_precedence)
        {
            // std::cout << "flipping tree rhs" << std::endl;
            auto newrootop = allocator->alloc<Node::Expression::Operation>();
            newrootop->oprator = balanced_rhs->oprator;
            newrootop->right_hand = balanced_rhs->right_hand;
            auto newleft = allocator->alloc<Node::Expression::Operation>();
            // newleft->left_hand = balanced_rhs->left_hand;
            newleft->left_hand = balanced_operation->left_hand;
            newleft->oprator = operation->oprator;
            newleft->right_hand = balanced_rhs->left_hand;
            // std::cout << "newleft=" << Node::Expression::operation_to_string(newleft).str() << std::endl;
            newrootop->left_hand = allocator->alloc<Node::Expression::Expression>();
            newrootop->left_hand->expression = newleft;
            // std::cout << "newrootop=" << Node::Expression::operation_to_string(newrootop).str() << std::endl;
            balanced_operation = newrootop;
            // std::cout << "balanced_operation=" << Node::Expression::operation_to_string(balanced_operation).str() << std::endl;
            // operation->right_hand->expression = (*rhs)->left_hand->expression;
            // std::cout << "l1" << std::endl;
            // (*rhs)->left_hand->expression = operation;
            // std::cout << "l2" << std::endl;
            // operation = (*rhs);
            // std::cout << "l3" << std::endl;
        }
        else
        {
            // std::cout << "not flipping tree rhs" << std::endl;
            if (balanced_operation == NULL)
            {
                assert(false); // should never happen
            }
            balanced_operation->right_hand = allocator->alloc<Node::Expression::Expression>();
            balanced_operation->right_hand->expression = balanced_rhs;
        }
        // std::cout << "l4" << std::endl;
    }
    else
    {
        if (balanced_operation == NULL)
        {
            // balanced_operation = allocator->alloc<Node::Expression::Operation>();
            // balanced_operation->oprator = operation->oprator;
            assert(false); // should never happen
        }
        balanced_operation->right_hand = allocator->alloc<Node::Expression::Expression>();
        balanced_operation->right_hand = operation->right_hand;
    }
    // std::cout << "Returning " << Node::Expression::operation_to_string(balanced_operation).str() << std::endl;
    return balanced_operation;
}

class Parser
{
public:
    inline explicit Parser(const std::vector<Token> &tokens, ArenaAllocator *allocator) : m_tokens(std::move(tokens)), m_allocator(allocator)
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
            auto node_expression_int_lit = m_allocator->alloc<Node::Expression::IntLiteral>();
            node_expression_int_lit->int_lit = consume().value();
            auto node_expression = m_allocator->alloc<Node::Expression::Term>();
            node_expression->term = node_expression_int_lit;
            return node_expression;
        }
        else if (peek().has_value() && peek().value().type == TokenType::IDENT)
        {
            auto node_expression_identifier = m_allocator->alloc<Node::Expression::Identifier>();
            node_expression_identifier->ident = consume().value();
            auto node_expression = m_allocator->alloc<Node::Expression::Term>();
            node_expression->term = node_expression_identifier;
            return node_expression;
        }
        else
        {
            return {};
        }
    }
    std::optional<Node::Expression::Expression *> parse_expression(bool compute_precedence = true)
    {
        if (auto term = parse_term())
        {
            auto node_expression = m_allocator->alloc<Node::Expression::Expression>();
            if (peek().has_value() && peek().value().type == TokenType::OPERATOR)
            {
                auto left_hand = m_allocator->alloc<Node::Expression::Expression>();
                left_hand->expression = term.value();
                auto operation_expression = m_allocator->alloc<Node::Expression::Operation>();
                auto oprator = consume();
                if (auto rhs = parse_expression(false))
                {
                    operation_expression->left_hand = left_hand;
                    operation_expression->oprator = oprator.value();
                    operation_expression->right_hand = rhs.value();

                    if (compute_precedence)
                    {
                        // std::cout << "Before reorder " << Node::Expression::operation_to_string(operation_expression).str() << std::endl;
                        node_expression->expression = compute_precedence_tree(m_allocator, operation_expression);
                        // std::cout << "After reorder " << node_expression->to_string().str() << std::endl;
                    }
                    else
                    {
                        node_expression->expression = operation_expression;
                    }
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
                auto exit_node = m_allocator->alloc<Node::Statement::Exit>();
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

    std::optional<Node::Statement::Let *> parse_let()
    {
        std::optional<Node::Statement::Let *> op_let_node = {};
        // std::cout << "let " << peek().value().type << " : " << peek().value().value.value_or("") << std::endl;
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
                auto let_node = m_allocator->alloc<Node::Statement::Let>();
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
            auto node_statement = m_allocator->alloc<Node::Statement::Statement>();
            node_statement->statement = exit_node.value();
            return node_statement;
        }
        else if (auto let_node = parse_let())
        {
            auto node_statement = m_allocator->alloc<Node::Statement::Statement>();
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
    ArenaAllocator *m_allocator;
};