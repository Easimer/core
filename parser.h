#pragma once

#include "types.h"
#include "ast.h"

#include <vector>
#include <string>

namespace core {
    ast_real_ptr parse_real(const std::string& expr);
    ast_int_ptr parse_int(const std::string& expr);
    ast_int_ptr parse_uint(const std::string& expr);
    ast_literal_ptr parse_literal(const std::string& expr);
    bool is_literal(const std::string& expr);
    
    struct function_argument {
        bool bound;
        ast_expr_ptr value; // ast_var_ptr or ast_literal_ptr depending on the value of bound
    };
    
    struct function_overload {
        std::vector<function_argument> arguments;
        std::vector<ast_expr_ptr> lines;
    };
    
    struct function {
        std::string name;
        std::vector<function_overload> overloads;
    };
}