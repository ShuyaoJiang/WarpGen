#include "header.h"

class DependencyNode;


enum eSimpleType
{
	eVoid,
	eChar,
	eInt,
	eShort,
	eLong,
	eLongLong,
	eUChar,
	eUInt,
	eUShort,
	eULong,
	eFloat,
	eDouble,
	eULongLong,
	eInt128,
	eUInt128,
};

enum eTypeDesc
{
    eSimple,
    ePointer,
    eUnion,
    eStruct,
    eTypedef,
    eArray,
    eFunctionPointer,
    eConst,
    eEnum
};

class TypeBase {
public:
    eTypeDesc eType;					
    unsigned int sid; 					
    string name;						
    unsigned int ptr_type_sid;		    
    TypeBase* ptr_type;                 
    
    TypeBase(eTypeDesc eType, unsigned int sid, string n):eType(eType), sid(sid), name(n), ptr_type_sid(0), ptr_type(nullptr) {};
    virtual ~TypeBase() {};
    
    virtual json dump();
    
    bool isBasicType();
    
    TypeBase* getCannonicalTypePtr() {
        TypeBase* cur = this;
        while(cur->ptr_type != nullptr) {
            cur = cur->ptr_type;
        } 
        return cur;
    }
    
    
    
    
    virtual void restorePtr(map<unsigned int, TypeBase*>& sid_type_map);
    
    virtual DependencyNode* saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp)=0;
    
    virtual string getDecl();
    
    virtual string getName(string v_name);
    
    virtual string getHashStmt(string name, set<string>& hashed);
    
    virtual TypeBase* getDesugaredTypePtr() {return this;};
    
    virtual string getInitialization(vector<TypeBase*>& global_vars);
    
    bool isEqual(TypeBase* a);
    
    TypeBase* getDesugaredAndDeconstTypePtr() {
        TypeBase* res = this;
        while(res->eType == eTypedef || res->eType == eConst) {
            res = res->ptr_type;
        }
        return res;
    }
};


class SimpleImpl : public TypeBase {
public:
    static vector<string> typeName;
    SimpleImpl(unsigned int sid, string n): TypeBase(eSimple, sid, n) {};
    json dump();
    ~SimpleImpl() {cout << "free simple" << endl;};
    DependencyNode* saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp);
    string getHashStmt(string name, set<string>& hashed);
    string getInitialization(vector<TypeBase*>& global_vars);
};


class PointerImpl : public TypeBase {
public:
    
    PointerImpl(unsigned int sid, string n): TypeBase(ePointer, sid, n) {};
    ~PointerImpl() {};
    DependencyNode* saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp);
    string getHashStmt(string name, set<string>& hashed);
    string getInitialization(vector<TypeBase*>& global_vars);
};


class CompoundImpl : public TypeBase {
public:
    vector<unsigned int> fields_type;       
    vector<TypeBase*> fields_type_ptr;       
    vector<string> fields_name;       
    bool isUnname;						            
    bool isTypedef;                       
    
    CompoundImpl(unsigned int sid, string n, bool isUnion): TypeBase(isUnion ? eUnion : eStruct, sid, n), isUnname(false), isTypedef(false) {};
    ~CompoundImpl() {};
    json dump();
    void restorePtr(map<unsigned int, TypeBase*>& sid_type_map);
    DependencyNode* saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp);
    string getDecl();
    string getHashStmt(string name, set<string>& hashed);
    string getName(string v_name);
    string getInitialization(vector<TypeBase*>& global_vars);
};


class TypedefImpl: public TypeBase {
public:
    TypedefImpl(unsigned int sid, string n): TypeBase(eTypedef, sid, n) {};
    ~TypedefImpl() {};
    DependencyNode* saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp);
    string getDecl();
    string getHashStmt(string name, set<string>& hashed);
    TypeBase*getDesugaredTypePtr();
    string getInitialization(vector<TypeBase*>& global_vars);
};


class ArrayImpl : public TypeBase {
public:
    vector<string> dimensions;    
    ArrayImpl(unsigned int sid, string n, vector<string> dim):TypeBase(eArray, sid, n), dimensions(dim) {};
    ~ArrayImpl() {};
    json dump();
    DependencyNode* saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp);
    string getHashStmt(string name, set<string>& hashed);
    string getName(string v_name);
    virtual string getInitialization(vector<TypeBase*>& global_vars);
};



class FunctionPointerImpl : public TypeBase {
public:
    vector<unsigned int> params_sid;        
    vector<TypeBase*> params;               
    FunctionPointerImpl(unsigned int sid, string n): TypeBase(eFunctionPointer, sid, n) {};
    ~FunctionPointerImpl() {};
    void restorePtr(map<unsigned int, TypeBase*>& sid_type_map);
    DependencyNode* saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp);
    string getName(string v_name);  
    json dump();
};


class ConstImpl : public TypeBase {
public:
    unsigned int qualifier;             
    ConstImpl(unsigned int sid, string n, unsigned int qualifier): TypeBase(eConst, sid, n), qualifier(qualifier) {};
    json dump();
    ~ConstImpl() {};
    DependencyNode* saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp);
    string getHashStmt(string name, set<string>& hashed);
    string getInitialization(vector<TypeBase*>& global_vars);
};

class EnumImpl : public TypeBase {
public:
    vector<string> tags;  
    EnumImpl(unsigned int sid, string n, vector<string> tags): TypeBase(eEnum, sid, n), tags(tags) {};
    json dump();
    DependencyNode* saveDependency(set<string>& used_dependency_node, vector<TypeBase*>& new_gv_tp);
    ~EnumImpl() {};
    string getDecl();
    string getInitialization(vector<TypeBase*>& global_vars);
};

TypeBase* restoreType(json j, map<string, TypeBase*>& name_type_map);

class DependencyNode {
public:
    TypeBase* type;
    vector<DependencyNode*> dependency;
    string print(set<string>& printed_dependency_node);
    DependencyNode(): type(nullptr), dependency(vector<DependencyNode*>()) {};
    
    
    
};