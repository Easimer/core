#include "stdafx.h"
#include <cassert>
#include "log.h"
#include "parser.h"

namespace core {
    static up<ast_expression> parse_expression(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr);
    static up<ast_expression> parse_primary(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr);
    
    static int operator_precedence(char op) {
        switch(op) {
            case '?': return 3;
            case '!': return 3;
            case '<': return 3;
            case '>': return 3;
            case '+': return 5;
            case '-': return 5;
            case '*': return 10;
            case '/': return 10;
            case '=': return 0;
        }
        return -1;
    }
    
    static up<ast_expression> parse_typedef(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        up<ast_expression> ret = nullptr;
        assert(ts.type() == tok_t::type);
        if(ts.type() != tok_t::type) {
            log_err(ts, "Expected keyword 'type' in typedef\n");
            return ret;
        }
        
        ts.step();
        
        if(ts.type() != tok_t::identifier) {
            log_err(ts, "Expected identifier in typedef\n");
            return ret;
        }
        
        auto ID_str = ts.current();
        //auto ID = std::make_unique<ast_identifier>(ID_str.c_str());
        ts.step();
        
        if(ts.type() != tok_t::colon) {
            log_err(ts, "Expected colon in typedef between identifier and type, got %d\n", (int)ts.type());
            return ret;
        }
        ts.step();
        
        auto type = parse_type(ts, ctx, type_mgr, ID_str);
        
        if(ts.type() != tok_t::semicolon) {
            log_err(ts, "Expected semicolon at end of the typedef!\n");
            return ret;
        }
        ts.step();
        
        type_mgr.add_type(ID_str, type);
        
        ret = std::make_unique<ast_empty>();
        
        return ret;
    }
    
    static ast_declaration parse_declaration(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        std::string name;
        sp<core::type> type;
        ast_declaration ret;
        
        assert(ts.type() == tok_t::identifier);
        
        if(ts.type() != tok_t::identifier) {
            log_err(ts, "Expected identifier in declaration\n");
            return ret;
        }
        
        name = ts.current();
        
        ret.line = ts.line();
        ret.col = ts.col();
        
        ts.step();
        
        if(ts.type() != tok_t::colon) {
            log_err(ts, "Expected colon in declaration\n");
            return ret;
        }
        
        ts.step();
        
        if(ts.type() != tok_t::identifier) {
            log_err(ts, "Expected type in declaration\n");
            return ret;
        }
        
        type = parse_atom_type(ts, ctx, type_mgr);
        
        ts.step();
        
        ret.identifier = std::make_unique<ast_identifier>(name.c_str());
        ret.type = type;
        
        return ret;
    }
    
    static up<ast_expression> parse_paren_expr(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        assert(ts.type() == tok_t::paren_open);
        ts.step();
        auto E = parse_expression(ts, ctx, type_mgr);
        if(!E) {
            return nullptr;
        }
        
        if(ts.type() != tok_t::paren_close) {
            log_err(ts, "Expected closing parentheses\n");
            return nullptr;
        }
        assert(ts.type() == tok_t::paren_close);
        ts.step();
        return E;
    }
    
    static up<ast_expression> parse_literal(token_stream& ts, llvm_ctx& ctx) {
        block_msg __bpl("parse literal");
        const auto& s = ts.current();
        int line = ts.line(), col = ts.col();
        // Is real number
        bool is_number = true;
        bool is_real = false;
        bool is_int = false;
        bool is_bool = false;
        for(int i = 0; i < s.size() && is_number; i++) {
            if((s[i] < '0' || s[i] > '9') && (s[i] != '.')) {
                is_number = false;
            }
            if(is_number && s[i] == '.') {
                is_real = true;
            }
        }
        if(is_number && !is_real) {
            is_int = true;
        }
        if(s == "false" || s == "true") {
            is_bool = true;
            is_real = is_int = false;
        }
        if(is_real || is_int || is_bool) {
            auto ret = std::make_unique<ast_literal>(s);
            ret->is_real = is_real;
            ret->is_int = is_int;
            ret->is_bool = is_bool;
            ret->line = line; ret->col = col;
            ts.step();
            return ret;
        }
        
        log_err(ts, "Unknown type of literal encountered: '%s'\n", s.c_str());
        ts.step();
        return nullptr;
    }
    
