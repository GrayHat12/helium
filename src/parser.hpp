#pragma once
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "./arena.hpp"
#include "./tokenization.hpp"

namespace Node {

struct BaseNode {
    std::pair<size_t, size_t> position;

    [[nodiscard]] std::stringstream current_position(std::string prefix = "error at ") const
    {
        std::stringstream out;
        out << prefix << position.first << ":" << position.second;
        return out;
    }
};

namespace Statement {
struct Statement;
}
struct Scope : BaseNode {
    std::vector<Statement::Statement*> stmts;
};
namespace Expression {
struct IntLiteral : BaseNode {
    Token int_lit;
    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        out << "IntLiteral{.int_lit=" << int_lit.to_string().str() << "}";
        return out;
    }
};
struct Identifier : BaseNode {
    Token ident;
    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        out << "Identifier{.int_lit=" << ident.to_string().str() << "}";
        return out;
    }
};
struct Expression;
struct Operation : BaseNode {
    Expression* left_hand;
    Token oprator;
    Expression* right_hand;
};
struct ParenthExpression : BaseNode {
    Expression* expression;
};
struct Term : BaseNode {
    std::variant<IntLiteral*, Identifier*, ParenthExpression*> term;
};
struct Expression : BaseNode {
    std::variant<Term*, Operation*> expression;

    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        if (std::holds_alternative<Operation*>(expression)) {
            auto operation = std::get<Operation*>(expression);
            std::stringstream operationout;
            operationout << "Operation{.left=" << operation->left_hand->to_string().str()
                         << ", .operator=" << operation->oprator.to_string().str()
                         << ", .right=" << operation->right_hand->to_string().str() << "}";
            out << "Expression{.expression=" << operationout.str() << "}";
            // out << "Expression{.expression=" << operation_to_string(std::get<Operation *>(expression)).str() << "}";
        }
        else if (std::holds_alternative<Term*>(expression)) {
            Term* term = std::get<Term*>(expression);
            std::stringstream termout;
            // out << "Expression{.expression=" << std::get<Term *>(expression)->to_string().str() << "}";
            if (std::holds_alternative<IntLiteral*>(term->term)) {
                termout << "Term{.expression=" << std::get<IntLiteral*>(term->term)->to_string().str() << "}";
            }
            else if (std::holds_alternative<Identifier*>(term->term)) {
                termout << "Term{.expression=" << std::get<Identifier*>(term->term)->to_string().str() << "}";
            }
            else if (std::holds_alternative<ParenthExpression*>(term->term)) {
                // auto pexpression = std::get<ParenthExpression *>(term->term);
                std::stringstream pexout;
                pexout << "ParenthExpression{.expression="
                       << std::get<ParenthExpression*>(term->term)->expression->to_string().str() << "}";
                termout << "Term{.expression=" << pexout.str() << "}";
            }
            out << "Term{.term=" << termout.str() << "}";
        }
        return out;
    }
};

inline std::stringstream term_to_string(const Term* term)
{
    std::stringstream out;
    if (std::holds_alternative<IntLiteral*>(term->term)) {
        out << "Term{.expression=" << std::get<IntLiteral*>(term->term)->to_string().str() << "}";
    }
    else if (std::holds_alternative<Identifier*>(term->term)) {
        out << "Term{.expression=" << std::get<Identifier*>(term->term)->to_string().str() << "}";
    }
    else if (std::holds_alternative<ParenthExpression*>(term->term)) {
        // auto pexpression = std::get<ParenthExpression *>(term->term);
        std::stringstream pexout;
        pexout << "ParenthExpression{.expression="
               << std::get<ParenthExpression*>(term->term)->expression->to_string().str() << "}";
        out << "Term{.expression=" << pexout.str() << "}";
        // out << "Term{.expression=" << std::get<ParenthExpression *>(term->term)->to_string().str() << "}";
    }
    return out;
}

inline std::stringstream parenth_expression_to_string(const ParenthExpression* expression)
{
    return expression->expression->to_string();
}

