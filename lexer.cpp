#include "types.h"
#include "lexer.h"

namespace core {
    static std::string get_next_token(input_file& f) {
        char buf[4096];
        int buf_len = 0;
        char c = f.last_char;
        if(!c) c = ' ';
        
        // eat whitespace
        while((c == ' ' || c == '\n' || c == '\t') && !is_eof(f)) {
            if(c == '\n') {
                f.line++;
                f.col = 0;
            }
            c = fgetc(f.fd);
            f.col++;
        }
        
        if(!is_eof(f)) {
            if(!isalnum(c)) {
                buf[0] = c;
                buf[1] = 0;
                f.last_char = ' ';
                return std::string(buf);
            }
            
            while(c != ' ' && buf_len < 4095 && (isalnum(c) || c == '.')) {
                buf[buf_len++] = c;
                c = fgetc(f.fd);
                f.col++;
            }
            buf[buf_len] = 0;
            
            f.last_char = c;
            return std::string(buf);
        }
        return std::string();
    }
    
    static bool is_literal(const std::string& s) {
        bool ret = false;
        if(!ret)
        {
            bool is_digit_or_point = true;
            for(int i = 0; i < s.size() && is_digit_or_point; i++) {
                if((s[i] < '0' || s[i] > '9') && (s[i] != '.')) {
                    is_digit_or_point = false;
                }
            }
            ret = is_digit_or_point;
        }
        if(!ret) {
            ret = s == "false" || s == "true";
        }
        if(!ret) {
            // TODO: check for other types here
        }
        return ret;
    }
    
    static bool is_type(const std::string& s) {
        bool ret = false;
        if(s == "real") {
            ret = true;
        } else if(s == "bool") {
            ret = true;
        }
        // TODO: etc.
        return ret;
    }
    
    static bool is_identifier(const std::string& s) {
        bool ret = true;
        
        if(!isalpha(s[0])) {
            ret = false;
        }
        
        for(int i = 1; i < s.size() && ret; i++) {
            if(!isalnum(s[i])) {
                ret = false;
            }
        }
        
        return ret;
    }
    
    static bool is_operator(const std::string& s) {
        bool ret = false;
        
        if(s == "+") {
            ret = true;
        } else if(s == "-") {
            ret = true;
        } else if(s == "*") {
            ret = true;
        } else if(s == "/") {
            ret = true;
        } else if(s == "=") {
            ret = true;
        } else if(s == "?") {
            ret = true;
        } else if(s == "!") {
            ret = true;
        } else if(s == "<") {
            ret = true;
        } else if(s == ">") {
            ret = true;
        }
        
        return ret;
    }
    
    token get_token(input_file& f) {
        auto s = get_next_token(f);
        if(s == "fn") {
            return {tok_t::fn, s};
        } else if(s == "extern") {
            return {tok_t::ext, s};
        } else if(s == "(") {
            return {tok_t::paren_open, s};
        } else if(s == ")") {
            return {tok_t::paren_close, s};
        } else if(s == ";") {
            return {tok_t::semicolon, s};
        } else if(s == "{") {
            return {tok_t::curly_open, s};
        } else if(s == "}") {
            return {tok_t::curly_close, s};
        } else if(s == ":") {
            return {tok_t::colon, s};
        } else if(s == "if") {
            return {tok_t::cif, s};
        } else if(s == "then") {
            return {tok_t::cthen, s};
        } else {
            if(is_literal(s)) {
                return {tok_t::literal, s};
            } else if(is_type(s)) {
                return {tok_t::type, s};
            } else if(is_operator(s)) {
                return {tok_t::oper, s};
            } else if(is_identifier(s)) {
                return {tok_t::identifier, s};
            } else {
                return {tok_t::unknown, s};
            }
        }
    }
}