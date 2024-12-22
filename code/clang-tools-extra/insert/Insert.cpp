#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <regex>
#include <stdexcept>
#include <utility>

#include "../common/common.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

#define DEBUG 0

using namespace clang;
using namespace clang::tooling;
using namespace llvm;
using namespace clang::ast_matchers;
using std::cout;
using std::make_pair;
using std::map;
using std::pair;
using std::rand;
using std::srand;
using std::time;

static llvm::cl::OptionCategory MatcherSampleCategory("Matcher Sample");

Block *Ingredient = new Block();

vector<vector<string>> typeGroups = {
    {"long long", "int64_t"},
    {"int", "int32_t", "long"},
    {"int16_t", "short"},
    {"int8_t", "char"},
    {"uint64_t", "unsigned long long"},
    {"uint32_t", "unsigned", "unsigned int", "unsigned long"},
    {"uint16_t", "unsigned short"},
    {"uint8_t", "unsigned char"},
    {"float", "double"},
    {"float *", "double *"}};

vector<std::regex> customisedTypesRegex = {
    std::regex(
        R"(^(char|short|int|long|long\s+long|float|double|unsigned\s+char|unsigned\s+short|unsigned\s+int|unsigned\s+long|unsigned\s+long\s+long)$)"),
    std::regex(
        R"(^(char|short|int|long|long\s+long|float|double|unsigned\s+char|unsigned\s+short|unsigned\s+int|unsigned\s+long|unsigned\s+long\s+long)\s*\*\s*$)"),
    std::regex(
        R"(^(char|short|int|long|long\s+long|float|double|unsigned\s+char|unsigned\s+short|unsigned\s+int|unsigned\s+long|unsigned\s+long\s+long)\s*\[(\d+)\]\s*$)"),
    std::regex(
        R"(^(char|short|int|long|long\s+long|float|double|unsigned\s+char|unsigned\s+short|unsigned\s+int|unsigned\s+long|unsigned\s+long\s+long)\s*\(\*\)\s*\[(\d+)\]\s*$)")};

static std::random_device RD;
static std::mt19937 E(RD());

map<string, int> CustomizedTypesMap = {
#define MAP_GENERATOR(TYPE, IDX) {#TYPE, IDX},
#include "./CustomizedType.def"
};

#define INT_GENERATOR(TYPE, NAME, MIN, MAX)                                    \
  static std::uniform_int_distribution<TYPE> Genetaror##NAME(MIN, MAX);
#include "./CustomizedType.def"

#define REAL_GENERATOR(TYPE, NAME, MIN, MAX)                                   \
  static std::uniform_real_distribution<TYPE> Genetaror##NAME(MIN, MAX);
#include "./CustomizedType.def"

const int customizedTypeNum = 5;
bool insertionPositionFlag = false;
string outputPath;
int customizedArgvNum = 3;

class InsertIngredient : public MatchFinder::MatchCallback {
public:
  static bool InsertStatus;
  static vector<LocalVarInUse *> localVariablesInUse;
  static vector<unsigned> liveLines;
  static vector<pair<unsigned, unsigned>> insertionPosition;
  static pair<unsigned, unsigned> chosenPos;
  static map<string, vector<string>> customizedVariables;
  static string preCode;

  InsertIngredient(Rewriter &Rewrite) : Rewrite(Rewrite) {
    srand(std::time(nullptr));
    if (!insertionPositionFlag) {
      int chosenPositionIdx =
          std::rand() / ((RAND_MAX + 1u) / (insertionPosition.size()));
      chosenPos = make_pair(insertionPosition[chosenPositionIdx].first,
                            insertionPosition[chosenPositionIdx].second);
      outs() << "The chosen insertion position is [" << chosenPos.first << " , "
             << chosenPos.second << "]\n";
    }
  }

