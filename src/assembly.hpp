#pragma once

#include "./parser.hpp"
#include <cassert>
#include <ranges>
#include <utility>

class AssGenerator {
public:
    AssGenerator(Node::Program prog, ArenaAllocator* allocator)
        : m_prog(std::move(prog))
        , m_allocator(allocator)
    {
    }

    std::string generate_program()
    {
        m_asmout.clear();
        m_asmout << "global _start\n_start:\n";

        for (const Node::Statement::Statement* statement : m_prog.stmts) {
            generate_statement(statement);
        }

        // default this runs
        m_asmout << "    ; default execution\n";
        m_asmout << "    mov rax, 60\n";
        m_asmout << "    mov rdi, 0\n";
        m_asmout << "    syscall";
        return m_asmout.str();
    }

private:
    void stack_push(const std::string& reg)
    {
        m_asmout << "    push " << reg << "\n";
        m_stack_counter++;
    }
    void stack_pop(const std::string& reg)
    {
        m_asmout << "    pop " << reg << "\n";
        m_stack_counter--;
    }

    void begin_scope()
    {
        m_scopes.push_back(m_variables.size());
    }

    void end_scope()
    {
        const size_t pop_count = m_variables.size() - m_scopes.back();
        m_asmout << "    add rsp, " << pop_count * 8 << "\n";
        m_stack_counter -= pop_count;
        for (size_t i = 0; i < pop_count; i++) {
            m_variables.pop_back();
        }
        m_scopes.pop_back();
    }

    std::string create_label()
    {
        return "label" + std::to_string(m_label_count++);
    }

    void generate_term(const Node::Expression::Term* term)
    {
        struct TermVisitor {
            AssGenerator& generator;

            void operator()(const Node::Expression::Identifier* identifier_node) const
            {
                // std::cout << "Variable referenced" << " : " << identifier_node.ident.value.value() << std::endl;
                // std::cout << "Existing variables " << generator->coutmap().str() << "\n";
                // if (generator->m_variables.count(identifier_node->ident.value.value()) == 0)
                const auto variable
                    = std::ranges::find_if(std::as_const(generator.m_variables), [&](const Variable& var) {
                          return var.name == identifier_node->ident.value.value();
                      });
                if (variable == generator.m_variables.cend()) {
                    std::cerr << "ya using undeclared variables ya ass" << std::endl;
                    exit(EXIT_FAILURE);
                }
                generator.m_asmout << "    ; generate identifier" << "\n";
                std::stringstream register_name;
                // const auto &variable = (*it);
                register_name << "QWORD [rsp + " << (generator.m_stack_counter - (*variable).stack_loc - 1) * 8 << "]";
                generator.stack_push(register_name.str());
            };
            void operator()(const Node::Expression::ParenthExpression* parenth_expression) const
            {
                generator.m_asmout << "    ; generate parenthesis expression" << "\n";
                generator.generate_expression(parenth_expression->expression);
            };
            void operator()(const Node::Expression::IntLiteral* int_literal) const
            {
                generator.m_asmout << "    ; generate literal" << "\n";
                generator.m_asmout << "    mov rax, " << int_literal->int_lit.value.value() << "\n";
                generator.stack_push("rax");
            };
        };

        TermVisitor visitor = { .generator = *this };
        std::visit(visitor, term->term);
    }

    void generate_scope(const Node::Scope* scope)
    {
        m_asmout << "    ; generate scope" << "\n";
        begin_scope();
        for (const Node::Statement::Statement* statement : scope->stmts) {
            generate_statement(statement);
        }
        end_scope();
    }

