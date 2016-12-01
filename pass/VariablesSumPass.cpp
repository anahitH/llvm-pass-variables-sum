#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Module.h"

#include <memory>
#include <cassert>

namespace {
using namespace llvm;

class FunctionVariableSumPass : public FunctionPass
{
public:
		static char ID;

		FunctionVariableSumPass()
						: FunctionPass(ID)
		{
		}

		virtual bool runOnFunction(Function& F)
		{
				bool modified = false;
				int sum = 0;
				LLVMContext& context = F.getContext();
				IRBuilder<> builder(context);

				// allocating sum and storeing initial value
				Instruction* sumallocInstr = builder.CreateAlloca(Type::getInt32Ty(context), 0, "sum");
				Instruction* sumstoreInstr = builder.CreateStore(
												ConstantInt::get(Type::getInt32Ty(context), sum),
												sumallocInstr);

				auto& instructionList = F.getEntryBlock().getInstList();
				instructionList.push_front(sumallocInstr);
				instructionList.insertAfter(instructionList.begin(), sumstoreInstr);

				Constant* printFunc = F.getParent()->getOrInsertFunction(
										"print", Type::getVoidTy(context), Type::getInt32Ty(context), nullptr);

				for (auto& block : F) {
						for (auto& instr : block) {
								if (auto* op = dyn_cast<StoreInst>(&instr)) {
										if (op == sumstoreInstr) {
													continue;
										}

										Instruction* loadInstr = builder.CreateLoad(sumallocInstr);
										loadInstr->insertAfter(op);

										Value* right_op = op->getOperand(0);
										Value* add = builder.CreateAdd(loadInstr, right_op);
										auto* add_instr = dyn_cast<Instruction>(add);
										add_instr->insertAfter(loadInstr);

										sumstoreInstr = builder.CreateStore(add_instr, sumallocInstr);
										sumstoreInstr->insertAfter(add_instr);

										modified = true;

								}
						}
				}

				Instruction* loadInstr = builder.CreateLoad(sumallocInstr);
				auto& instructions = F.getBasicBlockList().back().getInstList();

				const auto& returnInstr = instructions.remove(instructions.back());

				instructions.push_back(loadInstr);
				Value* args[] = {loadInstr};

				//builder.SetInsertPoint(&F.getBasicBlockList().back(), ++builder.GetInsertPoint());
				CallInst* callInst = builder.CreateCall(printFunc, args);
				instructions.push_back(callInst);
				instructions.push_back(returnInstr);

				return modified;
		}

};

char FunctionVariableSumPass::ID = 0;

static void registerFunctionVariableSumPass(const llvm::PassManagerBuilder &,
                         			   						llvm::legacy::PassManagerBase &PM) {
  PM.add(new FunctionVariableSumPass());
}

static llvm::RegisterStandardPasses
  RegisterMyPass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                 registerFunctionVariableSumPass);


}