  string generateDigit(string basicType) {
    if (CustomizedTypesMap.find(basicType) == CustomizedTypesMap.end())
      return string("");
    switch (CustomizedTypesMap[basicType]) {
#define NUM_GENERATOR(TYPE, NAME, IDX)                                         \
  case IDX:                                                                    \
    return std::to_string(Genetaror##NAME(E));
#include "./CustomizedType.def"
    default:
      return string("");
    }
  }

  bool initCustomizedVariables(string typeStr, int varType, string basicType,
                               int size) {
    for (int i = 0; i < customizedTypeNum; ++i) {
      string varName =
          basicType + std::to_string(varType) + "_temp_" + std::to_string(i);
      string varNum = basicType + std::to_string(varType) + "_temp_num_" +
                      std::to_string(i);
      string digitStr = generateDigit(basicType);
      if (digitStr.size() <= 0)
        return false;
      switch (varType) {
      case 0: // basic type
        preCode += basicType + " " + varName + " = " + digitStr + ";\n";
        break;
      case 1: // pointer (one-level)
        preCode += basicType + " " + varNum + " = " + digitStr + ";\n";
        preCode += basicType + "* " + varName + " = &" + varNum + ";\n";
        break;
      case 2: // array (one-level)
        if (size <= 0)
          return false;
        preCode +=
            basicType + " " + varName + "[" + std::to_string(size) + "];\n";
        preCode += "for(int i = 0; i < " + std::to_string(size) + "; ++i)\n\t" +
                   varName + "[i] = " + digitStr + ";\n";
        break;
      case 3: // pointer array (one-level)
        if (size <= 0)
          return false;
        preCode += basicType + " " + varNum + " = " + digitStr + ";\n";
        preCode +=
            basicType + "* " + varName + "[" + std::to_string(size) + "];\n";
        preCode += "for(int i = 0; i < " + std::to_string(size) + "; ++i)\n\t" +
                   varName + "[i] = &" + varNum + ";\n";
        break;
      default:
        return false;
      }
      customizedVariables[typeStr].push_back(varName);
    }
    return true;
  }

  virtual void run(const MatchFinder::MatchResult &Result) override {
    ASTContext *Context = Result.Context;
    SourceManager &srcMgr = Context->getSourceManager();
    const Stmt *stmt = Result.Nodes.getNodeAs<Stmt>("stmt");
    const CompoundStmt *compoundStmt =
        Result.Nodes.getNodeAs<CompoundStmt>("compoundStmt");
    const FunctionDecl *func = Result.Nodes.getNodeAs<FunctionDecl>("func");
    if (!stmt ||
        !Context->getSourceManager().isWrittenInMainFile(stmt->getBeginLoc()) ||
        InsertStatus)
      return;

    string funcName = func->getNameAsString();
    //  Save the current local variables
    if (isa<DeclStmt>(stmt)) {
      const DeclStmt *declStmt = dyn_cast<DeclStmt>(stmt);
      unsigned beginLineNum =
          srcMgr.getPresumedLoc(declStmt->getBeginLoc()).getLine();
      unsigned endLineNum =
          srcMgr.getPresumedLoc(compoundStmt->getRBracLoc()).getLine();
      for (auto *D : declStmt->decls()) {
        if (auto *VD = dyn_cast<VarDecl>(D)) {
          if (!VD->hasInit())
            continue;
          // Extract variable name and type
          string VarName = VD->getNameAsString();
          QualType VarType = VD->getType();

          LocalVar *LocV = new LocalVar(VarName, VarType.getAsString(),
                                        beginLineNum, endLineNum);
          LocalVariables[funcName].push_back(LocV);

#if DEBUG
          cout << "Variable Name: " << VarName
               << ",\tVariable Type: " << VarType.getAsString() << ",\t["
               << beginLineNum << ", " << endLineNum << "]\n";
#endif
        }
      }
    }

#if DEBUG
    cout << "stmt\t" << stmt->getStmtClassName() << "\t"
         << stmt->getBeginLoc().printToString(srcMgr) << "\t"
         << stmt->getEndLoc().printToString(srcMgr) << "\n";
    stmt->dumpPretty(*Context);
    stmt->dump();
#endif

    if (!insertionPositionFlag &&
        chosenPos.first != srcMgr.getPresumedLoc(stmt->getEndLoc()).getLine() &&
        chosenPos.second !=
            srcMgr.getPresumedLoc(stmt->getEndLoc()).getColumn())
      return;

    unsigned currentLineNum =
        srcMgr.getPresumedLoc(stmt->getBeginLoc()).getLine();
    if (std::find(liveLines.begin(), liveLines.end(), currentLineNum) ==
        liveLines.end()) {
      return;
    }

    // Get the global variables
    DeclContext *CurrentScope = Context->getTranslationUnitDecl();
    // type --> [variable1, variable2...]
    map<string, vector<string>> GlobalVariables;
    for (DeclContext *DC = CurrentScope; DC != nullptr; DC = DC->getParent()) {
      for (Decl *D : DC->decls()) {
        if (VarDecl *VD = dyn_cast<VarDecl>(D)) {
          if (VD->hasInit())
            GlobalVariables[VD->getType().getAsString()].push_back(
                VD->getNameAsString());
        }
      }
    }

    // Get the current local available variables
    vector<LocalVar *> &localVariablesInFunc = LocalVariables[funcName];
    vector<LocalVar *> localPreVariablesInFunc;
    map<string, vector<string>> LocalPreVariables;
    for (auto rit = localVariablesInFunc.rbegin();
         rit != localVariablesInFunc.rend(); ++rit) {
      // impossible to exceed beginLineNum
      if (currentLineNum >= (*rit)->endLineNum ||
          currentLineNum < (*rit)->beginLineNum)
        continue;
      string type = (*rit)->type;
      string name = (*rit)->name;

      vector<string> &temp = LocalPreVariables[type];
      if (std::find(temp.begin(), temp.end(), name) == temp.end()) {
        temp.push_back(name);
#if DEBUG
        std::cout << "pre variable: " << name << ",\t" << type << "\n";
#endif
        localPreVariablesInFunc.push_back(*rit);
      }
    }

    map<string, vector<string>> LocalPostVariables;
    for (LocalVar *var : localPreVariablesInFunc) {
      string type = var->type;
      string name = var->name;
      unsigned beginLineNum = var->beginLineNum;
      vector<string> &temp = LocalPostVariables[type];
      // if current line number < variable last used line number
      for (const auto &item : localVariablesInUse) {
        if (item->name == name && item->beginLineNum == beginLineNum &&
            item->lastUsedLineNum > currentLineNum) {
          temp.push_back(name);
          break;
        }
      }
    }

    map<string, string> VariablesMap;
    auto MatchVariable = [&](vector<Variable> &IngredientVariables,
                             map<string, vector<string>> &AvailableVariables) {
      for (auto &V : IngredientVariables) {
        // try to match the type in the local variables
        string type = V.getType();
        auto it = std::find_if(
            typeGroups.begin(), typeGroups.end(), [&](vector<string> &item) {
              return std::find(item.begin(), item.end(), type) != item.end();
            });

        vector<string> t = {type};
        auto &matchedTypeGroup = (it != typeGroups.end()) ? *it : t;

        vector<string> candidates;

        for (string &s : matchedTypeGroup) {
          if (AvailableVariables.find(s) != AvailableVariables.end()) {
            candidates.insert(candidates.end(), AvailableVariables[s].begin(),
                              AvailableVariables[s].end());
          }
        }
        // cannot find the matched type in local variables, then find them in
        // global variables
        if (candidates.size() == 0) {
          for (string &s : matchedTypeGroup) {
            if (GlobalVariables.find(s) != GlobalVariables.end()) {
              candidates.insert(candidates.end(), GlobalVariables[s].begin(),
                                GlobalVariables[s].end());
            }
          }
        }

        // cannot find the matched type in local & global variables, use
        // customized variables
        if (candidates.size() == 0) {
          bool isFound = false;
          for (string &s : matchedTypeGroup) {
            if (customizedVariables.find(s) != customizedVariables.end()) {
              candidates.insert(candidates.end(),
                                customizedVariables[s].begin(),
                                customizedVariables[s].end());
              isFound = true;
            }
          }
          if (!isFound) {
            // check whether is in the basic type list
            const int regSize = customisedTypesRegex.size();
            for (int i = 0; i < regSize; ++i) {
              std::smatch matches;
              if (std::regex_search(type, matches, customisedTypesRegex[i]) &&
                  initCustomizedVariables(
                      type, i, matches[1].str(),
                      matches.size() > 2 ? std::stoi(matches[2].str()) : 0)) {
                // customize the correponding type
                candidates.insert(candidates.end(),
                                  customizedVariables[type].begin(),
                                  customizedVariables[type].end());
                break;
              }
            }
          }
        }

        if (candidates.size() == 0) {
          std::cout << "Cannot find the corresponding type " << V.getType()
                    << "\n";
          return false;
        }

        // randomly choose one in candidate variables
        VariablesMap[V.getName()] =
            candidates[std::rand() / ((RAND_MAX + 1u) / candidates.size())];
      }
      return true;
    };

    // match the variables
    if (!(MatchVariable(Ingredient->getPreContext(), LocalPreVariables) &&
          MatchVariable(Ingredient->getPostContext(), LocalPostVariables))) {
      return;
    }

    if (insertionPositionFlag) {
      insertionPosition.push_back(make_pair<unsigned, unsigned>(
          srcMgr.getPresumedLoc(stmt->getEndLoc()).getLine(),
          srcMgr.getPresumedLoc(stmt->getEndLoc()).getColumn()));
      return;
    }

    // sort location by descending sort
    vector<pair<int, string>> VariablesLoc;
    auto AppendVariablesLoc = [&](vector<Variable> &IngredientVariables) {
      for (auto &V : IngredientVariables) {
        for (auto &P : V.getLocations())
          VariablesLoc.push_back(
              make_pair<int &, string &>(P.getIndex(), V.getName()));
      }
    };
    AppendVariablesLoc(Ingredient->getPreContext());
    AppendVariablesLoc(Ingredient->getPostContext());
    std::sort(
        VariablesLoc.begin(), VariablesLoc.end(),
        [&](const auto &I1, const auto &I2) { return I1.first > I2.first; });

    for (auto &[originVar, newVar] : VariablesMap) {
      cout << originVar << "\t--->\t" << newVar << "\n";
    }

    // replace in ingredient
    for (auto &[Loc, Name] : VariablesLoc) {
      // replace the variable `Name` in `Loc` to `VariablesMap[Name]`
      Ingredient->getCode().replace(Loc, Name.size(), VariablesMap[Name]);
    }

    if (!Rewrite.InsertTextAfterToken(stmt->getEndLoc(),
                                      ";\n" + preCode + Ingredient->getCode() +
                                          "\n")) {
      cout << preCode + Ingredient->getCode() << "\n";
      cout << "insert success in line " << chosenPos.first << "\n";

      SourceLocation Loc = srcMgr.getLocForStartOfFile(srcMgr.getMainFileID());
      Rewrite.InsertText(Loc, "\n#include \"insert.h\"\n", true, true);
      InsertStatus = true;
    }

    return;
  }

private:
  // Save the local variables in every function
  static map<string, vector<LocalVar *>> LocalVariables;
  Rewriter &Rewrite;
};

class MyASTConsumer : public ASTConsumer {
public:
  MyASTConsumer(Rewriter &R) : InsertHandler(R) {
    // https://clang.llvm.org/docs/LibASTMatchersReference.html
    // need to modify
    Matcher.addMatcher(
        stmt(hasParent(compoundStmt(hasAncestor(functionDecl().bind("func")))
                           .bind("compoundStmt")))
            .bind("stmt"),
        &InsertHandler);
  }

