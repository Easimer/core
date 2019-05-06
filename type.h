#pragma once

#include "ast.h"

namespace core {
    struct type {
        llvm::Type* llvm_type = nullptr;
        
        virtual llvm::Type* get_llvm_type(llvm_ctx& ctx) = 0;
    };
    
    template<typename T>
        struct ranged_type : public type {
        bool ranged;
        T from, to;
        
        virtual llvm::Type* get_llvm_type(llvm_ctx& ctx) override;
    };
    
    using type_real = ranged_type<double>;
    using type_int = ranged_type<int>;
    
    struct array_type : public type {
        array_type(type* contained, int max_count)
            : contained(contained), max_count(max_count) {}
        type* contained;
        int max_count;
        
        virtual llvm::Type* get_llvm_type(llvm_ctx& ctx) override;
    };
    
    struct aggregate_type : public type {
        std::string name;
        std::vector<type*> members;
        
        virtual llvm::Type* get_llvm_type(llvm_ctx& ctx) override;
    };
}