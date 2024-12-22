#include "common.h"

/** ================================================================== /
 *                               Pos
 * ================================================================== */

bool Pos::operator< (const Pos& b){
  if(line < b.line) {
    return true;
  } else {
    if(line == b.line) {
      return column <= b.column;
    } else {
      return false;
    }
  }
}
bool Pos::operator== (const Pos& b) {
  return line == b.line && column == b.column;
}

string Pos::str() {
  return to_string(line) + ":" + to_string(column);
}

json Pos::dump() {
  json j;
  j["line"] = line;
  j["column"] = column;
  return j;
}

Pos Pos::restore(json j) {
  int line, column;
  j["line"].get_to(line);
  j["column"].get_to(column);
  Pos p = Pos(line, column);
  // cout << p.str() << endl;
  return p;
}

/** ================================================================== /
 *                               Variable
 * ================================================================== */

string Variable::str() {
  return "name:\t"+name+"\ttype:\t"+type+"\t"+declRef.str();
}
bool Variable::operator==(const Variable& b) {
  return type == b.type && name == b.name;
}

json Variable::dump() {
  json j;
  j["name"] = name;
  j["type"] = type;
  j["declRef"] = declRef.dump();
  j["type_sid"] = type_sid;
  j["has_init"] = hasInit;
  return j;
}

Variable Variable::restore(json j, map<unsigned int, TypeBase*>& sid_type_map) {
  string name = j["name"].get<string>();
  string type = j["type"].get<string>();
  Pos declRef = Pos::restore(j["declRef"]);
  Variable v(type, name);
  j["type_sid"].get_to(v.type_sid);
  j["has_init"].get_to(v.hasInit);
  v.type_ptr = sid_type_map[v.type_sid];
  v.declRef = declRef;
  return v;
}


/** ================================================================== /
 *                               Bricks
 * ================================================================== */

void Brick::print() {
  cout << "===========================================" << endl;
  cout << code << endl;
  cout << "In Brick from " << start.str() << " to " << end.str() << "body" << bodyStart.str() << endl;
  cout << "PreContext:" << endl;
  for(int j=0;j<preContext.size();j++) {
    Variable v = preContext[j];
    cout << "\t" << v.str() << endl;
  }
  cout << "PostContext:" << endl;
  for(int j=0;j<postContext.size();j++) {
    Variable v = postContext[j];
    cout << "\t" << v.str() << endl;
  }
}

void Brick::addInitialization(vector<Variable>& global_vars) {
  vector<TypeBase*> global_tps;
  for(Variable v : global_vars) global_tps.push_back(v.type_ptr);
  vector<string> lines = split(code, '\n');
  for(int l=0;l<lines.size();l++) {
    string line = lines[l];
    vector<Variable*> decls;
    for(int j=0;j<postContext.size();j++) {
      Variable* v = &(postContext[j]);
      if(v->declRef.line == start.line + l) {
        decls.push_back(v);
        int cur = decls.size() - 1;
        while(cur > 0 && decls[cur]->declRef.column < decls[cur-1]->declRef.column) {
          Variable* tmp = decls[cur];
          decls[cur] = decls[cur-1];
          decls[cur-1] = tmp;
        }
      }
    }
    vector<string> parts;
    int end = 0;
    for(int i=0;i<decls.size();i++)  {
        Variable* decl = decls[i];
        int first_line_column = (l == 0) ? start.column : 1;
        string a = line.substr(end, (decl->declRef.column-first_line_column)-end);
        parts.push_back(a);
        parts.push_back(decl->name);
        if(!decl->hasInit && 
            decl->type_ptr->getDesugaredAndDeconstTypePtr()->eType != eArray &&
            decl->type_ptr->getDesugaredAndDeconstTypePtr()->eType != eFunctionPointer) {
          parts.push_back(" = " + decl->type_ptr->getInitialization(global_tps));
        }
        end = (decl->declRef.column-first_line_column) + decl->name.size();
    }
    parts.push_back(line.substr(end, line.size() - end));
    lines[l] = "";
    for(string p : parts) lines[l] += p;
  }
  code = join('\n', lines);
  return;
}