  void HandleTranslationUnit(ASTContext &Context) override {
    // Run the matchers when we have the whole TU parsed.
    Matcher.matchAST(Context);
  }

private:
  InsertIngredient InsertHandler;
  MatchFinder Matcher;
};

class MyFrontendAction : public ASTFrontendAction {
public:
  MyFrontendAction() {}
  void EndSourceFileAction() override {
    if (insertionPositionFlag) // just collect insertion position, do nothing.
      return;
    std::error_code EC;
    raw_fd_ostream output(outputPath, EC);
    if (EC) {
      std::cerr << "Failed to open " << outputPath << std::endl;
      TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
          .write(llvm::outs());
    } else {
      TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID())
          .write(output);
    }
  }

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI,
                                                 StringRef file) override {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());

    return std::make_unique<MyASTConsumer>(TheRewriter);
  }

private:
  Rewriter TheRewriter;
};

bool InsertIngredient::InsertStatus = false;
map<string, vector<LocalVar *>> InsertIngredient::LocalVariables;
vector<LocalVarInUse *> InsertIngredient::localVariablesInUse;
vector<unsigned> InsertIngredient::liveLines;
vector<pair<unsigned, unsigned>> InsertIngredient::insertionPosition;
pair<unsigned, unsigned> InsertIngredient::chosenPos;
map<string, vector<string>> InsertIngredient::customizedVariables;
string InsertIngredient::preCode;

