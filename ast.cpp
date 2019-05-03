#include "ast.h"

#include <cstring>
#include <iostream>

namespace core {
    void ast_real::dump() {
        std::cerr << "real(" << value << ')' << std::endl;
    }
    
    void ast_int::dump() {
        if(is_signed) {
            std::cerr << "signed(" << value.value_signed << ')' << std::endl;
        } else {
            std::cerr << "unsigned(" << value.value_unsigned << ')' << std::endl;
        }
    }
    
    void ast_var::dump() {
        std::cerr << "var(" << name << ')' << std::endl;
    }
    
    void ast_binary_op::dump() {
        std::cerr << "binop(";
        left->dump();
        std::cerr << ' ' << (int)op << ' ';
        right->dump();
        std::cerr << ')' << std::endl;
    }
    
    void ast_fn_call::dump()  {
        std::cerr << "fncall(" << name->get() << ", ";
        for(auto& arg : args) {
            ast_var* pvar;
            ast_literal* plit;
            
            pvar = dynamic_cast<ast_var*>(arg.get());
            plit = dynamic_cast<ast_literal*>(arg.get());
            if(pvar) {
                pvar->dump();
            } else if(plit) {
                plit->dump();
            } else {
                std::cerr << "???";
            }
            std::cerr << ", ";
        }
        std::cerr << ")" << std::endl;
    }
}