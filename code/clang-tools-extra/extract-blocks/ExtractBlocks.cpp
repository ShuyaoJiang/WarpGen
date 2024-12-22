#include "Common.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"

using namespace clang;
using namespace clang::tooling;
using namespace std;

const int customizedArgvNum = 1;
static llvm::cl::OptionCategory StmtVisitorCategory("Stmt Visitor");
vector<Block> blockList;
int verbose;

set<string> c_standard_functions = {
    "printf", "scanf", "memset", "malloc", "memcpy", "fprintf", "fputs", "fwrite", 
    "free", "fabs", "putchar", "puts", "atoi", "strlen", "strcmp", "stdout", "stderr"
    // ... (Add more C standard functions as needed)
};

set<string> special_functions = {
    "toASCIILower", "gen_random"
    // ... (Add more special functions as needed)
};

class StmtVisitor : public RecursiveASTVisitor<StmtVisitor> {
public:
  explicit StmtVisitor(ASTContext *Context) : Context(Context) {}

  bool VisitStmt(Stmt *stmt) {
    if (verbose == 1)
      printf("I am here 3\n");
    // Determine if stmt is in main file
    if (Context->getSourceManager().isWrittenInMainFile(stmt->getBeginLoc())) {
      if (verbose == 1)
        printf("I am here 4\n");
      const char *stmtClass = stmt->getStmtClassName();
      // if (stmtClass == "ForStmt" || stmtClass == "WhileStmt" || stmtClass == "DoStmt" || stmtClass == "IfStmt" || stmtClass == "CompoundStmt") 
      if (!strcmp(stmtClass, "ForStmt") || !strcmp(stmtClass, "WhileStmt") || !strcmp(stmtClass, "DoStmt") || !strcmp(stmtClass, "IfStmt") || !strcmp(stmtClass, "CompoundStmt")) 
        ProcessStmt(stmt);
    }

    return true;
  }

  void ProcessStmt(Stmt *stmt) {
    if (verbose == 1)
      printf("I am here 5\n");
    // stmt->dump();
    
    // Get source range of stmt
    SourceManager &sm = Context->getSourceManager();
    SourceRange range = stmt->getSourceRange();

    if (range.isValid()) {
      if (verbose == 1)
       printf("I am here 6\n");
      // Get source code
      StringRef stmtCode = Lexer::getSourceText(CharSourceRange::getTokenRange(range), sm, LangOptions(), nullptr);
      string stmtCodeStr = stmtCode.str();
      if (stmtCodeStr.back() != '}') {
        stmtCodeStr += ";\n";
      }
      
      // Create Block
      Block block(stmtCodeStr);
      // llvm::outs() << block.getCode() << "\n";
    
      // Visit sub elements in stmt
      vector<VarDecl*> varDecls;
      vector<DeclRefExpr*> declRefExprs;
      vector<BinaryOperator*> assignOps;
      vector<CallExpr*> callExprs;

      InnerVisitor innerVisitor(Context, varDecls, declRefExprs, assignOps, callExprs);
      innerVisitor.TraverseStmt(stmt);

      set<string> varDeclNames;
      for (VarDecl* varDecl : varDecls) {
        // llvm::outs() << "Variable: " << varDecl->getNameAsString();
        // llvm::outs() << " Type: " << varDecl->getType().getAsString() << "\n";
        varDeclNames.insert(varDecl->getNameAsString());
        if (verbose == 1)
          printf("I am here 7\n");
      } 

      set<string> callExprNames;
      for (CallExpr* callExpr : callExprs) {
        string calleeName = callExpr->getDirectCallee()->getNameInfo().getName().getAsString();
        // llvm::outs() << "CallExpr: " << calleeName << "\n";
        callExprNames.insert(calleeName);
      }
      
      // Process post context
      for (BinaryOperator* B : assignOps) {
        if (verbose == 1)
          printf("I am here 8\n");
        //llvm::outs() << "  Assignment Operator found at location: ";
        //B->getOperatorLoc().print(llvm::outs(), Context->getSourceManager());
        //llvm::outs() << "\n";

        // Process the LHS
        if (DeclRefExpr *D = dyn_cast<DeclRefExpr>(B->getLHS()->IgnoreParenImpCasts())) {
          if (verbose == 1)
            printf("I am here 9\n");
          // llvm::outs() << "    LHS Variable: " << D->getDecl()->getNameAsString() << "\n";

          string varType = D->getDecl()->getType().getAsString();
          string varName = D->getDecl()->getNameAsString();

          if(!varDeclNames.count(varName) && !isVarExist(varName, block.getPostContext())){
            if(varType.find("UChar") != string::npos) {
              varType = replaceSubstring(varType, "UChar", "unsigned short");
            }

            Variable var(varType, varName);

            // Find all positions of var in stmt
            var.setLocations(findVarPositions(block.getCode(), varName));
            
            /*
            llvm::outs() << " Post Context: getType and getName: " << var.getType() << " and " << var.getName() << "\n";
            for(Pos& pos: var.getLocations()) {
              llvm::outs() << pos.getIndex() << " ";
            }
            llvm::outs() << "\n";
            */

            block.addPostContext(var);
            if (verbose == 1)
              printf("I am here 10\n");
          } 
        }               
      }

      // Process pre context
      for (DeclRefExpr* D : declRefExprs) {
        // llvm::outs() << "Reference to variable: " << D->getDecl()->getNameAsString();
        // llvm::outs() << " Type: " << D->getDecl()->getType().getAsString() << "\n";

        string varType = D->getDecl()->getType().getAsString();
        string varName = D->getDecl()->getNameAsString();

        if (!varDeclNames.count(varName) && !c_standard_functions.count(varName) && !special_functions.count(varName)) {

          if(!isVarExist(varName, block.getPostContext()) && !isVarExist(varName, block.getPreContext())) {
            if(varType.find("UChar") != string::npos) {
              varType = replaceSubstring(varType, "UChar", "unsigned short");
            }
            Variable var(varType, varName);

            // Find all positions of var in stmt
            var.setLocations(findVarPositions(block.getCode(), varName));
            
            /*
            llvm::outs() << " Pre Context: getType and getName: " << var.getType() << " and " << var.getName() << "\n";
            for(Pos& pos: var.getLocations()) {
              llvm::outs() << pos.getIndex() << " ";
            }
            llvm::outs() << "\n";
            */

            block.addPreContext(var);
            if (verbose == 1)
              printf("I am here 11\n");
          }
        }
      }

      blockList.push_back(block);
      if (verbose == 1)
        printf("I am here 12\n");

    }
  }


private:
  ASTContext *Context;

