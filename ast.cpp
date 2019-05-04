#include <cstdio>
#include <cstring>

#include "ast.h"

namespace core {
    void ast_real::dump() {
        printf("literal(%f)", value);
    }
    
    ast_identifier::ast_identifier(const char* pszName) {
        strncpy(name, pszName, 128);
    }
    
    void ast_identifier::dump() {
        printf("identifier(%s)", name);
    }
    
    void ast_declaration::dump() {
        printf("declaration(");
        identifier->dump();
        printf(" : %s)", type.c_str());
    }
    
    void ast_prototype::dump() {
        printf("prototype(");
        name->dump();
        printf("; ");
        for(auto& arg : args) {
            arg.dump();
            printf(", ");
        }
        printf(")");
    }
    
    void ast_function::dump() {
        printf("function(");
        prototype->dump(); printf(" {\n");
        for(auto& line : lines) {
            line->dump(); printf(";\n");
        }
        printf("})");
    }
    
    void ast_binary_op::dump() {
        printf("bin_op(%c;", op);
        if(lhs) lhs->dump();
        printf(";");
        if(rhs) rhs->dump();
        printf(")");
    }
    
    void ast_function_call::dump() {
        printf("fncall(");
        name->dump(); printf("; ");
        for(auto& arg : args) {
            arg->dump(); printf(", ");
        }
        printf(")");
    }
}