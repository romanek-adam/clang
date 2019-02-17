// TODO: copyright

#ifndef LLVM_CLANG_STATICANALYZER_CHECKERS_CPPCHECKDEFINITIONSPARSER_H
#define LLVM_CLANG_STATICANALYZER_CHECKERS_CPPCHECKDEFINITIONSPARSER_H

#include "DeclarativeFunctionClassifier.h"
#include "clang/Config/config.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"
#include <functional>
#include <memory>
#include <system_error>

#ifdef CLANG_HAVE_LIBXML
#include <libxml/parser.h>
#endif

namespace clang {
namespace ento {

class CppcheckDefinitionsParser {
  public:
    CppcheckDefinitionsParser() = default;

    using ParserCallback = std::function<void(DeclarativeFunctionClassifier::ResourceFuncFamily)>;
    llvm::Error parse(StringRef Text, ParserCallback Callback);

  private:
    llvm::Error parseDefinitionsFromXmlDoc(xmlDocPtr Doc, xmlNode *RootElement);
    llvm::Error parseMemoryDefintion(xmlDocPtr Doc, xmlNode *Node);

    ParserCallback *Callback = nullptr;
};

} // end namespace ento
} // end namespace clang

#endif
