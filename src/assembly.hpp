#pragma once

#include <unordered_map>
#include "./parser.hpp"

class AssGenerator
{
public:
    inline AssGenerator(Node::Program prog) : m_prog(std::move(prog)) {}

    std::string generate_program()
    {
        m_asmout.clear();
        m_asmout << "global _start\n_start:\n";

        for (const Node::Statement::Statement &statement : m_prog.stmts)
        {
            generate_statement(statement);
        }

        // default this runs
        m_asmout << "    mov rax, 60\n";
        m_asmout << "    mov rdi, 0\n";
        m_asmout << "    syscall";
        return m_asmout.str();
    }

private:
    void stack_push(const std::string &reg)
    {
        m_asmout << "    push " << reg << "\n";
        m_stack_counter++;
    }
    void stack_pop(const std::string &reg)
    {
        m_asmout << "    pop " << reg << "\n";
        m_stack_counter--;
    }

    void generate_expression(const Node::Expression::Expression &expression)
    {
        struct ExpressionVisitor
        {
            AssGenerator *generator;

            void operator()(const Node::Expression::Identifier &identifier_node) const
            {
                // std::cout << "Variable referenced" << " : " << identifier_node.ident.value.value() << std::endl;
                // std::cout << "Existing variables " << generator->coutmap().str() << "\n";
                if (generator->m_variables.count(identifier_node.ident.value.value()) == 0)
                {
                    std::cerr << "ya using undeclared variables ya ass" << std::endl;
                    exit(EXIT_FAILURE);
                }
                std::stringstream register_name;
                const auto &variable = generator->m_variables.at(identifier_node.ident.value.value());
                register_name << "QWORD [rsp + " << (generator->m_stack_counter - variable.stack_loc - 1) * 8 << "]";
                generator->stack_push(register_name.str());
            };
            void operator()(const Node::Expression::IntLiteral &int_literal) const
            {
                generator->m_asmout << "    mov rax, " << int_literal.int_lit.value.value() << "\n";
                generator->stack_push("rax");
            };
        };

        ExpressionVisitor visitor = {.generator = this};
        std::visit(visitor, expression.expression);
    }

    void generate_statement(const Node::Statement::Statement &statement)
    {
        struct StatementVisitor
        {
            AssGenerator *generator;

            void operator()(const Node::Statement::Exit &exit_node) const
            {
                generator->generate_expression(exit_node.expression);
                generator->m_asmout << "    mov rax, 60\n";
                generator->stack_pop("rdi");
                generator->m_asmout << "    syscall\n";
            };
            void operator()(const Node::Statement::Let &let_node) const
            {
                // std::cout << "Variable created" << " : " << let_node.identifier.value.value() << std::endl;

                if (generator->m_variables.count(let_node.identifier.value.value()) > 0)
                {
                    std::cerr << "ya reusin variables ya bitch" << std::endl;
                    exit(EXIT_FAILURE);
                }

                generator->m_variables.insert({
                    let_node.identifier.value.value(),
                    Variable{.stack_loc = generator->m_stack_counter},
                });
                generator->generate_expression(let_node.expression);
            };
        };

        StatementVisitor visitor = {.generator = this};
        std::visit(visitor, statement.statement);
    }

    struct Variable
    {
        size_t stack_loc;
    };

    std::stringstream coutmap()
    {
        std::stringstream out;
        for (const std::pair pair : m_variables)
        {
            out << "Variable name=" << pair.first << " value=" << pair.second.stack_loc << " | ";
        }
        return out;
    }

    const Node::Program m_prog;
    std::stringstream m_asmout;
    size_t m_stack_counter = 0;
    std::unordered_map<std::string, Variable> m_variables;
};