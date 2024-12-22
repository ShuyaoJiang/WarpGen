#include "type.h"


/** ================================================================== /
 *                             Dump
 * ================================================================== */

json TypeBase::dump() {
  json j;
  j["eType"] = eType;
  j["sid"] = sid;
  j["name"] = name;
  j["ptr_type_sid"] = ptr_type_sid;
  // j["qualifier"] = qualifier;
  return j;
}

json SimpleImpl::dump() {
  json j = TypeBase::dump();
  return j;
}

json CompoundImpl::dump() {
    json j = TypeBase::dump();
    json field_json_t;
    for(unsigned int f : fields_type) {
        field_json_t.push_back(f);
    }
    j["fields_type"] = field_json_t;
    json field_json_n;
    for(string f : fields_name) {
        field_json_n.push_back(f);
    }
    j["fields_name"] = field_json_n;
    j["isUnname"] = isUnname;
    j["isTypedef"] = isTypedef;
    return j;
}

json ArrayImpl::dump() {
    json j = TypeBase::dump();
    json dim_j;
    for(string d : dimensions) dim_j.push_back(d);
    j["dim"] = dim_j;
    return j;
}

json ConstImpl::dump() {
  json j = TypeBase::dump();
  j["qual"] = qualifier;
  return j;
}

json FunctionPointerImpl::dump() {
    json j = TypeBase::dump();
    json params_json;
    for(unsigned int f : params_sid) {
        params_json.push_back(f);
    }
    j["params"] = params_json;
    return j;
}

json EnumImpl::dump() {
    json j = TypeBase::dump();
    json tags_j;
    for(string d : tags) tags_j.push_back(d);
    j["tags"] = tags_j;
    return j;
}

/** ================================================================== /
 *                             Restore
 * ================================================================== */

void TypeBase::restorePtr(map<unsigned int, TypeBase*>& sid_type_map) {
    
  if(ptr_type != nullptr) return;
  if(!isBasicType()) 
    ptr_type = sid_type_map[ptr_type_sid];
  return;
}

void CompoundImpl::restorePtr(map<unsigned int, TypeBase*>& sid_type_map) {
    
  if(fields_type_ptr.size() == fields_type.size()) return;
  for(unsigned int f : fields_type) {
    TypeBase* f_ptr = sid_type_map[f];
    fields_type_ptr.push_back(f_ptr);
  }
  
  for(int i=0;i<fields_type_ptr.size();i++) {
    
    if(fields_type_ptr[i] != nullptr) fields_type_ptr[i]->restorePtr(sid_type_map);
  }
  return;
}

void FunctionPointerImpl::restorePtr(map<unsigned int, TypeBase*>& sid_type_map) {
  
  if(params.size() == params_sid.size()) return;
  for(unsigned int p : params_sid) {
    TypeBase* p_ptr = sid_type_map[p];
    params.push_back(p_ptr);
  }
  
  for(int i=0;i<params.size();i++) {
    
    if(params[i] != nullptr) params[i]->restorePtr(sid_type_map);
  }
  return;
}

string DependencyNode::print(set<string>& printed_dependency_node) {
  string res = "";
  for(DependencyNode* d : dependency) {
    res += d->print(printed_dependency_node);
    res += "\n";
  }
  
  if(type != nullptr && printed_dependency_node.find(type->name) == printed_dependency_node.end()) {
    res += type->getDecl();
    res += "\n";
  }
  return res;
}



/** ================================================================== /
 *                    Declaration && Name
 * ================================================================== */


string TypeBase::getDecl() {
  
  assert("This type shouldn't getdecl by dependency !");
  return "";
}

string TypedefImpl::getDecl() {
  string res = "";
  switch (ptr_type->getDesugaredTypePtr()->eType){
    case eFunctionPointer: {
      res += "typedef ";
      res += ptr_type->getName(name);
      res += " ;\n";
      break;
    }
    case eArray: {
      res += "typedef ";
      
      res += ptr_type->getName(name);
      res += " ;\n";
      break;
    }
    default: {
      res += "typedef ";
      res += ptr_type->getName(name);
      res += " ";
      res += name;
      res += " ;\n";
      break;
    }
    // default:
    
    //   break;
  }
  return res;
}

string EnumImpl::getDecl() {
  string res = "";
  res += name;
  res += " { ";
  for(string tag : tags) {
    res += tag;
    res += " ";
  }
  res += " };\n";
  return res;
}


