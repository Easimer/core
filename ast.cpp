#include "stdafx.h"
#include <cstdio>
#include <cstring>

#include "ast.h"
#include "log.h"

using namespace llvm;

namespace core {
    void ast_literal::dump() {
        printf("literal(%s; %d %d %d)", value.c_str(), is_real, is_int, is_bool);
    }
    
    ast_identifier::ast_identifier(const char* pszName) {
        strncpy(name, pszName, 128);
    }
    
    void ast_identifier::dump() {
        printf("identifier(%s)", name);
    }
    
    void ast_declaration::dump() {
        printf("declaration(");
        identifier->dump();
        printf(" : %s)", type->get_type_name().c_str());
    }
    
    void ast_prototype::dump() {
        printf("prototype(");
        name->dump();
        printf("; ");
        type->dump();
        printf("; ");
        for(auto& arg : args) {
            arg.dump();
            printf(", ");
        }
        printf(")");
    }
    
    void ast_function::dump() {
        printf("function(");
        prototype->dump(); printf(" {\n");
        for(auto& line : lines) {
            line->dump(); printf(";\n");
        }
        printf("})");
    }
    
    void ast_binary_op::dump() {
        printf("bin_op(%c;", op);
        if(lhs) lhs->dump();
        printf(";");
        if(rhs) rhs->dump();
        printf(")");
    }
    
    void ast_function_call::dump() {
        printf("fncall(");
        name->dump(); printf("; ");
        for(auto& arg : args) {
            arg->dump(); printf(", ");
        }
        printf(")");
    }
    
    void ast_branching::dump() {
        printf("branch(");
        condition->dump(); printf(" -> "); line->dump(); printf(")");
    }
    
    void ast_type::dump() {
        printf("type(%s)", pType->get_type_name().c_str());
    }
    
    // Codegen
    
    static llvm::AllocaInst* create_entry_block_alloca(llvm_ctx& ctx, llvm::Function* pFunc, const llvm::StringRef name, llvm::Type* pType) {
        IRBuilder<> TmpB(&pFunc->getEntryBlock(), pFunc->getEntryBlock().begin());
        return TmpB.CreateAlloca(pType, 0, name);
    }
    
    static llvm::AllocaInst* create_entry_block_alloca(llvm_ctx& ctx, llvm::Function* pFunc, const llvm::StringRef name, llvm::Type* pType, llvm::Value* len) {
        IRBuilder<> TmpB(&pFunc->getEntryBlock(), pFunc->getEntryBlock().begin());
        return TmpB.CreateAlloca(pType, len, name);
    }
    
    static llvm::Type* str_to_type(llvm_ctx& ctx, const std::string& s) {
        llvm::Type* ret = nullptr;
        if(s == "real") {
            return Type::getDoubleTy(ctx.ctx);
        } else if(s == "bool") {
            return Type::getInt1Ty(ctx.ctx);
        }
        return ret;
    }
    
    // Converts an llvm::Type to a string
    static std::string type_to_str(const llvm::Type* pType) {
        std::string ret;
        llvm::raw_string_ostream sret(ret);
        pType->print(sret);
        return ret;
    }
    
    llvm::Value* ast_literal::generate_ir(llvm_ctx& ctx) {
        if(is_real) {
            return ConstantFP::get(ctx.ctx, APFloat(std::stod(value.c_str())));
        } else if(is_int) {
            return ConstantInt::get(ctx.ctx, APInt(64, std::stoi(value.c_str())));
        } else if(is_bool) {
            return ConstantInt::get(ctx.ctx, APInt(1, value == "true"));
        }
        log_err(this, "Literal has unknown type\n");
        return nullptr;
    }
    
    llvm::Value* ast_identifier::generate_ir(llvm_ctx& ctx) {
        auto sname = std::string(name);
        if(ctx.locals.count(sname)) {
            auto ret = ctx.locals[sname];
            return ctx.builder.CreateLoad(ret, name);
        }
        if(ctx.globals.count(sname)) {
            auto ret = ctx.globals[sname];
            return ctx.builder.CreateLoad(ret, name);
        }
        log_err(this, "Referencing unknown variable '%s'\n", name);
        return nullptr;
    }
    
