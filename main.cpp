#include "stdafx.h"

#include "lexer.h"
#include "parser.h"

struct cpu_feature_request {
    bool vector = false;
};

core::token_stream tokenize(const char* pszSource) {
    core::token_stream ts;
    input_file f(pszSource);
    core::token t;
    
    while(!is_eof(f)) {
        t = core::get_token(f);
        t.line = f.line;
        t.col = f.col;
        if(t.second.size()) {
            ts.c.push_back(std::move(t));
        }
    }
    
    return ts;
}

bool codegen(core::llvm_ctx& ctx, const char* pszDest, core::token_stream& ts) {
    bool ret = true;
    while(!ts.empty() && ret) {
        auto expr = core::parse(ts);
        if(expr) {
            auto ir = expr->generate_ir(ctx);
            if(ir) {
                //ir->print(llvm::errs());
            } else {
                ret = false;
            }
        } else {
            ret = false;
        }
    }
    return ret;
}

bool emit_object(core::llvm_ctx& ctx, const char* pszDest, const cpu_feature_request& feat_req) {
    std::string error;
    std::error_code ec;
    llvm::legacy::PassManager pass;
    
    auto target_triple = llvm::sys::getDefaultTargetTriple();
    
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    
    auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);
    
    if(!target) {
        fprintf(stderr, "%s\n", error.c_str());
        return false;
    }
    
    auto pszCPU = "generic";
    auto pszFeatures = "";
    
    llvm::TargetOptions target_opts;
    
    auto rm = llvm::Optional<llvm::Reloc::Model>();
    auto target_machine = target->createTargetMachine(target_triple, pszCPU, pszFeatures, target_opts, rm);
    
    ctx.module.setDataLayout(target_machine->createDataLayout());
    ctx.module.setTargetTriple(target_triple);
    
    llvm::raw_fd_ostream dest(pszDest, ec, llvm::sys::fs::F_None);
    
    if(ec) {
        fprintf(stderr, "Couldn't open destination object file '%s'\n", pszDest);
        return false;
    }
    
    if(target_machine->addPassesToEmitFile(pass, dest, nullptr, llvm::TargetMachine::CGFT_ObjectFile)) {
        fprintf(stderr, "TargetMachine can't emit a file of this type\n");
        return false;
    }
    
    pass.run(ctx.module);
    dest.flush();
    return true;
}

int main(int argc, char** argv) {
    const char* pszSource = nullptr;
    const char* pszDest = nullptr;
    cpu_feature_request feat_req;
    
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i],  "-c") == 0) {
            if(i + 1 < argc) {
                if(argv[i + 1][0] != '-') {
                    pszSource = argv[i + 1];
                } else {
                    fprintf(stderr, "Expected source filename after -c\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "Expected source filename after -c\n");
                return 1;
            }
        } else if(strcmp(argv[i], "-o") == 0) {
            if(i + 1 < argc) {
                if(argv[i + 1][0] != '-') {
                    pszDest = argv[i + 1];
                } else {
                    fprintf(stderr, "Expected destination filename after -o\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "Expected destination filename after -o\n");
                return 1;
            }
        }
    }
    
    if(pszSource && pszDest) {
        auto ts = tokenize(pszSource);
        core::llvm_ctx ctx(pszDest);
        if(codegen(ctx, pszDest, ts)) {
            if(emit_object(ctx, pszDest, feat_req)) {
                return 0;
            } else {
                return 4;
            }
        } else {
            return 3;
        }
    } else {
        return 2;
    }
}