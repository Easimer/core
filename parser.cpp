#include "parser.h"

namespace core {
    ast_real_ptr parse_real(const std::string& expr) {
        auto val = std::stod(expr.c_str());
        return std::make_unique<ast_real>(val);
    }
    
    ast_int_ptr parse_int(const std::string& expr) {
        s32 val = std::stoi(expr.c_str());
        return std::make_unique<ast_int>(val);
    }
    
    ast_int_ptr parse_uint(const std::string& expr) {
        u32 val = std::stoi(expr.c_str());
        return std::make_unique<ast_int>(val);
    }
    
    ast_literal_ptr parse_literal(const std::string& expr) {
        ast_literal_ptr ret = nullptr;
        bool is_number = true;
        bool is_real = false;
        bool is_signed = false;
        
        for(int i = 0; i < expr.size(); i++) {
            char c = expr[i];
            bool digit = (c >= '0' && c <= '9');
            bool dot = c == '.';
            bool sign = c == '-' || c == '+';
            
            if(!digit && !dot && !sign) {
                is_number = false;
            }
            if(dot) {
                is_real = true;
            }
            if(sign) {
                is_signed = true;
            }
        }
        
        if(is_number) {
            if(is_real) {
                auto value = std::stod(expr.c_str());
                ret = std::unique_ptr<ast_literal>(new ast_real(value));
            } else {
                if(is_signed) {
                    s32 value = std::stoi(expr.c_str());
                    ret = std::unique_ptr<ast_literal>(new ast_int(value));
                } else {
                    u32 value = std::stoi(expr.c_str());
                    ret = std::unique_ptr<ast_literal>(new ast_int(value));
                }
            }
        }
        
        return ret;
    }
    
    bool is_literal(const std::string& expr) {
        bool is_number = true;
        bool is_real = false;
        bool is_signed = false;
        
        for(int i = 0; i < expr.size(); i++) {
            char c = expr[i];
            bool digit = (c >= '0' && c <= '9');
            bool dot = c == '.';
            bool sign = c == '-' || c == '+';
            
            if(!digit && !dot && !sign) {
                is_number = false;
            }
            if(dot) {
                is_real = true;
            }
            if(sign) {
                is_signed = true;
            }
        }
    }
    
    ast_operator parse_operator(const std::string& expr) {
        if(expr == "=") return ast_operator::assignment;
        if(expr == "+") return ast_operator::addition;
        if(expr == "-") return ast_operator::subtraction;
        if(expr == "*") return ast_operator::multiplication;
        if(expr == "/") return ast_operator::division;
        return ast_operator::invalid;
    }
    
    
}