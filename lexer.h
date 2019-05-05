#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <list>

namespace core {
    enum class tok_t {
        invalid = 0,
        unknown,
        
        literal,
        type,
        identifier,
        oper,
        
        // keywords
        fn, ext, cif, cthen, pure,
        
        paren_open, paren_close,
        semicolon,
        curly_open, curly_close,
        colon,
        
        max
    };
    
    struct token {
        tok_t first;
        std::string second;
        int line;
        int col;
    };
    
    token get_token(input_file& f);
    
    struct token_stream {
        tok_t type() const {
            return c.front().first;
        }
        
        const std::string& current() const {
            return c.front().second;
        }
        
        void step() {
            c.pop_front();
        }
        
        bool empty() const {
            return c.size() == 0;
        }
        
        int line() const {
            return c.front().line;
        }
        
        int col() const {
            return c.front().col;
        }
        
        std::list<token> c;
    };
}