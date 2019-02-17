#include "DeclarativeFunctionClassifier.h"
#include "clang/Basic/VirtualFileSystem.h"
#include "clang/Config/config.h"
#include "llvm/Support/Debug.h"
#include <cassert>
#include <type_traits>

#define DEBUG_TYPE "DeclarativeFunctionClassifier"

namespace clang {
namespace ento {

llvm::Error DeclarativeFunctionClassifier::loadDefinitions(StringRef Paths, DefinitionsFileCallback Callback) {
  if (Paths.empty()) {
    LLVM_DEBUG(llvm::dbgs() << "the list of paths to definitions is empty, loading definitions skipped\n");
    return llvm::Error::success();
  }

  llvm::SmallVector<StringRef, 5> SplittedPaths;
  Paths.split(SplittedPaths, StringRef(","));

  auto FS = vfs::getRealFileSystem();
  assert(FS);

  for (auto Path : SplittedPaths) {
    auto Text = FS->getBufferForFile(Path);
    if (Text.getError()) {
      return llvm::createStringError(
          Text.getError(), "failed to read '%s': %s", Path.str().c_str(), Text.getError().message().c_str());
    }
    LLVM_DEBUG(llvm::dbgs() << "read '" << Path << "'\n");
    Callback(Text.get()->getBuffer());
  }
  return llvm::Error::success();
}

void DeclarativeFunctionClassifier::addResourceFuncFamily(ResourceFuncFamily Family) {
  LLVM_DEBUG(llvm::dbgs() << "added function family { " << Family.AcquireName << " -> " << Family.ReleaseName << " }\n");
  Repository.push_back(std::move(Family));
}

void DeclarativeFunctionClassifier::identifierInit(ASTContext &ASTCtx) {
  for (auto &Family : Repository) {
    assert(!Family.II_acquire && !Family.II_release && "init must not be called more than once");
    assert(!Family.AcquireName.empty() && "internal error - acquire name should not be empty");
    assert(!Family.ReleaseName.empty() && "internal error - release name should not be empty");
    Family.II_acquire = &ASTCtx.Idents.get(Family.AcquireName);
    Family.II_release = &ASTCtx.Idents.get(Family.ReleaseName);
  }
}

DeclarativeFunctionClassifier::FuncInfo
    DeclarativeFunctionClassifier::getFuncInfo(const IdentifierInfo *const IdentInfo) const {
  // TODO: improve search algorithm performance if needed
  for (auto &Family : Repository) {
    if (Family.II_acquire == IdentInfo) {
      return FuncInfo::ACQUIRE;
    } else if (Family.II_release == IdentInfo) {
      return FuncInfo::RELEASE;
    }
  }
  return FuncInfo::UNKNOWN;
}

const DeclarativeFunctionClassifier::ResourceFuncFamily *
DeclarativeFunctionClassifier::getFamily(const IdentifierInfo *const AllocIdentInfo) const {
  for (auto &Family : Repository) {
    if (Family.II_acquire == AllocIdentInfo) {
      return &Family;
    }
  }
  return nullptr;
}

} // end namespace ento
} // end namespace clang
