#include "parser.h"
#include <cassert>

namespace core {
    static up<ast_expression> parse_expression(token_stream& ts);
    static up<ast_expression> parse_primary(token_stream& ts);
    
    static int operator_precedence(char op) {
        switch(op) {
            case '+': return 5;
            case '-': return 5;
            case '*': return 10;
            case '/': return 10;
            case '=': return 0;
        }
        return -1;
    }
    
    static ast_declaration parse_declaration(token_stream& ts) {
        std::string name, type;
        ast_declaration ret;
        
        assert(ts.type() == tok_t::identifier);
        
        if(ts.type() != tok_t::identifier) {
            fprintf(stderr, "Expected identifier in declaration\n");
            return ret;
        }
        
        name = ts.current();
        
        ts.step();
        
        if(ts.type() != tok_t::colon) {
            fprintf(stderr, "Expected colon in declaration\n");
            return ret;
        }
        
        ts.step();
        
        if(ts.type() != tok_t::type) {
            fprintf(stderr, "Expected type in declaration\n");
            return ret;
        }
        
        type = ts.current();
        
        ts.step();
        
        ret.identifier = std::make_unique<ast_identifier>(name.c_str());
        ret.type = type;
        
        return ret;
    }
    
    static up<ast_expression> parse_paren_expr(token_stream& ts) {
        assert(ts.type() == tok_t::paren_open);
        ts.step();
        auto E = parse_expression(ts);
        if(!E) {
            return nullptr;
        }
        
        assert(ts.type() == tok_t::paren_close);
        ts.step();
        return E;
    }
    
    static up<ast_expression> parse_literal(token_stream& ts) {
        block_msg __bpl("parse literal");
        const auto& s = ts.current();
        // Is real number
        bool is_real = true;
        for(int i = 0; i < s.size() && is_real; i++) {
            if((s[i] < '0' || s[i] > '9') && (s[i] != '.')) {
                is_real = false;
            }
        }
        if(is_real) {
            auto ret = std::make_unique<ast_real>();
            ret->value = std::stod(s.c_str());
            ts.step();
            return ret;
        }
        
        printf("Unknown type of literal encountered: '%s'\n", s.c_str());
        ts.step();
        return nullptr;
    }
    
    static up<ast_expression> parse_identifier(token_stream& ts, bool not_a_call = false) {
        assert(ts.type() == tok_t::identifier);
        
        block_msg __bpi("parse identifier");
        
        if(ts.type() != tok_t::identifier) {
            fprintf(stderr, "Expected identifier in declaration\n");
            return nullptr;
        }
        
        auto name = ts.current();
        auto id = std::make_unique<ast_identifier>(name.c_str());
        
        ts.step(); // Eat identifier
        if(ts.type() == tok_t::colon) { // declaration with type
            block_msg __bdwt("declaration with type");
            auto ret = std::make_unique<ast_declaration>();
            ret->identifier = std::move(id);
            ts.step();
            ret->type = ts.current();
            ts.step();
            return ret;
        } else if(ts.type() == tok_t::paren_open && !not_a_call) {
            block_msg __bfc("fn call");
            auto ret = std::make_unique<ast_function_call>();
            ret->name = std::move(id);
            ts.step(); // Eat paren open
            while(ts.type() != tok_t::paren_close) {
                auto arg = parse_primary(ts);
                ret->args.push_back(std::move(arg));
                if(ts.current() == ",") {
                    ts.step();
                }
            }
            ts.step();
            return ret;
        } else {
            return id;
        }
    }
    
    static up<ast_expression> parse_primary(token_stream& ts) {
        block_msg __bpp("parse primary");
        switch(ts.type()) {
            case tok_t::identifier:
            return parse_identifier(ts);
            case tok_t::literal:
            return parse_literal(ts);
            case tok_t::paren_open:
            return parse_paren_expr(ts);
            default:
            printf("Unknown token '%s' of type %d\n",ts.current().c_str(), (int)ts.type());
            break;
        }
        return nullptr;
    }
    
