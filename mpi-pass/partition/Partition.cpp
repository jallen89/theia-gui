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
#include <set>

using namespace llvm;

namespace {
  struct PartitionPass : public ModulePass {
    static char ID;
    PartitionPass() : ModulePass(ID) {}
    // Save creations/instantions of indicators & delegators.
    std::set<Value*> indicators;
    std::set<Value*> delegators;
    std::set<Value*> identifiers;
    std::set<Value*> del_identifiers;
    std::set<Value*> del_indicators;
    // Functions created for instrumentation.
    Constant *IDInitFunc = nullptr;
    Constant *IndicatorInitFunc = nullptr;
    Constant *DelInitFunc = nullptr;
    Constant *DelIDInitFunc = nullptr;
    Module *mM = nullptr;

    //Maps A Indicator/Delegator struct mapped to the index into ID field.
    std::map<Type *, std::vector<Value *>> id_map;
  
    virtual bool runOnModule(Module &M) {
      this->mM = &M;
      // Add the functions from runtime lib to this module.
      create_instrumentation_funcs();
      // Step 1: Extract the necessary annotations.
      find_global_annotations(M);
      find_local_annotations(M);
      find_identifiers(M);
      find_del_identifiers(M);
      find_del_indicators(M);
      // Step 2: Begin Instrumenting Source code. 
      instrument_indicators();
      instrument_delegators();
      instrument_del_indicators();
    }

  
    virtual void create_instrumentation_funcs() {
      // Create Function that instruments around ID switches.

      LLVMContext &Ctx = mM->getContext();
      std::vector<Type*> paramTypes = {};
      Type *retType = Type::getVoidTy(Ctx);
      FunctionType *IDIni = FunctionType::get(retType, paramTypes, false);
      IDInitFunc = mM->getOrInsertFunction("hello", IDIni);
      paramTypes.clear();

      // Create Function that instruments around indicator switches.
      paramTypes.insert(paramTypes.begin(), Type::getInt32Ty(Ctx));
      retType = Type::getVoidTy(Ctx);
      FunctionType *IndicatorInit = FunctionType::get(
            retType, paramTypes, false);
      IndicatorInitFunc = mM->getOrInsertFunction(
            "instrument_indicator", IndicatorInit);
      paramTypes.clear();

      paramTypes.insert(paramTypes.begin(), Type::getInt32Ty(Ctx));
      retType = Type::getVoidTy(Ctx);
      FunctionType *DelInit = FunctionType::get(retType, paramTypes, false);
      DelInitFunc = mM->getOrInsertFunction("instrument_delegator", DelInit);
      paramTypes.clear();

      // Create function to instrument a delegator indicator. This function
      // changes the context of the current executing context to the ctx id of
      // the delegator (subtask) that it is about to execute.
      paramTypes.insert(paramTypes.begin(), Type::getInt32Ty(Ctx));
      retType = Type::getVoidTy(Ctx);
      FunctionType *DelIDInit = FunctionType::get(retType, paramTypes, false);
      DelIDInitFunc = mM->getOrInsertFunction("instrument_del_indicator", DelIDInit);
      paramTypes.clear();
    }

    void instrument_del_indicators() {
      for (Value *DI : del_indicators) {
        auto *SI = dyn_cast<StoreInst>(DI);
        auto idx = ArrayRef<Value*>(id_map[v2u(SI)->getOperand(0)->getType()]);
        IRBuilder<> builder(SI); 
        builder.SetInsertPoint(SI->getParent(), ++builder.GetInsertPoint());
        auto *gep = builder.CreateGEP(SI->getOperand(0), idx, "gep");
        auto *cid = builder.CreateLoad(gep, "CID");
        auto *delID = builder.CreateCall(this->DelIDInitFunc, {cid});
        errs() << "Instrumented del\n" << *SI->getParent() << "\n";
        errs() << "DelID: " << *delID << "\n";
      }
    }
  
    /* ------------------Methods for instrumenting code.-----------------------
     * TODO: Update runtime library to work correctly.
     * TODO: Instrument creation of delegators, so they can inherit the
     * parent's context.
     */
    void instrument_identifiers() {
      errs() << "Instrumentating indentifiers!\n";
      for(auto *ind : identifiers) {
        //XXX. Is there cases where this will not be a store instruction.
        if (auto *AI = dyn_cast<StoreInst>(ind)) {
          IRBuilder<> builder(AI);
          builder.SetInsertPoint(AI->getParent(), ++builder.GetInsertPoint());
          builder.CreateCall(this->IDInitFunc);
        }
      }
    }


