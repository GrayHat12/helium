#pragma once
#include <iostream>
#include <variant>
#include <string>
#include <vector>
#include <optional>

#include "./arena.hpp"
#include "./tokenization.hpp"

namespace Node
{

    namespace Statement
    {
        struct Statement;
    }
    struct Scope
    {
        std::vector<Statement::Statement *> stmts;
    };
    namespace Expression
    {
        struct IntLiteral
        {
            Token int_lit;
            [[nodiscard]] std::stringstream to_string() const
            {
                std::stringstream out;
                out << "IntLiteral{.int_lit=" << int_lit.to_string().str() << "}";
                return out;
            }
        };
        struct Identifier
        {
            Token ident;
            [[nodiscard]] std::stringstream to_string() const
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
        struct ParenthExpression
        {
            Expression *expression;
        };
        struct Term
        {
            std::variant<IntLiteral *, Identifier *, ParenthExpression *> term;
        };
        struct Expression
        {
            std::variant<Term *, Operation *> expression;

            [[nodiscard]] std::stringstream to_string() const
            {
                std::stringstream out;
                if (std::holds_alternative<Operation *>(expression))
                {
                    auto operation = std::get<Operation *>(expression);
                    std::stringstream operationout;
                    operationout << "Operation{.left=" << operation->left_hand->to_string().str() << ", .operator=" << operation->oprator.to_string().str() << ", .right=" << operation->right_hand->to_string().str() << "}";
                    out << "Expression{.expression=" << operationout.str() << "}";
                    // out << "Expression{.expression=" << operation_to_string(std::get<Operation *>(expression)).str() << "}";
                }
                else if (std::holds_alternative<Term *>(expression))
                {
                    Term *term = std::get<Term *>(expression);
                    std::stringstream termout;
                    // out << "Expression{.expression=" << std::get<Term *>(expression)->to_string().str() << "}";
                    if (std::holds_alternative<IntLiteral *>(term->term))
                    {
                        termout << "Term{.expression=" << std::get<IntLiteral *>(term->term)->to_string().str() << "}";
                    }
                    else if (std::holds_alternative<Identifier *>(term->term))
                    {
                        termout << "Term{.expression=" << std::get<Identifier *>(term->term)->to_string().str() << "}";
                    }
                    else if (std::holds_alternative<ParenthExpression *>(term->term))
                    {
                        // auto pexpression = std::get<ParenthExpression *>(term->term);
                        std::stringstream pexout;
                        pexout << "ParenthExpression{.expression=" << std::get<ParenthExpression *>(term->term)->expression->to_string().str() << "}";
                        termout << "Term{.expression=" << pexout.str() << "}";
                    }
                    out << "Term{.term=" << termout.str() << "}";
                }
                return out;
            }
        };

        inline std::stringstream term_to_string(const Term *term)
        {
            std::stringstream out;
            if (std::holds_alternative<IntLiteral *>(term->term))
            {
                out << "Term{.expression=" << std::get<IntLiteral *>(term->term)->to_string().str() << "}";
            }
            else if (std::holds_alternative<Identifier *>(term->term))
            {
                out << "Term{.expression=" << std::get<Identifier *>(term->term)->to_string().str() << "}";
            }
            else if (std::holds_alternative<ParenthExpression *>(term->term))
            {
                // auto pexpression = std::get<ParenthExpression *>(term->term);
                std::stringstream pexout;
                pexout << "ParenthExpression{.expression=" << std::get<ParenthExpression *>(term->term)->expression->to_string().str() << "}";
                out << "Term{.expression=" << pexout.str() << "}";
                // out << "Term{.expression=" << std::get<ParenthExpression *>(term->term)->to_string().str() << "}";
            }
            return out;
        }

        inline std::stringstream parenth_expression_to_string(const ParenthExpression *expression)
        {
            return expression->expression->to_string();
        }

