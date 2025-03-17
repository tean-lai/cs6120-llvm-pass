#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
// #include "llvm/IR/Constants.h"
// #include "llvm/IR/ConstantFolder.h"
// #include "llvm/Intrinsic/ConstantFolding.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdint>
#include <queue>

using namespace llvm;

namespace {

struct SkeletonPass : public PassInfoMixin<SkeletonPass> {
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    bool changed = false;

    for (auto &F : M) {

      for (auto &B : F) {
        // errs() << "Basic Blocks: ";
        // for (auto &I : B) {
        std::queue<Instruction *> q;
        for (auto I_it = B.begin(); I_it != B.end();) {
          Instruction &I = *I_it;
          ++I_it;

          IRBuilder<> builder(&I);
          Value *lhs, *rhs;

          switch (I.getOpcode()) {
          case Instruction::Mul:
            errs() << "I see multiplication\n";
            lhs = I.getOperand(0);
            rhs = I.getOperand(1);

            if (auto *C = dyn_cast<ConstantInt>(rhs)) {
              if (C->getValue().isPowerOf2()) {

                errs() << "Can turn multiplication into a shift!\n";
                Value *shl = builder.CreateShl(lhs, rhs);

                for (auto &U : I.uses()) {
                  User *user = U.getUser();
                  user->setOperand(U.getOperandNo(), shl);
                }
                I.eraseFromParent();
                changed = true;
              }
            } else if (auto *C = dyn_cast<ConstantInt>(I.getOperand(0))) {
              if (C->getValue().isPowerOf2()) {
                errs() << "Can turn multiplication into a shift!\n";
                Value *shl = builder.CreateShl(lhs, rhs);
                for (auto &U : I.uses()) {
                  User *user = U.getUser();
                  user->setOperand(U.getOperandNo(), shl);
                }
                I.eraseFromParent();
                changed = true;
              }
            }

            break;
          case Instruction::UDiv:
            errs() << "I see unsigned division\n";

          case Instruction::SDiv:
            errs() << "Signed division?\n";

            if (auto *C = dyn_cast<ConstantInt>(rhs)) {
              if (C->getValue().isPowerOf2()) {
                errs() << "Can turn division into a shift!";
                unsigned int shiftAmount = C->getValue().logBase2();
                Value *shr = builder.CreateAShr(lhs, rhs);
                for (auto &U : I.uses()) {
                  User *user = U.getUser();
                  user->setOperand(U.getOperandNo(), shr);
                }
                I.eraseFromParent();
                changed = true;
              }
            }
            break;
          case Instruction::URem:
          case Instruction::SRem:
            errs() << "Processing mods\n";
            if (auto *C = dyn_cast<ConstantInt>(rhs)) {

              if (C->getValue().isPowerOf2()) {
                Value *and_instr = builder.CreateAnd(lhs, C->getValue() - 1);
                for (auto &U : I.uses()) {
                  User *user = U.getUser();
                  user->setOperand(U.getOperandNo(), and_instr);
                }
                I.eraseFromParent();
                changed = true;
              }
            }

          default:
            break;
          }

          // if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
          //   errs() << "I see a Binary Operator: ";
          //   BO->print(errs());
          //   errs() << '\n';
          //   if (I.getOpcode() == Instruction::Mul) {
          //     errs() << "this is multiplication";
          //   }

          // } else if (auto *CI = dyn_cast<CallInst>(&I)) {
          //   if (Function *CalledF = CI->getCalledFunction()) {
          //     if (CalledF->isDeclaration()) {
          //       errs() << "Call to external function: " << CalledF->getName()
          //              << "\n";
          //     } else {
          //       errs() << "Call to internal function: " << CalledF->getName()
          //              << "\n";
          //     }
          //   } else {
          //     errs() << "Indirect function call detected.\n";
          //   }
          // }

          // errs() << "Instruction: ";
          // I.print(errs(), true);
          // errs() << "\n";
          // I.print(errs());
        }
      }
    }
    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
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
