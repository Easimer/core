#pragma once

#include "types.h"

#include <memory>
#include <string>
#include <vector>

namespace core {
    const int max_variable_name_len = 32;
    
    enum class ast_operator {
        invalid = 0,
        
        assignment,
        
        addition,
        subtraction,
        multiplication,
        division,
        
        max,
    };
    
    template<typename T>
        using ast_ptr = std::unique_ptr<T>;
    
    class ast_expr {
        public:
        virtual ~ast_expr() {}
        
        virtual void dump() = 0;
    };
    using ast_expr_ptr = ast_ptr<ast_expr>;
    
    class ast_literal : public ast_expr {};
    using ast_literal_ptr = ast_ptr<ast_literal>;
    
    class ast_real : public ast_literal {
        public:
        ast_real(float v) : value(v) {}
        float value;
        virtual void dump() override;
    };
    using ast_real_ptr = ast_ptr<ast_real>;
    
    class ast_int : public ast_literal {
        public:
        ast_int(s32 v) : is_signed(true) {
            value.value_signed = v;
        }
        
        ast_int(u32 v) : is_signed(false) {
            value.value_unsigned = v;
        }
        
        union {
            s32 value_signed;
            u32 value_unsigned;
        } value;
        bool is_signed;
        
        virtual void dump() override;
    };
    using ast_int_ptr = ast_ptr<ast_int>;
    
    class ast_var : public ast_expr {
        public:
        
        ast_var(const std::string& other) : name(other) {}
        ast_var(std::string&& other) : name(std::move(other)) {}
        
        const std::string& get() const {
            return name;
        }
        
        virtual void dump() override;
        
        private:
        std::string name;
    };
    using ast_var_ptr = ast_ptr<ast_var>;
    
    class ast_binary_op : public ast_expr {
        ast_operator op;
        ast_expr_ptr left, right;
        
        virtual void dump() override;
    };
    using ast_binary_op_ptr = ast_ptr<ast_binary_op>;
    
    class ast_fn_call : public ast_expr {
        ast_var_ptr name;
        std::vector<ast_expr_ptr> args;
        
        virtual void dump() override;
    };
    using ast_fn_call_ptr = ast_ptr<ast_fn_call>;
}