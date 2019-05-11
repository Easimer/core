#include "stdafx.h"
#include "lexer.h"
#include "ast.h"
#include "log.h"
#include "type.h"

namespace core {
    template<>
        llvm::Type* ranged_type<double>::get_llvm_type(llvm_ctx& ctx) {
        if(!llvm_type) {
            llvm_type = llvm::Type::getDoubleTy(ctx.ctx);
        }
        return llvm_type;
    }
    
    template<>
        llvm::Type* ranged_type<int>::get_llvm_type(llvm_ctx& ctx) {
        if(!llvm_type) {
            llvm_type = llvm::Type::getInt64Ty(ctx.ctx);
        }
        return llvm_type;
    }
    
    template<>
        llvm::Type* ranged_type<bool>::get_llvm_type(llvm_ctx& ctx) {
        if(!llvm_type) {
            llvm_type = llvm::Type::getInt1Ty(ctx.ctx);
        }
        return llvm_type;
    }
    
    template<> std::string ranged_type<double>::get_type_name() {return "real";}
    template<> std::string ranged_type<int>::get_type_name() {return "int";}
    template<> std::string ranged_type<bool>::get_type_name() {return "bool";}
    
    llvm::Type* array_type::get_llvm_type(llvm_ctx& ctx) {
        if(!llvm_type) {
            if(!contained) {
                fprintf(stderr, "Array has no contained type\n");
                return nullptr;
            }
            if(!contained->llvm_type) {
                contained->get_llvm_type(ctx);
            }
            assert(contained->llvm_type);
            llvm_type = llvm::ArrayType::get(contained->llvm_type, max_count);
        }
        return llvm_type;
    }
    std::string array_type::get_type_name() {
        return contained->get_type_name() + "[" + std::to_string(max_count) + "]";
    }
    
    std::string aggregate_type::get_type_name() {
        std::string ret = name + "<";
        
        for(auto& member : members) {
            ret += member->get_type_name() + ",";
        }
        
        ret += ">";
        
        return ret;
    }
    
    llvm::Type* aggregate_type::get_llvm_type(llvm_ctx& ctx) {
        if(!llvm_type) {
            llvm::SmallVector<llvm::Type*, 16> types;
            for(auto& member : members) {
                types.push_back(member->get_llvm_type(ctx));
            }
            auto pTyStruct = llvm::StructType::get(ctx.ctx, types);
            pTyStruct->setName(name);
            llvm_type = pTyStruct;
        }
        return llvm_type;
    }
    
    type_manager::type_manager() {
        auto ty_real = std::make_shared<type_real>();
        auto ty_int = std::make_shared<type_int>();
        auto ty_bool = std::make_shared<type_bool>();
        
        m_types.push_back(ty_real);
        m_types.push_back(ty_int);
        m_types.push_back(ty_bool);
        
        m_type_map["real"] = ty_real;
        m_type_map["int"] = ty_int;
        m_type_map["bool"] = ty_bool;
    }
    
    // Parses a type atom
    // E.g. that isn't a non-typedef'd aggregate type
    sp<type> parse_atom_type(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr) {
        sp<type> ret = nullptr;
        auto pszString = ts.current().c_str();
        int len = strlen(pszString);
        // Collect chars until EOStr or '['
        // If we've encountered an '[' then parse a number
        // until the next ']'
        // If the char after the '[' is a ']' then it's an array ref
        // If the substring is not an integer then throw an error
        // If we encounter EOStr without an ']' then throw an error
        // Check if the base type is defined in the type manager
        // If the type isn't an array then return the base type
        // If the type hasn't been defined then throw an error
        // If the type is an array then lookup if such an array type has been created yet and return it
        // Create the array type in llvm if it hasn't been yet
        // Return the type
        
        bool array_type = false;
        int array_len = -1;
        std::string buf_base_type;
        buf_base_type.reserve(len);
        
        int i = 0;
        
        while(pszString[i] != '[' && i < len) {
            buf_base_type += pszString[i];
            i++;
        }
        
        if(buf_base_type.size() == 0) {
            log_err(ts, "Type atom cannot have a base type identifier of zero length\n");
            return ret;
        }
        
        if(!type_mgr.is_type_defined(buf_base_type)) {
            log_err(ts, "Unknown type '%s'!\n", buf_base_type.c_str());
            return ret;
        }
        
        ret = type_mgr.m_type_map[buf_base_type];
        
        if(pszString[i] == '[') {
            while(pszString[i] != ']') {
                if(pszString[i] >= '0' && pszString[i] <= '9') {
                    if(array_len == -1) {
                        array_len = 0;
                    }
                    array_len *= 10;
                    array_len += (int)(pszString[i] - '0');
                }
                i++;
            }
            if(pszString[i] == ']') {
                if(i != len - 1) {
                    log_err(ts, "Array type atom must not end with anything other than a ']'\n");
                    ret = nullptr;
                    return ret;
                }
                
                ret = std::make_shared<core::array_type>(ret, array_len);
            }
        }
        
        assert(ret);
        
        return ret;
    }
    
    // Parses a complex type
    sp<type> parse_type(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr, const std::string& name) {
        sp<aggregate_type> ret = std::make_shared<core::aggregate_type>();
        ret->name = name;
        bool next_is_atom = true;
        while(ts.type() != tok_t::semicolon) {
            if(next_is_atom) {
                if(ts.type() != tok_t::identifier) {
                    log_err(ts, "Expected type in typedef\n");
                    return ret;
                }
                sp<type> atom;
                // Lookup type in the manager
                if(type_mgr.is_type_defined(ts.current())) {
                    atom = type_mgr.m_type_map[ts.current()];
                } else {
                    atom = parse_atom_type(ts, ctx, type_mgr);
                }
                if(atom) {
                    ret->members.push_back(atom);
                    next_is_atom = false;
                } else {
                    log_err(ts, "Unknown type '%s' in typedef\n", ts.current().c_str());
                    ret = nullptr;
                    return ret;
                }
                ts.step();
            } else {
                if(ts.type() != tok_t::oper) {
                    log_err(ts, "Expected operator '*' after type in typedef, got %s (%d)\n", ts.current().c_str(), (int)ts.type());
                    ret = nullptr;
                    return ret;
                }
                next_is_atom = true;
                ts.step();
            }
        }
        if(!ret) {
            log_err(ts, "Empty type in typedef!\n");
            ret = nullptr;
            return ret;
        }
        
        return ret;
    }
}