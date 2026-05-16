#pragma once

#include "./parser.hpp"
#include "./static.hpp"
#include <cassert>
#include <ranges>
#include <utility>
#include <variant>

const std::string FUNC_DEF_LABEL_PREFIX = "__func__";
const std::string FUNC_RET_LABEL_PREFIX = "__func__ret__";
const std::string FUNC_END_LABEL_PREFIX = "__func__end__";

class code {
public:
    std::stringstream generated;

    code()
    {
        std::string embedCode = "; %HELIUM_CODE% ;";
        size_t loc = EMBEDDED_DATA.find(embedCode);
        pref = EMBEDDED_DATA.substr(0, loc);
        post = EMBEDDED_DATA.substr(loc + embedCode.length());

        // 4 spaces for a tab
        auto lastnline = pref.find_last_of('\n');
        tabs = pref.substr(lastnline).length() / 4;
        pref = pref.substr(0, lastnline);
    }

    std::string get_tab()
    {
        std::stringstream tab;
        for (auto i = 0; i < tabs; i++) {
            tab << "    ";
        }
        return tab.str();
    }

    void start_nest()
    {
        tabs += 1;
    }

    void end_nest()
    {
        tabs -= 1;
    }

    std::stringstream get_full_asm()
    {
        std::stringstream out;
        out << pref << generated.str() << "\n" << get_tab() << post;
        return out;
    }

private:
    std::string pref;
    std::string post;
    size_t tabs;
};

std::string process_escape_sequences(const std::string& input, size_t& out_len)
{
    std::string result;
    out_len = 0;
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '\\' && i + 1 < input.length()) {
            switch (input[i + 1]) {
            case 'n':
                result += "\", 10, \"";
                out_len++;
                break;
            case 't':
                result += "\", 9, \"";
                out_len++;
                break;
            case 'r':
                result += "\", 13, \"";
                out_len++;
                break;
            case '\"':
                result += "\", 34, \"";
                out_len++;
                break;
            case '\\':
                result += "\", 92, \"";
                out_len++;
                break;
            default:
                result += input[i];
                out_len++;
                i--;
                break; // Not an escape
            }
            i++; // Skip the escaped character
        }
        else {
            result += input[i];
            out_len++;
        }
    }
    return result;
}

class AssGenerator {
public:
    AssGenerator(Node::Program prog, ArenaAllocator* allocator)
        : m_prog(std::move(prog))
        , m_allocator(allocator)
    {
    }

    std::string generate_program()
    {
        m_code = code();

        for (const Node::Statement::Statement* statement : m_prog.stmts) {
            generate_statement(statement);
        }

        // default this runs
        m_code.generated << m_code.get_tab() << "; default execution\n";
        m_code.generated << m_code.get_tab() << "mov rax, 60\n";
        m_code.generated << m_code.get_tab() << "mov rdi, 0\n";
        m_code.generated << m_code.get_tab() << "syscall\n";
        // static strings
        if (m_strings.size() > 0) {
            m_code.generated << m_code.get_tab() << "section .data\n";
        }
        for (const auto str : m_strings) {
            size_t length = 0;
            std::string processed = process_escape_sequences(str.value, length);
            m_code.generated << m_code.get_tab() << str.label << " db \"" << processed << "\", 0\n";
        }
        return m_code.get_full_asm().str();
    }

    std::stringstream coutmap() const
    {
        std::stringstream out;
        for (const Variable& variable : m_variables) {
            out << "Variable name=" << variable.name << " value=" << variable.stack_loc
                << " mutable=" << variable.mutable_ << " level=" << variable.level << " | ";
        }
        return out;
    }

private:
    void stack_push(const std::string& reg)
    {
        m_code.generated << m_code.get_tab() << "; add m_stack_counter=" << m_stack_counter << "\n";
        m_code.generated << m_code.get_tab() << "push " << reg << "\n";
        m_stack_counter++;
    }
    void stack_pop(const std::string& reg)
    {
        m_code.generated << m_code.get_tab() << "; sub m_stack_counter=" << m_stack_counter << "\n";
        m_code.generated << m_code.get_tab() << "pop " << reg << "\n";
        m_stack_counter--;
    }

    void begin_scope()
    {
        m_scopes.push_back(m_variables.size());
        m_code.start_nest();
    }

