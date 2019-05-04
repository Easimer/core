#pragma once

#include <vector>
#include "types.h"

namespace core {
    class ast_expression {
        public:
        virtual ~ast_expression() {}
        
        virtual void dump() = 0;
        virtual void codegen() {};
    };
    
    class ast_literal : public ast_expression {};
    
    class ast_real : public ast_literal {
        public:
        float value;
        
        virtual void dump() override;
    };
    
    class ast_identifier : public ast_expression {
        public:
        
        ast_identifier(const char* pszName);
        
        char name[128];
        virtual void dump() override;
    };
    
    class ast_declaration : public ast_expression {
        public:
        
        up<ast_identifier> identifier;
        std::string type;
        
        virtual void dump() override;
    };
    
    class ast_prototype : public ast_expression {
        public:
        up<ast_expression> name;
        std::vector<ast_declaration> args;
        
        virtual void dump() override;
    };
    
    class ast_function : public ast_expression {
        public:
        up<ast_prototype> prototype;
        std::vector<up<ast_expression>> lines;
        
        virtual void dump() override;
    };
    
    class ast_binary_op : public ast_expression {
        public:
        
        ast_binary_op(char op, up<ast_expression> lhs, up<ast_expression> rhs)
            : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
        
        char op;
        up<ast_expression> lhs, rhs;
        
        virtual void dump() override;
    };
    
    class ast_function_call : public ast_expression {
        public:
        up<ast_identifier> name;
        std::vector<up<ast_expression>> args;
        
        virtual void dump() override;
    };
}