    llvm::Value* ast_binary_op::generate_ir(llvm_ctx& ctx) {
        llvm::Value* ret = nullptr;
        auto R = rhs->generate_ir(ctx);
        if(!R) {
            log_err(rhs.get(), "Failure in right-hand side expression\n");
            return ret;
        }
        
        if(op == '=') {
            // lhs must be an lvalue
            auto pLHS = dynamic_cast<ast_identifier*>(lhs.get());
            if(!pLHS) {
                // check if lhs is a declaration
                auto pLHSDecl = dynamic_cast<ast_declaration*>(lhs.get());
                if(pLHSDecl) {
                    lhs->generate_ir(ctx); // Generate declaration code
                    pLHS = pLHSDecl->identifier.get();
                } else {
                    log_err(lhs.get(), "Left-hand side of assignment must be an lvalue!\n");
                    return ret;
                }
            }
            auto pVar = ctx.locals[pLHS->name];
            if(!pVar) {
                log_err(lhs.get(), "Unknown variable referenced\n");
                return ret;
            }
            
            auto pTyVar = pVar->getType()->getElementType();
            auto pTyRValue = R->getType();
            
            if(pTyVar != pTyRValue) {
                if(pTyVar->isFloatingPointTy() && pTyRValue->isIntegerTy()) {
                    // Convert rvalue to double
                    log_warn(rhs.get(), "Implicitly converting integer to real!\n");
                    R = ctx.builder.CreateSIToFP(R, pTyVar);
                } else {
                    auto stvar = type_to_str(pTyVar);
                    auto stval = type_to_str(pTyRValue);
                    log_err(this, "Assigning to '%s' from incompatible type '%s\n", stvar.c_str(), stval.c_str());
                    return ret;
                }
            }
            
            ctx.builder.CreateStore(R, pVar);
            ret = R;
            return ret;
        } else {
            auto L = lhs->generate_ir(ctx);
            if(!L) {
                log_err(lhs.get(), "Failure in left-hand side expression\n");
                return ret;
            }
            
            auto pTyL = L->getType();
            auto pTyR = R->getType();
            
            if(pTyL != pTyR) {
                if(pTyL->isFloatingPointTy() && pTyR->isIntegerTy()) {
                    // Convert R to double
                    log_warn(rhs.get(), "Implicitly converting integer to real!\n");
                    R = ctx.builder.CreateSIToFP(R, pTyL);
                } else if(pTyR->isFloatingPointTy() && pTyL->isIntegerTy()) {
                    // Convert L to double
                    log_warn(lhs.get(), "Implicitly converting integer to real!\n");
                    L = ctx.builder.CreateSIToFP(L, pTyR);
                } else {
                    auto sl = type_to_str(L->getType());
                    auto sr = type_to_str(R->getType());
                    log_err(this, "Type mismatch in binary operation; lhs : %s, rhs : %s\n", sl.c_str(), sr.c_str());
                    return ret;
                }
            }
            
            switch(op) {
                case '+':
                ret = ctx.builder.CreateFAdd(L, R, "addtmp");
                break;
                case '-':
                ret = ctx.builder.CreateFSub(L, R, "subtmp");
                break;
                case '*':
                ret = ctx.builder.CreateFMul(L, R, "multmp");
                break;
                case '/':
                ret = ctx.builder.CreateFDiv(L, R, "divtmp");
                break;
                case '?':
                ret = ctx.builder.CreateFCmpUEQ(L, R, "cmptmp");
                break;
                case '!':
                ret = ctx.builder.CreateFCmpUNE(L, R, "cmptmp");
                break;
                case '<':
                ret = ctx.builder.CreateFCmpULT(L, R, "cmptmp");
                break;
                case '>':
                ret = ctx.builder.CreateFCmpUGT(L, R, "cmptmp");
                break;
                default:
                log_err(this, "Unknown operator %c\n", op);
                break;
            }
        }
        
        ctx.builder.SetCurrentDebugLocation(llvm::DebugLoc::get(line, col, ctx.di_scope));
        
        return ret;
    }
    
    llvm::Value* ast_declaration::generate_ir(llvm_ctx& ctx) {
        llvm::AllocaInst* ret = nullptr;
        auto pFunc = ctx.builder.GetInsertBlock()->getParent();
        if(type->count() > 1) {
            auto len = ConstantInt::get(ctx.ctx, APInt(64, type->count()));
            ret = create_entry_block_alloca(ctx, pFunc, identifier->name, type->get_llvm_type(ctx), len);
        } else {
            ret = create_entry_block_alloca(ctx, pFunc, identifier->name, type->get_llvm_type(ctx));
        }
        ctx.locals.emplace(identifier->name, ret);
        return ret;
    }
    