    static up<ast_expression> parse_identifier(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr, bool not_a_call = false) {
        assert(ts.type() == tok_t::identifier);
        
        int line = ts.line(), col = ts.col();
        
        block_msg __bpi("parse identifier");
        
        if(ts.type() != tok_t::identifier) {
            log_err(ts, "Expected identifier in declaration\n");
            return nullptr;
        }
        
        auto name = ts.current();
        auto id = std::make_unique<ast_identifier>(name.c_str());
        id->line = line; id->col = col;
        
        ts.step(); // Eat identifier
        if(ts.type() == tok_t::colon) { // declaration with type
            block_msg __bdwt("declaration with type");
            auto ret = std::make_unique<ast_declaration>();
            ret->identifier = std::move(id);
            ret->line = line; ret->col = col;
            ts.step();
            ret->type = parse_atom_type(ts, ctx, type_mgr);
            ts.step();
            return ret;
        } else if(ts.type() == tok_t::paren_open && !not_a_call) {
            block_msg __bfc("fn call");
            auto ret = std::make_unique<ast_function_call>();
            ret->name = std::move(id);
            ret->line = line; ret->col = col;
            ts.step(); // Eat paren open
            while(ts.type() != tok_t::paren_close) {
                auto arg = parse_expression(ts, ctx, type_mgr);
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
    
    static up<ast_expression> parse_primary(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        block_msg __bpp("parse primary");
        switch(ts.type()) {
            case tok_t::identifier:
            return parse_identifier(ts, ctx, type_mgr);
            case tok_t::literal:
            return parse_literal(ts, ctx);
            case tok_t::paren_open:
            return parse_paren_expr(ts, ctx, type_mgr);
            default:
            log_err(ts, "Unknown token '%s' of type %d\n",ts.current().c_str(), (int)ts.type());
            break;
        }
        return nullptr;
    }
    
    static up<ast_expression> parse_binary_operation_rhs(token_stream& ts, llvm_ctx& ctx, int expr_prec, up<ast_expression> lhs, type_manager& type_mgr) {
        block_msg __bpbor("parse binary operation rhs");
        while(1) {
            int line = ts.line(), col = ts.col();
            char bin_op = ts.current()[0];
            int tok_prec = operator_precedence(bin_op);
            if(tok_prec < expr_prec) {
                return lhs;
            }
            
            ts.step();
            
            auto rhs = parse_primary(ts, ctx, type_mgr);
            if(!rhs) {
                log_err(ts, "Expected right hand side of operation\n");
                return nullptr;
            }
            
            int next_prec = operator_precedence(ts.current()[0]);
            if(tok_prec < next_prec) {
                block_msg __bpborr("parse binary operation rhs recurse");
                rhs = parse_binary_operation_rhs(ts, ctx, tok_prec + 1, std::move(rhs), type_mgr);
                if(!rhs) {
                    return nullptr;
                }
            }
            
            lhs = std::make_unique<ast_binary_op>(bin_op, std::move(lhs), std::move(rhs));
            
            lhs->line = line; lhs->col = col;
        }
    }
    
    static up<ast_expression> parse_expression(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        block_msg __bpe("parse expression");
        if(ts.type() == tok_t::cif) { // branching
            block_msg __bpeif("parsing branching");
            ts.step();
            auto cond = parse_paren_expr(ts, ctx, type_mgr);
            if(ts.type() != tok_t::cthen) {
                log_err(ts, "Expected 'then' after condition in branching, got %d\n", (int)ts.type());
                return nullptr;
            }
            ts.step();
            auto line = parse_expression(ts, ctx, type_mgr);
            
            auto ret = std::make_unique<ast_branching>();
            ret->condition = std::move(cond);
            ret->line = std::move(line);
            
            return ret;
        } else { // line
            auto lhs = parse_primary(ts, ctx, type_mgr);
            if(!lhs) {
                return nullptr;
            }
            
            return parse_binary_operation_rhs(ts, ctx, 0, std::move(lhs), type_mgr);
        }
    }
    
    static up<ast_expression> parse_line(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        auto ret = parse_expression(ts, ctx, type_mgr);
        assert(ts.type() == tok_t::semicolon);
        ts.step(); // Eat semicolon
        return ret;
    }
    
    static up<ast_prototype> parse_prototype(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        up<ast_prototype> ret;
        up<ast_expression> name;
        std::vector<ast_declaration> args;
        
        bool is_pure = false;
        int line = ts.line(), col = ts.col();
        
        if(ts.type() == tok_t::pure) {
            is_pure = true;
            ts.step(); // Eat pure
        }
        
        if(ts.type() != tok_t::identifier) {
            log_err(ts, "Expected identifier in function prototype, got %d\n", (int)ts.type());
            return nullptr;
        }
        
        name = parse_identifier(ts, ctx, type_mgr, true);
        
        if(ts.type() != tok_t::paren_open) {
            log_err(ts, "Expected opening parentheses in function prototype, got %d\n", (int)ts.type());
            return nullptr;
        }
        
        ts.step(); // Eat opening paren
        
        while(ts.type() != tok_t::paren_close) {
            args.push_back(std::move(parse_declaration(ts, ctx, type_mgr)));
            if(ts.type() == tok_t::unknown) {
                ts.step(); // Eat comma
            }
        }
        
        ts.step(); // Eat closing paren
        
        if(ts.type() != tok_t::colon) {
            log_err(ts, "Expected colon before return type in function prototype, got %d\n", (int)ts.type());
            return nullptr;
        }
        
        ts.step(); // Eat colon
        
        if(ts.type() != tok_t::identifier) {
            log_err(ts, "Expected return type in function prototype, got %d\n", (int)ts.type());
            return nullptr;
        }
        
        auto& type_str = ts.current();
        auto type = std::make_unique<ast_identifier>(type_str.c_str());
        
        ts.step(); // Eat type
        
        ret = std::make_unique<ast_prototype>();
        ret->name = std::move(name);
        ret->args = std::move(args);
        ret->type = std::move(type);
        ret->is_pure = is_pure;
        ret->line = line; ret->col = col;
        
        return ret;
    }
    
    static up<ast_expression> parse_function(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        tok_t type;
        up<ast_function> ret;
        up<ast_prototype> proto;
        std::vector<up<ast_expression>> lines;
        assert(ts.type() == tok_t::fn);
        
        int line = ts.line(), col = ts.col();
        
        ts.step(); // Eat 'fn'
        
        proto = parse_prototype(ts, ctx, type_mgr);
        
        if(ts.type() != tok_t::curly_open) {
            log_err(ts, "Expected opening curly braces\n");
            return nullptr;
        }
        
        ts.step(); // Eat curly open
        
        while(ts.type() != tok_t::curly_close) {
            lines.push_back(std::move(parse_line(ts, ctx, type_mgr)));
        }
        
        if(ts.type() != tok_t::curly_close) {
            log_err(ts, "Expected closing curly braces\n");
            return nullptr;
        }
        
        ts.step();
        
        ret = std::make_unique<ast_function>();
        ret->prototype = std::move(proto);
        ret->lines = std::move(lines);
        ret->line = line; ret->col = col;
        
        return ret;
    }
    
    static up<ast_expression> parse_extern(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        assert(ts.type() == tok_t::ext);
        
        ts.step(); // Eat extern
        
        auto ret = parse_prototype(ts, ctx, type_mgr);
        
        ts.step(); // eat semicolon
        
        return ret;
    }
    
    up<ast_expression> parse(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        up<ast_expression> ret;
        
        switch(ts.type()) {
            case tok_t::fn:
            ret = parse_function(ts, ctx, type_mgr);
            break;
            case tok_t::ext:
            ret = parse_extern(ts, ctx, type_mgr);
            break;
            case tok_t::type:
            ret = parse_typedef(ts, ctx, type_mgr);
            break;
            default:
            log_err(ts, "Unknown top level expression\n");
            assert(0);
            break;
        }
        
        return ret;
    }
}