string Brick::rename(map<int, string> rename_map) {
  vector<string> lines = split(code, '\n');
  for(int l=0;l<lines.size();l++) {
    string line = lines[l];
    vector<Variable*> decls;
    for(int j=0;j<preContext.size();j++) {
      Variable* v = &(preContext[j]);
      if(v->declRef.line == start.line + l) {
        decls.push_back(v);
        int cur = decls.size() - 1;
        while(cur > 0 && decls[cur]->declRef.column < decls[cur-1]->declRef.column) {
          Variable* tmp = decls[cur];
          decls[cur] = decls[cur-1];
          decls[cur-1] = tmp;
        }
      }
    }
    vector<string> parts;
    int end = 0;
    for(int i=0;i<decls.size();i++) {
      Variable* decl = decls[i];
      int first_line_column = (l == 0) ? start.column : 1;
      if(end > line.size()) {
        cout << "out_of_range error !" << endl;
        return "";
      }
      string a = line.substr(end, (decl->declRef.column-first_line_column)-end);
      // cout << a << endl;
      parts.push_back(a);
      parts.push_back(rename_map[decl->declRef.column]);
      // parts.push_back(line.substr(decl->scopeBegin.column, decl->name.size()));
      end = (decl->declRef.column-first_line_column) + decl->name.size();
      // cout << "end: " << end << endl;
    }
    if(end > line.size()) {
      cout << "out_of_range error !" << endl;
      return "";
    }
    parts.push_back(line.substr(end, line.size() - end));
    lines[l] = "";
    for(string p : parts) lines[l] += p;
    // cout << lines[l] << endl;
  }
  return join('\n', lines);
}

json Brick::dump() {
  json j;
  j["code"] = code;
  j["start"] = start.dump();
  j["end"] =  end.dump();
  j["bodyStart"] = bodyStart.dump();
  j["type"] = type;
  j["id"] = id;
  json preContext_j;
  for(Variable v : preContext) {
    preContext_j.push_back(v.dump());
  }
  j["preContext"] = preContext_j;
  json postContext_j;
  for(Variable v : postContext) {
    postContext_j.push_back(v.dump());
  }
  j["postContext"] = postContext_j;
  json preFunc_j;
  for(Signature s : preFunc) {
    preFunc_j.push_back(s.dump());
  }
  j["preFunc"] = preFunc_j;
  json types;
  for(unsigned int sid : dependency_types_sid) {
    types.push_back(sid);
  }
  j["dependency_types"] = types;
  return j;
}

Brick Brick::restore(json j, map<unsigned int, TypeBase*>& sid_type_map) {
  // cout << j.dump() << endl;
  string code, type;
  j["code"].get_to(code);
  j["type"].get_to(type);
  // cout << code << endl;
  Pos start = Pos::restore(j["start"]);
  Pos end = Pos::restore(j["end"]);
  Pos bodyStart = Pos::restore(j["bodyStart"]);
  Brick b(start, end);
  b.code = code;
  b.type = type;
  j["id"].get_to(b.id);
  json preContext = j["preContext"];
  json postContext = j["postContext"];
  json preFunc = j["preFunc"];
  json dependency_types = j["dependency_types"];
  for (json::iterator it = preContext.begin(); it != preContext.end(); ++it) {
    Variable v = Variable::restore(*it, sid_type_map);
    b.preContext.push_back(v);
  }
  for (json::iterator it = postContext.begin(); it != postContext.end(); ++it) {
    Variable v = Variable::restore(*it, sid_type_map);
    b.postContext.push_back(v);
  }
  for (json::iterator it = preFunc.begin(); it != preFunc.end(); ++it) {
    Signature s = Signature::restore(*it, sid_type_map);
    b.preFunc.push_back(s);
  }
  for (json::iterator it = dependency_types.begin(); it != dependency_types.end(); ++it) {
    b.dependency_types.push_back(sid_type_map[*it]);
    b.dependency_types_sid.push_back(*it);
  }
  return b;
}


/** ================================================================== /
 *                           Signature
 * ================================================================== */


json Signature::dump() {
  json j;
  j["start"] = start.dump();
  j["sig"] = sig;
  j["name"] = name;
  j["returnValue"] = returnValue.dump();
  json params_json;
  for(Variable v : params) {
    params_json.push_back(v.dump());
  }
  j["params"] = params_json;
  return j;
}

Signature Signature::restore(json j, map<unsigned int, TypeBase*>& sid_type_map) {
  Signature s;
  string sig, name;
  j["sig"].get_to(sig);
  j["name"].get_to(name);
  s.sig = sig;
  s.name = name;
  s.start = Pos::restore(j["start"]);
  s.returnValue = Variable::restore(j["returnValue"], sid_type_map);
  json params = j["params"];
  for (json::iterator it = params.begin(); it != params.end(); ++it) {
    Variable v = Variable::restore(*it, sid_type_map);
    s.params.push_back(v);
  }
  return s;
}

string Signature::print(vector<TypeBase*>& gv) {
  string res = sig;
  res += " { return ";
  if(returnValue.type != "void") res += returnValue.type_ptr->getInitialization(gv);
  res += " ;}; \n";
  return res;
}


/** ================================================================== /
 *                            Random
 * ================================================================== */

vector<int> randRecord;
int randAction = -1;

int getRand(int n) {
  assert(n != 0);
  struct timespec tp;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID,&tp);
  srand(tp.tv_nsec);
  int r;
  if(randAction == -1) {
      r = rand() % n;
      randRecord.push_back(r);
  } else {
    r = randRecord[randAction++];
  }
  // cout << "n: " << n << " r: " << r << endl;
  return r;
  // return n-1;
}

