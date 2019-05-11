#pragma once

#include "stdafx.h"
#include "ast.h"
#include "types.h"
#include "lexer.h"
#include <vector>
#include <memory>

namespace core {
    struct type {
        llvm::Type* llvm_type = nullptr;
        
        virtual llvm::Type* get_llvm_type(llvm_ctx& ctx) = 0;
        virtual std::string get_type_name() = 0;
        virtual int count() { return 1; }
    };
    
    template<typename T>
        struct ranged_type : public type {
        bool ranged;
        T from, to;
        
        virtual llvm::Type* get_llvm_type(llvm_ctx& ctx) override;
        virtual std::string get_type_name() override;
    };
    
    using type_real = ranged_type<double>;
    using type_int = ranged_type<int>;
    using type_bool = ranged_type<bool>;
    
    struct array_type : public type {
        array_type(sp<type>& contained, int max_count)
            : contained(contained), max_count(max_count) {}
        sp<type> contained;
        int max_count;
        
        virtual llvm::Type* get_llvm_type(llvm_ctx& ctx) override;
        virtual std::string get_type_name() override;
        virtual int count() override { return max_count; }
    };
    
    struct aggregate_type : public type {
        std::string name;
        std::vector<sp<type>> members;
        
        virtual llvm::Type* get_llvm_type(llvm_ctx& ctx) override;
        virtual std::string get_type_name() override;
    };
    
    struct type_manager {
        std::unordered_map<std::string, sp<type>> m_type_map;
        std::vector<sp<type>> m_types;
        
        type_manager();
        
        bool is_type_defined(const std::string& s) {
            return m_type_map.count(s);
        }
        
        void add_type(const std::string& name, sp<type> t) {
            m_type_map[name] = t;
            m_types.push_back(t);
        }
    };
    
    sp<type> parse_type(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr, const std::string& name);
    sp<type> parse_atom_type(token_stream& ts, llvm_ctx& ctx, type_manager& type_mgr);
}