#include <cstdio>
#include <cstring>

#include "ast.h"
#include "log.h"

using namespace llvm;

namespace core {
    void ast_real::dump() {
        printf("literal(%f)", value);
    }
    
    void ast_boolean::dump() {
        printf("literal(%s)", value ? "true" : "false");
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
        printf(" : %s)", type.c_str());
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
    
    // Codegen
    
    static llvm::AllocaInst* create_entry_block_alloca(llvm_ctx& ctx, llvm::Function* pFunc, const llvm::StringRef name, llvm::Type* pType) {
        IRBuilder<> TmpB(&pFunc->getEntryBlock(), pFunc->getEntryBlock().begin());
        return TmpB.CreateAlloca(pType, 0, name);
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
    
    llvm::Value* ast_real::generate_ir(llvm_ctx& ctx) {
        return ConstantFP::get(ctx.ctx, APFloat(value));
    }
    
    llvm::Value* ast_boolean::generate_ir(llvm_ctx& ctx) {
        return ConstantInt::get(ctx.ctx, APInt(1, value ? 1 : 0));
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
            ctx.builder.CreateStore(R, pVar);
            ret = R;
            return ret;
        } else {
            auto L = lhs->generate_ir(ctx);
            if(!L) {
                log_err(lhs.get(), "Failure in left-hand side expression\n");
                return ret;
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
        
        return ret;
    }
    
    // TODO:
    llvm::Value* ast_declaration::generate_ir(llvm_ctx& ctx) {
        auto pFunc = ctx.builder.GetInsertBlock()->getParent();
        auto ret = create_entry_block_alloca(ctx, pFunc, identifier->name, str_to_type(ctx, type));
        ctx.locals.emplace(identifier->name, ret);
        return ret;
    }
    
    llvm::Value* ast_function_call::generate_ir(llvm_ctx& ctx) {
        llvm::Value* ret = nullptr;
        
        if(strcmp(name->name, "return") == 0) { // return is technically a function call
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
            
            std::vector<llvm::Value*> vargs;
            for(int i = 0; i < args.size(); i++) {
                vargs.push_back(args[i]->generate_ir(ctx));
                if(!vargs.back()) {
                    return ret;
                }
            }
            
            ret = ctx.builder.CreateCall(pFunc, vargs, "calltmp");
            
            return ret;
        }
    }
    
    llvm::Value* ast_prototype::generate_ir(llvm_ctx& ctx) {
        FunctionType* pFuncTy;
        Function* pFunc;
        std::vector<llvm::Type*> type_signature;
        
        for(auto& arg : args) {
            auto pType = str_to_type(ctx, arg.type);
            if(!pType) {
                log_err(this, "Unknown type '%s' in function type signature\n", arg.type.c_str());
                return nullptr;
            }
            type_signature.push_back(pType);
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
        
        int i = 0;
        for(auto& arg : pFunc->args()) {
            arg.setName(args[i].identifier->name);
            i++;
        }
        
        return pFunc;
    }
    
    llvm::Value* ast_function::generate_ir(llvm_ctx& ctx) {
        auto pszFuncName = ((ast_identifier*)(prototype->name).get())->name;
        Function* pszFunc = ctx.module.getFunction(pszFuncName);
        
        if(!pszFunc) {
            // prototype's gen_ir returns a Function*
            pszFunc = (Function*)prototype->generate_ir(ctx);
        }
        
        assert(pszFunc);
        
        if(!pszFunc) {
            return nullptr;
        }
        
        if(!pszFunc->empty()) {
            log_err(this, "Attempted redefinition of function '%s'\n", pszFuncName);
            return nullptr;
        }
        
        BasicBlock* pBB = BasicBlock::Create(ctx.ctx, "entry", pszFunc);
        ctx.builder.SetInsertPoint(pBB);
        
        ctx.locals.clear();
        
        for(auto& arg : pszFunc->args()) {
            auto stackvar = create_entry_block_alloca(ctx, pszFunc, arg.getName(), arg.getType());
            ctx.builder.CreateStore(&arg, stackvar);
            ctx.locals.emplace(arg.getName(), stackvar);
        }
        
        bool succ = true;
        for(auto& line : lines) {
            if(!line->generate_ir(ctx)) {
                succ = false;
            }
        }
        
        if(succ) {
            //llvm::verifyFunction(*pszFunc);
            
            return pszFunc;
        } else {
            log_err(this, "Codegen for function '%s' has failed, erasing\n", pszFuncName);
            pszFunc->eraseFromParent();
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
}