    llvm::Value* ast_function_call::generate_ir(llvm_ctx& ctx) {
        llvm::Value* ret = nullptr;
        
        if(strcmp(name->name, "return") == 0) { // return is technically a function call
            // TODO: typecheck here
            auto n_args = args.size();
            if(n_args == 1) {
                auto V = args[0]->generate_ir(ctx);
                ret = ctx.builder.CreateRet(V);
            } else if (n_args == 0) {
                ret = ctx.builder.CreateRetVoid();
            } else {
                log_err(this, "Aggregate return is not allowed!\n");
            }
            return ret;
        } else if(strcmp(name->name, "idx") == 0) {
            auto n_args = args.size();
            if(n_args == 2) {
                auto array = args[0]->generate_ir(ctx);
                auto index = args[1]->generate_ir(ctx);
                if(!array->getType()->isArrayTy()) {
                    log_err(args[0].get(), "Not an array!\n");
                } else if(!index->getType()->isIntegerTy()) {
                    log_err(args[1].get(), "Not an integer!\n");
                } else {
                    auto pTyArray = (llvm::ArrayType*)array->getType();
                    // If the index can be casted to an ast_literal then do a bounds check
                    auto pIdxLiteral = dynamic_cast<ast_literal*>(args[1].get());
                    if(pIdxLiteral) {
                        if(!pIdxLiteral->is_int) {
                            log_err(pIdxLiteral, "Index must be an integer!\n");
                            return ret;
                        }
                        int i = std::stoi(pIdxLiteral->value);
                        uint64_t n = pTyArray->getNumElements();
                        if(i < 0 || (i > n && i != (uint64_t)-1)) {
                            log_warn(pIdxLiteral, "Indexing out of bounds; array length is %u, index is %u\n", n, i);
                        }
                    }
                    
                    auto pTyElem = pTyArray->getElementType();
                    auto array_ptr = ctx.builder.CreatePointerCast(array, array->getType()->getPointerTo());
                    ret = ctx.builder.CreateGEP(array_ptr, index);
                    ret = ctx.builder.CreateLoad(pTyElem, ret);
                }
            } else if(n_args == 3) {
                auto array = args[0]->generate_ir(ctx);
                auto index = args[1]->generate_ir(ctx);
                auto value = args[2]->generate_ir(ctx);
                if(!array->getType()->isArrayTy()) {
                    log_err(args[0].get(), "Not an array!\n");
                    return ret;
                }
                if(!index->getType()->isIntegerTy()) {
                    log_err(args[1].get(), "Not an integer!\n");
                    return ret;
                }
                auto pTyArray = (llvm::ArrayType*)array->getType();
                // If the index can be casted to an ast_literal then do a bounds check
                auto pIdxLiteral = dynamic_cast<ast_literal*>(args[1].get());
                if(pIdxLiteral) {
                    if(!pIdxLiteral->is_int) {
                        log_err(pIdxLiteral, "Index must be an integer!\n");
                        return ret;
                    }
                    uint64_t i = std::stoi(pIdxLiteral->value);
                    uint64_t n = pTyArray->getNumElements();
                    if(i < 0 || (i > n && i != (uint64_t)-1)) {
                        // NOTE: we could issue a warning only then let it crash tbh
                        log_warn(pIdxLiteral, "Indexing out of bounds; array length is %u, index is %u\n", n, i);
                    }
                }
                auto pTyElem = ((llvm::ArrayType*)array->getType())->getElementType();
                auto pTyVal = value->getType();
                if(pTyElem != pTyVal) {
                    log_err(args[2].get(), "Value needs to have the same type that's contained in the array!\n\tPassed: %s Contained: %s\n", type_to_str(pTyVal).c_str(), type_to_str(pTyElem).c_str());
                    return ret;
                }
                auto array_ptr = ctx.builder.CreatePointerCast(array, array->getType()->getPointerTo());
                ret = ctx.builder.CreateGEP(array_ptr, index);
                ret = ctx.builder.CreateStore(value, ret);
            } else {
                log_err(this, "Indexing operation requires two arguments: the array indexed and the index\n");
            }
            return ret;
        } else {
            Function* pFunc = ctx.module.getFunction(name->name);
            if(!pFunc) {
                log_err(this, "Referencing an unknown function '%s'!\n", name->name);
                return ret;
            }
            
            if(pFunc->arg_size() != args.size()) {
                log_err(this, "Function '%s' expects %d argument(s), but %d was passed!\n", name->name, (int)pFunc->arg_size(), (int)args.size());
                return ret;
            }
            
            if(ctx.current_function_pure && !ctx.func_is_pure[pFunc]) {
                log_err(this, "Cannot call impure function %s from a pure function!\n", name->name);
                return ret;
            }
            
            std::vector<llvm::Value*> vargs;
            int iArg = 0;
            for(auto& arg : pFunc->args()) {
                auto pVArg = args[iArg]->generate_ir(ctx);
                auto pTyArg = arg.getType();
                auto pTyVArg = pVArg->getType();
                
                if(pTyVArg != pTyArg) {
                    if(pTyArg->isFloatingPointTy() && pTyVArg->isIntegerTy()) {
                        // Convert R to double
                        log_warn(args[iArg].get(), "Implicitly converting integer to real!\n");
                        pVArg = ctx.builder.CreateSIToFP(pVArg, pTyArg);
                    } else if(pTyArg->isArrayTy() && pTyVArg->isArrayTy()) {
                        auto pTyArgArr = static_cast<llvm::ArrayType*>(pTyArg);
                        auto pTyVArgArr = static_cast<llvm::ArrayType*>(pTyVArg);
                        if(pTyArgArr->getElementType() != pTyVArgArr->getElementType()) {
                            auto sf = type_to_str(pTyArgArr->getElementType());
                            auto sp = type_to_str(pTyVArgArr->getElementType());
                            log_err(this, "Type mismatch in function call: argument %i of %s expect an array of type %s, but was passed a(n) array of %s\n", iArg, name->name, sf.c_str(), sp.c_str());
                            return ret;
                        }
                        if(pTyArgArr->getNumElements() != -1) {
                            if(pTyArgArr->getNumElements() > pTyVArgArr->getNumElements()) {
                                log_err(this, "Argument %i of %s expects an array of minimum length %d, but was passed one with length %d\n", iArg, name->name, pTyArgArr->getNumElements(), pTyVArgArr->getNumElements());
                                return ret;
                            }
                        }
                    } else {
                        auto sf = type_to_str(pTyArg);
                        auto sp = type_to_str(pTyVArg);
                        log_err(this, "Type mismatch in function call: argument %i of %s expects type %s, but was passed a(n) %s\n", iArg, name->name, sf.c_str(), sp.c_str());
                        return ret;
                    }
                }
                
                vargs.push_back(pVArg);
                if(!pVArg) {
                    // argument codegen failure
                    return ret;
                }
                iArg++;
            }
            
            ret = ctx.builder.CreateCall(pFunc, vargs, "calltmp");
            
            return ret;
        }
    }
    