bool InitIngredient(string path) {
  std::ifstream inputFile(path);
  if (!inputFile.is_open()) {
    outs() << "Cannot open " << path << "\n";
    return false;
  }

  json readData;
  inputFile >> readData;
  inputFile.close();

  Ingredient->getCode() = readData["code"];

  for (const auto &item : readData["preContext"]) {
    Variable v = Variable(item["type"], item["name"]);
    for (const auto &l : item["locations"]) {
      v.getLocations().push_back(Pos(l));
    }
    Ingredient->getPreContext().push_back(v);
  }

  for (const auto &item : readData["postContext"]) {
    Variable v = Variable(item["type"], item["name"]);
    for (const auto &l : item["locations"]) {
      v.getLocations().push_back(Pos(l));
    }
    Ingredient->getPostContext().push_back(v);
  }

  return true;
}

bool InitLocalVarInfo(string path) {
  std::ifstream inputFile(path);
  if (!inputFile.is_open()) {
    outs() << "Cannot open " << path << "\n";
    return false;
  }

  json readData;
  inputFile >> readData;
  inputFile.close();

  for (const auto &item : readData) {
    LocalVarInUse *t = new LocalVarInUse(item["name"], item["beginLineNum"],
                                         item["lastUsedLineNum"]);
    InsertIngredient::localVariablesInUse.push_back(t);
  }

  outs() << "The number of used local variables is "
         << InsertIngredient::localVariablesInUse.size() << "\n";
  return true;
}

