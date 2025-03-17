#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
// #include "llvm/IR/Constants.h"
// #include "llvm/IR/ConstantFolder.h"
// #include "llvm/Intrinsic/ConstantFolding.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <queue>

using namespace llvm;

namespace {

struct SkeletonPass : public PassInfoMixin<SkeletonPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    for (auto &F : M) {

      for (auto &B : F) {
        // errs() << "Basic Blocks: ";
        // for (auto &I : B) {
        std::queue<Instruction *> q;
        for (auto I_it = B.begin(); I_it != B.end();) {
          Instruction &I = *I_it;
          ++I_it;

          if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
            errs() << "I see a Binary Operator";
            BO->print(errs());
            errs() << '\n';
            if (auto *C1 = dyn_cast<Constant>(BO->getOperand(0))) {
              errs() << "I see a Constant\n";
              if (auto *C2 = dyn_cast<Constant>(BO->getOperand(1))) {
                if (Constant *FoldedInstr = ConstantFoldBinaryInstruction(
                        BO->getOpcode(), C1, C2)) {
                  errs() << "Folding binary op in " << F.getName() << '\n';
                  q.push(BO);

                  BO->replaceAllUsesWith(FoldedInstr);
                  BO->eraseFromParent();
                  continue;
                }
              }
            }

          } else if (auto *CI = dyn_cast<CallInst>(&I)) {
            if (Function *CalledF = CI->getCalledFunction()) {
              if (CalledF->isDeclaration()) {
                errs() << "Call to external function: " << CalledF->getName()
                       << "\n";
              } else {
                errs() << "Call to internal function: " << CalledF->getName()
                       << "\n";
              }
            } else {
              errs() << "Indirect function call detected.\n";
            }
          }

          // errs() << "Instruction: ";
          // I.print(errs(), true);
          // errs() << "\n";
          // I.print(errs());
        }
      }
    }
    return PreservedAnalyses::all();
  };
};

} // namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "Skeleton pass",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(SkeletonPass());
                });
          }};
}
