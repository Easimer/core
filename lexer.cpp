#include "lexer.h"

#include <string>

namespace core {
    core::tok_t get_token(std::string& buf, input_file& f) {
        static char c = ' ';
        while(isspace(c)) {
            c = fgetc(f.fd);
        }
        
        
        buf = std::string();
        
        if(isalpha(c)) {
            do {
                buf += c;
                c = fgetc(f.fd);
            } while(isalnum(c));
            
            if(buf == "extern") {
                return core::tok_t::ext;
            } else if(buf == "fn") {
                return core::tok_t::fn;
            } else {
                return core::tok_t::identifier;
            }
        } else if(isdigit(c)) {
            bool dot = false;
            do {
                if(c == '.') {
                    if(!dot) {
                        buf += c;
                        dot = true;
                    } else {
                        DIAG_ERR("Duplicate decimal point in literal %s\n", buf.c_str());
                    }
                } else {
                    buf += c;
                }
                c = fgetc(f.fd);
            } while(isdigit(c) || c == '.');
            return core::tok_t::literal;
        } else {
            if(c == '(') {
                buf = '(';
                c = fgetc(f.fd);
                return core::tok_t::paren_open;
            } else if(c == ')') {
                buf = ')';
                c = fgetc(f.fd);
                return core::tok_t::paren_close;
            } else if(c == ';') {
                buf = ';';
                c = fgetc(f.fd);
                return core::tok_t::eob;
            } else if(c == ',') {
                buf = ',';
                c = fgetc(f.fd);
                return core::tok_t::eol;
            } else {
                buf = c;
                c = fgetc(f.fd);
                return core::tok_t::unknown;
            }
        }
        
        return core::tok_t::invalid;
    }
}
