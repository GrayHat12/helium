#pragma once

#include "./parser.hpp"
#include <cassert>
#include <ranges>
#include <utility>
#include <variant>

const std::string RuntimeHelper = R"(
section .bss
    ; A small buffer for itoa conversions (max 20 digits for 64-bit int)
    itoa_buf resb 24 
    ; Store the current heap pointer
    heap_ptr resq 1

section .text

; --- itoa: converts RAX to string ---
; Returns: RAX = pointer, RDX = length
_itoa:
    mov rbx, 10
    mov rcx, itoa_buf + 23
    xor rsi, rsi
.loop:
    xor rdx, rdx
    div rbx
    add dl, '0'
    dec rcx
    mov [rcx], dl
    inc rsi
    test rax, rax
    jnz .loop
    mov rax, rcx
    mov rdx, rsi
    ret

; --- runtime_concat ---
; Inputs: R15/R14 (LHS ptr/len), R13/R12 (RHS ptr/len)
; Returns: RAX (new ptr), RDX (total len)
_runtime_concat:
    ; 1. Calculate total length
    mov rdx, r14
    add rdx, r12
    push rdx            ; Save total length to return later

    ; 2. Get current heap break if not initialized
    mov rax, [heap_ptr]
    test rax, rax
    jnz .allocate
    mov rax, 12         ; sys_brk
    xor rdi, rdi        ; 0 returns current break
    syscall
    mov [heap_ptr], rax

