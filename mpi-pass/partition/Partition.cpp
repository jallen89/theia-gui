#include "llvm/Pass.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Module.h"
using namespace llvm;

namespace {
  struct PartitionPass : public ModulePass {
    static char ID;
    PartitionPass() : ModulePass(ID) {}

    std::vector<Value *> indicators;
    std::vector<Value *> delegators;
    std::vector<Value *> identifiers;

    Module *mM;

    virtual bool runOnModule(Module &M) {
        
        print_globals(M);

        this->mM = &M;
        // Step 1: Extract the necessary annotations.
        errs() << *M.getFunction("main") << "\n";
        find_global_annotations(M);
        find_local_annotations(M);
        find_identifiers(M);
        //print_annotations();

        // Step 2: Begin Instrumenting Source code. 
        // Step 2.1: Identify defines of identifiers and delegators.
        instrument_indicators();
    }


    /* Methods for instrumenting code.---------------------------------------
     * TODO: Create Reference to Functions needed by runtime library.
     * TODO: Update runtime library to work correctly.
     * TODO: Instrument creation of delegators, so they can inherit the
     * parent's context.
     */

    /*
     *
     * TODO: Define all possible creations/instantiations of indicator vars.
     */
    void instrument_indicators() {
        errs() << "Instrumentation indicator vars.!\n";
        for(auto *ind : indicators) {
            for (User *u: ind->users()) {
                if (auto *I = dyn_cast<AllocaInst>(u)) {
                    errs() << *u << "\n";
                    //XXX. For local vars that represent indicators, we need to
                    //allocate alloca instructions, so they inherit the context
                    //of the parent.
                }
            }
        }
    }

    /* Methods for finding annotated variables--------------------------------
     * TODO: Extract Identifiers from annotated data structures.
     *
     * */

    bool find_identifiers(Module &M) {
        errs() << "Searching for Identifiers.\n";

        Function *TF = M.getFunction("llvm.ptr.annotation.p0i8");
        if (not TF) {
            errs() << "No Identifiers Found!\n";
            return false;
        }

        for (User *u : TF->users()) {
            auto *I = dyn_cast<Instruction>(u);
            // This is the member function, which repredents the identifiers.
            // TODO: Identify which delegator this ID belongs to.
            // TODO: Identify what is necessary to be tracked from here.
            auto *a_id = I->getOperand(0) << "\n";
        }
    }

    bool find_global_annotations(Module &M) {
        // Get a Reference to the global variable containing annotation info.
        GlobalVariable* gv = M.getGlobalVariable("llvm.global.annotations");
        if (not gv) {
            errs() <<  "No Global Annotations Var Found!\n";
            return false;
        }
        
        // Iterate through global annotations.
        for (Value *u : gv->operands()) {
          // Get Annotation type and variable annotated.
          auto *a = dyn_cast<ConstantArray>(u);
          auto *a_type = dyn_cast<ConstantExpr>(a->getOperand(0)->getOperand(1));
          auto *a_var = dyn_cast<User>(a->getOperand(0)->getOperand(0))->getOperand(0);
          add_annotated(a_type, a_var);
        }
        return true;
    }

    bool find_local_annotations(Module &M) {
        // Check if local annotations exist.
        Function *TF =  M.getFunction("llvm.var.annotation");
        if (not TF) {
            errs() << "No Local Annotation Vars Found!\n";
            return false;
        }

        for (User *u : TF->users()) {
          // Get Annotation type and variable annotated.
            auto *a_var = dyn_cast<User>(u->getOperand(0))->getOperand(0);
            auto *a_type = dyn_cast<ConstantExpr>(u->getOperand(1));
            find_id_field(a_var);
            add_annotated(a_type, a_var);
        }
        return true;
    }

    void find_id_field(Value *a_var) {
        errs() << "a_var id: " << *a_var << "\n";
        auto *a_struct = dyn_cast<StructType>(a_var->getType());

        if (auto *AI = dyn_cast<AllocaInst>(a_var)) {
            if(auto *a_type = dyn_cast<StructType>(AI->getAllocatedType())) {
                for (auto *e : a_type->elements()) {
                    errs() << *e << "\n";
                }
            }
        }
        //errs() << "a_type: " << *a_struct << "\n";

    }

    void print_globals(Module &M) {
        for (auto g = M.global_object_begin(); g != M.global_object_end(); g++) {
            errs() << "globals: " << *g << "\n";
            for(User *u: (*g).users()) {
                Value *v = dyn_cast<Value>(u);
                errs() << *u << "\n";
                for (User *vu : v->users()) {
                    errs() << *vu << "\n";
                }
            }
        }
    }
    void add_annotated(ConstantExpr *ann_struct_expr, Value *ann_var) {
        StringRef ann_type = gepToStr(ann_struct_expr);

        if (ann_type.contains("indicator")) {
            indicators.push_back(ann_var);
        } else if(ann_type.contains("delegator")) {
            delegators.push_back(ann_var);
        } else if(ann_type.contains("identifier")) {
            identifiers.push_back(ann_var);
        } else {
            errs() << "Invalid annotation type: " << ann_type << "\n";
        }
    }

    /* Util methods.---------------------------------------------------*/
    StringRef gepToStr(ConstantExpr *annotation) {
        auto *c_str = dyn_cast<GlobalVariable>(annotation->getOperand(0));
        auto *c_data = dyn_cast<ConstantDataArray>(c_str->getInitializer());
        return c_data->getAsString();
    }


    void print_annotations(){
        _print_annotation(indicators, "indicators");
        _print_annotation(delegators, "delegators");
        _print_annotation(identifiers, "identifiers");
    }

    void _print_annotation(std::vector<Value*> a_vec, std::string a_type) {
        errs() << a_type << "\n";
        for (auto *v: a_vec) {
            errs() << "\t" << *v << "\n";
        }
    }
  };
}

char PartitionPass::ID = 0;

static RegisterPass<PartitionPass> X("PartitionPass", "PartitionPass Pass",
                                     false,  // Look only at CFG?
                                     true); // Is this an analysis Pass?