/** ================================================================== /
 *                           Rename
 * ================================================================== */


enum {NotMatch, ImplicitMatch, Match};

int isMatch(TypeBase* p, TypeBase* c) {
  if(p->getDesugaredTypePtr()->eType != c->getDesugaredTypePtr()->eType) {
    return NotMatch;
  }
  // if(p->isConst() != c->isConst()) return false;
  switch(p->getDesugaredTypePtr()->eType) {
    case eStruct:
    case eFunctionPointer:
    case eUnion: {
      return p->name == c->name;
    }
    case eConst: {
      return isMatch(p->ptr_type, c->ptr_type);
    }
    case eArray: {
      if((((ArrayImpl*)p)->dimensions.size() == ((ArrayImpl*)c)->dimensions.size()) && isMatch(p->ptr_type, c->ptr_type) == Match) {
        return Match;
      } else {
        return NotMatch;
      }
    }
    case ePointer: {
      return isMatch(p->ptr_type, c->ptr_type) == Match;
    }
    case eSimple: {
      // SimpleImpl* p_simple = (SimpleImpl*) p->getDesugaredTypePtr();
      // SimpleImpl* c_simple = (SimpleImpl*) c->getDesugaredTypePtr();
      // if(p_simple->UnqualifiedType == "void" && c_simple->UnqualifiedType == "void") return true;
      // if(p_simple->UnqualifiedType == "void" || c_simple->UnqualifiedType == "void") return false;
      if(p->name == c->name) return Match;
      if(p->name == "void" || c->name == "void") return NotMatch;
      if((p->name == "float" && c->name != "double") || (p->name == "double" && c->name != "float")) return NotMatch;
      return ImplicitMatch; 
    }
    default: {
      return NotMatch;
    }
  }
}

map<int, string> isSubset(vector<Variable> parent, vector<Variable> child) {
  // vector<string> renameMap;
  map<int, string> renameMap;
  for(Variable c : child) {
    vector<Variable> participants;
    for(Variable p : parent) {
      if(isMatch(p.type_ptr, c.type_ptr)) {
        participants.push_back(p);
      }
    }
    if(participants.size() == 0) {
      renameMap.clear();
      return renameMap;
    } else {
      int rand = getRand(participants.size());
      renameMap[c.declRef.column] = participants[rand].name;
    }
  }
  return renameMap;
}


/** ================================================================== /
 *                               Others
 * ================================================================== */

vector<string> split(string code, char sep) {
  vector<string> res;
  int sp = 0,fp=0;
  while(fp < code.length()){
      fp = code.find(sep,sp);
      res.push_back(code.substr(sp, fp-sp));
      sp = fp + 1;
  }
  return res;
}

string join(char c, vector<string> src){
    string res = "";
    if(src.size() == 0) return res;

    vector<string>::iterator it = src.begin();
    res += *it;
    
    for(it++;it!=src.end();it++){
        res += c;
        res += *it;
    }
    return res;
}


string path_join(int num, ...) {
  va_list args; 
	va_start(args, num);  
  string path;
  while(num > 0) {
    char *arg;
    arg = va_arg(args, char*);
    int len = strlen(arg);
    while(len > 0 && arg[len-1] == '/') {
      len--;
    }
    string tmp(arg);
    path += tmp.substr(0, len);
    if(num != 1) path += "/";
    num--;
  }
  return path;
}


void writeFileJson(string path, json j) {
  ofstream os;
  os.open(path, ios::out);
  cout << j.dump(2);
  if (!os.is_open())
    cout << "error：can not find or create the file" << endl;
  os << j.dump(2);
  os.close();
}

vector<string> getFiles(string dir) {
  DIR* d = opendir(dir.c_str());
  if (d == NULL) {
    printf("d == NULL");
  }
  vector<string> res;
  struct dirent* entry;
  while ( (entry=readdir(d)) != NULL) {
    string filename(entry->d_name);
    res.push_back(filename);
  }

  closedir(d);
  return res;
}



vector<string> findSizeofType(string code) {
    regex pattern("sizeof\\((\\w+)\\)");
    smatch m;
    auto pos = code.cbegin();
    auto end = code.cend();
    vector<string> res;
    for (; regex_search(pos, end, m, pattern); pos = m.suffix().first)
    {
        cout << "find sizeof " << m.str(1) << std::endl;
        res.push_back(m.str(1));
    }
    return res;
}

string getFuncPointerStr(string type, string name) {
  int index = type.find("(*)");
  string tmp = "";
  string partA = type.substr(0, index+2);
  string partB = type.substr(index+2, type.size() - partA.size());
  tmp += partA;
  tmp += name;
  tmp += partB;
  return tmp;
}