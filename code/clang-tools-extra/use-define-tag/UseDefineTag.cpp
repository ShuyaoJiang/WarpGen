#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <utility>

#include "../common/common.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

using namespace clang;
using namespace clang::tooling;
using namespace llvm;

const int customizedArgvNum = 1;

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");

class UseDefineVisitor : public RecursiveASTVisitor<UseDefineVisitor> {
public:
  static std::vector<LocalVarInUse *> localVariablesInUse;
  explicit UseDefineVisitor(ASTContext *Context) : Context(Context) {}

  bool VisitVarDecl(VarDecl *VD) {
    if (VD->isLocalVarDecl()) {
      // This is a variable definition with initialization.
      std::string varName = VD->getNameAsString();
      defineChain[varName] = VD->getLocation();
    }
    return true;
  }

  bool VisitDeclRefExpr(DeclRefExpr *DRE) {
    // This is a variable usage.
    if (VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl())) {
      std::string varName = VD->getNameAsString();
      if (defineChain.count(varName) > 0) {
        useChain[varName].push_back(DRE->getLocation());
      }
    }
    return true;
  }

  void PrintUseDefineChain() {
    SourceManager &srcMgr = Context->getSourceManager();
    for (const auto &pair : useChain) {
      string name = pair.first;
      unsigned beginLine = srcMgr.getPresumedLoc(defineChain[name]).getLine();
      unsigned lastUseLineNumber = 0;
      for (const auto &loc : pair.second) {
        unsigned temp = srcMgr.getPresumedLoc(loc).getLine();
        if (temp > lastUseLineNumber)
          lastUseLineNumber = temp;
      }
      LocalVarInUse *v = new LocalVarInUse(name, beginLine, lastUseLineNumber);
      localVariablesInUse.push_back(v);
    }
  }

private:
  ASTContext *Context;
  std::map<std::string, SourceLocation> defineChain;
  std::map<std::string, std::vector<SourceLocation>> useChain;
};

class UseDefineConsumer : public ASTConsumer {
public:
  explicit UseDefineConsumer(ASTContext *Context) : Visitor(Context) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    Visitor.PrintUseDefineChain();
  }

private:
  UseDefineVisitor Visitor;
};

class UseDefineAction : public ASTFrontendAction {
public:
  UseDefineAction() {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    return std::make_unique<UseDefineConsumer>(&CI.getASTContext());
  }
};

std::vector<LocalVarInUse *> UseDefineVisitor::localVariablesInUse;

void dumpResToJson(string path) {
  json data;
  std::cout << "Size: " << UseDefineVisitor::localVariablesInUse.size() << "\n";
  for (LocalVarInUse *v : UseDefineVisitor::localVariablesInUse) {
    json item;
    item["name"] = v->name;
    item["beginLineNum"] = v->beginLineNum;
    item["lastUsedLineNum"] = v->lastUsedLineNum;
    std::cout << v->name << "\t" << v->beginLineNum << "\t"
              << v->lastUsedLineNum << "\n";
    data.push_back(item);
  }

  std::ofstream output_file(path, std::ios_base::out);
  if (!output_file.is_open()) {
    std::cerr << "Failed to open " << path << std::endl;
    return;
  }
  output_file << std::setw(4) << data << std::endl;
  output_file.close();
  std::cout << "JSON data written to " << path << std::endl;
}

int main(int argc, char *argv[]) {
  /***********************************
   *
   * Input: Seed program
   * Output: For every local variables, <name, declare line#, last used line#>
   *
   * Command: use-define-tag <seed_program_path> <output_path> header file...
   *
   ************************************/

  string output_path(argv[2]);

  if (output_path.size() == 0) {
    std::cout << "Must specify the output path at the second argument!\n";
    return 0;
  }

  std::cout << "output path: " << output_path << "\n";

  int argcFake = argc - customizedArgvNum;
  cout << "argcFake: " << argcFake << "\n";
  const char **argvFake = new const char *[argcFake];

  argvFake[0] = argv[0];
  argvFake[1] = argv[1];
  for (int i = 2; i < argcFake; ++i) {
    argvFake[i] = argv[i + customizedArgvNum];
  }

  auto ExpectedParser =
      CommonOptionsParser::create(argcFake, argvFake, MatcherSampleCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser &OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  if (int r = Tool.run(newFrontendActionFactory<UseDefineAction>().get())) {
    return r;
  }

  dumpResToJson(output_path);

  return 0;
}