.allocate:
    mov rdi, [heap_ptr]
    add rdi, [rsp]      ; new break = old break + total length
    mov rax, 12         ; sys_brk
    syscall             ; RAX now has the NEW break
    
    mov rbx, [heap_ptr] ; RBX = start of our new memory
    mov [heap_ptr], rax ; Update heap_ptr for next call
    
    ; 3. Copy LHS
    mov rsi, r15        ; src
    mov rdi, rbx        ; dest
    mov rcx, r14        ; len
    rep movsb           ; copy bytes
    
    ; 4. Copy RHS
    mov rsi, r13        ; src
    ; RDI is already pointing to the end of LHS after rep movsb
    mov rcx, r12        ; len
    rep movsb
    
    mov rax, rbx        ; Return the start of the new string
    pop rdx             ; Return the total length
    ret
)";

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
        m_asmout.clear();
        m_asmout << "global _start\n_start:\n";

        for (const Node::Statement::Statement* statement : m_prog.stmts) {
            generate_statement(statement);
        }

        // default this runs
        m_asmout << "    ; default execution\n";
        m_asmout << "    mov rax, 60\n";
        m_asmout << "    mov rdi, 0\n";
        m_asmout << "    syscall\n";
        // static strings
        if (m_strings.size() > 0) {
            m_asmout << "section .data\n";
        }
        for (const auto str : m_strings) {
            size_t length = 0;
            std::string processed = process_escape_sequences(str.value, length);
            m_asmout << "    " << str.label << " db \"" << processed << "\", 0\n";
        }
        // runtime helpers
        m_asmout << RuntimeHelper << "\n";
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
            m_asmout << "    add rsp, " << total_slots_to_pop * 8 << "\n";
            m_stack_counter -= total_slots_to_pop;
        }

        // Remove from our metadata tracking
        for (size_t i = 0; i < vars_in_scope; i++) {
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
                const auto variable
                    = std::ranges::find_if(std::as_const(generator.m_variables), [&](const Variable& var) {
                          return var.name == identifier_node->ident.value.value();
                      });
                if (variable == generator.m_variables.cend()) {
                    std::cerr << "ya using undeclared variables ya ass" << identifier_node->current_position().str()
                              << std::endl;
                    exit(EXIT_FAILURE);
                }
                generator.m_asmout << "    ; generate identifier" << "\n";
                if (variable->type == Node::VariableType::STR) {
                    size_t len_offset = (generator.m_stack_counter - variable->stack_loc - 1) * 8;
                    generator.m_asmout << "    mov rax, QWORD [rsp + " << len_offset << "]\n";
                    generator.stack_push("rax");

                    size_t ptr_offset = (generator.m_stack_counter - (variable->stack_loc + 1) - 1) * 8;
                    generator.m_asmout << "    mov rax, QWORD [rsp + " << ptr_offset << "]\n";
                    generator.stack_push("rax");
                }
                else {
                    std::stringstream register_name;
                    register_name << "QWORD [rsp + " << (generator.m_stack_counter - variable->stack_loc - 1) * 8
                                  << "]";
                    generator.stack_push(register_name.str());
                }
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
                generator.m_asmout << "    mov rax, " << actual_len << " ; string length\n";
                generator.stack_push("rax");

                // 5. Push the Address (Slot 2)
                // 'lea' (Load Effective Address) gets the memory address of our label
                generator.m_asmout << "    lea rax, [" << label << "] ; string pointer\n";
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
            if (auto* ident_ptr = std::get_if<Node::Expression::Identifier*>(&term->term)) {
                // Look up existing variable type
                auto var = std::ranges::find_if(m_variables, [&](const Variable& v) {
                    return v.name == (*ident_ptr)->ident.value.value();
                });
                return (var != m_variables.end()) ? var->type : Node::VariableType::NUM;
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
                generator.m_asmout << "    ; generate term" << "\n";
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

                generator.m_asmout << "    ; generate operation" << "\n";
                if (operation->oprator.value.value() == "+") {
                    if (left_type == Node::VariableType::STR || right_type == Node::VariableType::STR) {
                        generator.m_asmout << "    ; --- String Concatenation ---" << "\n";

                        generator.generate_expression(operation->left_hand);
                        generator.generate_expression(operation->right_hand);

                        // 1. Pop RHS (could be 1 or 2 slots)
                        if (right_type == Node::VariableType::STR) {
                            generator.stack_pop("r13"); // ptr
                            generator.stack_pop("r12"); // len
                        }
                        else {
                            generator.stack_pop("rax");
                            generator.m_asmout << "    call _itoa\n"; // Convert RAX to fat pointer in RAX/RDX
                            generator.m_asmout << "    mov r13, rax\n";
                            generator.m_asmout << "    mov r12, rdx\n";
                        }

                        // 2. Pop LHS (could be 1 or 2 slots)
                        if (left_type == Node::VariableType::STR) {
                            generator.stack_pop("r15"); // ptr
                            generator.stack_pop("r14"); // len
                        }
                        else {
                            generator.stack_pop("rax");
                            generator.m_asmout << "    call _itoa\n";
                            generator.m_asmout << "    mov r15, rax\n";
                            generator.m_asmout << "    mov r14, rdx\n";
                        }

                        generator.m_asmout << "    call _runtime_concat\n";
                        // 4. Push resulting fat pointer
                        generator.stack_push("rdx"); // length
                        generator.stack_push("rax"); // pointer
                    }
                    else {
                        generator.m_asmout << "    ; generate add" << "\n";
                        generator.generate_expression(operation->left_hand);
                        generator.generate_expression(operation->right_hand);
                        generator.stack_pop("rax");
                        generator.stack_pop("rbx");
                        generator.m_asmout << "    add rax, rbx\n";
                        generator.stack_push("rax");
                    }
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
            void operator()(const Node::Statement::Print* print_node) const
            {
                generator.m_asmout << "    ; --- generate print ---" << "\n";

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
                    generator.m_asmout << "    call _itoa\n";
                    generator.m_asmout << "    mov rsi, rax\n";
                    generator.m_asmout << "    mov rdx, rdx\n";
                }

                generator.m_asmout << "    mov rax, 1      ; sys_write\n";
                generator.m_asmout << "    mov rdi, 1      ; stdout\n";
                generator.m_asmout << "    syscall\n";
            };
            void operator()(const Node::Statement::Let* let_node) const
            {

                const auto variable
                    = std::ranges::find_if(std::as_const(generator.m_variables), [&](const Variable& var) {
                          return var.name == let_node->identifier.value.value();
                      });

                if (variable != generator.m_variables.cend()) {
                    std::cerr << "ya reusin variables ya bitch" << let_node->current_position().str() << std::endl;
                    exit(EXIT_FAILURE);
                }
                generator.m_asmout << "    ; generate variable" << "\n";
                generator.m_variables.push_back(
                    {
                        .name = let_node->identifier.value.value(),
                        .mutable_ = let_node->mutable_,
                        .stack_loc = generator.m_stack_counter,
                        .type = generator.infer_type(let_node->expression),
                    });
                generator.generate_expression(let_node->expression);
            };
            void operator()(const Node::Statement::Assignment* assign_node) const
            {

                const auto variable
                    = std::ranges::find_if(std::as_const(generator.m_variables), [&](const Variable& var) {
                          return var.name == assign_node->identifier.value.value();
                      });

                if (variable == generator.m_variables.cend()) {
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
                generator.m_asmout << "    ; reassign variable" << "\n";
                generator.generate_expression(assign_node->expression);
                if (variable->type == Node::VariableType::STR) {
                    // Pop the new fat pointer (ptr, then len)
                    generator.stack_pop("rax"); // new ptr
                    generator.stack_pop("rbx"); // new len

                    generator.m_asmout << "    mov [rsp + " << (generator.m_stack_counter - variable->stack_loc - 1) * 8
                                       << "], rbx" << "\n";
                    generator.m_asmout << "    mov [rsp + "
                                       << (generator.m_stack_counter - (variable->stack_loc + 1) - 1) * 8 << "], rax"
                                       << "\n";
                }
                else {
                    generator.stack_pop("rax");
                    generator.m_asmout << "    mov [rsp + " << (generator.m_stack_counter - variable->stack_loc - 1) * 8
                                       << "], rax" << "\n";
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
                generator.m_asmout << "    test rax, rax" << "\n";
                if (if_node->else_.has_value()) {
                    generator.m_asmout << "    ; jump to else" << "\n";
                    generator.m_asmout << "    jz " << elselabel << "\n";
                }
                else {
                    generator.m_asmout << "    ; jump to skip" << "\n";
                    generator.m_asmout << "    jz " << skiplabel << "\n";
                }
                generator.m_asmout << "    ; inside if" << "\n";
                generator.generate_scope(if_node->scope);
                generator.m_asmout << "    jmp " << skiplabel << "\n";

                if (if_node->else_.has_value()) {
                    generator.m_asmout << elselabel << ":" << "\n";
                    generator.m_asmout << "    ; inside else" << "\n";
                    if (std::holds_alternative<Node::Scope*>(if_node->else_.value()->else_)) {
                        auto scope = std::get<Node::Scope*>(if_node->else_.value()->else_);
                        generator.generate_scope(scope);
                    }
                    else {
                        auto elseifnode = std::get<Node::Statement::If*>(if_node->else_.value()->else_);
                        (*this)(elseifnode);
                    }
                    generator.m_asmout << "    jmp " << skiplabel << "\n";
                }

                generator.m_asmout << skiplabel << ":" << "\n";
                generator.m_asmout << "    ; outside if-elif chain" << "\n";
            };
            void operator()(const Node::Statement::While* while_node) const
            {
                Node::VariableType type = generator.infer_type(while_node->expression);
                auto conditionlabel = generator.create_label();
                generator.m_asmout << conditionlabel << ":" << "\n";
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
                generator.m_asmout << "    test rax, rax" << "\n";
                generator.m_asmout << "    ; jump to skip" << "\n";
                generator.m_asmout << "    jz " << skiplabel << "\n";
                generator.m_asmout << "    ; inside while" << "\n";
                generator.generate_scope(while_node->scope);
                generator.m_asmout << "    jmp " << conditionlabel << "\n";

                generator.m_asmout << skiplabel << ":" << "\n";
                generator.m_asmout << "    ; outside while loop" << "\n";
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
    };
    struct StringConstant {
        std::string label;
        std::string value;
    };

    std::stringstream coutmap() const
    {
        std::stringstream out;
        for (const Variable& variable : m_variables) {
            out << "Variable name=" << variable.name << " value=" << variable.stack_loc
                << " mutable=" << variable.mutable_ << " | ";
        }
        return out;
    }

    const Node::Program m_prog;
    std::stringstream m_asmout;
    size_t m_stack_counter = 0;
    std::vector<Variable> m_variables {};
    std::vector<StringConstant> m_strings {};
    std::vector<size_t> m_scopes {};
    ArenaAllocator* m_allocator;
    int m_label_count = 0;
};