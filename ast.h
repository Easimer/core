#pragma once

#include <vector>
#include <unordered_map>

#include "types.h"
#include "type.h"

#define OVERRIDE_GEN_IR() virtual llvm::Value* generate_ir(llvm_ctx& ctx) override

namespace core {
    class ast_expression {
        public:
        int line = 0, col = 0;
        
        virtual ~ast_expression() {}
        
        virtual void dump() = 0;
        virtual bool is_empty() { return false; }
        virtual llvm::Value* generate_ir(llvm_ctx& ctx) { return nullptr; };
    };
    
    class ast_empty : public ast_expression {
        virtual void dump() override {};
        virtual bool is_empty() override { return true; }
    };
    
    class ast_literal : public ast_expression {
        public:
        ast_literal(const std::string& value) : value(value) {}
        
        std::string value;
        bool is_real = false;
        bool is_int = false;
        bool is_bool = false;
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_identifier : public ast_expression {
        public:
        
        ast_identifier(const char* pszName);
        
        char name[128];
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_declaration : public ast_expression {
        public:
        
        up<ast_identifier> identifier;
        sp<core::type> type;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_prototype : public ast_expression {
        public:
        up<ast_expression> name;
        std::vector<ast_declaration> args;
        up<ast_identifier> type;
        bool is_pure;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_function : public ast_expression {
        public:
        up<ast_prototype> prototype;
        std::vector<up<ast_expression>> lines;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_binary_op : public ast_expression {
        public:
        
        ast_binary_op(char op, up<ast_expression> lhs, up<ast_expression> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        
        char op;
        up<ast_expression> lhs, rhs;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_function_call : public ast_expression {
        public:
        up<ast_identifier> name;
        std::vector<up<ast_expression>> args;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_branching : public ast_expression {
        public:
        up<ast_expression> condition;
        up<ast_expression> line;
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
    
    class ast_type : public ast_expression {
        public:
        sp<type> pType;
        
        virtual void dump() override;
        OVERRIDE_GEN_IR();
    };
}