    llvm::Value* ast_prototype::generate_ir(llvm_ctx& ctx) {
        FunctionType* pFuncTy;
        Function* pFunc;
        std::vector<llvm::Type*> type_signature;
        llvm::SmallVector<Metadata*, 8> di_type_signature;
        
        di_type_signature.push_back(ctx.di_types[type->name]);
        
        for(auto& arg : args) {
            auto pType = arg.type;
            if(!pType) {
                log_err(this, "Unknown type '%s' in function type signature\n", arg.type->get_type_name().c_str());
                return nullptr;
            }
            type_signature.push_back(pType->get_llvm_type(ctx));
            if(ctx.di_types.count(arg.type->get_type_name())) {
                di_type_signature.push_back(ctx.di_types[arg.type->get_type_name()]);
            } else {
                di_type_signature.push_back(ctx.di_types["_unknown"]);
            }
        }
        
        if(strcmp(type->name, "bool") == 0) {
            pFuncTy = FunctionType::get(Type::getInt1Ty(ctx.ctx), type_signature, false);
        } else if(strcmp(type->name, "real") == 0) {
            pFuncTy = FunctionType::get(Type::getDoubleTy(ctx.ctx), type_signature, false);
        } else {
            log_err(this, "Unknown type '%s' in function return type\n", type->name);
            return nullptr;
        }
        
        if(!name) {
            log_err(this, "Function has no name!\n");
            return nullptr;
        }
        
        auto id = (ast_identifier*)name.get();
        
        assert(id);
        if(!id) {
            return nullptr;
        }
        
        pFunc = Function::Create(pFuncTy, Function::ExternalLinkage, id->name, &ctx.module);
        
        ctx.func_is_pure.emplace(pFunc, is_pure);
        
        int i = 0;
        for(auto& arg : pFunc->args()) {
            arg.setName(args[i].identifier->name);
            i++;
        }
        
        llvm::DISubroutineType* pTySub = ctx.dbuilder.createSubroutineType(ctx.dbuilder.getOrCreateTypeArray(di_type_signature));
        
        ctx.di_func_sigs[pFunc] = pTySub;
        
        return pFunc;
    }
    