inline std::stringstream operation_to_string(const Operation* operation)
{
    std::stringstream out;
    out << "Operation{.left=" << operation->left_hand->to_string().str() << ", .operator="
        << operation->oprator.to_string().str() << ", .right=" << operation->right_hand->to_string().str() << "}";
    return out;
}
};
namespace Statement {
struct Exit : BaseNode {
    Expression::Expression* expression;
    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        out << "Exit{.expression=" << expression->to_string().str() << "}";
        return out;
    }
};
struct Let : BaseNode {
    Token identifier;
    Expression::Expression* expression;
    bool mutable_;
    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        out << "Let{.identifier=" << identifier.to_string().str() << ", .mutable=" << mutable_
            << ", .expression=" << expression->to_string().str() << "}";
        return out;
    }
};
struct Assignment : BaseNode {
    Token identifier;
    Expression::Expression* expression;
    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        out << "Assign{.identifier=" << identifier.to_string().str()
            << ", .expression=" << expression->to_string().str() << "}";
        return out;
    }
};
struct Else;
struct If : BaseNode {
    Node::Expression::Expression* expression;
    Node::Scope* scope;
    std::optional<Else*> else_;
};

struct Else : BaseNode {
    std::variant<If*, Scope*> else_;
};
struct Statement : BaseNode {
    std::variant<Exit*, Let*, Scope*, If*, Assignment*> statement;

    [[nodiscard]] std::stringstream to_string(If* ifnode) const
    {
        std::stringstream out;
        out << "If{.expression=" << ifnode->expression->to_string().str() << ",.scope=[";
        for (Statement* statmt : ifnode->scope->stmts) {
            out << statmt->to_string().str() << ", ";
        }
        out << "],";
        if (ifnode->else_.has_value()) {
            if (std::holds_alternative<Scope*>(ifnode->else_.value()->else_)) {
                auto scope = std::get<Scope*>(ifnode->else_.value()->else_);
                out << ".else=";
                out << to_string(scope).str();
                out << ",";
            }
            else if (std::holds_alternative<If*>(ifnode->else_.value()->else_)) {
                auto if_node = std::get<If*>(ifnode->else_.value()->else_);
                out << ".else=" << to_string(if_node).str() << "}";
            }
        }
        out << "}";
        return out;
    }

    [[nodiscard]] std::stringstream to_string(Scope* scope) const
    {
        std::stringstream out;
        out << "Scope{.stmts=[";
        for (Statement* statmt : scope->stmts) {
            out << statmt->to_string().str() << ", ";
        }
        out << "]}";
        return out;
    }

    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        if (std::holds_alternative<Exit*>(statement)) {
            out << "Statement{.statement=" << std::get<Exit*>(statement)->to_string().str() << "}";
        }
        else if (std::holds_alternative<Let*>(statement)) {
            out << "Statement{.statement=" << std::get<Let*>(statement)->to_string().str() << "}";
        }
        else if (std::holds_alternative<Assignment*>(statement)) {
            out << "Statement{.statement=" << std::get<Assignment*>(statement)->to_string().str() << "}";
        }
        else if (std::holds_alternative<Scope*>(statement)) {
            auto scope = std::get<Scope*>(statement);
            out << "Statement{.statement=Scope{.stmts=[";
            for (Statement* statmt : scope->stmts) {
                out << statmt->to_string().str() << ", ";
            }
            out << "]}}";
        }
        else if (std::holds_alternative<If*>(statement)) {
            auto ifnode = std::get<If*>(statement);
            out << "Statement{.statement=" << to_string(ifnode).str() << "}";
        }
        return out;
    }
};
};
struct Program : BaseNode {
    std::vector<Statement::Statement*> stmts;
    [[nodiscard]] std::stringstream to_string() const
    {
        std::stringstream out;
        out << "Program{.stmts=[";
        for (Statement::Statement* statmt : stmts) {
            out << statmt->to_string().str() << ", ";
        }
        out << "]}";
        return out;
    }
};
}

