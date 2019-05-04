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
        fn, ext, cif, cthen,
        
        paren_open, paren_close,
        semicolon,
        curly_open, curly_close,
        colon,
        
        max
    };
    
    using token = std::pair<tok_t, std::string>;
    
    token get_token(input_file& f);
    
    struct token_stream {
        tok_t type() {
            return c.front().first;
        }
        
        const std::string& current() {
            return c.front().second;
        }
        
        void step() {
            c.pop_front();
        }
        
        bool empty() {
            return c.size() == 0;
        }
        
        std::list<token> c;
    };
}