    llvm::Value* ast_function::generate_ir(llvm_ctx& ctx) {
        auto pszFuncName = ((ast_identifier*)(prototype->name).get())->name;
        Function* pFunc = ctx.module.getFunction(pszFuncName);
        
        if(!pFunc) {
            // prototype's gen_ir returns a Function*
            pFunc = (Function*)prototype->generate_ir(ctx);
        }
        
        assert(pFunc);
        
        if(!pFunc) {
            return nullptr;
        }
        
        if(!pFunc->empty()) {
            log_err(this, "Attempted redefinition of function '%s'\n", pszFuncName);
            return nullptr;
        }
        
        // DI
        DIFile* pUnit = ctx.dbuilder.createFile(ctx.compile_unit->getFilename(), ctx.compile_unit->getDirectory());
        DIScope* pScope = pUnit;
        DISubprogram *SP = ctx.dbuilder.createFunction(pScope, pszFuncName, llvm::StringRef(), pUnit, prototype->line + 1, ctx.di_func_sigs[pFunc], false, true, prototype->line + 1, llvm::DINode::FlagPrototyped, false);
        pFunc->setSubprogram(SP);
        ctx.di_scope = SP;
        // DI
        
        BasicBlock* pBB = BasicBlock::Create(ctx.ctx, "entry", pFunc);
        ctx.builder.SetInsertPoint(pBB);
        
        ctx.locals.clear();
        
        int iArg = 0;
        for(auto& arg : pFunc->args()) {
            auto stackvar = create_entry_block_alloca(ctx, pFunc, arg.getName(), arg.getType());
            ctx.builder.CreateStore(&arg, stackvar);
            ctx.locals.emplace(arg.getName(), stackvar);
            
            //auto type_name = prototype->args[iArg].type;
            auto type_name = prototype->args[iArg].type->get_type_name();
            
            DILocalVariable *D = ctx.dbuilder.createParameterVariable(SP, arg.getName(), ++iArg, pUnit, line, ctx.di_types[type_name], true);
            ctx.dbuilder.insertDeclare(stackvar, D, ctx.dbuilder.createExpression(), llvm::DebugLoc::get(line, 0, SP), ctx.builder.GetInsertBlock());
        }
        
        ctx.builder.SetCurrentDebugLocation(llvm::DebugLoc::get(line, col, pUnit));
        
        ctx.current_function_pure = prototype->is_pure;
        
        bool succ = true;
        for(auto& line : lines) {
            if(!line->generate_ir(ctx)) {
                succ = false;
            }
        }
        ctx.current_function_pure = false;
        
        if(succ) {
            return pFunc;
        } else {
            log_err(this, "Codegen for function '%s' has failed, erasing\n", pszFuncName);
            pFunc->eraseFromParent();
            return nullptr;
        }
    }
    
    llvm::Value* ast_branching::generate_ir(llvm_ctx& ctx) {
        Value* pVCond = condition->generate_ir(ctx);
        if(!pVCond) {
            log_err(this, "Bad condition\n");
            return nullptr;
        }
        
        if(!pVCond->getType()->isIntegerTy()) {
            log_err(this, "Condition does not evaluate to boolean!\n");
            return nullptr;
        }
        
        // Check equality of the condition and '1'
        pVCond = ctx.builder.CreateICmpEQ(pVCond, ConstantInt::get(ctx.ctx, APInt(1, 1)));
        Function* pFunc = ctx.builder.GetInsertBlock()->getParent();
        
        // Create then BB and the continuation BB
        BasicBlock* pBBThen = BasicBlock::Create(ctx.ctx, "then", pFunc);
        BasicBlock* pBBElse = BasicBlock::Create(ctx.ctx, "else", pFunc);
        
        // if 'cond' is true go to 'then' otherwise to 'else'
        auto br = ctx.builder.CreateCondBr(pVCond, pBBThen, pBBElse);
        
        // Generate 'then' code
        ctx.builder.SetInsertPoint(pBBThen);
        Value* pVThen = line->generate_ir(ctx);
        if(!pVThen) {
            log_err(line.get(), "Error after branch\n");
            return nullptr;
        }
        
        // Unconditional jump to the 'else' block
        ctx.builder.CreateBr(pBBElse);
        pBBThen = ctx.builder.GetInsertBlock();
        
        //pFunc->getBasicBlockList().push_back(pBBElse);
        ctx.builder.SetInsertPoint(pBBElse);
        //PHINode* pPhi = ctx.builder.CreatePHI(Type::getVoidTy(ctx.ctx), 0);
        return br;
    }
    
    llvm::Value* ast_type::generate_ir(llvm_ctx& ctx) {
        return nullptr;
    }
}