    void end_scope()
    {
        const size_t pop_count = m_variables.size() - m_scopes.back();
        size_t total_slots_to_pop = 0;
        const size_t vars_in_scope = m_variables.size() - m_scopes.back();

        // Iterate backwards through the variables we are about to remove
        for (size_t i = 0; i < vars_in_scope; ++i) {
            const auto& var = m_variables.at(m_variables.size() - 1 - i);
            if (var.type == Node::VariableType::STR) {
                total_slots_to_pop += 2;
            }
            else {
                total_slots_to_pop += 1;
            }
        }
        // Adjust the physical stack pointer
        if (total_slots_to_pop > 0) {
            m_code.generated << m_code.get_tab() << "add rsp, " << total_slots_to_pop * 8 << "\n";
            m_stack_counter -= total_slots_to_pop;
        }

        // Remove from our metadata tracking
        for (size_t i = 0; i < vars_in_scope; i++) {
            m_variables.pop_back();
        }
        m_scopes.pop_back();
        m_code.end_nest();
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
                const auto reverse_variables = generator.m_variables | std::views::reverse;
                const auto variable = std::ranges::find_if(std::as_const(reverse_variables), [&](const Variable& var) {
                    return var.name == identifier_node->ident.value.value();
                });
                if (variable == reverse_variables.end()) {
                    std::cerr << "ya using undeclared variables ya ass "
                              << "variable=" << identifier_node->to_string().str() << " "
                              << identifier_node->current_position().str() << std::endl;
                    exit(EXIT_FAILURE);
                }
                generator.m_code.generated << generator.m_code.get_tab() << "; generate identifier " << variable->name
                                           << " " << variable->level << " " << generator.m_stack_counter << "\n";

                if (variable->type == Node::VariableType::STR) {
                    if (variable->argument) {
                        size_t ptr_offset = variable->func_stack_loc + 8;
                        generator.m_code.generated
                            << generator.m_code.get_tab() << "mov rax, [rbp + " << ptr_offset << "]\n";
                        generator.stack_push("rax");

                        generator.m_code.generated
                            << generator.m_code.get_tab() << "mov rax, [rbp + " << variable->func_stack_loc << "]\n";
                        generator.stack_push("rax");
                    }
                    else {
                        size_t len_offset = (generator.m_stack_counter - variable->stack_loc - 1) * 8;
                        generator.m_code.generated
                            << generator.m_code.get_tab() << "mov rax, QWORD [rsp + " << len_offset << "]\n";
                        generator.stack_push("rax");

                        size_t ptr_offset = (generator.m_stack_counter - (variable->stack_loc + 1) - 1) * 8;
                        generator.m_code.generated
                            << generator.m_code.get_tab() << "mov rax, QWORD [rsp + " << ptr_offset << "]\n";
                        generator.stack_push("rax");
                    }
                }
                else {
                    std::stringstream register_name;
                    if (variable->argument) {
                        register_name << "QWORD [rbp + " << variable->func_stack_loc << "]\n";
                    }
                    else {
                        register_name << "QWORD [rsp + " << (generator.m_stack_counter - variable->stack_loc - 1) * 8
                                      << "]";
                    }
                    generator.stack_push(register_name.str());
                }
            };
            void operator()(const Node::Expression::ParenthExpression* parenth_expression) const
            {
                generator.m_code.generated << generator.m_code.get_tab() << "; generate parenthesis expression" << "\n";
                generator.generate_expression(parenth_expression->expression);
            };
            void operator()(const Node::Expression::IntLiteral* int_literal) const
            {
                generator.m_code.generated << generator.m_code.get_tab() << "; generate literal" << "\n";
                generator.m_code.generated
                    << generator.m_code.get_tab() << "mov rax, " << int_literal->int_lit.value.value() << "\n";
                generator.stack_push("rax");
            };
            void operator()(const Node::Expression::StrLiteral* str_literal) const
            {
                // 1. Extract the raw string value
                std::string val = str_literal->str_lit.value.value();

                // 2. Create a unique label for the .data section
                // We use the current size of m_strings to ensure it's unique (str_0, str_1, etc.)
                std::string label = "str_" + std::to_string(generator.m_strings.size());

                size_t actual_len = 0;
                process_escape_sequences(val, actual_len);

                // 3. Register the string for the .data section emission
                generator.m_strings.push_back({ label, val });

                // 4. Push the Length (Slot 1)
                generator.m_code.generated
                    << generator.m_code.get_tab() << "mov rax, " << actual_len << " ; string length\n";
                generator.stack_push("rax");

                // 5. Push the Address (Slot 2)
                // 'lea' (Load Effective Address) gets the memory address of our label
                generator.m_code.generated
                    << generator.m_code.get_tab() << "lea rax, [" << label << "] ; string pointer\n";
                generator.stack_push("rax");
            };
            void operator()(const Node::Expression::FunctionCall* func_call) const
            {
                // assert(false && "not implemented");
                const auto reverse_functions = generator.m_functions | std::views::reverse;
                const auto function = std::ranges::find_if(std::as_const(reverse_functions), [&](const Function& fun) {
                    return fun.name == func_call->ident.value.value();
                });
                if (function == reverse_functions.end()) {
                    std::cerr << "ya using undeclared functions ya ass" << func_call->current_position().str()
                              << std::endl;
                    exit(EXIT_FAILURE);
                }
                generator.m_code.generated
                    << generator.m_code.get_tab() << "; generate function call " << function->name << "\n";
                if (function->argument_types.size() != func_call->arguments.size()) {
                    std::cerr << func_call->ident.value.value() << "expects " << function->argument_types.size()
                              << " arguments ya ass" << func_call->current_position().str() << std::endl;
                    exit(EXIT_FAILURE);
                }
                size_t args_size = 0;
                for (const Node::Expression::Expression* argument : func_call->arguments) {
                    generator.generate_expression(argument);
                    args_size += generator.infer_type(argument) == Node::VariableType::NUM ? 8 : 16;
                }
                generator.m_code.generated << generator.m_code.get_tab() << "call " << function->function_label << "\n";
                if (args_size > 0) {
                    generator.m_code.generated << generator.m_code.get_tab() << "add rsp, " << args_size << "\n";
                    for (const Node::Expression::Expression* argument : func_call->arguments) {
                        generator.m_stack_counter -= generator.infer_type(argument) == Node::VariableType::NUM ? 1 : 2;
                    }
                }
                if (function->return_type == Node::VariableType::STR) {
                    generator.stack_push("rbx"); // len
                    generator.stack_push("rax"); // ptr
                }
                else {
                    generator.stack_push("rax");
                }
            };
        };

        TermVisitor visitor = { .generator = *this };
        std::visit(visitor, term->term);
    }

    void generate_scope(const Node::Scope* scope)
    {
        m_code.generated << m_code.get_tab() << "; generate scope" << "\n";
        begin_scope();
        for (const Node::Statement::Statement* statement : scope->stmts) {
            generate_statement(statement);
        }
        end_scope();
    }

    Node::VariableType infer_type(const Node::Expression::Expression* expr)
    {
        if (auto* op_ptr = std::get_if<Node::Expression::Operation*>(&expr->expression)) {
            auto* op = *op_ptr;
            if (infer_type(op->left_hand) == Node::VariableType::STR
                || infer_type(op->right_hand) == Node::VariableType::STR) {
                return Node::VariableType::STR;
            }
            return Node::VariableType::NUM;
        }
        else if (auto* term_ptr = std::get_if<Node::Expression::Term*>(&expr->expression)) {
            auto* term = *term_ptr;
            if (std::holds_alternative<Node::Expression::IntLiteral*>(term->term)) {
                return Node::VariableType::NUM;
            }
            if (std::holds_alternative<Node::Expression::StrLiteral*>(term->term)) {
                return Node::VariableType::STR;
            }
            if (std::holds_alternative<Node::Expression::FunctionCall*>(term->term)) {
                return get_function_return_type(
                    std::get<Node::Expression::FunctionCall*>(term->term)->ident.value.value());
            }
            if (auto* ident_ptr = std::get_if<Node::Expression::Identifier*>(&term->term)) {
                // Look up existing variable type
                const auto reverse_variables = m_variables | std::views::reverse;
                auto var = std::ranges::find_if(reverse_variables, [&](const Variable& v) {
                    return v.name == (*ident_ptr)->ident.value.value();
                });
                return (var != reverse_variables.end()) ? var->type : Node::VariableType::NUM;
            }
            if (auto* paren_ptr = std::get_if<Node::Expression::ParenthExpression*>(&term->term)) {
                return infer_type((*paren_ptr)->expression);
            }
        }
        assert(false && "Should never happen");
    }

    void generate_expression(const Node::Expression::Expression* expression)
    {
        struct ExpressionVisitor {
            AssGenerator& generator;

            void operator()(const Node::Expression::Term* term) const
            {
                generator.m_code.generated << generator.m_code.get_tab() << "; generate term" << "\n";
                generator.generate_term(term);
            };
            void operator()(Node::Expression::Operation* operation) const
            {
                auto left_type = generator.infer_type(operation->left_hand);
                auto right_type = generator.infer_type(operation->right_hand);

                if (left_type == Node::VariableType::STR || right_type == Node::VariableType::STR) {
                    if (operation->oprator.value.value() != "+") {
                        auto op = operation->oprator.value.value();
                        std::cerr << "ya cannot perform " << op << "on strings ya ass "
                                  << operation->current_position().str() << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }

                generator.m_code.generated << generator.m_code.get_tab() << "; generate operation" << "\n";
                if (operation->oprator.value.value() == "+") {
                    if (left_type == Node::VariableType::STR || right_type == Node::VariableType::STR) {
                        generator.m_code.generated
                            << generator.m_code.get_tab() << "; --- String Concatenation ---" << "\n";

                        generator.generate_expression(operation->left_hand);
                        generator.generate_expression(operation->right_hand);

                        // 1. Pop RHS (could be 1 or 2 slots)
                        if (right_type == Node::VariableType::STR) {
                            generator.stack_pop("r13"); // ptr
                            generator.stack_pop("r12"); // len
                        }
                        else {
                            generator.stack_pop("rax");
                            generator.m_code.generated << generator.m_code.get_tab() << "UINT2STR rax\n";
                            generator.m_code.generated << generator.m_code.get_tab() << "mov r13, rsi\n";
                            generator.m_code.generated << generator.m_code.get_tab() << "mov r12, rdx\n";
                        }

                        // 2. Pop LHS (could be 1 or 2 slots)
                        if (left_type == Node::VariableType::STR) {
                            generator.stack_pop("r15"); // ptr
                            generator.stack_pop("r14"); // len
                        }
                        else {
                            generator.stack_pop("rax");
                            generator.m_code.generated << generator.m_code.get_tab() << "UINT2STR rax\n";
                            generator.m_code.generated << generator.m_code.get_tab() << "mov r15, rsi\n";
                            generator.m_code.generated << generator.m_code.get_tab() << "mov r14, rdx\n";
                        }

                        generator.m_code.generated << generator.m_code.get_tab() << "CONCAT r14, r15, r12, r13\n";
                        // 4. Push resulting fat pointer
                        generator.stack_push("rdx"); // length
                        generator.stack_push("rsi"); // pointer
                    }
                    else {
                        generator.m_code.generated << generator.m_code.get_tab() << "; generate add" << "\n";
                        generator.generate_expression(operation->left_hand);
                        generator.generate_expression(operation->right_hand);
                        generator.stack_pop("rax");
                        generator.stack_pop("rbx");
                        generator.m_code.generated << generator.m_code.get_tab() << "add rax, rbx\n";
                        generator.stack_push("rax");
                    }
                }
                else if (operation->oprator.value.value() == "-") {
                    generator.m_code.generated << generator.m_code.get_tab() << "; generate subtract" << "\n";
                    generator.generate_expression(operation->left_hand);
                    generator.generate_expression(operation->right_hand);
                    generator.stack_pop("rbx");
                    generator.stack_pop("rax");
                    generator.m_code.generated << generator.m_code.get_tab() << "sub rax, rbx\n";
                    generator.stack_push("rax");
                }
                else if (operation->oprator.value.value() == "*") {
                    generator.m_code.generated << generator.m_code.get_tab() << "; generate multiply" << "\n";
                    generator.generate_expression(operation->left_hand);
                    generator.generate_expression(operation->right_hand);
                    generator.stack_pop("rax");
                    generator.stack_pop("rbx");
                    generator.m_code.generated << generator.m_code.get_tab() << "mul rbx\n";
                    generator.stack_push("rax");
                }
                else if (operation->oprator.value.value() == "/") {
                    generator.m_code.generated << generator.m_code.get_tab() << "; generate divide" << "\n";
                    generator.generate_expression(operation->left_hand);
                    generator.generate_expression(operation->right_hand);
                    generator.stack_pop("rbx");
                    generator.stack_pop("rax");
                    generator.m_code.generated << generator.m_code.get_tab() << "xor rdx, rdx\n";
                    generator.m_code.generated << generator.m_code.get_tab() << "div rbx\n";
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

    Node::VariableType get_function_return_type(const std::string& name)
    {
        const auto function = std::ranges::find_if(std::as_const(m_functions), [&](const Function& function) {
            return function.name == name;
        });
        assert(function != m_functions.end() && "Should not happen");
        return function->return_type;
    }

    void generate_statement(const Node::Statement::Statement* statement)
    {
        struct StatementVisitor {
            AssGenerator& generator;

            void operator()(const Node::Statement::Exit* exit_node) const
            {
                generator.m_code.generated << generator.m_code.get_tab() << "; generate exit" << "\n";
                generator.generate_expression(exit_node->expression);
                generator.m_code.generated << generator.m_code.get_tab() << "mov rax, 60\n";
                generator.stack_pop("rdi");
                generator.m_code.generated << generator.m_code.get_tab() << "syscall\n";
            };
            void operator()(const Node::Statement::Print* print_node) const
            {
                generator.m_code.generated << generator.m_code.get_tab() << "; --- generate print ---" << "\n";

                Node::VariableType type = generator.infer_type(print_node->expression);

                generator.generate_expression(print_node->expression);

                if (type == Node::VariableType::STR) {
                    // Stack has: [Length, Pointer]
                    // We pop in reverse order of the push
                    generator.stack_pop("rsi"); // Pop Pointer into RSI (address of string)
                    generator.stack_pop("rdx"); // Pop Length into RDX (count of bytes)
                }
                else {
                    // Stack has: [Integer Value]
                    generator.stack_pop("rax");
                    generator.m_code.generated << generator.m_code.get_tab() << "UINT2STR rax\n";
                }

                generator.m_code.generated << generator.m_code.get_tab() << "PRINT\n";
            };
            void operator()(const Node::Statement::Let* let_node) const
            {

                const auto reverse_variables = generator.m_variables | std::views::reverse;
                const auto variable = std::ranges::find_if(std::as_const(reverse_variables), [&](const Variable& var) {
                    return var.name == let_node->identifier.value.value();
                });

                if (variable != reverse_variables.end()) {
                    std::cerr << "ya reusin variables ya bitch" << let_node->current_position().str() << std::endl;
                    exit(EXIT_FAILURE);
                }
                generator.m_code.generated << generator.m_code.get_tab() << "; generate variable" << "\n";
                generator.m_variables.push_back(
                    {
                        .name = let_node->identifier.value.value(),
                        .mutable_ = let_node->mutable_,
                        .stack_loc = generator.m_stack_counter,
                        .type = generator.infer_type(let_node->expression),
                        .level = let_node->parent,
                    });
                generator.generate_expression(let_node->expression);
            };
            void operator()(const Node::Statement::Assignment* assign_node) const
            {

                const auto reverse_variables = generator.m_variables | std::views::reverse;
                const auto variable = std::ranges::find_if(std::as_const(reverse_variables), [&](const Variable& var) {
                    return var.name == assign_node->identifier.value.value();
                });

                if (variable == reverse_variables.end()) {
                    std::cerr << "ya usin imaginary variables ya ugly piece of shit"
                              << assign_node->current_position().str() << std::endl;
                    exit(EXIT_FAILURE);
                }

                if (!variable->mutable_) {
                    std::cerr << "ya messign with an immutable variable you dingus"
                              << assign_node->current_position().str() << std::endl;
                    exit(EXIT_FAILURE);
                }
                if (variable->type != generator.infer_type(assign_node->expression)) {
                    std::cerr << "ya cannot reassign types, dingus " << assign_node->current_position().str()
                              << std::endl;
                    exit(EXIT_FAILURE);
                }
                generator.m_code.generated << generator.m_code.get_tab() << "; reassign variable" << "\n";
                generator.generate_expression(assign_node->expression);
                if (variable->type == Node::VariableType::STR) {
                    // Pop the new fat pointer (ptr, then len)
                    generator.stack_pop("rax"); // new ptr
                    generator.stack_pop("rbx"); // new len

                    generator.m_code.generated
                        << generator.m_code.get_tab() << "mov [rsp + "
                        << (generator.m_stack_counter - variable->stack_loc - 1) * 8 << "], rbx"
                        << "\n";
                    generator.m_code.generated
                        << generator.m_code.get_tab() << "mov [rsp + "
                        << (generator.m_stack_counter - (variable->stack_loc + 1) - 1) * 8 << "], rax"
                        << "\n";
                }
                else {
                    generator.stack_pop("rax");
                    generator.m_code.generated
                        << generator.m_code.get_tab() << "mov [rsp + "
                        << (generator.m_stack_counter - variable->stack_loc - 1) * 8 << "], rax"
                        << "\n";
                }
            };
            void operator()(const Node::Scope* scope_node) const
            {
                generator.generate_scope(scope_node);
            };
            void operator()(const Node::Statement::If* if_node) const
            {
                Node::VariableType type = generator.infer_type(if_node->expression);
                generator.generate_expression(if_node->expression);
                if (type == Node::VariableType::STR) {
                    // Stack has: [Length, Pointer]
                    generator.stack_pop("rax"); // Pop the pointer (we don't need it for truthiness)
                    generator.stack_pop("rax"); // Pop the length into RAX
                }
                else {
                    // Stack has: [Integer]
                    generator.stack_pop("rax");
                }
                auto elselabel = generator.create_label();
                auto skiplabel = generator.create_label();
                generator.m_code.generated << generator.m_code.get_tab() << "test rax, rax" << "\n";
                if (if_node->else_.has_value()) {
                    generator.m_code.generated << generator.m_code.get_tab() << "; jump to else" << "\n";
                    generator.m_code.generated << generator.m_code.get_tab() << "jz " << elselabel << "\n";
                }
                else {
                    generator.m_code.generated << generator.m_code.get_tab() << "; jump to skip" << "\n";
                    generator.m_code.generated << generator.m_code.get_tab() << "jz " << skiplabel << "\n";
                }
                generator.m_code.generated << generator.m_code.get_tab() << "; inside if" << "\n";
                generator.generate_scope(if_node->scope);
                generator.m_code.generated << generator.m_code.get_tab() << "jmp " << skiplabel << "\n";

                if (if_node->else_.has_value()) {
                    generator.m_code.generated << generator.m_code.get_tab() << elselabel << ":" << "\n";
                    generator.m_code.generated << generator.m_code.get_tab() << "; inside else" << "\n";
                    if (std::holds_alternative<Node::Scope*>(if_node->else_.value()->else_)) {
                        auto scope = std::get<Node::Scope*>(if_node->else_.value()->else_);
                        generator.generate_scope(scope);
                    }
                    else {
                        auto elseifnode = std::get<Node::Statement::If*>(if_node->else_.value()->else_);
                        (*this)(elseifnode);
                    }
                    generator.m_code.generated << generator.m_code.get_tab() << "jmp " << skiplabel << "\n";
                }

                generator.m_code.generated << generator.m_code.get_tab() << skiplabel << ":" << "\n";
                generator.m_code.generated << generator.m_code.get_tab() << "; outside if-elif chain" << "\n";
            };
            void operator()(const Node::Statement::While* while_node) const
            {
                Node::VariableType type = generator.infer_type(while_node->expression);
                auto conditionlabel = generator.create_label();
                generator.m_code.generated << generator.m_code.get_tab() << conditionlabel << ":" << "\n";
                generator.generate_expression(while_node->expression);
                if (type == Node::VariableType::STR) {
                    // Stack has: [Length, Pointer]
                    generator.stack_pop("rax"); // Pop the pointer (we don't need it for truthiness)
                    generator.stack_pop("rax"); // Pop the length into RAX
                }
                else {
                    // Stack has: [Integer]
                    generator.stack_pop("rax");
                }
                auto skiplabel = generator.create_label();
                generator.m_code.generated << generator.m_code.get_tab() << "test rax, rax" << "\n";
                generator.m_code.generated << generator.m_code.get_tab() << "; jump to skip" << "\n";
                generator.m_code.generated << generator.m_code.get_tab() << "jz " << skiplabel << "\n";
                generator.m_code.generated << generator.m_code.get_tab() << "; inside while" << "\n";
                generator.generate_scope(while_node->scope);
                generator.m_code.generated << generator.m_code.get_tab() << "jmp " << conditionlabel << "\n";

                generator.m_code.generated << generator.m_code.get_tab() << skiplabel << ":" << "\n";
                generator.m_code.generated << generator.m_code.get_tab() << "; outside while loop" << "\n";
            };
            void operator()(const Node::Statement::Function* function_definition) const
            {
                // assert(false && "not implemented");
                Function func;
                func.level = function_definition->parent;
                func.name = function_definition->identifier.value.value();
                func.function_label = FUNC_DEF_LABEL_PREFIX + func.name + "__" + std::to_string(func.level);
                func.function_end_label = FUNC_END_LABEL_PREFIX + func.name + "__" + std::to_string(func.level);
                func.function_ret_label = FUNC_RET_LABEL_PREFIX + func.name + "__" + std::to_string(func.level);
                func.return_type = function_definition->returnType;

                auto actual_stack_counter = generator.m_stack_counter;

                for (const auto& arg : function_definition->arguments) {
                    func.argument_types.push_back(arg->datatype);
                }
                generator.m_functions.push_back(func);

                generator.m_code.generated << generator.m_code.get_tab() << "jmp " << func.function_end_label << "\n";
                generator.m_code.generated << generator.m_code.get_tab() << func.function_label << ":\n";
                generator.begin_scope();
                generator.stack_push("rbp");
                generator.m_code.generated << generator.m_code.get_tab() << "mov rbp, rsp\n";

                // size_t index = 0;
                size_t offset = 16;
                auto& args = function_definition->arguments;
                for (auto it = args.rbegin(); it != args.rend(); ++it) {
                    auto& arg = *it;
                    size_t arg_size = (arg->datatype == Node::VariableType::STR) ? 16 : 8;

                    generator.m_variables.push_back(
                        {
                            .name = arg->identifier.value.value(),
                            .mutable_ = true,
                            .stack_loc = 0,
                            .type = arg->datatype,
                            .argument = true,
                            .level = function_definition->parent,
                            .func_stack_loc = offset,
                        });

                    offset += arg_size;
                }

                generator.m_activeFunction = &generator.m_functions.back();

                for (const Node::Statement::Statement* statement : function_definition->scope->stmts) {
                    generator.generate_statement(statement);
                }
                generator.m_code.generated << generator.m_code.get_tab() << func.function_ret_label << ":\n";

                generator.m_code.start_nest();
                generator.m_code.generated << generator.m_code.get_tab() << "leave\n";
                generator.m_code.generated << generator.m_code.get_tab() << "ret\n";
                generator.m_code.end_nest();

                generator.end_scope();
                generator.m_code.generated << generator.m_code.get_tab() << func.function_end_label << ":\n";

                generator.m_stack_counter = actual_stack_counter;
            };
            void operator()(const Node::Statement::Return* return_stmt) const
            {
                // assert(false && "not implemented");
                if (generator.m_activeFunction == nullptr) {
                    std::cerr << "return from wat ya nasty ass ? return wrks an functions ya stupid mf"
                              << return_stmt->current_position().str() << std::endl;
                    exit(EXIT_FAILURE);
                }
                auto type = generator.infer_type(return_stmt->expression);
                generator.generate_expression(return_stmt->expression);
                if (type == Node::VariableType::NUM) {
                    generator.stack_pop("rax");
                }
                else {
                    generator.stack_pop("rax"); // ptr
                    generator.stack_pop("rbx"); // len
                }
                generator.m_code.generated
                    << generator.m_code.get_tab() << "jmp " << generator.m_activeFunction->function_ret_label << "\n";
            };
        };

        StatementVisitor visitor = { .generator = *this };
        std::visit(visitor, statement->statement);
    }

    struct Variable {
        std::string name;
        bool mutable_;
        size_t stack_loc;
        Node::VariableType type;

        bool argument = false;
        unsigned int level;
        size_t func_stack_loc = 0;
    };
    struct StringConstant {
        std::string label;
        std::string value;
    };

    struct Function {
        std::string name;
        Node::VariableType return_type;
        std::vector<Node::VariableType> argument_types;

        std::string function_label;
        std::string function_ret_label;
        std::string function_end_label;

        unsigned int level;
    };

    const Node::Program m_prog;
    size_t m_stack_counter = 0;
    std::vector<Variable> m_variables { };
    std::vector<StringConstant> m_strings { };
    std::vector<Function> m_functions { };
    std::vector<size_t> m_scopes { };
    ArenaAllocator* m_allocator;
    int m_label_count = 0;
    Function* m_activeFunction = nullptr;
    code m_code;
};