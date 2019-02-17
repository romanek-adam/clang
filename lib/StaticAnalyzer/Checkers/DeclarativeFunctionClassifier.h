// TODO: copyright

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_DECLARATIVEFUNCTIONCLASSIFIER_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_DECLARATIVEFUNCTIONCLASSIFIER_H

#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include <functional>
#include <memory>
#include <string>
#include <system_error>

namespace clang {
namespace ento {

class DeclarativeFunctionClassifier {
  public:
    enum class Status {
      SUCCESS,
      ERROR
    };

    enum class FuncInfo {
      UNKNOWN,
      ACQUIRE,
      RELEASE
    };

    struct ResourceFuncFamily {
      ResourceFuncFamily()
          : II_acquire(nullptr), II_release(nullptr) {}
      ResourceFuncFamily(IdentifierInfo* II_acquire, IdentifierInfo* II_release)
          : II_acquire(II_acquire), II_release(II_release) {}

      // TODO: can IdentifierInfo::getName() be used to check the name?

      std::string AcquireName;
      IdentifierInfo* II_acquire;

      std::string ReleaseName;
      IdentifierInfo* II_release;
    };

    using DefinitionsFileCallback = std::function<void(StringRef)>;
    llvm::Error loadDefinitions(StringRef Paths, DefinitionsFileCallback Callback);

    void addResourceFuncFamily(ResourceFuncFamily Family);

    void identifierInit(ASTContext &ASTCtx);

    FuncInfo getFuncInfo(const IdentifierInfo *const IdentInfo) const;

    const ResourceFuncFamily *getFamily(const IdentifierInfo *const AllocIdentInfo) const;

  private:
    using ResourceFuncFamilyRepository = std::vector<ResourceFuncFamily>;
    ResourceFuncFamilyRepository Repository;
};

} // end namespace ento
} // end namespace clang

#endif
