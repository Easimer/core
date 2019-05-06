#include "stdafx.h"
#include "ast.h"
#include "type.h"

template<>
llvm::Type* core::ranged_type<double>::get_llvm_type(llvm_ctx& ctx) {
    if(!llvm_type) {
        llvm_type = llvm::Type::getDoubleTy(ctx.ctx);
    }
    return llvm_type;
}

template<>
llvm::Type* core::ranged_type<int>::get_llvm_type(llvm_ctx& ctx) {
    if(!llvm_type) {
        llvm_type = llvm::Type::getInt64Ty(ctx.ctx);
    }
    return llvm_type;
}

llvm::Type* core::array_type::get_llvm_type(llvm_ctx& ctx) {
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

llvm::Type* core::aggregate_type::get_llvm_type(llvm_ctx& ctx) {
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