        inline std::stringstream operation_to_string(const Operation *operation)
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
            [[nodiscard]] std::stringstream to_string() const {
                std::stringstream out;
                out << "Exit{.expression=" << expression->to_string().str() << "}";
                return out;
            }
        };
        struct Let
        {
            Token identifier;
            Expression::Expression *expression;
            [[nodiscard]] std::stringstream to_string() const
            {
                std::stringstream out;
                out << "Let{.identifier=" << identifier.to_string().str() << ", .expression=" << expression->to_string().str() << "}";
                return out;
            }
        };
        struct If
        {
            Node::Expression::Expression *expression;
            Node::Scope *scope;
        };
        struct Statement
        {
            std::variant<Exit *, Let *, Scope *, If *> statement;
            [[nodiscard]] std::stringstream to_string() const {
                std::stringstream out;
                if (std::holds_alternative<Exit *>(statement))
                {
                    out << "Statement{.statement=" << std::get<Exit *>(statement)->to_string().str() << "}";
                }
                else if (std::holds_alternative<Let *>(statement))
                {
                    out << "Statement{.statement=" << std::get<Let *>(statement)->to_string().str() << "}";
                }
                else if (std::holds_alternative<Scope *>(statement))
                {
                    auto scope = std::get<Scope *>(statement);
                    out << "Statement{.statement=Scope{.stmts=[";
                    for (Statement *statmt : scope->stmts)
                    {
                        out << statmt->to_string().str() << ", ";
                    }
                    out << "]}}";
                }
                else if (std::holds_alternative<If *>(statement))
                {
                    auto ifnode = std::get<If *>(statement);
                    out << "Statement{.statement=If{.expression=" << ifnode->expression->to_string().str() << ",.scope=[";
                    for (Statement *statmt : ifnode->scope->stmts)
                    {
                        out << statmt->to_string().str() << ", ";
                    }
                    out << "]}}";
                }
                return out;
            }
        };
    };
    struct Program
    {
        std::vector<Statement::Statement *> stmts;
        [[nodiscard]] std::stringstream to_string() const {
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

inline size_t expression_precedence(const Node::Expression::Expression *expression)
{
    if (const auto term = std::get_if<Node::Expression::Term *>(&expression->expression))
    {
        if (std::holds_alternative<Node::Expression::ParenthExpression *>((*term)->term))
        {
            return 1;
        }
        return 0;
    }
    else
    {
        return 0;
    }
}

class Parser
{
public:
    explicit Parser(const std::vector<Token> &tokens, ArenaAllocator *allocator) : m_tokens(tokens), m_allocator(allocator)
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
        else if (peek().has_value() && peek().value().type == TokenType::OPEN_PAREN)
        {
            consume();
            auto expr = parse_expression();
            if (!expr.has_value())
            {
                std::cerr << "whers ya expression ya dimwit" << std::endl;
                exit(EXIT_FAILURE);
            }
            if (peek().has_value() && peek().value().type == TokenType::CLOSE_PAREN)
            {
                consume();
            }
            else
            {
                std::cerr << "ya waitin n ya daddy to add the close parenthesis ya dong" << std::endl;
                exit(EXIT_FAILURE);
            }
            auto term_paren = m_allocator->alloc<Node::Expression::ParenthExpression>();
            term_paren->expression = expr.value();
            auto term = m_allocator->alloc<Node::Expression::Term>();
            term->term = term_paren;
            return term;
        }
        else
        {
            return {};
        }
    }
    std::optional<Node::Expression::Expression *> parse_expression(size_t min_prec = 0)
    {
        std::optional<Node::Expression::Term *> term_lhs = parse_term();
        // std::cout << "Entering with min_prec=" << min_prec << std::endl;
        if (!term_lhs.has_value())
        {
            return {};
        }

        auto expr_lhs = m_allocator->alloc<Node::Expression::Expression>();
        expr_lhs->expression = term_lhs.value();

        while (true)
        {
            auto cur_tok = peek();
            if (!cur_tok.has_value())
            {
                break;
            }
            auto precedence = bin_precedence(cur_tok.value());
            if (!precedence.has_value() || precedence < min_prec)
            {
                break;
            }
            Token op = consume().value();
            auto next_min_prec = precedence.value() + 1;
            auto expr_rhs = parse_expression(next_min_prec);

            if (!expr_rhs.has_value())
            {
                std::cerr << "wers da rigt and expression ya neandrathal" << std::endl;
                exit(EXIT_FAILURE);
            }

            auto expr = m_allocator->alloc<Node::Expression::Expression>();
            auto operation = m_allocator->alloc<Node::Expression::Operation>();

            operation->oprator = op;
            operation->left_hand = expr_lhs;
            operation->right_hand = expr_rhs.value();

            expr->expression = operation;

            expr_lhs = expr;
        };

        return expr_lhs;
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

    std::optional<Node::Scope *> parse_scope()
    {
        if (peek().has_value() && peek().value().type == TokenType::OPEN_CURLY)
        {
            consume();
            auto scope = m_allocator->alloc<Node::Scope>();
            while (auto statement = parse_statement())
            {
                scope->stmts.push_back(statement.value());
            }
            if (peek().has_value() && peek().value().type == TokenType::CLOSE_CURLY)
            {
                consume();
            }
            else
            {
                std::cerr << "ya need em iq pointz to close ya scopes mf" << std::endl;
                exit(EXIT_FAILURE);
            }
            return scope;
        }
        return {};
    }

    std::optional<Node::Statement::If *> parse_if()
    {
        if (peek().has_value() && peek().value().type == TokenType::IF)
        {
            // std::cout << "checking if " << peek().value().to_string().str() << std::endl;
            consume();
            auto expression = parse_expression();
            if (!expression.has_value())
            {
                std::cerr << "if what mf! if what ? be clear" << std::endl;
                exit(EXIT_FAILURE);
            }
            auto scope = parse_scope();
            if (!scope.has_value())
            {
                std::cerr << "if then wat mf. say it, type it. don't fuck it up" << std::endl;
                exit(EXIT_FAILURE);
            }
            auto if_statement = m_allocator->alloc<Node::Statement::If>();
            if_statement->expression = expression.value();
            if_statement->scope = scope.value();
            return if_statement;
        }
        return {};
    }

    std::optional<Node::Statement::Statement *> parse_statement()
    {
        if (auto exit_node = parse_exit())
        {
            auto node_statement = m_allocator->alloc<Node::Statement::Statement>();
            node_statement->statement = exit_node.value();
            return node_statement;
        }
        if (auto let_node = parse_let())
        {
            auto node_statement = m_allocator->alloc<Node::Statement::Statement>();
            node_statement->statement = let_node.value();
            return node_statement;
        }
        if (auto scope_node = parse_scope())
        {
            auto scope_statement = m_allocator->alloc<Node::Statement::Statement>();
            scope_statement->statement = scope_node.value();
            return scope_statement;
        }
        if (auto if_node = parse_if())
        {
            auto if_statement = m_allocator->alloc<Node::Statement::Statement>();
            if_statement->statement = if_node.value();
            return if_statement;
        }
        return {};
        // std::cerr << "ya messed up wat ts shit" << std::endl;
        // exit(EXIT_FAILURE);
    }

    [[nodiscard]] std::optional<Token> peek(const size_t ahead = 0) const
    {
        if (m_index + ahead >= m_tokens.size())
        {
            return {};
        }
        return m_tokens.at(m_index + ahead);
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