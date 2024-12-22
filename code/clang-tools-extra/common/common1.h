#ifndef COMMON1_H
#define COMMON1_H

#include "header.h"
#include "type.h"

extern vector<int> randRecord;
extern int randAction;

class Pos {
public:
  int line, column;
  Pos():line(0),column(0){};
  Pos(int l, int c): line(l), column(c) {}; 
  bool operator< (const Pos& b);
  bool operator== (const Pos& b);
  string str(); 
  json dump();
  static Pos restore(json j);
};


class Variable {
public:
  string type;
  unsigned int type_sid;
  TypeBase* type_ptr;
  string name;
  vector<Pos> locations;
  Pos declRef;
  bool hasInit;
  Variable(string t, string n):type(t), name(n), declRef(Pos(0, 0)), type_sid(-1), type_ptr(nullptr),hasInit(false) {};
  string str();
  bool operator==(const Variable& b);
  json dump();
  static Variable restore(json j, map<unsigned int, TypeBase*>& sid_type_map);
};

class Signature {
public:
  Pos start;
  vector<Variable> params;
  Variable returnValue;
  string name;
  string sig;
  Signature():returnValue(Variable("void", "r1")){};
  json dump();
  static Signature restore(json j, map<unsigned int, TypeBase*>& sid_type_map);
  string print(vector<TypeBase*>& gv);
};

class Shift {
public:
  string op;
  Pos ls, le, rs, re;
  Shift(){};
  Shift(Pos ls, Pos le, Pos rs, Pos re, string op): ls(ls), le(le), rs(rs), re(re), op(op){};
};

class Brick {
public:
  
  string id;
  
  Pos start, end;
  
  Pos bodyStart;
  
  vector<Variable> preContext, postContext;
  
  vector<Signature> preFunc;
  
  string code;
  
  string type;
  
  string file;
  
  vector<unsigned int> dependency_types_sid;
  vector<TypeBase*> dependency_types;
  
  int fail_count;
  
  Shift* sh;
  Brick() {};
  Brick(Pos s, Pos e): fail_count(0),sh(nullptr) {start=s;end=e;type="unknown";Pos bodyStart(0,0);};
  void print();
  string rename(map<int, string> rename_map);
  json dump();
  static Brick restore(json j, map<unsigned int, TypeBase*>& sid_type_map);
  void addInitialization(vector<Variable>& global_vars);
};

int getRand(int n);

vector<string> split(string code, char sep);

string join(char c, vector<string> src);

vector<int> getNfromRangeM(int n, int m);

vector<string> getMap(vector<Variable> context, vector<Variable> preContext);

map<int, string> isSubset(vector<Variable> parent, vector<Variable> child);

// void writeFileJson(string path, Value root);
void writeFileJson(string path, json j);

vector<string> getFiles(string dir);

string path_join(int num, ...);

string removeQualOfType(string type);

string getFuncPointerStr(string type, string name);

vector<string> findSizeofType(string code);


#endif