string CompoundImpl::getDecl() {
  // TODO：unname
  string res = "";
  if(isUnname) res += (eType == eUnion) ? "union " : "struct ";
  else if(isTypedef) {
    res += "typedef ";
    res += (eType == eUnion) ? "union " : "struct ";
  } else res += name;
  res += " {\n";
  for(int i = 0; i < fields_type_ptr.size(); i++) {
    string tmp = "\t";
    switch (fields_type_ptr[i]->getDesugaredTypePtr()->eType){
      case eArray:
      case eFunctionPointer: {
        tmp += fields_type_ptr[i]->getDesugaredTypePtr()->getName(fields_name[i]);
        tmp += " ;\n";
        break;
      }
      default: {
        tmp += fields_type_ptr[i]->getName(fields_name[i]);
        tmp += " ";
        tmp += fields_name[i];
        tmp += ";\n";
        break;
      }
    }
    res += tmp;
  }
  res += "}";
  if(isTypedef) res += name;
  res += ";\n";
  return res;
}


string TypeBase::getName(string v_name) {
  return name;
}


string FunctionPointerImpl::getName(string v_name) {
  int index = name.find("(*)");
  string tmp = "";
  string partA = name.substr(0, index+2);
  string partB = name.substr(index+2, name.size() - partA.size());
  tmp += partA;
  tmp += v_name;
  tmp += partB;
  return tmp;
}

string ArrayImpl::getName(string v_name) {
  // TODO：multi-dimension array
  int index = name.find("[");
  string tmp = "";
  string partA = name.substr(0, index);
  string partB = name.substr(index, name.size() - partA.size());
  tmp += partA;
  tmp += " ";
  tmp += v_name;
  tmp += partB;
  return tmp;
}


string CompoundImpl::getName(string v_name) {
  if(isUnname) {
    string decl = getDecl();
    decl = decl.substr(0, decl.size()-2); 
    return decl;
  } else {
    return name;
  }
}



/** ================================================================== /
 *                               Hash
 * ================================================================== */

string TypeBase::getHashStmt(string name, set<string>& hashed) {return "";}

string CompoundImpl::getHashStmt(string name, set<string>& hashed) {
  string res = "";
  
  // res += "// hash compound variable " + name + " \n";
  // for(int i=0;i<fields_type_ptr.size();i++) {
  //   string f_name = "";
  //   if(!isUnname) f_name = name + "." + fields_name[i];
  //   else f_name = name + fields_name[i];
  //   res += fields_type_ptr[i]->getHashStmt(f_name, hashed);
  // }
  return res;
}

string TypedefImpl::getHashStmt(string name, set<string>& hashed) { return ptr_type->getHashStmt(name, hashed);}
string ConstImpl::getHashStmt(string name, set<string>& hashed) { return ptr_type->getHashStmt(name, hashed);}



string PointerImpl::getHashStmt(string name, set<string>& hashed) { 
  // return ptr_type->getHashStmt("(*" + name + ")");
  return "";
}

string ArrayImpl::getHashStmt(string name, set<string>& hashed) { 
  return "";
}


string SimpleImpl::getHashStmt(string vname, set<string>& hashed) {
  if(hashed.find(vname) != hashed.end()) return "";
  hashed.insert(vname);
  string res = "";
  
  if(name.find("double") != name.npos || name.find("float") != name.npos) {
    res += "transparent_crc_bytes(&(";
    res += vname;
    res += "), sizeof(";
    res += vname;
    res += "), \"" + vname + "\"";
    res += " , ";
    res += "1);\n";
  } else {
    res += "transparent_crc(";
    res += vname;
    res += ", \"" + vname + "\"";
    res += " , 1);\n";
  }
  return res;
}


/** ================================================================== /
 *                       Dependency Node
 * ================================================================== */


DependencyNode* TypeBase::saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp) {
    return ptr_type->saveDependency(used_dependency_node, new_gv_tp);
}


DependencyNode* SimpleImpl::saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp) {return nullptr;}



DependencyNode* ArrayImpl::saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp) {return ptr_type->saveDependency(used_dependency_node, new_gv_tp);}
DependencyNode* PointerImpl::saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp) {
  
  if(used_dependency_node.find(name) != used_dependency_node.end()) return nullptr;
  used_dependency_node.insert(name);
  DependencyNode* d = ptr_type->saveDependency(used_dependency_node, new_gv_tp);
  new_gv_tp.push_back(this);
  return d;
}
DependencyNode* ConstImpl::saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp) {return ptr_type->saveDependency(used_dependency_node, new_gv_tp);}