    static up<ast_expression> parse_binary_operation_rhs(token_stream& ts, int expr_prec, up<ast_expression> lhs) {
        block_msg __bpbor("parse binary operation rhs");
        while(1) {
            char bin_op = ts.current()[0];
            int tok_prec = operator_precedence(bin_op);
            if(tok_prec < expr_prec) {
                return lhs;
            }
            
            ts.step();
            
            auto rhs = parse_primary(ts);
            if(!rhs) {
                printf("Expected right hand side of operation\n");
                return nullptr;
            }
            
            int next_prec = operator_precedence(ts.current()[0]);
            if(tok_prec < next_prec) {
                block_msg __bpborr("parse binary operation rhs recurse");
                rhs = parse_binary_operation_rhs(ts, tok_prec + 1, std::move(rhs));
                if(!rhs) {
                    return nullptr;
                }
            }
            
            lhs = std::make_unique<ast_binary_op>(bin_op, std::move(lhs), std::move(rhs));
        }
    }
    
    static up<ast_expression> parse_expression(token_stream& ts) {
        block_msg __bpe("parse expression");
        auto lhs = parse_primary(ts);
        if(!lhs) {
            return nullptr;
        }
        
        return parse_binary_operation_rhs(ts, 0, std::move(lhs));
    }
    
    // TODO: actually implement; this currently just skips the line
    static up<ast_expression> parse_line(token_stream& ts) {
        auto ret = parse_expression(ts);
        assert(ts.type() == tok_t::semicolon);
        ts.step(); // Eat semicolon
        return ret;
    }
    
    static up<ast_prototype> parse_prototype(token_stream& ts) {
        up<ast_prototype> ret;
        up<ast_expression> name;
        std::vector<ast_declaration> args;
        
        name = parse_identifier(ts, true);
        
        if(ts.type() != tok_t::paren_open) {
            fprintf(stderr, "Expected opening parentheses in function prototype, got %d\n", (int)ts.type());
            return nullptr;
        }
        
        ts.step(); // Eat opening paren
        
        while(ts.type() != tok_t::paren_close) {
            args.push_back(std::move(parse_declaration(ts)));
            if(ts.type() == tok_t::unknown) {
                ts.step(); // Eat comma
            }
        }
        
        ts.step(); // Eat closing paren
        
        if(ts.type() != tok_t::colon) {
            fprintf(stderr, "Expected colon before return type in function prototype, got %d\n", (int)ts.type());
            return nullptr;
        }
        
        ts.step(); // Eat colon
        
        if(ts.type() != tok_t::type) {
            fprintf(stderr, "Expected return type in function prototype, got %d\n", (int)ts.type());
            return nullptr;
        }
        
        ts.step(); // Eat type
        
        ret = std::make_unique<ast_prototype>();
        ret->name = std::move(name);
        ret->args = std::move(args);
        
        return ret;
    }
    
    static up<ast_expression> parse_function(token_stream& ts) {
        tok_t type;
        up<ast_function> ret;
        up<ast_prototype> proto;
        std::vector<up<ast_expression>> lines;
        assert(ts.type() == tok_t::fn);
        
        ts.step(); // Eat 'fn'
        
        proto = parse_prototype(ts);
        
        if(ts.type() != tok_t::curly_open) {
            fprintf(stderr, "Expected opening curly braces\n");
            return nullptr;
        }
        
        ts.step(); // Eat curly open
        
        while(ts.type() != tok_t::curly_close) {
            lines.push_back(std::move(parse_line(ts)));
        }
        
        if(ts.type() != tok_t::curly_close) {
            fprintf(stderr, "Expected closing curly braces\n");
            return nullptr;
        }
        
        ts.step();
        
        ret = std::make_unique<ast_function>();
        ret->prototype = std::move(proto);
        ret->lines = std::move(lines);
        
        return ret;
    }
    
    static up<ast_expression> parse_extern(token_stream& ts) {
        assert(ts.type() == tok_t::ext);
        
        printf("eat extern\n");
        ts.step(); // Eat extern
        
        printf("enter proto parse\n");
        auto ret = parse_prototype(ts);
        
        printf("exit proto parse\n");
        ts.step(); // eat semicolon
        
        return ret;
    }
    
    up<ast_expression> parse(token_stream& ts) {
        up<ast_expression> ret;
        
        switch(ts.type()) {
            case tok_t::fn:
            ret = parse_function(ts);
            break;
            case tok_t::ext:
            printf("parsing extern\n");
            ret = parse_extern(ts);
            break;
            default:
            fprintf(stderr, "Unknown top level expression\n");
            assert(0);
            break;
        }
        
        return ret;
    }
}