inline size_t expression_precedence(const Node::Expression::Expression* expression)
{
    if (const auto term = std::get_if<Node::Expression::Term*>(&expression->expression)) {
        if (std::holds_alternative<Node::Expression::ParenthExpression*>((*term)->term)) {
            return 1;
        }
        return 0;
    }
    else {
        return 0;
    }
}

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens, ArenaAllocator* allocator)
        : m_tokens(tokens)
        , m_allocator(allocator)
    {
    }

    Node::Program parse()
    {
        m_index = 0;
        Node::Program program_node;
        while (peek().has_value()) {
            if (auto statement = parse_statement()) {
                program_node.stmts.push_back(statement.value());
            }
            else {
                std::cerr << "wat di statement ya twat" << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        return program_node;
    }

private:
    std::optional<Node::Expression::Term*> parse_term()
    {
        if (peek().has_value() && peek().value().type == TokenType::INT_LT) {
            auto node_expression_int_lit = m_allocator->alloc<Node::Expression::IntLiteral>();
            node_expression_int_lit->int_lit = consume().value();
            node_expression_int_lit->position = node_expression_int_lit->position;
            auto node_expression = m_allocator->alloc<Node::Expression::Term>();
            node_expression->term = node_expression_int_lit;
            node_expression->position = node_expression_int_lit->position;
            return node_expression;
        }
        else if (peek().has_value() && peek().value().type == TokenType::IDENT) {
            auto node_expression_identifier = m_allocator->alloc<Node::Expression::Identifier>();
            node_expression_identifier->ident = consume().value();
            node_expression_identifier->position = node_expression_identifier->position;
            auto node_expression = m_allocator->alloc<Node::Expression::Term>();
            node_expression->term = node_expression_identifier;
            node_expression->position = node_expression_identifier->position;
            return node_expression;
        }
        else if (peek().has_value() && peek().value().type == TokenType::OPEN_PAREN) {
            consume();
            auto expr = parse_expression();
            if (!expr.has_value()) {
                std::cerr << "whers ya expression ya dimwit " << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }
            if (peek().has_value() && peek().value().type == TokenType::CLOSE_PAREN) {
                consume();
            }
            else {
                std::cerr << "ya waitin n ya daddy to add the close parenthesis ya dong " << current_position().str()
                          << std::endl;
                exit(EXIT_FAILURE);
            }
            auto term_paren = m_allocator->alloc<Node::Expression::ParenthExpression>();
            term_paren->expression = expr.value();
            term_paren->position = term_paren->expression->position;
            auto term = m_allocator->alloc<Node::Expression::Term>();
            term->term = term_paren;
            term->position = term_paren->position;
            return term;
        }
        else {
            return {};
        }
    }
    std::optional<Node::Expression::Expression*> parse_expression(size_t min_prec = 0)
    {
        std::optional<Node::Expression::Term*> term_lhs = parse_term();
        // std::cout << "Entering with min_prec=" << min_prec << std::endl;
        if (!term_lhs.has_value()) {
            return {};
        }

        auto expr_lhs = m_allocator->alloc<Node::Expression::Expression>();
        expr_lhs->expression = term_lhs.value();
        expr_lhs->position = term_lhs.value()->position;

        while (true) {
            auto cur_tok = peek();
            if (!cur_tok.has_value()) {
                break;
            }
            auto precedence = bin_precedence(cur_tok.value());
            if (!precedence.has_value() || precedence < min_prec) {
                break;
            }
            Token op = consume().value();
            auto next_min_prec = precedence.value() + 1;
            auto expr_rhs = parse_expression(next_min_prec);

            if (!expr_rhs.has_value()) {
                std::cerr << "wers da rigt and expression ya neandrathal " << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }

            auto expr = m_allocator->alloc<Node::Expression::Expression>();
            auto operation = m_allocator->alloc<Node::Expression::Operation>();

            operation->oprator = op;
            operation->left_hand = expr_lhs;
            operation->right_hand = expr_rhs.value();
            operation->position = expr_lhs->position;

            expr->expression = operation;
            expr->position = operation->position;

            expr_lhs = expr;
        };

        return expr_lhs;
    }

    std::optional<Node::Statement::Exit*> parse_exit()
    {
        std::optional<Node::Statement::Exit*> op_exit_node;
        // std::cout << peek().value().type << " : " << peek().value().value.value_or("") << std::endl;
        if (peek().value().type == TokenType::EXIT && peek(1).has_value()
            && peek(1).value().type == TokenType::OPEN_PAREN) {
            auto exittoken = consume();
            consume();
            if (auto node_expr = parse_expression()) {
                // exit_node = Node::Statement::Exit{.expression = node_expr.value()};
                auto exit_node = m_allocator->alloc<Node::Statement::Exit>();
                exit_node->expression = node_expr.value();
                op_exit_node = exit_node;
                exit_node->position = exittoken.value().position;
            }
            else {
                std::cerr << "ya messed up bitches " << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }
            // consume close paren
            if (!peek().has_value() || peek().value().type != TokenType::CLOSE_PAREN) {
                std::cerr << "ya messed up ya parenthesis twat " << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }
            else {
                consume();
            }

            // consume semicolon
            if (!peek().has_value() || peek().value().type != TokenType::SEMICL) {
                std::cerr << "ya messed up ya semicolon twat " << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }
            else {
                consume();
            }
        }

        return op_exit_node;
    }

    std::optional<Node::Statement::Let*> parse_let()
    {
        std::optional<Node::Statement::Let*> op_let_node = {};
        // std::cout << "let " << peek().value().type << " : " << peek().value().value.value_or("") << std::endl;
        if (peek().value().type == TokenType::LET) {
            auto non_mutable = peek(1).has_value() && peek(1).value().type == TokenType::IDENT && peek(2).has_value()
                && peek(2).value().type == TokenType::EQUALS;

            auto mutable_ = peek(1).has_value() && peek(1).value().type == TokenType::MUTABLE && peek(2).has_value()
                && peek(2).value().type == TokenType::IDENT && peek(3).has_value()
                && peek(3).value().type == TokenType::EQUALS;

            if (mutable_ || non_mutable) {
                auto lettoken = consume().value(); // consume let
                if (mutable_) {
                    consume(); // consume mut
                }
                Token ident = consume().value(); // consume token
                consume(); // consume equals
                if (auto node_expr = parse_expression()) {
                    auto let_node = m_allocator->alloc<Node::Statement::Let>();
                    let_node->identifier = ident;
                    let_node->expression = node_expr.value();
                    let_node->mutable_ = mutable_;
                    let_node->position = lettoken.position;
                    op_let_node = let_node;
                    // Node::Statement::Let{.identifier = ident, .expression = node_expr.value()};
                }
                else {
                    std::cerr << "ya messed up bitches " << current_position().str() << std::endl;
                    exit(EXIT_FAILURE);
                }
                // consume semicolon
                if (!peek().has_value() || peek().value().type != TokenType::SEMICL) {
                    std::cerr << "ya messed up ya semicolon twat " << current_position().str() << std::endl;
                    exit(EXIT_FAILURE);
                }
                else {
                    consume();
                }
            }
        }

        return op_let_node;
    }

    std::optional<Node::Statement::Assignment*> parse_assign()
    {
        std::optional<Node::Statement::Assignment*> op_assign_node = {};
        if (peek().value().type == TokenType::IDENT && peek(1).has_value()
            && peek(1).value().type == TokenType::EQUALS) {
            Token ident = consume().value();
            consume();
            if (auto node_expr = parse_expression()) {
                auto assign_node = m_allocator->alloc<Node::Statement::Assignment>();
                assign_node->identifier = ident;
                assign_node->expression = node_expr.value();
                assign_node->position = ident.position;
                op_assign_node = assign_node;
            }
            else {
                std::cerr << "watchu trynna do n...." << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }
            // consume semicolon
            if (!peek().has_value() || peek().value().type != TokenType::SEMICL) {
                std::cerr << "ya messed up ya semicolon twat " << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }
            else {
                consume();
            }
        }

        return op_assign_node;
    }

    std::optional<Node::Scope*> parse_scope()
    {
        if (peek().has_value() && peek().value().type == TokenType::OPEN_CURLY) {
            auto curlytoken = consume();
            auto scope = m_allocator->alloc<Node::Scope>();
            scope->position = curlytoken.value().position;
            while (auto statement = parse_statement()) {
                scope->stmts.push_back(statement.value());
            }
            if (peek().has_value() && peek().value().type == TokenType::CLOSE_CURLY) {
                consume();
            }
            else {
                std::cerr << "ya need em iq pointz to close ya scopes mf " << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }
            return scope;
        }
        return {};
    }

    std::optional<Node::Statement::Else*> parse_else()
    {
        if (peek().has_value() && peek().value().type == TokenType::ELSE) {
            auto elsetoken = consume().value();
            auto else_statement = m_allocator->alloc<Node::Statement::Else>();
            else_statement->position = elsetoken.position;
            auto ifnode = parse_if();
            if (ifnode.has_value()) {
                else_statement->else_ = ifnode.value();
            }
            else {
                auto scope = parse_scope();
                if (!scope.has_value()) {
                    std::cerr << "if then wat mf. say it, type it. don't fuck it up " << current_position().str()
                              << std::endl;
                    exit(EXIT_FAILURE);
                }
                else_statement->else_ = scope.value();
            }
            return else_statement;
        }
        return {};
    }

    std::optional<Node::Statement::If*> parse_if()
    {
        if (peek().has_value() && peek().value().type == TokenType::IF) {
            // std::cout << "checking if " << peek().value().to_string().str() << std::endl;
            auto iftoken = consume().value();
            auto expression = parse_expression();
            if (!expression.has_value()) {
                std::cerr << "if what mf! if what ? be clear" << current_position().str() << std::endl;
                exit(EXIT_FAILURE);
            }
            auto scope = parse_scope();
            if (!scope.has_value()) {
                std::cerr << "if then wat mf. say it, type it. don't fuck it up" << current_position().str()
                          << std::endl;
                exit(EXIT_FAILURE);
            }
            auto if_statement = m_allocator->alloc<Node::Statement::If>();
            if_statement->position = iftoken.position;
            if_statement->expression = expression.value();
            if_statement->scope = scope.value();
            auto else_statement = parse_else();
            if_statement->else_ = else_statement;
            return if_statement;
        }
        return {};
    }

    std::optional<Node::Statement::Statement*> parse_statement()
    {
        if (auto exit_node = parse_exit()) {
            auto node_statement = m_allocator->alloc<Node::Statement::Statement>();
            node_statement->statement = exit_node.value();
            node_statement->position = exit_node.value()->position;
            return node_statement;
        }
        if (auto let_node = parse_let()) {
            auto node_statement = m_allocator->alloc<Node::Statement::Statement>();
            node_statement->statement = let_node.value();
            node_statement->position = let_node.value()->position;
            return node_statement;
        }
        if (auto assign_node = parse_assign()) {
            auto node_statement = m_allocator->alloc<Node::Statement::Statement>();
            node_statement->statement = assign_node.value();
            node_statement->position = assign_node.value()->position;
            return node_statement;
        }
        if (auto scope_node = parse_scope()) {
            auto scope_statement = m_allocator->alloc<Node::Statement::Statement>();
            scope_statement->statement = scope_node.value();
            scope_statement->position = scope_node.value()->position;
            return scope_statement;
        }
        if (auto if_node = parse_if()) {
            auto if_statement = m_allocator->alloc<Node::Statement::Statement>();
            if_statement->statement = if_node.value();
            if_statement->position = if_node.value()->position;
            return if_statement;
        }
        return {};
        // std::cerr << "ya messed up wat ts shit" << std::endl;
        // exit(EXIT_FAILURE);
    }

    [[nodiscard]] std::optional<Token> peek(const size_t ahead = 0) const
    {
        if (m_index + ahead >= m_tokens.size()) {
            return {};
        }
        return m_tokens.at(m_index + ahead);
    }

    std::optional<Token> consume()
    {
        if (m_index >= m_tokens.size()) {
            return {};
        }
        auto token = m_tokens.at(m_index++);
        m_position = token.position;
        return token;
    }
    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator* m_allocator;

    std::pair<size_t, size_t> m_position = { 0, 0 };

    std::stringstream current_position(std::string prefix = "error at ")
    {
        std::stringstream out;
        out << prefix << m_position.first << ":" << m_position.second;
        return out;
    }
};