DependencyNode* CompoundImpl::saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp) {
  if(used_dependency_node.find(name) != used_dependency_node.end()) return nullptr;
  used_dependency_node.insert(name);
  DependencyNode* d = new DependencyNode();
  d->type = this;
  for(TypeBase* f : fields_type_ptr) {
      DependencyNode* field_denpendency = f->saveDependency(used_dependency_node, new_gv_tp);
      if(field_denpendency) d->dependency.push_back(field_denpendency);
  }
  

  if(!isUnname) new_gv_tp.push_back(this);
  return d;
}


DependencyNode* FunctionPointerImpl::saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp) {
    DependencyNode* d = new DependencyNode();
    
    d->type = nullptr;
    for(TypeBase* p : params) {
        DependencyNode* params_denpendency = p->saveDependency(used_dependency_node, new_gv_tp);
        if(params_denpendency) d->dependency.push_back(params_denpendency);
    }
    return d;
}


DependencyNode* TypedefImpl::saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp) {
  if(used_dependency_node.find(name) != used_dependency_node.end()) return nullptr;
  used_dependency_node.insert(name);
  DependencyNode* d = new DependencyNode();
  d->type = this;
  DependencyNode* ptr_type_dependency = ptr_type->saveDependency(used_dependency_node, new_gv_tp);
  if(ptr_type_dependency) d->dependency.push_back(ptr_type_dependency);
  return d;
}


DependencyNode* EnumImpl::saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp) {
    if(used_dependency_node.find(name) != used_dependency_node.end()) return nullptr;
    used_dependency_node.insert(name);
    DependencyNode* d = new DependencyNode();
    d->type = this;
    return d;
}

/** ================================================================== /
 *                            Initialize
 * ================================================================== */

int findTpInGV(TypeBase* tp, vector<TypeBase*>& global_vars) {
  int match_index = -1;
  for(int i=0;i<global_vars.size();i++) {
    if(global_vars[i]->isEqual(tp)) {
      match_index = i;
      break;
    }
  }
  return match_index;
}


string TypeBase::getInitialization(vector<TypeBase*>& global_vars)  { return  "NULL";}

string ArrayImpl::getInitialization(vector<TypeBase*>& global_vars)  { return  "{}";}


string SimpleImpl::getInitialization(vector<TypeBase*>& global_vars) {
  // if(name == "char") return "\'a\'";
  // if(name == "void") return "";
  // else return "1";
  int match_index = findTpInGV(this, global_vars);
  string gv_name = "g_" + to_string(match_index);
  if(match_index != -1) {
    return gv_name;
  } else {
    assert("Should find !!");
    return "NULL";
  }
}



string PointerImpl::getInitialization(vector<TypeBase*>& global_vars) {
  string ret = "";
  
  int match_index = findTpInGV(this->ptr_type->getDesugaredAndDeconstTypePtr(), global_vars);
  string gv_name = "g_" + to_string(match_index);
  if(match_index != -1) {
    
    
    ret += " &" + gv_name;
  } else {
    
    ret += "NULL";
  }
  return ret;
}


string CompoundImpl::getInitialization(vector<TypeBase*>& global_vars) {
  int match_index = findTpInGV(this->ptr_type, global_vars);
  string gv_name = "g_" + to_string(match_index);
  
  if(match_index != -1) return gv_name;
  string ret = "{";
  if(eType == eUnion) {
    if(fields_type_ptr.size()) ret += fields_type_ptr[0]->getInitialization(global_vars);
    ret += "}";
  } else {
    for(int i = 0; i < this->fields_name.size(); i++) {
      ret += fields_type_ptr[i]->getInitialization(global_vars);
      ret += (i == this->fields_name.size() - 1 ? "" : ", ");
    }
    ret += "}";
  }
  return ret;
}

string TypedefImpl::getInitialization(vector<TypeBase*>& global_vars) {
  return ptr_type->getInitialization(global_vars);
}

string ConstImpl::getInitialization(vector<TypeBase*>& global_vars) {
  return ptr_type->getInitialization(global_vars);
}

string EnumImpl::getInitialization(vector<TypeBase*>& global_vars) {return "0";};