    void generate_expression(const Node::Expression::Expression* expression)
    {
        struct ExpressionVisitor {
            AssGenerator& generator;

            void operator()(const Node::Expression::Term* term) const
            {
                generator.m_asmout << "    ; generate term" << "\n";
                generator.generate_term(term);
            };
            void operator()(Node::Expression::Operation* operation) const
            {
                // if (auto operation = std::get_if<Node::Expression::Operation *>(&node_expr.value()->expression))
                // {
                //     node_expr.value()->expression = compute_precedence_tree(&m_allocator, (*operation));
                // }
                generator.m_asmout << "    ; generate operation" << "\n";
                // std::cout << "Operation encountered " << operation_to_string(operation).str() << std::endl;
                // auto balanced_operation = compute_precedence_tree(generator->m_allocator, operation);
                // std::cout << "Operation balancing completed " << operation_to_string(balanced_operation).str() <<
                // std::endl;
                if (operation->oprator.value.value() == "+") {
                    generator.m_asmout << "    ; generate add" << "\n";
                    generator.generate_expression(operation->left_hand);
                    generator.generate_expression(operation->right_hand);
                    generator.stack_pop("rax");
                    generator.stack_pop("rbx");
                    generator.m_asmout << "    add rax, rbx\n";
                    generator.stack_push("rax");
                }
                else if (operation->oprator.value.value() == "-") {
                    generator.m_asmout << "    ; generate subtract" << "\n";
                    generator.generate_expression(operation->left_hand);
                    generator.generate_expression(operation->right_hand);
                    generator.stack_pop("rbx");
                    generator.stack_pop("rax");
                    generator.m_asmout << "    sub rax, rbx\n";
                    generator.stack_push("rax");
                }
                else if (operation->oprator.value.value() == "*") {
                    generator.m_asmout << "    ; generate multiply" << "\n";
                    generator.generate_expression(operation->left_hand);
                    generator.generate_expression(operation->right_hand);
                    generator.stack_pop("rax");
                    generator.stack_pop("rbx");
                    generator.m_asmout << "    mul rbx\n";
                    generator.stack_push("rax");
                }
                else if (operation->oprator.value.value() == "/") {
                    generator.m_asmout << "    ; generate divide" << "\n";
                    generator.generate_expression(operation->left_hand);
                    generator.generate_expression(operation->right_hand);
                    generator.stack_pop("rbx");
                    generator.stack_pop("rax");
                    generator.m_asmout << "    div rbx\n";
                    generator.stack_push("rax");
                }
                else {
                    assert(false); // not implemented
                }
            };
        };

        ExpressionVisitor visitor = { .generator = *this };
        std::visit(visitor, expression->expression);
    }

    void generate_statement(const Node::Statement::Statement* statement)
    {
        struct StatementVisitor {
            AssGenerator& generator;

            void operator()(const Node::Statement::Exit* exit_node) const
            {
                generator.m_asmout << "    ; generate exit" << "\n";
                generator.generate_expression(exit_node->expression);
                generator.m_asmout << "    mov rax, 60\n";
                generator.stack_pop("rdi");
                generator.m_asmout << "    syscall\n";
            };
            void operator()(const Node::Statement::Let* let_node) const
            {
                // std::cout << "Variable created" << " : " << let_node.identifier.value.value() << std::endl;

                const auto variable
                    = std::ranges::find_if(std::as_const(generator.m_variables), [&](const Variable& var) {
                          return var.name == let_node->identifier.value.value();
                      });

                if (variable != generator.m_variables.cend()) {
                    std::cerr << "ya reusin variables ya bitch" << std::endl;
                    exit(EXIT_FAILURE);
                }
                generator.m_asmout << "    ; generate variable" << "\n";
                generator.m_variables.push_back(
                    { .name = let_node->identifier.value.value(), .stack_loc = generator.m_stack_counter });
                generator.generate_expression(let_node->expression);
            };
            void operator()(const Node::Scope* scope_node) const
            {
                generator.generate_scope(scope_node);
            };
            void operator()(const Node::Statement::If* if_node) const
            {
                generator.generate_expression(if_node->expression);
                generator.stack_pop("rax");
                auto label = generator.create_label();
                generator.m_asmout << "    test rax, rax" << "\n";
                generator.m_asmout << "    jz " << label << "\n";
                generator.generate_scope(if_node->scope);
                generator.m_asmout << label << ":" << "\n";
            };
        };

        StatementVisitor visitor = { .generator = *this };
        std::visit(visitor, statement->statement);
    }

    struct Variable {
        std::string name;
        size_t stack_loc;
    };

    std::stringstream coutmap() const
    {
        std::stringstream out;
        for (const Variable& variable : m_variables) {
            out << "Variable name=" << variable.name << " value=" << variable.stack_loc << " | ";
        }
        return out;
    }

    const Node::Program m_prog;
    std::stringstream m_asmout;
    size_t m_stack_counter = 0;
    std::vector<Variable> m_variables {};
    std::vector<size_t> m_scopes {};
    ArenaAllocator* m_allocator;
    int m_label_count = 0;
};