    void instrument_indicators() {
      errs() << "Instrumenting Indicators.\n";
      for (auto *indi : indicators) {
        for (auto *u : indi->users()) {
          if (auto *I = dyn_cast<StoreInst>(u)) {
            // The indexes into the struct where the ID field exists.
            auto idx = ArrayRef<Value*>(id_map[I->getOperand(0)->getType()]);
            // Create GEP, Store, and IndiFunc Call.
            IRBuilder<> builder(I);
            builder.SetInsertPoint(I->getParent(), ++builder.GetInsertPoint());
            auto *gep = builder.CreateGEP(I->getOperand(0), idx);
            auto *cid = builder.CreateLoad(gep);
            builder.CreateCall(this->IndicatorInitFunc, {cid});
          }
        }
      }
    }


    void instrument_delegators() {
      errs() << "Instrumenting delegators!\n";
      for (auto *del : del_identifiers) {
        if (auto *SI = dyn_cast<StoreInst>(del)) {
          errs() << *SI << "\n";
          IRBuilder<> builder(SI);
          builder.SetInsertPoint(SI->getParent(), ++builder.GetInsertPoint());
          builder.CreateCall(this->DelInitFunc, {SI->getOperand(0)});
        }
      }
    }


    void find_del_indicators(Module &M) {
      auto *del_ind = _find_global(M, "del_indicator");
      if (!del_ind) {
        errs() << "Did not find delegation indicator!\n";
        return;
      }

      errs() << *del_ind << "\n";
      for (User *gep: del_ind->users()) {
        for (User *CI: gep->users()) {
          auto *ind = v2u(CI->getOperand(0))->getOperand(0);
          for (User *u: ind->users()) {
            if (auto *SI = dyn_cast<StoreInst>(u)) {
              del_indicators.insert(SI);
            }
          }
        }
      }
    }


    bool find_del_identifiers(Module &M) {
      auto *del_ident = _find_global(M, "del_identifier");
      
      errs() << *M.getFunction("main");
      // GEP --> CALL --> Bitcast --> Store Function.
      for (User *gep: del_ident->users()) {
        for (User *CI: gep->users()) {
          for (User *BI: CI->users()) {
            for (User *id : BI->users()) {
              errs() << *id << "\n";
              del_identifiers.insert(u2v(id));
            }
          }
        }
      }
    }
    
    /*-----------------Methods for finding annotated variables-----------------*/
    bool find_identifiers(Module &M) {
      errs() << "Searching for Identifiers.\n";
      // Get a reference to annotation function for pointers. 
      Function *TF = M.getFunction("llvm.ptr.annotation.p0i8");
      if (not TF) {
        errs() << "No Identifiers Found!\n";
        return false;
      }
 
      // Get the ID field for each ID field.
      for (auto *TFu : TF->users()) {
        auto *gep = v2u(v2u(TFu->getOperand(0))->getOperand(0));
        auto *S_ty = gep->getOperand(0)->getType();
        auto g_op = [gep](int idx){return gep->getOperand(idx);};
        id_map[S_ty] = std::vector<Value*>({g_op(1), g_op(2)});
      }
      return true;
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
        auto *a_var = dyn_cast<User>(
              a->getOperand(0)->getOperand(0))->getOperand(0);
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
        add_annotated(a_type, a_var);
      }
      return true;
    }
  
    void print_globals(Module &M) {
      for (auto g = M.global_object_begin(); g != M.global_object_end(); g++) {
        errs() << "globals----------------: " << *g << "\n";
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
        indicators.insert(ann_var);
      } else if(ann_type.contains("delegator")) {
        delegators.insert(ann_var);
      } else {
        errs() << "[BUG]: Invalid annotation type: " << ann_type << "\n";
      }
    }
  
    /*--------------------Util methods.---------------------------------------*/
    StringRef gepToStr(ConstantExpr *annotation) {
      auto *c_str = dyn_cast<GlobalVariable>(annotation->getOperand(0));
      auto *c_data = dyn_cast<ConstantDataArray>(c_str->getInitializer());
      return c_data->getAsString();
    }
  
    void print_annotations(){
      auto pa = [](std::set<Value*> a, std::string as) {
        errs() << as << "\n";
        for (auto *v: a) {
          errs() << "\t" << *v << "\n";
        }
      };
  
      pa(indicators, "indicators");
      pa(delegators, "delegators");
      pa(delegators, "indicators");
    }

    GlobalVariable * _find_global(Module &M, std::string contains) {
      for (GlobalVariable &g : mM->globals()) {
        if (!g.hasInitializer()) { continue;}
        // Search for identifier.
        if (auto *g_str = dyn_cast<ConstantDataArray>(g.getInitializer())) {
          if (g_str->getAsString().contains(contains)) {
            return &g;
          }
        }
      }
      return nullptr;
    }
    User *v2u(Value *v) {return dyn_cast<User>(v);}
    Value *u2v(User *u) {return dyn_cast<Value>(u);}
    //User *uOp(Value *v, int idx) {return v2u(v->getOperand(idx));}
  };
}

char PartitionPass::ID = 0;
static RegisterPass<PartitionPass> X(
    "PartitionPass", "PartitionPass Pass",
    false, 
    true);
