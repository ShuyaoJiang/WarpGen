#ifndef COMMON_H
#define COMMON_H
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <algorithm>
#include <filesystem>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;


class Pos {
public:
  Pos() : index(0){};
  Pos(int index) : index(index){};
  int& getIndex() { return index; }

private:
  int index;
};

class Variable {
public:
  Variable(string type, string name) : type(type), name(name) {}
  const string& getType() const { return type; }
  const string& getName() const { return name; }
  vector<Pos> &getLocations() { return locations; }
  void setLocations(vector<Pos> locs) { locations = locs; }
  json toJson();

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
  void addPreContext(Variable pre) { preContext.push_back(pre); }
  void addPostContext(Variable post) { postContext.push_back(post); }
  json toJson();

private:
  string code;
  vector<Variable> preContext;
  vector<Variable> postContext;
};


extern vector<Block> blockList;

string readFileToString(const string& filePath);

bool isVarExist(const string& varName, const vector<Variable>& context);

vector<Pos> findVarPositions(const string& code, const string& var);

string replaceSubstring(const std::string& source, const std::string& from, const std::string& to);

void writeJsonFile(const string& sourcePath, const string& outputRootPath, vector<Block>& blockList);

#endif