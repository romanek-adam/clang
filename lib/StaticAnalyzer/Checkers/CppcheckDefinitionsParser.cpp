#include "CppcheckDefinitionsParser.h"
#include <cassert>
#include <memory>
#include <type_traits>

namespace clang {
namespace ento {
namespace {
#ifdef CLANG_HAVE_LIBXML

using XmlDocUPtr = std::unique_ptr<std::remove_pointer<xmlDocPtr>::type, void (*)(xmlDocPtr)>;
using XmlCharUPtr = std::unique_ptr<xmlChar, void (*)(void *)>;

const xmlChar* cast(const char* Str) {
  return reinterpret_cast<const xmlChar*>(Str);
}

const char* cast(const xmlChar* Str) {
  return reinterpret_cast<const char*>(Str);
}

XmlDocUPtr wrap(xmlDocPtr Doc) {
  return {Doc, xmlFreeDoc};
}

XmlCharUPtr wrap(xmlChar* String) {
  return {String, xmlFree};
}

#endif
} // end namespace anonymous

llvm::Error CppcheckDefinitionsParser::parse(StringRef Text, ParserCallback Callback) {
  this->Callback = &Callback;
  XmlDocUPtr Doc = wrap(xmlParseMemory(Text.data(), Text.size()));
  if (!Doc) {
    xmlErrorPtr XmlError = xmlGetLastError();
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "XML parsing error: %s",
				   XmlError ? XmlError->message : "unknown error");
  }
  xmlNode *RootElement = xmlDocGetRootElement(Doc.get());
  return parseDefinitionsFromXmlDoc(Doc.get(), RootElement);
}

llvm::Error CppcheckDefinitionsParser::parseDefinitionsFromXmlDoc(xmlDocPtr Doc, xmlNode *RootElement) {
  XmlCharUPtr DefFormat = wrap(xmlGetProp(RootElement, cast("format")));
  if (!DefFormat) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "could not get 'format' attribute from <def> element");
  }
  if (!StringRef(cast(DefFormat.get())).equals("2")) {
    return llvm::createStringError(
	llvm::inconvertibleErrorCode(), "unsupported file format: %s", cast(DefFormat.get()));
  }

  for (xmlNode *CurrNode = RootElement->children; CurrNode != nullptr; CurrNode = CurrNode->next) {
    if (CurrNode->type == XML_ELEMENT_NODE) {
      StringRef Name = cast(CurrNode->name);
      if (Name.equals("memory") || Name.equals("resource")) {
	if (auto Error = parseMemoryDefintion(Doc, CurrNode)) {
	  return Error;
	}
      }
    }
  }

  return llvm::Error::success();
}

llvm::Error CppcheckDefinitionsParser::parseMemoryDefintion(xmlDocPtr Doc, xmlNode *Node) {
  assert(Node && "Node must not be NULL");

  DeclarativeFunctionClassifier::ResourceFuncFamily Family;

  for (xmlNode *CurrNode = Node->children; CurrNode != nullptr; CurrNode = CurrNode->next) {
    if (CurrNode->type != XML_ELEMENT_NODE) {
      continue;
    }
    StringRef Name = cast(CurrNode->name);
    if (Name.equals("alloc")) {
      XmlCharUPtr Value = wrap(xmlNodeListGetString(Doc, CurrNode->children, 0));
      if (Value && Family.AcquireName.empty()) {
	Family.AcquireName = cast(Value.get());
      }
      // TODO: handle the 'init' attribute
    } else if (Name.equals("dealloc")) {
      XmlCharUPtr Value = wrap(xmlNodeListGetString(Doc, CurrNode->children, 0));
      if (Value && Family.ReleaseName.empty()) {
	Family.ReleaseName = cast(Value.get());
      }
    } else {
      return llvm::createStringError(
          llvm::inconvertibleErrorCode(), "unsupported element found: %s", Name.data());
    }
  }

  if (Family.AcquireName.empty() || Family.ReleaseName.empty()) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
	"incomplete family defintion { %s -> %s }", Family.AcquireName.c_str(), Family.ReleaseName.c_str());
  }

  assert(Callback != nullptr && "internal error - Callback must not be nullptr!");
  (*Callback)(Family);
  return llvm::Error::success();
}

} // end namespace ento
} // end namespace clang