  class InnerVisitor : public RecursiveASTVisitor<InnerVisitor> {
    public:
      InnerVisitor(ASTContext *Context, vector<VarDecl*>& varDecls, vector<DeclRefExpr*>& declRefExprs, vector<BinaryOperator*>& assignOps, vector<CallExpr*>& callExprs)
       : Context(Context), varDecls(varDecls), declRefExprs(declRefExprs), assignOps(assignOps), callExprs(callExprs) {}
        
      bool VisitVarDecl(VarDecl *varDecl) {
        if (verbose == 1)
          printf("I am here 13\n");
        varDecls.push_back(varDecl);
        return true;
      }
        
      bool VisitDeclRefExpr(DeclRefExpr *declRefExpr) {
        if (verbose == 1)
          printf("I am here 14\n");
        declRefExprs.push_back(declRefExpr);
        return true;
      }
    
      bool VisitBinaryOperator(BinaryOperator *B) {
        if (verbose == 1)
          printf("I am here 15\n");
        if (B->isAssignmentOp()) {
          assignOps.push_back(B);
        }
        return true;
      }

      bool VisitCallExpr(CallExpr *callExpr) {
        callExprs.push_back(callExpr);
        return true;
      }

    private:
      ASTContext *Context;
      vector<VarDecl*>& varDecls;
      vector<DeclRefExpr*>& declRefExprs;
      vector<BinaryOperator*>& assignOps;
      vector<CallExpr*>& callExprs;
    };
};

class FindStmtConsumer : public clang::ASTConsumer {
public:
  explicit FindStmtConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    if (verbose == 1)
      printf("I am here 2\n");
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  StmtVisitor Visitor;
};

class FindStmtAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) {
    if (verbose == 1)
      printf("I am here 1\n");
    return std::make_unique<FindStmtConsumer>(&Compiler.getASTContext());
  }
};



int main(int argc, char **argv) {
/***********************************
   *
   * Input: Source program
   * Output: Code blocks in the source program, including pre/post context
   *
   * Command: extract-blocks <source_path> <output_path> -- -I<header_path_1> -I<header_path_2>...
   *
   ************************************/

  if (argc < 4) {
    cerr << "Please input the corret command:" << endl 
    << "extract-blocks <source_path> <output_path> -- -I<header_path_1> -I<header_path_2>..."
    << endl;
    return 1;
  }
   
  string source_path(argv[1]);
  string output_path(argv[2]);
  const char *include_prefix = argv[3];

  // Check include_prefix ("--")
  if (strcmp(include_prefix, "--") != 0) {
    cerr << "The third argument must be '--'." << endl;
    return 1;
  }

  // Check source_path
  if (!filesystem::exists(source_path)) {
    cerr << "The source program '" << source_path << "' does not exist." << endl;
    return 1;
  }
  cout << "Source program: " << source_path << endl;

  // Check output_path
  if (!filesystem::exists(output_path)) {
    if (!filesystem::create_directory(output_path)) {
      cerr << "Failed to create the output directory '" << output_path << "'." << endl;
      return 1;
    }
  } 
  cout << "Output path: " << output_path << endl;

  int argcFake = argc - customizedArgvNum;
  cout << "argcFake: " << argcFake << "\n";
  const char **argvFake = new const char *[argcFake];
  argvFake[0] = argv[0];
  argvFake[1] = argv[1];
  for (int i = 2; i < argcFake; ++i) {
    argvFake[i] = argv[i + customizedArgvNum];
  }
    
  auto ExpectedParser =
      CommonOptionsParser::create(argcFake, argvFake, StmtVisitorCategory); 
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    cerr << "Option parser failed!" << endl;
    return 1;
  }
    
  CommonOptionsParser &OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  if (int r = Tool.run(newFrontendActionFactory<FindStmtAction>().get())) {
    return r;
  }
    
  writeJsonFile(source_path, output_path, blockList);

  return 0;
}


/*
int main(int argc, char **argv) {
  if (argc == 3 || argc == 4) {
    if (argc == 4) verbose = 1;
    // Visit source code
    string source_path = argv[1];
    string source_code = readFileToString(source_path);
    cout << "source code in main: " << endl << source_code << endl;
    clang::tooling::runToolOnCode(std::make_unique<FindStmtAction>(), source_code);

    // Write to json
    string output_path = argv[2];
    writeJsonFile(source_path, output_path, blockList);
    exit(0);
  } else {
    cerr << "Invalid arguments!" << endl;
    exit(1);
  }
}
*/