/** ================================================================== /
 *                               Others
 * ================================================================== */

TypeBase* TypedefImpl::getDesugaredTypePtr() {
  
  TypeBase* res = ptr_type;
  while(res->eType == eTypedef) {
    res = res->ptr_type;
  }
  return res;
}


bool TypeBase::isBasicType() {
  return !(eType == eArray || eType == ePointer || eType == eTypedef || eType == eConst);
}


vector<string> SimpleImpl::typeName = {"char", "int", "short", "long", "long long", "unsigned char", "unsigned int", "unsigned short", 
            "unsigned long", "float", "double", "unsigned long long", "signed char"};

TypeBase* restoreType(json j, map<string, TypeBase*>& name_type_map) {
  unsigned int typeDesc;
  string name;
  unsigned int sid;
  unsigned int ptr_type_sid;
  j["eType"].get_to(typeDesc);
  j["name"].get_to(name);
  j["sid"].get_to(sid);
  j["ptr_type_sid"].get_to(ptr_type_sid);
  
  if(name_type_map.find(name) != name_type_map.end()) return name_type_map[name];
  TypeBase* tp = nullptr;
  switch (typeDesc) {
    case eSimple: {
      SimpleImpl* st = new SimpleImpl(sid, name);
      tp = st;
      break;
    }
    case ePointer: {
      PointerImpl* pt = new PointerImpl(sid, name);
      pt->ptr_type_sid = ptr_type_sid;
      tp = pt;
      break;
    }
    case eUnion:      
    case eStruct: {
      CompoundImpl* ct = new CompoundImpl(sid, name, typeDesc == eUnion);
      j["isUnname"].get_to(ct->isUnname);
      j["isTypedef"].get_to(ct->isTypedef);
      json fields_type = j["fields_type"];
      for (json::iterator it = fields_type.begin(); it != fields_type.end(); ++it) {
        unsigned int f_sid;
        it->get_to(f_sid);
        ct->fields_type.push_back(f_sid);
      }
      json fields_name = j["fields_name"];
      for (json::iterator it = fields_name.begin(); it != fields_name.end(); ++it) {
        string name;
        it->get_to(name);
        ct->fields_name.push_back(name);
      }
      tp = ct;
      break;
    }
    case eTypedef: {
      TypedefImpl* tdt = new TypedefImpl(sid, name);
      tdt->ptr_type_sid = ptr_type_sid;
      tp = tdt;
      break;
    }
    case eArray: {
      json dims_json = j["dim"];
      vector<string> dims;
      for (json::iterator it = dims_json.begin(); it != dims_json.end(); ++it) {
        string dim;
        it->get_to(dim);
        dims.push_back(dim);
      }
      ArrayImpl* at = new ArrayImpl(sid, name, dims);
      at->ptr_type_sid = ptr_type_sid;
      tp = at;
      break;
    }
    case eFunctionPointer: {
      FunctionPointerImpl* fpt = new FunctionPointerImpl(sid, name);
      json params_json = j["params"];
      for (json::iterator it = params_json.begin(); it != params_json.end(); ++it) {
        unsigned int p;
        it->get_to(p);
        fpt->params_sid.push_back(p);
      }
      tp = fpt;
      break;
    }
    case eConst: {
      unsigned int qual;
      j["qual"].get_to(qual);
      ConstImpl* ctp = new ConstImpl(sid, name, qual);
      ctp->ptr_type_sid = ptr_type_sid;
      tp = ctp;
      break;
    }
    case eEnum: {  
      json tags_j = j["tags"];
      vector<string> tags;
      for (json::iterator it = tags_j.begin(); it != tags_j.end(); ++it) {
        string tag;
        it->get_to(tag);
        tags.push_back(tag);
      }
      EnumImpl* et = new EnumImpl(sid, name, tags);
      tp = et;
      break;
    }
    default:
      break;
  }
  name_type_map[name] = tp;
  return tp;
}

bool TypeBase::isEqual(TypeBase* a) {
  if(a == nullptr) return false;
  
  a = a->getDesugaredTypePtr();
  TypeBase* b = this->getDesugaredTypePtr();
  if(a->eType != b->eType) return false;
  
  if(a->ptr_type == nullptr && b->ptr_type == nullptr) {
    return a->eType == b->eType && a->name == b->name;
  }
  
  return b->ptr_type->isEqual(a->ptr_type);
}

