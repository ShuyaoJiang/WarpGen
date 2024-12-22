#ifndef COMMON_H
#define COMMON_H

#include "header.h"

struct LocalVarInUse {
  string name;
  unsigned beginLineNum;
  unsigned lastUsedLineNum;
  LocalVarInUse(string name, unsigned beginLineNum, unsigned lastUsedLineNum)
      : name(name), beginLineNum(beginLineNum),
        lastUsedLineNum(lastUsedLineNum) {}
};

struct LocalVar {
  string name;
  string type;
  unsigned beginLineNum;
  unsigned endLineNum;
  LocalVar(string name, string type, unsigned beginLineNum, unsigned endLineNum)
      : name(name), type(type), beginLineNum(beginLineNum),
        endLineNum(endLineNum) {}
};

class Pos {
public:
  Pos() : index(0){};
  Pos(int index) : index(index){};
  int &getIndex() { return index; }

private:
  int index;
};

class Variable {
public:
  Variable(string type, string name) : type(type), name(name) {}
  string &getType() { return type; }
  string &getName() { return name; }
  vector<Pos> &getLocations() { return locations; }

private:
  string type;
  string name;
  vector<Pos> locations;
};

class Block {
public:
  Block() {}
  Block(string code) : code(code) {}
  string &getCode() { return code; }
  vector<Variable> &getPreContext() { return preContext; }
  vector<Variable> &getPostContext() { return postContext; }

private:
  string code;
  vector<Variable> preContext;
  vector<Variable> postContext;
};

#endif