bool InitLiveLines(string path) {
  std::ifstream inputFile(path);
  if (!inputFile.is_open()) {
    outs() << "Cannot open " << path << "\n";
    return false;
  }

  char buf[64] = {0};
  while (inputFile.getline(buf, 64)) {
    string s(buf);
    InsertIngredient::liveLines.push_back(stof(s));
  }
  outs() << "The number of live lines is " << InsertIngredient::liveLines.size()
         << "\n";
  inputFile.close();
  return true;
}

bool InitInsertPos(string path) {
  std::ifstream inputFile(path);
  if (!inputFile.is_open()) {
    outs() << "Cannot open " << path << "\n";
    return false;
  }

  int line, col;
  while (inputFile >> line >> col) {
    InsertIngredient::insertionPosition.push_back(make_pair(line, col));
  }

  inputFile.close();
  unsigned insertionPosSize = InsertIngredient::insertionPosition.size();
  cout << "The number of insertion position is " << insertionPosSize << "\n";
  return insertionPosSize > 0;
}

int main(int argc, char *argv[]) {
  /***********************************
   *
   *
   * Input: Seed program, Ingredient
   * Output: Inserted program
   * Insert the given ingredient into the seed program
   *
   * 1. Match every statment in AST
   *
   * 2. Get the current context information at the entry point
   *     - before the current statement, the defined variables context [<name,
   *type>]
   *     - after the current statment, the used variables context [<name, type>]
   *
   * 3. Use the validation rule to check if the ingredient can be inserted here;
   *if not, continue to the next statement
   *     - ingredient's pre-condition is the subset of the defined variables
   *context
   *     - ingredient's post-condition is the subset of the used variables
   *context

   * 4. If yse, match the every variable in the ingredient to the current
   *context and replace them

   * 5. Append the modified ingredient to the seed program

   * 6. Once insert successfully, return 0; if always insert failed, return 1
   *(change another ingredient)
   *
   *
   * Command: insert <seed_program_path> <block_path> <local_variable_info_path>
   <live_line_path> <output_path> <insertion_position_number>
   <insertion_positon_output_path> header files...
   *
   ************************************/

  string seedPath(argv[1]);
  string blockPath(argv[2]);
  string insertionPosOutputPath(argv[4]);
  outputPath = string(argv[3]);

  auto checkFileRead = [](string &path) {
    std::ifstream inputFile(path);
    if (!inputFile.is_open()) {
      outs() << "Cannot open " << path << " to read.\n";
      return false;
    }
    inputFile.close();
    return true;
  };

  auto checkFileWrite = [](string &path) {
    std::ofstream outputFile(path);
    if (!outputFile.is_open()) {
      outs() << "Cannot open " << path << " to write.\n";
      return false;
    }
    outputFile.close();
    return true;
  };

  if (seedPath.size() == 0 || !checkFileRead(seedPath)) {
    outs() << "1st argument: the seed path\n";
    return 1;
  }
  string::size_type const p(seedPath.find_last_of('.'));
  if (p == string::npos) {
    outs() << "the seed file should be a c file and end with .c.\n";
    return 1;
  }
  string file_without_extension = seedPath.substr(0, p);
  string localVarPath(file_without_extension + "_var.json");
  string liveLinePath(file_without_extension + "_live.txt");

  // if the sixth argument is 1, then collect all insertion position
  if (insertionPosOutputPath == "1") {
    insertionPositionFlag = true;
    insertionPosOutputPath = string(argv[5]);
    customizedArgvNum += 1;
  }

  outs() << "block path: " << blockPath << "\n";
  outs() << "local variable info path: " << localVarPath << "\n";
  outs() << "live lines path: " << liveLinePath << "\n";
  outs() << "insertion position path: " << insertionPosOutputPath << "\n";
  outs() << "output path" << outputPath << "\n";

  if ((blockPath.size() == 0 || !checkFileRead(blockPath)) ||
      (localVarPath.size() == 0 || !checkFileRead(localVarPath)) ||
      (liveLinePath.size() == 0 || !checkFileRead(liveLinePath)) ||
      (outputPath.size() == 0 || !checkFileWrite(outputPath)) ||
      (insertionPosOutputPath.size() == 0 ||
       ((insertionPositionFlag && !checkFileWrite(insertionPosOutputPath)) ||
        (!insertionPositionFlag && !checkFileRead(insertionPosOutputPath))))) {
    outs() << "2nd argument: the block path\n"
           << "3rd argument: the output path\n"
           << "4th argument: the insertion position flag(1) or the insertion "
              "position path(otherwise in the 5th argument)\n";
    return 1;
  }

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
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }

  // read the info above
  if (!InitIngredient(blockPath) || !InitLocalVarInfo(localVarPath) ||
      !InitLiveLines(liveLinePath) ||
      (!insertionPositionFlag && !InitInsertPos(insertionPosOutputPath))) {
    outs() << "Cannot read the block or local variable info or live line or "
              "insertion position successfully! Or empty insertion position!\n";
    return 1;
  }

  CommonOptionsParser &OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());
  Tool.run(newFrontendActionFactory<MyFrontendAction>().get());

  // dump insertionPosition
  if (insertionPositionFlag) {
    if (InsertIngredient::insertionPosition.size() == 0)
      return 100;

    std::ofstream outFile(insertionPosOutputPath);
    if (!outFile.is_open()) {
      outs() << "Failed to open insertion position path at "
             << insertionPosOutputPath << "\n";
      return 1;
    }
    for (auto &pos : InsertIngredient::insertionPosition) {
      outFile << pos.first << " " << pos.second << "\n";
    }
    outFile.close();
    return 0;
  }

  return InsertIngredient::InsertStatus ? 0 : 100;
}