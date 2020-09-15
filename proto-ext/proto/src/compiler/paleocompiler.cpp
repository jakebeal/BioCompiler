/* Proto compiler
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

#include "paleocompiler.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <list>
#include <map>
#include <string>

#include "config.h"

#include "lisp.h"
#include "plugin_manager.h"
#include "reader.h"

using namespace std; // allow c-strings, etc; note: shadows 'pair'

int is_optimizing_lets = 0; // off due to bugs

Path* proto_path;
string srcdir;

struct TYPE; struct AST_FUN; struct AST_OP; struct AST_GOP;
struct AST; 

/****** VARIABLES ******/
struct VAR {
  const char *name; 
  TYPE *type; 
  int   n_refs;
  AST  *ast;

  VAR(const char* name, TYPE *type) {
    this->name=name; this->type=type; n_refs=0; ast=NULL;
  }
};

/****** SCRIPT ******/
// This is the output target

struct Script {
  list<uint8_t> contents;
  
  Script() {}
  Script(int n, ...) { 
    va_list v; va_start(v,n); 
    for(int i=0;i<n;i++) { contents.push_back(va_arg(v,int)); } 
    va_end(v);
  };
  ~Script() {}
  
  int size() { return contents.size(); }
  
  void add(uint8_t v1) { contents.push_back(v1); }
  void add(uint8_t v1,uint8_t v2) {addN(2,v1,v2);}
  void add(uint8_t v1,uint8_t v2,uint8_t v3) {addN(3,v1,v2,v3);}
  void add(uint8_t v1,uint8_t v2,uint8_t v3,uint8_t v4) {addN(4,v1,v2,v3,v4);}
  void add_op(OPCODE op, unsigned int argument) {
  	add(op);
  	uint8_t a[5];
  	a[0] = 0x80 | ((argument >> 28) & 0x7F);
  	a[1] = 0x80 | ((argument >> 21) & 0x7F);
  	a[2] = 0x80 | ((argument >> 14) & 0x7F);
  	a[3] = 0x80 | ((argument >>  7) & 0x7F);
  	a[4] =          argument        & 0x7F ;
  	size_t i = 0;
  	while(a[i] == 0x80) i++;
  	for(;i<5;i++) add(a[i]);
  }
  void addN(int n, ...) {
    va_list v; va_start(v,n); 
    for(int i=0;i<n;i++) { contents.push_back(va_arg(v,int)); } 
    va_end(v);
  }
  void append(Script* tail) {contents.splice(contents.end(),tail->contents);}
  void prepend(Script* head){contents.splice(contents.begin(),head->contents);}
  // for extracting script afterward
  uint8_t pop() {uint8_t v=contents.front(); contents.pop_front(); return v; }
};

/****** PROTO TYPES ******/
typedef enum {
  NUL_KIND, ANY_KIND, NUM_KIND, VEC_KIND, TUP_KIND, FUN_KIND, 
  ONE_NUM_KIND, ONE_FUN_KIND, ONE_OP_KIND, ONE_GOP_KIND,
  NUM_TYPE_KINDS
} TYPE_KIND;
const char* type_kind_name[NUM_TYPE_KINDS]={"NUL","ANY","NUM","VEC","TUP","FUN","ONE_NUM","ONE_FUN","ONE_OP","ONE_GOP"};

struct TYPE { 
  TYPE_KIND kind;
  
  TYPE(TYPE_KIND kind) { this->kind = kind; }
  virtual const char* name() { return type_kind_name[kind]; }
  virtual int size() { 
    if(kind==NUL_KIND || kind==ANY_KIND)
      uerror("ILLEGAL SIZE OF ABSTRACT TYPE %d", kind);
    return 1;
  }
  virtual bool subtype(TYPE* parent) {
    if(parent->kind==ANY_KIND) return true;
    if(parent->kind==kind) {
      if(kind==ONE_OP_KIND || kind==ONE_GOP_KIND || kind==ONE_FUN_KIND)
        uerror("UNKNOWN TYPE RULE");
      return true;
    }
    return false;
  }
};

TYPE num_type(NUM_KIND), nul_type(NUL_KIND), any_type(ANY_KIND);
TYPE vec_type(VEC_KIND), fun_type(FUN_KIND);

struct ONE_FUN_TYPE : public TYPE { 
  AST_FUN *value; 
  ONE_FUN_TYPE(AST_FUN *fun) : TYPE(ONE_FUN_KIND) { value=fun; }
};
struct ONE_OP_TYPE : public TYPE { 
  AST_OP *op; 
  ONE_OP_TYPE(AST_OP *op) : TYPE(ONE_OP_KIND) { this->op=op; }
};
struct ONE_GOP_TYPE : public TYPE { 
  AST_GOP *gop; 
  ONE_GOP_TYPE(AST_GOP *gop) : TYPE(ONE_GOP_KIND) { this->gop=gop; }
};
struct ONE_NUM_TYPE : public TYPE {
  flo num; 
  ONE_NUM_TYPE(flo num) : TYPE(ONE_NUM_KIND) { this->num=num; }
  string namestr;
  virtual const char* name() { 
    namestr=""; namestr=namestr+"(ONE_NUM "+flo2str(num)+")"; 
    return namestr.c_str();
  }
  virtual bool subtype(TYPE* parent) {
    if(parent->kind==ONE_NUM_KIND) return num==((ONE_NUM_TYPE*)parent)->num;
    return parent->kind==NUM_KIND || TYPE::subtype(parent);
  }
};

TYPE* blur_type(TYPE* t);

struct TUP_TYPE : public TYPE { 
  int len; TYPE **elt_types;
  TUP_TYPE() : TYPE(TUP_KIND) { // initializer for generic topmost tuple
    len=-1; elt_types = new TYPE*[1]; elt_types[0]=&any_type;
  }
  TUP_TYPE(int len, TYPE** types) : TYPE(TUP_KIND) { 
    this->len = len;
    elt_types = new TYPE*[len];
    for(int i=0;i<len;i++) { elt_types[i]=types[i]; }
  }
  string namestr;
  virtual const char* name() {
    namestr="(TUP";
    for(int i=0;i<len;i++) { namestr=namestr+" "+elt_types[i]->name(); }
    if(len==-1) namestr+=" *";
    namestr+=")";
    return namestr.c_str();
  }
  
  virtual int size() {
    int sum=TYPE::size();
    if(sum<0 || len<0)
      uerror("COMPILER INTERNAL ERROR: TUPLE MUST HAVE NON-NEGATIVE SIZE");
    for(int i=0;i<len;i++) { 
      if(elt_types[i]->size()<1)
        uerror("COMPILER INTERNAL ERROR: TUPLE ELT MUST HAVE POSITIVE SIZE");
      sum += elt_types[i]->size();
    }
    return sum;
  }
  virtual bool subtype(TYPE* parent) {
    if(parent->kind==TUP_KIND) {
      TUP_TYPE* p = (TUP_TYPE*)parent;
      if(p->len==-1) return true; // generic tuple is an "any tuple" match
      if(len != p->len) return false;
      for(int i=0;i<len;i++) 
        if(!elt_types[i]->subtype(p->elt_types[i])) return false;
      return true;
    } else {
      return TYPE::subtype(parent);
    }
  }
};

struct VEC_TYPE : public TYPE { 
  int len; TYPE *elt_type; 
  VEC_TYPE(int len, TYPE* elt_type) : TYPE(VEC_KIND)
  { this->len=len; this->elt_type=elt_type; }

  string namestr;
  virtual const char* name() { 
    namestr=""; namestr=namestr+"(VEC "+elt_type->name()+" "+int2str(len)+")"; 
    return namestr.c_str();
  }
  virtual int size() { return TYPE::size() + (len * elt_type->size()); }
  virtual bool subtype(TYPE* parent) {
    if(parent->kind==VEC_KIND) { // for vectors, length doesn't matter
      return elt_type->subtype(((VEC_TYPE*)parent)->elt_type);
    } else if(parent->kind==TUP_KIND) {
      TUP_TYPE* p = (TUP_TYPE*)parent;
      return (p->len==-1 && elt_type->subtype(p->elt_types[0]));
    } else {
      return TYPE::subtype(parent); 
    }
  }
};

TUP_TYPE tup_type; // the generic tuple type
VEC_TYPE num_vec_type(-1,&num_type);
VEC_TYPE num_vec3_type(3,&num_type);

struct FUN_TYPE : public TYPE {
  TYPE *result_type;
  int   arity;
  int   is_nary;
  TYPE **param_types;

  FUN_TYPE(int arity, int is_nary) : TYPE(FUN_KIND) {
    this->arity=arity; this->is_nary = is_nary;
    result_type = &any_type;
    param_types = new TYPE*[arity];
    for(int i=0;i<arity;i++) { param_types[i]=&any_type; }
  }
  FUN_TYPE(vector<TYPE*> *types) : TYPE(FUN_KIND) {
    this->result_type = (*types)[0];
    arity = types->size()-1;
    param_types = new TYPE*[arity];
    is_nary=false;
    for(int i=0;i<arity;i++) param_types[i] = (*types)[i+1];
  }
  FUN_TYPE(TYPE* result_type, ...) : TYPE(FUN_KIND) {
    int i;
    TYPE* types[32];
    va_list ap;
    va_start(ap, result_type);
    for (i = 0; ; i++) {
      types[i] = va_arg(ap, TYPE*);
      if (types[i] == (TYPE*)0) {
        is_nary = 0; break;
      } else if (types[i] == (TYPE*)1) {
        is_nary = 1; break;
      } 
    }
    va_end(ap);
    arity = i;
    this->result_type = result_type;
    param_types = new TYPE*[arity];
    for (i = 0; i < arity; i++)
      param_types[i] = types[i];
  }
  virtual ~FUN_TYPE() { delete param_types; }
  string namestr;
  virtual const char* name() {
    namestr = "(FUN (";
    for(int i=0;i<arity;i++) 
      { if(i) namestr+=" "; namestr+=param_types[i]->name(); }
    if(is_nary) namestr+=" ...";
    namestr=namestr+") "+result_type->name()+")";
    return namestr.c_str();
  }
  virtual bool subtype(TYPE* parent) {
    if(parent->kind==FUN_KIND) {
      FUN_TYPE* p = (FUN_TYPE*)parent;
      if(arity!=p->arity || is_nary!=p->is_nary) return false;
      if(!result_type->subtype(p->result_type)) return false;
      for(int i=0;i<arity;i++)
        if(!param_types[i]->subtype(p->param_types[i])) return false;
      return true;
    } else { return TYPE::subtype(parent);
    }
  }
};

#define NUMT (&num_type)
#define NUM_VEC_TYPE  ((TYPE*)&num_vec_type)
#define VECT  NUM_VEC_TYPE
#define NUM_VEC3_TYPE ((TYPE*)&num_vec3_type)
#define VEC3T NUM_VEC3_TYPE
#define TUPT ((TYPE*)&tup_type)
#define FUNT  (&fun_type)
#define ANYT  (&any_type)


/****** AST ZONE ******/
typedef enum {
  LIFT_WALK,
  LET_WALK,
  N_WALKERS
} AST_WALKER_KIND;

typedef AST* (*AST_WALKER)(AST_WALKER_KIND walker, AST* val, void* arg);

AST *default_walk (AST_WALKER_KIND action, AST *ast, void *arg) { return ast; }

typedef enum { 
  AST_BASE_CLASS, AST_LIT_CLASS, AST_REF_CLASS, AST_GLO_REF_CLASS, 
  AST_OP_CLASS, AST_GOP_CLASS, AST_FUN_CLASS, AST_LET_CLASS, 
  AST_OP_CALL_CLASS, AST_CALL_CLASS
} AST_CLASS;

template<class T>
T& list_nth(list<T> *lst, int n) {
  typename list<T>::iterator it = lst->begin();
  while(n--) it++;
  return *it;
}

// ASTs are program elements ("abstract syntax tree")
// They need to be renamed at some point, because AST isn't quite right
struct AST {
  const char* name;
  TYPE  *type; 
  AST_WALKER walkers[N_WALKERS];

  AST() {
    walkers[0]=walkers[1]=&default_walk;
    type = &nul_type;  // is this still needed?
  }
  
  virtual void print() {}
  virtual void emit(Script* script) = 0; // turn prog. elt into bytecodes
  virtual int stack_size() { return 1; }
  virtual int env_size() { return 1; } // for ast_zero_size; it's a mystery why
  virtual AST_CLASS ast_class() { return AST_BASE_CLASS; }
};

// to be phased out
AST *ast_walk (AST_WALKER_KIND action, AST *ast, void *arg) {
  return ast->walkers[action](action, ast, arg);
}

typedef struct {
  list<VAR*> *env;
  list<AST*> *glos;
  int  n_state;
  int  n_exports;
  int  export_len;
  int  n_channels;
  bool is_fun_lift;
} LIFT_DATA;

AST *ast_lift (AST *ast, LIFT_DATA *data) {
  return ast_walk(LIFT_WALK, ast, data);
}

AST *ast_optimize_lets (AST *ast) {
  return ast_walk(LET_WALK, ast, NULL);
}

/******* ERROR REPORTING *******/
bool oldc_test_mode = false; // Note: 'oldc_' deconflicts w. neocompiler
extern FILE* dump_target; // declared later
inline FILE* error_log() { return (oldc_test_mode ? dump_target : stderr); }

void cerror (AST* ast, const char* message, ...) {
  va_list ap;
  ast->print(); fprintf(error_log(),"\n");
  va_start(ap, message);
  vfprintf(error_log(),message, ap);  fprintf(error_log(),"\n");
  va_end(ap);
  exit(oldc_test_mode ? 0 : 1);
  return; 
}

void clerror (const char* name, list<AST*> *args, const char* message, ...) {
  va_list ap;
  fprintf(error_log(),"(%s", name);
  for (list<AST*>::iterator it = args->begin();
       it != args->end(); it++) {
    fprintf(error_log()," ");
    (*it)->print();
  }
  fprintf(error_log(),")\n");
  va_start(ap, message);
  vfprintf(error_log(), message, ap);
  fprintf(error_log(),"\n");
  va_end(ap);
  exit(oldc_test_mode ? 0 : 1);
  return;
}


/**** AST_LIT ****/
typedef enum { LIT_INT, LIT_FLO, LIT_TUP, LIT_VEC } LIT_KIND;
typedef union { flo val; uint8_t bytes[4]; } FLO_BYTES;

struct AST_LIT : public AST{
  flo      val;
  LIT_KIND kind;

  AST_LIT(flo num) {
    val = num; name="LIT";
    type = new ONE_NUM_TYPE(num);
    unsigned int n = (unsigned int)num;
    kind = (n == num && (n <= 0xffffffff)) ? LIT_INT : LIT_FLO;
  }

  void print() { fprintf(error_log(),"%.1f", val); }
  AST_CLASS ast_class() { return AST_LIT_CLASS; }

  void emit(Script* script) {
    // post("EMIT LIT %d %f\n", ast->kind, num);
    switch (kind) {
    case LIT_INT: { 
      if (val >= 0 && val < MAX_LIT_OPS) { script->add(LIT_0_OP+(uint8_t)val);
      } else { script->add_op(LIT_OP,(unsigned int)val); }
      break; }
    case LIT_FLO: {
      FLO_BYTES f; f.val=val;
      script->addN(5,LIT_FLO_OP,f.bytes[0],f.bytes[1],f.bytes[2],f.bytes[3]);
      break; }
      //  case LIT_TUP: {
      //    *bytes = PAIR(new_num(num), PAIR(new_num(DEF_TUP_OP), *bytes));
      //    break; }
      //  case LIT_VEC: {
      //    *bytes = PAIR(new_num(DEF_VEC_OP), *bytes);
      //    break; }
    default:
      cerror(this, "UNKNOWN LIT KIND %d", kind);
    }
  }
};

/**** AST_REF ****/
struct AST_REF : public AST {
  VAR  *var;
  int   offset;
  list<VAR*> *env;
  
  AST_REF(VAR *var, list<VAR*> *env);
  void print() { fprintf(error_log(),"%s", var->name); }
  void emit(Script* script);
  AST_CLASS ast_class() { return AST_REF_CLASS; }
};

AST *ast_ref_lift_walk (AST_WALKER_KIND action, AST *ast_, void *arg) {
  // TODO: RIP OUT
  AST_REF *ast    = (AST_REF*)ast_;
  LIFT_DATA *data = (LIFT_DATA*)arg;
  ast->env = data->env;
  return ast_;
}

extern AST_CLASS ast_fun_class;
int is_inlining_var (VAR *var) {
  return is_optimizing_lets && var->ast != NULL && 
    ((var->n_refs == 1) || (var->ast->ast_class() == AST_FUN_CLASS));
}

AST *ast_ref_let_walk (AST_WALKER_KIND action, AST *ast_, void *arg) {
  AST_REF *ast = (AST_REF*)ast_;
  VAR     *var = ast->var;
  // post("WALKING %s %lx\n", var->name, var->ast);
  if (is_inlining_var(var)) {
    // post("  INLINING %s\n", var->name);
    return ast_walk(action, ast->var->ast, arg);
  } else
    return ast_;
}

AST_REF::AST_REF(VAR *var, list<VAR*> *env) {
  walkers[0]=&ast_ref_lift_walk; walkers[1]=&ast_ref_let_walk;
  this->var=var; offset=-1; this->env=env; name="REF";
  // post("NEW REF %s AT %d ", var->name, lookup_index(var, env));  env_print(env); post("\n");
  type = var->type;
}

int lookup_index (VAR *var, list<VAR*> *env) {
  int j;
  list<VAR*>::iterator it;
  for (j = 0, it = env->begin(); it != env->end(); it++, j++) {
    if (*it == var)
      return j;
  }
  return -1;
}

void AST_REF::emit(Script* script) {
  int n = lookup_index(var, env);
  // post("EMIT VAR %s AT %d %d %lx\n", var->name, n, lst_len(env), env);
  if (n < MAX_REF_OPS) script->add(REF_OP+n+1);
  else script->add(REF_OP,n);
}

/**** AST_GLO_REF ****/
struct AST_GLO_REF : public AST {
  AST *dat;
  int  offset; // Note: offset is always positive

  AST_GLO_REF(AST *dat, int offset) {
    this->dat=dat; this->offset=offset;
    type = dat->type; name="REF";
  }

  void print() {
    fprintf(error_log(),"REF("); dat->print(); 
    fprintf(error_log()," %d)", offset);
  }
  void emit(Script* script);
  AST_CLASS ast_class() { return AST_GLO_REF_CLASS; }
};

void AST_GLO_REF::emit(Script* script) {
  if(offset < MAX_GLO_REF_OPS) script->add(GLO_REF_OP+offset+1);
  else script->add_op(GLO_REF_OP,offset);
}

/**** AST_OP, AST_GOP ****/
typedef TYPE* (*TYPE_INFER)(AST* ast);
struct AST_OP_CALL;

typedef void (*AST_EMIT)(AST_OP_CALL* val, Script* script);
extern void default_ast_op_call_emit(AST_OP_CALL* ast, Script* script);

struct AST_OP : public AST {
  const char      *name;
  const char      *opname;
  OPCODE    code;
  int        arity;
  int        is_nary;
  TYPE_INFER type_infer;
  AST_EMIT   emit_fn; // this is a leftover of some sort...

  AST_OP(const char *name, const char *opname, OPCODE code, int arity, int is_nary, TYPE *type, TYPE_INFER type_infer) {
    AST::name="OP";
    this->name=name; this->opname=opname; this->code=code;
    this->arity=arity; this->is_nary=is_nary; this->type=type;
    this->type_infer=type_infer; 
    emit_fn=&default_ast_op_call_emit;
  }

  AST_CLASS ast_class() { return AST_OP_CLASS; }
  int stack_size() { return 1; } // for ast_zero_size; it's a mystery why
  void emit(Script* script) { cerror(this,"OPs should never be emitted directly."); }
  void emit_call(AST_OP_CALL *ast,Script* script) { emit_fn(ast,script); }
};

struct AST_GOP : public AST {
  char *name;
  int   arity;
  int   is_nary;
  list<AST_OP*> *ops;
  
  AST_GOP(char *name, int arity, int is_nary, TYPE *type) {
    AST::name="GOP";
    this->name=name;
    this->arity=arity;
    this->is_nary=is_nary;
    this->ops = new list<AST_OP*>;
    this->type=type;
  }
  
  int stack_size() { return 1; } // for ast_zero_size; it's a mystery why
  AST_CLASS ast_class() { return AST_GOP_CLASS; }
  void emit(Script* script) { cerror(this,"GOP emit is undefined"); }
};

/**** AST_FUN ****/
#define MAX_OP_ARITY 2
struct AST_OP_CALL : public AST {
  AST_OP *op;
  list<AST*> *args;
  int     offsets[MAX_OP_ARITY];

  AST_OP_CALL(AST_OP *op, list<AST*> *args);

  void print();
  void emit(Script* script) { op->emit_call(this, script); }
  int stack_size();
  int env_size();
  AST_CLASS ast_class() { return AST_OP_CALL_CLASS; }

};

struct AST_FUN : public AST{
  const char       *name;
  FUN_TYPE   *fun_type;
  list<VAR*> *vars;
  Obj        *body;
  AST        *ast_body;

  AST_FUN(const char *name, list<VAR*> *vars, Obj *body, AST *ast_body);
  void print() {
    int i;
    fprintf(error_log(),"(FUN (");
    list<VAR*>::iterator it;
    for (i = 0, it = vars->begin(); it != vars->end(); it++, i++) {
            if (i != 0)
        fprintf(error_log()," ");
            fprintf(error_log(),"%s", (*it)->name);
    }
    fprintf(error_log(),") ");
    if (ast_body == NULL)
      fprintf(error_log(),"...");
    else
      ast_body->print();
    fprintf(error_log(),")");
  }
  
  void emit(Script* script); // turn program element into bytecodes
  AST_CLASS ast_class() { return AST_FUN_CLASS; }
};

TYPE* blur_type(TYPE* t) {
  switch (t->kind) {
  case ONE_NUM_KIND: return &num_type;
  case ONE_FUN_KIND: return (TYPE*)((((ONE_FUN_TYPE*)t)->value)->fun_type);
  default:           return t;
  }
}

extern AST_OP *tup_op, *fab_tup_op, *fab_vec_op, *fab_num_vec_op;
extern AST_OP *def_op, *def_num_vec_op, *def_vec_op, *def_tup_op;
extern AST_OP *def_num_vec_ops[];

extern AST* null_of(TYPE* t);

extern AST *new_ast_op_call_offset(AST_OP *op, list<AST*> *args, int offset);

AST* new_fab_vec (int len, TYPE* elt_type) {
  AST *ast;
  if (elt_type->kind == NUM_KIND) {
    ast = new_ast_op_call_offset(fab_num_vec_op, new list<AST*>(), len);
  } else {
    list<AST*> *lst = new list<AST*>;
    lst->push_back(null_of(elt_type));
    ast = new_ast_op_call_offset(fab_vec_op, lst, len);
  }
  return ast; 
}

AST* null_of(TYPE* t) {
  int i;
  switch (t->kind) {
  case NUM_KIND: 
  case ONE_NUM_KIND: 
    // post("NUM\n");
    return new AST_LIT(0);
  case VEC_KIND: {
    VEC_TYPE *vt = (VEC_TYPE*)t;
    // post("VEC %d\n", vt->len);
    return new_fab_vec(vt->len, vt->elt_type); }
  case TUP_KIND: {
    TUP_TYPE *tt = (TUP_TYPE*)t;
    list<AST*> *args = new list<AST*>;
    // post("TUP %d\n", tt->len);
    for (i = 0; i < tt->len; i++)
      args->push_back(null_of(tt->elt_types[i]));
    return new AST_OP_CALL(fab_tup_op, args);
  }
  default:
    uerror("unable to build null of type %s", t->name());
    return NULL;
  }
}

extern TYPE* tup_type_elt (TYPE*, int);
extern int   tup_type_len (TYPE*);

AST* real_null_of(TYPE* t) {
  int i;
  switch (t->kind) {
  case NUM_KIND: 
  case ONE_NUM_KIND: 
    // post("NUM\n");
    return new AST_LIT(0);
  case VEC_KIND: 
  case TUP_KIND: {
    list<AST*> *args = new list<AST*>;
    // post("VEC/TUP %d\n", tup_type_len(t));
    for (i = 0; i < tup_type_len(t); i++)
      args->push_back(real_null_of(tup_type_elt(t, i)));
    return new AST_OP_CALL(tup_op, args);
  }
  default:
    uerror("unable to build null of type %s", t->name());
    return NULL;
  }
}

int is_same_type (TYPE* t1_, TYPE* t2_) {
  TYPE* t1 = blur_type(t1_);
  TYPE* t2 = blur_type(t2_);
  return t1->subtype(t2) && t2->subtype(t1);
}

AST *ast_fun_walk (AST_WALKER_KIND action, AST *ast_, void *arg) {
  AST_FUN *ast = (AST_FUN*)ast_;
  if (ast->ast_body == NULL) 
    cerror(ast_, "ILLEGAL REF TO FUN NOT IN CALL POSITION");
  ast->ast_body = ast_walk(action, ast->ast_body, arg);
  return ast_;
}

int has_same_fun_bodies (AST *a1, AST *a2) {
  if (a1->ast_class() == AST_LIT_CLASS && a2->ast_class() == AST_LIT_CLASS) {
    return (((AST_LIT*)a1)->kind == ((AST_LIT*)a2)->kind &&
            ((AST_LIT*)a1)->val  == ((AST_LIT*)a2)->val); 
  } else
    return 0;
}

int maybe_lift_fun (AST_FUN *ast, LIFT_DATA *data) {
  int k, n = data->glos->size();
  list<AST*>::reverse_iterator it;
  // post("LIFTING %s\n", ast->name);
  for (k = 0, it = data->glos->rbegin();
       it != data->glos->rend();
       it++, k++) {
    AST_FUN *fun = (AST_FUN*)*it;
    if (fun->ast_class() == AST_FUN_CLASS) {
      if (fun == ast || has_same_fun_bodies(fun->ast_body, ast->ast_body)) {
        // post("FOUND OLD AST %d AT %d\n", fun == ast, n-k-1);
        return n-k-1;
      }
    }
  }
  data->glos->push_back(ast);
  return n;
}

list<VAR*> *augment_env (list<VAR*> *vars, list<VAR*> *env) {
  list<VAR*> *newenv = new list<VAR*>(*env);
  newenv->insert(newenv->begin(), vars->rbegin(), vars->rend());
  return newenv;
}

AST *ast_fun_lift_walk (AST_WALKER_KIND action, AST *ast_, void *arg) {
  AST_FUN *ast    = (AST_FUN*)ast_;
  LIFT_DATA *data = (LIFT_DATA*)arg;
  list<VAR*> *env       = data->env;
  data->env       = augment_env(ast->vars, data->env);
  ast->ast_body   = ast_walk(action, ast->ast_body, arg);
  data->env       = env;
  if (data->is_fun_lift)
    return new AST_GLO_REF(ast_, maybe_lift_fun(ast, data));
  else
    return ast_;
}

void emit_def_fun_op (int n, Script* script) {
  if(n > 1 && n <= MAX_DEF_FUN_OPS) script->add(DEF_FUN_2_OP+(n-2));
  else script->add_op(DEF_FUN_OP, n);
}

AST_FUN::AST_FUN(const char *name, list<VAR*> *vars, Obj *body, AST *ast_body) {
  AST::name="FUN"; walkers[0]=&ast_fun_lift_walk; walkers[1]=&ast_fun_walk;
  this->name = name; this->vars = vars;
  this->type     = new ONE_FUN_TYPE(this);
  this->fun_type = new FUN_TYPE(vars->size(), 0);
  this->body = body;
  this->ast_body = ast_body;
}

void AST_FUN::emit(Script* script) {
  Script body; ast_body->emit(&body);
  emit_def_fun_op(body.size()+1,script);
  script->append(&body);
  script->add(RET_OP);
}

/**** AST_LET ****/
struct AST_LET : public AST {
  list<VAR*> *vars;
  list<AST*> *inits;
  AST  *body;
  list<VAR*> *env;

  AST_LET(list<VAR*> *vars, list<AST*> *inits, AST *body, list<VAR*> *env);
  void print();
  void emit(Script* script);
  int stack_size();
  int env_size();
  AST_CLASS ast_class() { return AST_LET_CLASS; }
};

extern AST* parse (Obj *e, list<VAR*> *env);

AST *ast_let_lift_walk (AST_WALKER_KIND action, AST *ast_, void *arg) {
  AST_LET *ast    = (AST_LET*)ast_;
  LIFT_DATA *data = (LIFT_DATA*)arg;
  list<AST*> *inits = ast->inits;
  list<VAR*> *env   = data->env;
  for(list<AST*>::iterator it = inits->begin();
      it != inits->end(); it++)
    *it = ast_walk(action, *it, arg);
  data->env  = augment_env(ast->vars, data->env);
  ast->body  = ast_walk(action, ast->body, arg);
  data->env  = env;
  return ast_;
}

AST *ast_let_let_walk (AST_WALKER_KIND action, AST *ast_, void *arg) {
  AST_LET *ast = (AST_LET*)ast_;
  list<VAR*>::iterator vit;
  list<AST*>::iterator iit;

  ast->body  = ast_walk(action, ast->body, arg);
  for (vit = ast->vars->begin(), iit = ast->inits->begin();
       vit != ast->vars->end();) {
    if (!is_inlining_var(*vit) && !(is_optimizing_lets && (*vit)->n_refs == 0)) {
      *iit = ast_walk(action, *iit, arg);
      vit++; iit++;
    } else {
      vit = ast->vars->erase(vit);
      iit = ast->inits->erase(iit);
    }
  }
  if (ast->vars->empty()) {
    return ast->body;
  } else
    return ast_;
}

void AST_LET::print() {
  fprintf(error_log(),"(LET (");
  int i;
  list<VAR*>::iterator vit;
  list<AST*>::iterator iit;
  for(i=0, vit=vars->begin(), iit=inits->begin();
      vit != vars->end(); i++, vit++, iit++) {
    if (i != 0)
      fprintf(error_log(), " ");
    fprintf(error_log(), "(%s ", (*vit)->name);
    (*iit)->print();
    fprintf(error_log(), ")");
  }
  fprintf(error_log(),") ");
  body->print();
  fprintf(error_log(),")");
}

void emit_let_op (int n, Script* script) {
  if (n > 0) {
    if (n <= MAX_LET_OPS) script->add(LET_OP+n);
    else script->add(LET_OP,n);
  }
}

void emit_pop_let_op (int n, Script* script) {
  if (n > 0) {
    if (n <= MAX_LET_OPS) script->add(POP_LET_OP+n);
    else script->add(POP_LET_OP,n);
  }
}

void AST_LET::emit(Script* script) {
  int n=0;
  list<AST*>::iterator iit;
  for(iit=inits->begin(); iit != inits->end(); iit++) {
    (*iit)->emit(script);
    n += 1;
  }
  emit_let_op(n, script);
  body->emit(script);
  emit_pop_let_op(n, script);
}

inline int int_max (int x, int y) { return x < y ? y : x; }

void post_space (int depth) {
  int i;
  fprintf(error_log(),"[%2d] ", depth);
  for (i = 0; i < depth; i++)
    fprintf(error_log(),"  ");
}

int dep=0;
int AST_LET::stack_size() {
  int size = 0;

  for (list<AST*>::iterator it = inits->begin();
       it != inits->end(); it++) {
    size = int_max(size, (*it)->stack_size() - 1);
  }
  size = int_max(size, body->stack_size()) + int_max(1, inits->size());

  return size;
}

int AST_LET::env_size() {
  int size = 0;

  for (list<AST*>::iterator it = inits->begin();
       it != inits->end(); it++) {
    size = int_max(size, (*it)->env_size());
  }

  size = int_max(size, body->env_size() + inits->size());

  return size;
}

AST_LET::AST_LET(list<VAR*> *vars, list<AST*> *inits, AST *body, list<VAR*> *env) {
  name="LET";
  walkers[0]=&ast_let_lift_walk;
  walkers[1]=&ast_let_let_walk;
  this->vars=vars;
  this->inits=inits;
  this->body=body;
  this->env=env;
  type = body->type;
}

AST *new_ast_let (list<VAR*> *vars, list<AST*> *inits, AST *body, list<VAR*> *env) {
  return (vars->size() > 0) ? new AST_LET(vars,inits,body,env) : body;
}

/**** AST_OP_CALL ****/
// definition moved above for the benefit of AST_FUN

int add_offset (AST_OP_CALL *ast, int offset) {
  int i;
  for (i = 0; i < ast->op->arity; i++)
    if (ast->offsets[i] == -1)
      return ast->offsets[i] = offset;
  cerror((AST*)ast, "UNABLE TO ASSIGN OFFSET %d", offset);
  return 0; // scratch value - never reached due to error
}

AST *new_ast_op_call_offset(AST_OP *op, list<AST*> *args, int offset) {
  AST_OP_CALL *call = (AST_OP_CALL*)new AST_OP_CALL(op, args);
  add_offset(call, offset);
  call->type = op->type_infer((AST*)call);
  return (AST*)call;
}

AST *ast_op_call_walk (AST_WALKER_KIND action, AST *ast_, void *arg) {
  AST_OP_CALL *ast = (AST_OP_CALL*)ast_;

  for(list<AST*>::iterator it = ast->args->begin();
      it != ast->args->end(); it++) {
    *it = ast_walk(action, *it, arg);
  }
  return ast_;
}

AST *insert_def_tup (AST *tup_) {
  AST_OP_CALL *tup = (AST_OP_CALL*)tup_;
  int len = tup->offsets[0];
  if (tup->op == fab_num_vec_op) {
    if (len == 0)
      uerror("LEN == 0\n");
    if (len > 0 && len <= MAX_DEF_NUM_VEC_OPS)
      return new AST_OP_CALL(def_num_vec_ops[len], tup->args);
    else
      return new_ast_op_call_offset(def_num_vec_op, tup->args, len);
  } else if (tup->op == fab_tup_op) {
    return new AST_OP_CALL(def_tup_op, tup->args);
  } else if (tup->op == fab_vec_op)
    return new_ast_op_call_offset(def_vec_op, tup->args, len);
  else {
    cerror(tup_, "UNABLE TO FIND TUP DEF");
    return NULL;
  }
}

void map_lift (AST_OP_CALL *ast, LIFT_DATA *data) {
  // AST *vec_lit = new_ast_op_call(def_op, PAIR(null_of(ast->type), lisp_nil));
  AST *vec_lit = insert_def_tup(null_of(ast->type));
  data->glos->push_back(vec_lit);
  add_offset(ast, data->glos->size()-1);
}

void channel_lift (AST_OP_CALL *ast, LIFT_DATA *data) {
  add_offset(ast, data->n_channels);
  data->n_channels = data->n_channels + 1;
}

void tup_lift (AST_OP_CALL *ast, LIFT_DATA *data) {
  AST *fab_vec = new_fab_vec(tup_type_len(ast->type), NUMT);
  data->glos->push_back(insert_def_tup(fab_vec));
  add_offset(ast, data->glos->size()-1);
}

void init_feedback_lift (AST_OP_CALL *ast, LIFT_DATA *data) {
  add_offset(ast, data->n_state++);
}

extern AST_CLASS ast_op_call_class;

void fold_hood_lift (AST_OP_CALL *ast, LIFT_DATA *data) {
  list<AST*>::iterator it = ast->args->begin();
  it++; it++;
  AST* arg = *it;
  int size = arg->type->size();
  if (size <= 0)
    cerror(arg, "UNDERSPECIFIED TYPE SIZE IN EXPORT");
  data->export_len += size;
  add_offset(ast, data->n_exports++);
}

// FEEDBACK      - (ALL (UNLESS IS-EXEC? (SET INIT (INIT))) (SET IS-EXEC? T) OFF)
// SET-FEEDBACK  - (SET OFF (EXEC))

AST_OP_CALL *feedback_init(AST_OP_CALL *ast) {
  AST *arg = ast->args->front();
  if (arg->ast_class() == AST_REF_CLASS) {
    AST_REF* ref = (AST_REF*)arg;
    return (AST_OP_CALL*)ref->var->ast;
  } else if (arg->ast_class() == AST_OP_CALL_CLASS) {
    return (AST_OP_CALL*)arg;
  } else {
    cerror((AST*)ast, "UNKNOWN INIT-FEEDBACK FROM FEEDBACK");
    return NULL;
  }
}

void feedback_lift (AST_OP_CALL *ast, LIFT_DATA *data) {
  AST_OP_CALL *init = feedback_init(ast);
  add_offset(ast, init->offsets[0]);
  // post("LIFTING FEEDBACK %d\n", init->offsets[0]);
}

// feedback_op offset init_bytes
//  update_bytes

AST_OP *all_op;
AST_OP *hsv_op;
AST_OP *init_feedback_op;
AST_OP *feedback_op;
AST_OP *nbr_vec_op;
AST_OP *map_op;
AST_OP *channel_op;
AST_OP *vadd_op;
AST_OP *vsub_op;
AST_OP *vmul_op;
AST_OP *fold_hood_op;
AST_OP *vfold_hood_op;
AST_OP *fold_op;
AST_OP *vfold_op;
AST_OP *fold_hood_plus_op;
AST_OP *vfold_hood_plus_op;
AST_OP *apply_op;
AST_OP *nul_tup_op;
AST_OP *tup_op;
AST_OP *if_op;
AST_OP *mux_op, *vmux_op;
AST_OP *fab_tup_op;
AST_OP *fab_vec_op;
AST_OP *fab_num_vec_op;
AST_OP *def_num_vec_op;
AST_OP *def_num_vec_ops[MAX_DEF_NUM_VEC_OPS+1];
AST_OP *def_vec_op;
AST_OP *def_tup_op;
AST_OP *def_op;

AST *ast_op_call_lift_walk (AST_WALKER_KIND action, AST *ast_, void *arg) {
  AST_OP_CALL *ast = (AST_OP_CALL*)ast_;
  AST_OP *op = ast->op;
  LIFT_DATA *data = (LIFT_DATA*)arg;
  AST *res = ast_op_call_walk(action, ast_, arg);
  if (data->is_fun_lift) {
    if (op == fold_hood_op || op == vfold_hood_op ||
        op == fold_hood_plus_op || op == vfold_hood_plus_op)
      fold_hood_lift(ast, data);
  } else {
    if (op == map_op || op == vadd_op || op == vsub_op || op == vmul_op ||
        op == vfold_op || op == vfold_hood_op || op == vfold_hood_plus_op ||
        op == nbr_vec_op || op == hsv_op || op == vmux_op)
      map_lift(ast, data);
    else if (op == tup_op )
      tup_lift(ast, data);
    else if (op == feedback_op)
      feedback_lift(ast, data);
    else if (op == init_feedback_op)
      init_feedback_lift(ast, data);
    else if (op == channel_op)
      channel_lift(ast, data);
  }
  return res;
}

void ast_args_print (list<AST*> *args) {
  for (list<AST*>::iterator i = args->begin(); i != args->end(); i++) {
    fprintf(error_log()," ");
    (*i)->print();
  }
}

void AST_OP_CALL::print() {
  fprintf(error_log(),"(");
  fprintf(error_log(),"%s", op->name);
  ast_args_print(args);
  fprintf(error_log(),")");
}

void default_ast_op_call_emit(AST_OP_CALL* ast, Script* script) {
  for (list<AST*>::iterator it = ast->args->begin();
       it != ast->args->end(); it++)
    (*it)->emit(script);
  script->add(ast->op->code);
  if (ast->op->arity > MAX_OP_ARITY)
    uerror("ILLEGAL ARITY FOR OP %d > %d", ast->op->arity, MAX_OP_ARITY);
  for (int i = 0; i < ast->op->arity; i++)
    script->add(ast->offsets[i]);
  if (ast->op->is_nary)
    script->add(ast->args->size());
}

//                                   <- DONE-OFF ->
//                 <--- THEN OFF --->
// TST IF_OP T1 T2 ELSE JMP_OP D1 D2 THEN          NEXT
//                 IF_LEN            JMP_LEN
void ast_if_emit(AST_OP_CALL* ast, Script* script) {
  Script if_script;
  Script then_script;
  Script jmp_script;

  list<AST*>::iterator it = ast->args->begin();
  (*it++)->emit(&if_script);
  (*it++)->emit(&then_script);
  (*it++)->emit(&jmp_script);
  
  jmp_script.add_op(JMP_OP,then_script.size());
  
  script->append(&if_script);
  script->add_op(IF_OP,jmp_script.size());
  script->append(&jmp_script);
  script->append(&then_script);
}

int fun_stk_size (AST *rfun) {
  AST_FUN* fun = ((ONE_FUN_TYPE*)rfun->type)->value;
  FUN_TYPE* type = (FUN_TYPE*)fun->fun_type;
  return type->arity + fun->ast_body->stack_size();
}

int AST_OP_CALL::stack_size() {
  int size = 0, base_size = 0;

  if (op == map_op ||
      op == fold_hood_op || op == fold_op ||
      op == vfold_hood_op || op == vfold_op ||
      op == apply_op) {
    base_size = fun_stk_size(args->front());
  } else if ( op == feedback_op ) {
    base_size =  fun_stk_size(feedback_init(this)->args->front());
  } else if ( op == fold_hood_plus_op || op == vfold_hood_plus_op) {
    list<AST*>::iterator it = args->begin();
    base_size = int_max(fun_stk_size(*it++),
                        fun_stk_size(*it++));
  }
  for (list<AST*>::iterator it = args->begin();
       it != args->end(); it++ ) {
    size = int_max(size, (*it)->stack_size() - 1);
  }
  size = base_size + size + int_max(1, args->size());
  return size;
}

int fun_env_size (AST *rfun) {
  AST_FUN* fun = ((ONE_FUN_TYPE*)rfun->type)->value;
  FUN_TYPE* type = (FUN_TYPE*)fun->fun_type;
  return type->arity + fun->ast_body->env_size();
}

int AST_OP_CALL::env_size() {
  int size = 0, base_size = 0;
  if (op == map_op || 
      op == fold_hood_op || op == vfold_hood_op || 
      op == fold_op || op == vfold_op || op == apply_op) {
    base_size = fun_env_size(args->front());
  } else if ( op == feedback_op ) {
    base_size =  fun_stk_size(feedback_init(this)->args->front());
  } else if (op == fold_hood_plus_op || op == vfold_hood_plus_op) {
    list<AST*>::iterator it = args->begin();
    base_size = int_max(fun_env_size(*it++),
                        fun_env_size(*it++));
  }
  for (list<AST*>::iterator it = args->begin();
       it != args->end(); it++ ) {
    size = int_max(size, (*it)->env_size());
  }

  size = base_size + size;

  return size;
}

AST_OP_CALL::AST_OP_CALL(AST_OP *op, list<AST*> *argv) : args(argv) {
  name="OP_CALL";
  walkers[0]=&ast_op_call_lift_walk; walkers[1]=&ast_op_call_walk;
  this->op=op;
  type = op->type_infer(this);
  for(int i = 0; i < op->arity; i++) offsets[i] = -1;
}

/**** AST_CALL ****/
struct AST_CALL : public AST {
  AST_FUN  *fun;
  list<AST*> *args;

  AST_CALL(AST *fun, list<AST*> *args);

  void print() {
    fprintf(error_log(),"(");
    fun->print();
    ast_args_print(args);
    fprintf(error_log(),")");
  }
  void emit(Script* script) { cerror(this,"ILLEGAL CALL IN RUNTIME"); }
  int stack_size() { return 1; } // for ast_zero_size; it's a mystery why
  AST_CLASS ast_class() { return AST_CALL_CLASS; }
};

AST *ast_call_walk (AST_WALKER_KIND action, AST *ast_, void *arg) {
  AST_CALL *ast = (AST_CALL*)ast_;
  list<AST*> *new_args = new list<AST*>;
  for (list<AST*>::iterator it = ast->args->begin();
       it != ast->args->end(); it++) {
    new_args->push_back(ast_walk(action, *it, arg));
  }
  ast->args = new_args;
  ast->fun  = (AST_FUN*)ast_walk(action, ast->fun, arg);
  return ast_;
}

AST_CALL::AST_CALL(AST *fun, list<AST*> *argv) : args(argv) {
  name="CALL";
  walkers[0]=walkers[1]=&ast_call_walk;
  if(!(fun->ast_class() == AST_FUN_CLASS)) cerror(this,"CALL OF NON-FUNCTION");
  this->fun=(AST_FUN*)fun; 
  type = this->fun->ast_body->type;
}

/****** OPERATION DEFINITION ******/
list<VAR*> *ops;
list<VAR*> *user_ops;
string op_names[256];

extern VAR* lookup_name (const char *name, list<VAR*> *bindings);

AST_OP *add_op_full
    (const char *name, const char *opname, OPCODE code, int arity, int is_nary, 
     FUN_TYPE *fun_type, TYPE_INFER type_infer) {
  AST_OP *op = (AST_OP*)new AST_OP(name, opname, code, arity, is_nary, (TYPE*)fun_type, type_infer);
  VAR *var = lookup_name(name, ops);
  op_names[code] = name; // fill in op_names record of ops
  if (var == NULL)
    ops->push_front(new VAR(name, new ONE_OP_TYPE(op)));
  else {
    // post("FOUND OLD OP %s\n", name);
    switch (var->type->kind) {
    case ONE_OP_KIND: {
      AST_GOP *gop = (AST_GOP*)new AST_GOP((char*)name, arity, is_nary, (TYPE*)fun_type);
      AST_OP *oop  = ((ONE_OP_TYPE*)var->type)->op;
      TYPE *type   = new ONE_GOP_TYPE(gop);
      // post("ADDING OP TO NEW GOP\n");
      gop->ops  = new list<AST_OP*>;
      gop->ops->push_back(op);
      gop->ops->push_back(oop);
      var->type = type;
      break; }
    case ONE_GOP_KIND: {
      AST_GOP *gop = ((ONE_GOP_TYPE*)var->type)->gop;
      // post("ADDING OP TO EXISTING GOP\n");
      gop->ops->push_back(op);
      break; }
    default:
      uerror("ADDING OP TO NON OP VAR %s\n", name);
    }
  }
  // post("ADDING %s %d\n", name, code);
  return op;
}

extern TYPE* op_type_infer (AST* ast_);

AST_OP *add_op_named
    (const char *name, const char *opname, OPCODE code, int arity, int is_nary, 
     FUN_TYPE *fun_type) {
  return add_op_full
    (name, opname, code, arity, is_nary, fun_type, &op_type_infer);
}

AST_OP *add_op_typed
    (const char *name, OPCODE code, int arity, int is_nary, 
     FUN_TYPE *fun_type, TYPE_INFER type_infer) {
  return add_op_full(name, name, code, arity, is_nary, fun_type, type_infer);
}

AST_OP *add_op_named_typed
    (const char *name, const char *opname, OPCODE code, int arity, int is_nary, 
     FUN_TYPE *fun_type, TYPE_INFER type_infer) {
  return add_op_full(name, opname, code, arity, is_nary, fun_type, type_infer);
}

AST_OP *add_op
    (const char *name, OPCODE code, int arity, int is_nary, FUN_TYPE *fun_type) {
  return add_op_full
    (name, name, code, arity, is_nary, fun_type, &op_type_infer);
}

AST_OP *def_op_alias (const char *name, AST_OP *op) {
  ops->push_front(new VAR(name, new ONE_OP_TYPE(op)));
  return op;
}

char *cpy_str (const char *s) {
  char *r = (char*)MALLOC(strlen(s)+1);
  strcpy(r, s);
  return r;
}

int tup_type_len (TYPE* type) {
  switch (type->kind) {
  case TUP_KIND: return ((TUP_TYPE*)type)->len;
  case VEC_KIND: return ((VEC_TYPE*)type)->len;
  default: 
    uerror("EXPECTED TUP/VEC TYPE %s\n", type->name());
    return 0; // scratch value - never reached due to error
  }
}

TYPE* tup_type_elt (TYPE* type, int i) {
  switch (type->kind) {
  case TUP_KIND: return ((TUP_TYPE*)type)->elt_types[i];
  case VEC_KIND: return ((VEC_TYPE*)type)->elt_type;
  default:
    uerror("EXPECTED TUP/VEC TYPE %s\n", type->name());
    return NULL;
  }
}

TYPE* op_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  FUN_TYPE *type = (FUN_TYPE*)(ast->op)->type;
  return type->result_type;
}

TYPE* tup_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  // Are all the nodes the same?  If so, is VEC; otherwise is TUP
  bool is_same = true;
  int i = 0;
  int len = ast->args->size();
  if(len==0)
    return new TUP_TYPE(0,NULL);
  TYPE* types[len];
  for(list<AST*>::iterator it = ast->args->begin();
      it != ast->args->end(); it++, i++) {
    types[i]=blur_type((*it)->type);
    is_same &= is_same_type(types[i],types[0]);
  }
  if(is_same)
    return new VEC_TYPE(len,types[0]);
  else
    return new TUP_TYPE(len,types);
}

TYPE* vec_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  TYPE* elt  = ast->args->front()->type;
  int k      = ast->offsets[0];
  return new VEC_TYPE(k, elt);
}

TYPE* num_vec_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  int k      = ast->offsets[0];
  return new VEC_TYPE(k, &num_type);
}

TYPE* elt_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  if(ast->args->size()<2)
    cerror(ast_,"ELT CANNOT INFER TYPE WHEN NOT ENOUGH ARGUMENTS");
  list<AST*>::iterator it = ast->args->begin();
  TYPE* elts = (*it++)->type;
  TYPE* idx  = (*it++)->type;
  if (elts->kind == VEC_KIND)
    return ((VEC_TYPE*)elts)->elt_type;
  else if (idx->kind == ONE_NUM_KIND) {
    if (elts->kind == TUP_KIND) {
      TYPE *res = ((TUP_TYPE*)elts)->elt_types[(int)((ONE_NUM_TYPE*)idx)->num];
      // post("ELT TYPE %s\n", type_name(res));
      return res;
    } else {
      cerror(ast_, "ELT: TYPE ERROR EXPECTED VEC/TUP GOT %s", elts->name());
      return NULL;
    }
  } else {
    cerror(ast_, "ELT: UNABLE TO TYPE ELT %s %s", idx->name(), elts->name());
    return NULL;
  }
}

TYPE* map_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  ONE_FUN_TYPE* fun = (ONE_FUN_TYPE*)ast->args->front()->type;
  return fun->value->ast_body->type;
}

TYPE* fold_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  ONE_FUN_TYPE* fun = (ONE_FUN_TYPE*)ast->args->front()->type;
  return fun->value->ast_body->type;
}

TYPE* fold_hood_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  ONE_FUN_TYPE* fun = (ONE_FUN_TYPE*)ast->args->front()->type;
  return fun->value->ast_body->type;
}

TYPE* fold_hood_plus_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  list<AST*>::iterator it = ast->args->begin();
  it++;
  ONE_FUN_TYPE* fun = (ONE_FUN_TYPE*)(*it)->type;
  return fun->value->ast_body->type;
}

TYPE* apply_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  ONE_FUN_TYPE* fun = (ONE_FUN_TYPE*)ast->args->front()->type;
  return fun->value->ast_body->type;
}

TYPE* mux_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  list<AST*>::iterator it = ast->args->begin();
  it++; // skip the condition
  AST* con = (AST*)(*it++);
  return blur_type(con->type);
}

TYPE* vadd_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  list<AST*>::iterator it = ast->args->begin();
  TYPE* t1 = (*it++)->type;
  TYPE* t2 = (*it++)->type;
  return (tup_type_len(t1) > tup_type_len(t2)) ? t1 : t2;
}

TYPE* vmul_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  list<AST*>::iterator it = ast->args->begin();
  it++;
  return (*it)->type;
}

TYPE* probe_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  AST* val = ast->args->front();
  return val->type;
}

TYPE* init_feedback_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  AST_FUN* fun = (AST_FUN*)ast->args->front();
  return fun->ast_body->type;
}

TYPE* all_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  int len = ast->args->size();
  if (len > 0)
    return ast->args->back()->type;
  else
    return &any_type;
}

TYPE* feedback_type_infer (AST* ast_) {
  AST_OP_CALL* ast = (AST_OP_CALL*)ast_;
  AST* init = ast->args->front();
  return init->type;
}

void init_ops () {
  int i;
  AST_OP *op;
  char name[100];
  ops = new list<VAR*>();
  fold_hood_plus_op =
    add_op_typed("FOLD-HOOD-PLUS", FOLD_HOOD_PLUS_OP, 1, 0, 
                 new FUN_TYPE(ANYT,ANYT,ANYT,ANYT,0), 
                 &fold_hood_plus_type_infer);
  vfold_hood_plus_op =
    add_op_typed("VFOLD-HOOD-PLUS", VFOLD_HOOD_PLUS_OP, 2, 0, 
                 new FUN_TYPE(ANYT,ANYT,ANYT,ANYT,0), 
                 &fold_hood_plus_type_infer);
  fold_hood_op =
    add_op_typed("FOLD-HOOD", FOLD_HOOD_OP, 1, 0, 
                 new FUN_TYPE(ANYT,ANYT,ANYT,ANYT,0), &fold_hood_type_infer);
  vfold_hood_op =
    add_op_typed("VFOLD-HOOD", VFOLD_HOOD_OP, 2, 0, 
                 new FUN_TYPE(ANYT,ANYT,ANYT,ANYT,0), &fold_hood_type_infer);
  add_op_typed("ELT", ELT_OP, 0, 0, 
               new FUN_TYPE(ANYT,ANYT,NUMT,0), &elt_type_infer);
  def_op_alias("MIX", fold_hood_op);
  add_op("DT", DT_OP, 0, 0, new FUN_TYPE(NUMT,0));
  op = add_op("MOV", MOV_OP, 0, 0, new FUN_TYPE(VEC3T,VEC3T,0));
  add_op("SPEED", SPEED_OP, 0, 0, new FUN_TYPE(NUMT,0));
  add_op("BEARING", BEARING_OP, 0, 0, new FUN_TYPE(NUMT,0));
  add_op("SET-DT", SET_DT_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op_typed("PROBE", PROBE_OP, 0, 0, 
               new FUN_TYPE(ANYT,ANYT,NUMT,0),&probe_type_infer);
  add_op("FLEX", FLEX_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));

  add_op("NOT", NOT_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("RND", RND_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op_named("INF", "INF", INF_OP, 0, 0, new FUN_TYPE(NUMT,0));
  
  //Mathematical Comparison Operators
  add_op_named("<", "LT", LT_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op_named("<=", "LTE", LTE_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op_named(">", "GT", GT_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op_named(">=", "GTE", GTE_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op_named("=", "EQ", EQ_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  
  //Mathematical Operators
  op = add_op_named("B+", "ADD", ADD_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  def_op_alias("+", op);
  add_op_named("-", "SUB", SUB_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  op = add_op_named("B*", "MUL", MUL_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  def_op_alias("*", op);
  add_op_named("/", "DIV", DIV_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  
  //Math Functions
  add_op("FLOOR", FLOOR_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("CEIL", CEIL_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));  
  add_op("ROUND", ROUND_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("ABS", ABS_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("MAX", MAX_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op("MIN", MIN_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op("MOD", MOD_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op("REM", REM_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op("POW", POW_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op("SQRT", SQRT_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("LOG", LOG_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("SIN", SIN_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("COS", COS_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("TAN", TAN_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("ASIN", ASIN_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("ACOS", ACOS_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("ATAN2", ATAN2_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,NUMT,0));
  add_op("SINH", SINH_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("COSH", COSH_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  add_op("TANH", TANH_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));
  
  all_op = 
    add_op_typed("ALL", ALL_OP, 0, 1, 
                 new FUN_TYPE(ANYT,ANYT,1), &all_type_infer);
  def_op_alias("PAR", all_op);

  //Vector Operations
  vadd_op =
    add_op_named_typed("B+", "VADD", VADD_OP, 1, 0, 
                 new FUN_TYPE(VECT,VECT,VECT,0), &vadd_type_infer);
  def_op_alias("VADD", vadd_op);

  vsub_op = 
    add_op_named_typed("-", "VSUB", VSUB_OP, 1, 0, 
                 new FUN_TYPE(VECT,VECT,VECT,0), &vadd_type_infer);
  def_op_alias("VSUB", vsub_op);

  vmul_op =
    add_op_named_typed("B*", "VMUL", VMUL_OP, 1, 0, 
                 new FUN_TYPE(VECT,NUMT,VECT,0), &vmul_type_infer);
  def_op_alias("VMUL", vmul_op);

  add_op_named("LEN", "LEN", LEN_OP, 0, 0,new FUN_TYPE(NUMT,TUPT,0));

  //BUG: SLICE NEEDS A TYPE INFERENCE STYLE THAT WORKS.	
  add_op_named_typed("SLICE", "SLICE", VSLICE_OP, 0, 0,
	      new FUN_TYPE(VECT,NUMT,NUMT,VECT,0), &elt_type_infer);
	

  
  //Vector Comparison Operations
  add_op_named_typed("<", "VLT", VLT_OP, 0, 0, 
               new FUN_TYPE(VECT,VECT,VECT,0), &vadd_type_infer);
  add_op_named_typed(">", "VGT", VGT_OP, 0, 0, 
               new FUN_TYPE(VECT,VECT,VECT,0), &vadd_type_infer);
  add_op_named_typed("<=", "VLTE", VLTE_OP, 0, 0, 
               new FUN_TYPE(VECT,VECT,VECT,0), &vadd_type_infer);
  add_op_named_typed(">=", "VGTE", VGTE_OP, 0, 0, 
               new FUN_TYPE(VECT,VECT,VECT,0), &vadd_type_infer);
  add_op_named_typed("=", "VEQ", VEQ_OP, 0, 0, 
               new FUN_TYPE(VECT,VECT,VECT,0), &vadd_type_infer);
  add_op_named_typed("MIN", "VMIN", VMIN_OP, 0, 0, 
               new FUN_TYPE(VECT,VECT,VECT,0), &vadd_type_infer);
  add_op_named_typed("MAX", "VMAX", VMAX_OP, 0, 0, 
               new FUN_TYPE(VECT,VECT,VECT,0), &vadd_type_infer);
               
  //Tuple Operations
  nul_tup_op =
    add_op_typed("NUL-TUP", NUL_TUP_OP, 0, 0, 
                 new FUN_TYPE(ANYT,0), &tup_type_infer);
  tup_op =
    add_op_typed("TUP", TUP_OP, 1, 1, 
                 new FUN_TYPE(ANYT,ANYT,1), &tup_type_infer);
  map_op =
    add_op_typed("MAP", MAP_OP, 1, 0, 
                 new FUN_TYPE(ANYT,ANYT,ANYT,0), &map_type_infer);
  apply_op =
    add_op_typed("APPLY", APPLY_OP, 0, 0, 
                 new FUN_TYPE(ANYT,ANYT,ANYT,0), &apply_type_infer);
  fold_op =
    add_op_typed("FOLD", FOLD_OP, 0, 0, 
                 new FUN_TYPE(ANYT,ANYT,ANYT,ANYT,0), &fold_type_infer);
  vfold_op =
    add_op_typed("VFOLD", VFOLD_OP, 1, 0, 
                 new FUN_TYPE(ANYT,ANYT,ANYT,ANYT,0), &fold_type_infer);
  nbr_vec_op =
    add_op("NBR-VEC", NBR_VEC_OP, 1, 0, new FUN_TYPE(VEC3T,0));
  add_op("VDOT", VDOT_OP, 0, 0, new FUN_TYPE(NUMT,VECT,VECT,0));
  mux_op =
    add_op_typed("MUX", MUX_OP, 0, 0, 
                 new FUN_TYPE(ANYT,NUMT,ANYT,ANYT,0), &mux_type_infer);
  vmux_op =
    add_op_typed("VMUX", VMUX_OP, 1, 0, 
                 new FUN_TYPE(ANYT,NUMT,ANYT,ANYT,0), &mux_type_infer);
  if_op = 
    add_op_typed("IF", IF_OP, -1, 0, 
                 new FUN_TYPE(ANYT,NUMT,ANYT,ANYT,0), &mux_type_infer);
  def_op_alias("WHERE", if_op);
  if_op->emit_fn = &ast_if_emit;
  add_op("DENSITY", DENSITY_OP, 0, 0, new FUN_TYPE(NUMT,0));
  add_op("INFINITESIMAL", INFINITESIMAL_OP, 0, 0, new FUN_TYPE(NUMT,0));
  add_op("NBR-RANGE", NBR_RANGE_OP, 0, 0, new FUN_TYPE(NUMT,0));
  def_op_alias("NBR-ANGLE", add_op("NBR-BEARING", NBR_BEARING_OP, 0, 0, new FUN_TYPE(NUMT,0)));
  op = add_op("HOOD-RADIUS", HOOD_RADIUS_OP, 0, 0, new FUN_TYPE(NUMT,0));
  def_op_alias("RADIO-RANGE", op);
  add_op("AREA", AREA_OP, 0, 0, new FUN_TYPE(NUMT,0));
  add_op("NBR-LAG", NBR_LAG_OP, 0, 0, new FUN_TYPE(NUMT,0));

  add_op("MID", MID_OP, 0, 0, new FUN_TYPE(NUMT,0));

  // New experimental field operations
  add_op("NBR-IDS", NBR_IDS_OP, 0, 0, new FUN_TYPE(NUMT,0));
  add_op("NEW-MIN-HOOD", MIN_HOOD_OP, 0, 0, new FUN_TYPE(NUMT,NUMT,0));

  user_ops = ops;

  // LOW LEVEL

  add_op("JMP", JMP_OP, -1, 0, new FUN_TYPE(ANYT,0));
  add_op("RET", RET_OP, 0, 0, new FUN_TYPE(ANYT,0));
  add_op("EXIT", EXIT_OP, 0, 0, new FUN_TYPE(ANYT,0));
  add_op("LIT", LIT_OP, -1, 0, new FUN_TYPE(NUMT,0));
  add_op("LIT-FLO", LIT_FLO_OP, 4, 0, new FUN_TYPE(NUMT,0));
  fab_tup_op
    = add_op_typed("FAB-TUP", FAB_TUP_OP, 0, 1, new FUN_TYPE(VECT,ANYT,0), &tup_type_infer);
  fab_vec_op
    = add_op_typed("FAB-VEC", FAB_VEC_OP, 1, 0, new FUN_TYPE(VECT,ANYT,0), &vec_type_infer);
  def_tup_op
    = add_op("DEF-TUP", DEF_TUP_OP, 0, 1, new FUN_TYPE(VECT,ANYT,0));
  def_vec_op
    = add_op("DEF-VEC", DEF_VEC_OP, 1, 0, new FUN_TYPE(VECT,ANYT,0));
  fab_num_vec_op
    = add_op_typed("FAB-NUM-VEC", FAB_NUM_VEC_OP, 1, 0, new FUN_TYPE(VECT,0), &num_vec_type_infer);
  def_num_vec_op 
    = add_op("DEF-NUM-VEC", DEF_NUM_VEC_OP, 1, 0, new FUN_TYPE(VECT,0));
  for (i = 1; i <= MAX_DEF_NUM_VEC_OPS; i++) {
    sprintf(name, "DEF-NUM-VEC-%d", i);
    def_num_vec_ops[i] = add_op(cpy_str(name), (OPCODE)(DEF_NUM_VEC_OP+i), 0, 0, new FUN_TYPE(VECT,0));
  }
  def_op 
    = add_op("DEF",     DEF_OP, 0, 0, new FUN_TYPE(ANYT,ANYT,0));
  for (i = 0; i < MAX_LIT_OPS; i++) {
    sprintf(name, "LIT-%d", i);
    add_op(cpy_str(name), (OPCODE)(LIT_0_OP+i), 0, 0, new FUN_TYPE(NUMT,0));
  }
  add_op("REF", REF_OP, -1, 0, new FUN_TYPE(NUMT,0));
  for (i = 0; i < MAX_REF_OPS; i++) {
    sprintf(name, "REF-%d", i);
    add_op(cpy_str(name), (OPCODE)(REF_OP+i+1), 0, 0, new FUN_TYPE(NUMT,0));
  }
  add_op("DEF-VM", DEF_VM_OP, 8, 0, new FUN_TYPE(NUMT,0));
  add_op("GLO-REF", GLO_REF_OP, -1, 0, new FUN_TYPE(NUMT,0));
  for (i = 0; i < MAX_GLO_REF_OPS; i++) {
    sprintf(name, "GLO-REF-%d", i);
    add_op(cpy_str(name), (OPCODE)(GLO_REF_OP+i+1), 0, 0, new FUN_TYPE(NUMT,0));
  }
  add_op("LET", LET_OP, -1, 0, new FUN_TYPE(NUMT,0));
  add_op("POP-LET", POP_LET_OP, -1, 0, new FUN_TYPE(NUMT,0));
  for (i = 1; i <= MAX_LET_OPS; i++) {
    sprintf(name, "LET-%d", i);
    add_op(cpy_str(name), (OPCODE)(LET_OP+i), 0, 0, new FUN_TYPE(NUMT,0));
    sprintf(name, "POP-LET-%d", i);
    add_op(cpy_str(name), (OPCODE)(POP_LET_OP+i), 0, 0, new FUN_TYPE(NUMT,0));
  }
  for (i = 0; i < MAX_DEF_FUN_OPS; i++) {
    sprintf(name, "DEF-FUN-%d", i+2);
    add_op(cpy_str(name), (OPCODE)(DEF_FUN_2_OP+i), 0, 0, new FUN_TYPE(NUMT,0));
  }
  add_op("DEF-FUN", DEF_FUN_OP, -1, 0, new FUN_TYPE(NUMT,0));
  init_feedback_op =
    add_op_typed("INIT-FEEDBACK", INIT_FEEDBACK_OP, 1, 0, 
                 new FUN_TYPE(ANYT,ANYT,0), &init_feedback_type_infer);
  feedback_op =
    add_op_typed("FEEDBACK", FEEDBACK_OP, 1, 0, 
                 new FUN_TYPE(ANYT,ANYT,ANYT,0), &feedback_type_infer);
};

/// PARSING

char* sym_elt (List *e, int offset) {
  Obj *n = lst_elt((List*)e, offset);
  if (symbolp(n))
    return strdup(((Symbol*)n)->getName().c_str());
  else {
    uerror("LOOKING FOR STRING FOUND OTHER TYPE %s", n->typeName());
    return NULL;
  }
}

list<VAR*> *globals;

struct ltstr {
  bool operator()(const char *s1, const char *s2) const {
    return strcmp(s1, s2) < 0;
  }
};

static map<const char*, int, ltstr> obarray;
static int obval = 0;

int intern_symbol (const char *name) {
  map<const char*, int, ltstr>::iterator it = obarray.find(name);
  if (it != obarray.end()) {
    return it->second;
  } else {
    return (obarray[name] = obval++);
  }
}

VAR* lookup_name (const char *name, list<VAR*> *bind) {
  for (list<VAR*>::iterator it = bind->begin();
       it != bind->end(); it++) {
    if (strcasecmp((*it)->name, name) == 0) {
      (*it)->n_refs++;
      return (*it);
    }
  }
  return NULL;
}

VAR* lookup_op (const char *name) {
  return lookup_name(name, ops);
}

extern "C" AST_OP* lookup_op_by_code (int code, char **name) {
  for (list<VAR*>::iterator it = ops->begin();
       it !=  ops->end(); it++) {
    VAR *var = *it;
    if (var->type->kind == ONE_GOP_KIND) {
      ONE_GOP_TYPE *gop_type = (ONE_GOP_TYPE*)var->type;
      list<AST_OP*> *gop_ops = gop_type->gop->ops;
      for (list<AST_OP*>::iterator it2 = gop_ops->begin();
           it2 != gop_ops->end(); it2++) {
        AST_OP *o = *it2;
        if (o->code == code) {
          *name = (char*)o->opname;
          return o;
        }
      }
    } else {
      ONE_OP_TYPE *type = (ONE_OP_TYPE*)var->type;
      AST_OP *o = type->op;
      if (o->code == code) {
        *name = (char*)o->opname;
        return o;
      }
    }
  }
  return NULL;
}

void load_def (const char *name) {
  List *expr;
  // LOOK IT UP IN FILE

  string filename = name; filename+=".proto";
  expr = read_objects_from_dirs(filename, proto_path);
  if (expr == NULL)
    return;
  else {
    Obj *body = PAIR(new Symbol("all"), expr);
    parse(body, new list<VAR*>());
  }
}

VAR* lookup (const char *name, list<VAR*> *stack) {
  // First, see if it's a local variable
  VAR *res = lookup_name(name, stack);
  if (res != NULL)
    return res;
  // Second, see if it's a global
  res = lookup_name(name, globals);
  if (res != NULL)
    return res;
  // next, check if it's a built-in op
  res = lookup_op(name);
  if (res != NULL)
    return res;
  // finally, search for it in files
  load_def(name);
  return lookup_name(name, globals);
}

map<string, AST*> fun_ops;

AST* parse_op_ref (char *op_name, FUN_TYPE *type, list<VAR*> *env) {
  int i;
  AST *res;
  string opname(op_name);
  Obj *fun_sym   = new Symbol("FUN");
  List *args     = lisp_nil;
  int  n         = type->arity;

  map<string, AST*>::iterator it = fun_ops.find(opname);
  
  if(it != fun_ops.end())
    return it->second;
  
  for (i = n-1; i >= 0; i--) {
    char name[10];
    sprintf(name, "a%d", i);
    args = PAIR(new Symbol(name), args);
  }
  res = parse(_list(fun_sym, args, PAIR(new Symbol(op_name), args), NULL), env);

  fun_ops[opname] = res;
  return res;
}

AST* parse_reference (const char *name, list<VAR*> *env) {
  VAR *var = lookup(name, env);
  // post("REF %s\n", name);
  if (var == NULL) 
    return NULL;
  TYPE *type = var->type;
  switch (type->kind) {
  case ONE_OP_KIND: {
    AST_OP *op = ((ONE_OP_TYPE*)type)->op;
    return parse_op_ref((char*)op->name, (FUN_TYPE*)(op->type), env); }
  case ONE_GOP_KIND: {
    AST_GOP *gop = ((ONE_GOP_TYPE*)type)->gop;
    return parse_op_ref(gop->name, (FUN_TYPE*)(gop->type), env); }
  case ONE_FUN_KIND:
    return (AST*)(((ONE_FUN_TYPE*)type)->value);
  default: 
    return new AST_REF(var, env);
  }
}

AST* parse_symbol (const char *name) {
  int i;
  // post("LOOKING UP %s\n", name);
  i = intern_symbol(name);
  // post("FOUND %d\n", i);
  return new AST_LIT((flo)i);
}

Obj *rewrite_cases (Obj *var, List *cases) {
  Obj *if_sym = new Symbol("IF");
  Obj *eq_sym = new Symbol("=");
  Obj *qt_sym = new Symbol("QUOTE");
  if (cases == lisp_nil)
    return new Number(0);
  else {
    List *clause = (List*)lst_head(cases);
    Obj  *key    = PAIR(qt_sym, PAIR(lst_elt(clause, 0), lisp_nil));
    Obj  *val    = lst_elt(clause, 1);
    return _list(if_sym,
                 _list(eq_sym, key, var, NULL), 
                 val,
                 rewrite_cases(var, lst_tail(cases)),
                 NULL);
  }
}

Obj *cat_sym2 (char *s1, char *s2) {
  char str[100];  sprintf(str, "%s%s", s1, s2);
  return new Symbol(str);
}

Obj *cat_sym3 (char *s1, char *s2, char *s3) {
  char str[100]; sprintf(str, "%s%s%s", s1, s2, s3);
  return new Symbol(str);
}

Obj *build_getters (char *name, int off, List *fields) {
  Obj *def_sym = new Symbol("DEF");
  Obj *elt_sym = new Symbol("ELT");
  Obj *var_sym = new Symbol("VAR");
  Obj *off_num = new Number(off);
  if (fields == lisp_nil)
    return lisp_nil;
  else {
    const std::string &field_name = ((Symbol*)lst_head(fields))->getName();
    Symbol *getter   = new Symbol(name + ("-" + field_name));
    // post("BUILDING GETTER %s %s %d\n", field_name, getter->name, off);
    return PAIR(_list(def_sym, getter, _list(var_sym, NULL),
                      _list(elt_sym, var_sym, off_num, NULL), NULL),
                build_getters(name, off + 1, lst_tail(fields)));
  }
}

Obj *rewrite_letstar (List *bindings, List *body) {
  if (bindings == lisp_nil)
    return PAIR(new Symbol("all"),body);
  else {
    return PAIR(new Symbol("let"),
                PAIR(PAIR(bindings->getHead(), lisp_nil),
                     PAIR(rewrite_letstar((List*)(bindings->getTail()), body), lisp_nil)));
  }
}

Obj *rewrite_nary (Obj *fun, List *args) {
  switch (lst_len(args)) {
  case 0: case 1:
    uerror("NARY WITH FEWER THAN TWO INPUTS");
  case 2:
    return _list(fun, lst_elt(args, 0), lst_elt(args, 1), NULL);
  default:
    return _list(fun, lst_head(args), rewrite_nary(fun, lst_tail(args)), NULL);
  }
}

list<VAR*> *params_to_vars (List *params) {
  list<VAR*> *vars = new list<VAR*>;
  int  i, n  = lst_len(params);
  for (i = 0; i < n; i++) {
    char *name = sym_elt(params, i);
    vars->push_back(new VAR(name, &any_type));
  }
  return vars;
}

list<AST*> *parse_args (List* args, list<VAR*> *env) {
  int i;
  list<AST*> *ast_args = new list<AST*>;
  for (i = 0; i < lst_len(args); i++)
    ast_args->push_back(parse(lst_elt(args, i), env));
  return ast_args;
}

AST *parse_fun_body (list<VAR*> *vars, Obj *body, list<VAR*> *env) {
  return parse(body, augment_env(vars, env));
}

void maybe_decr_n_refs (AST *ast) {
  if (ast->ast_class() == AST_REF_CLASS) {
    AST_REF *ref = (AST_REF*)ast;
    ref->var->n_refs -= 1;
  }
}

Obj* read_select (int i, Obj *idx, Obj *first, List *args) {
  if (lst_len(args) == 0) {
    return read_qq("(null $x)", qq_env("$x", first, NULL));
  } else {
    List *env
      = qq_env("$i", new Number(i), 
               "$o", idx,
               "$x", lst_elt(args, 0), 
               "$r", read_select(i + 1, idx, first, lst_tail(args)),
               NULL);
    return read_qq("(if (= $o $i) $x $r)", env);
  }
}


int n_nbr_refs(Obj *expr) {
  if (expr != lisp_nil && listp(expr)) {
    Obj *fun = lst_elt((List*)expr, 0);
    if (symbolp(fun) && sym_name(fun) == "nbr")
      return 1;
    else 
      return n_nbr_refs(fun) + n_nbr_refs(lst_tail((List*)expr));
  } else 
    return 0;
}

Obj* do_rewrite_fold_hood_star(Obj *expr, Obj **nexprs, int *i, int n) {
  if (expr != lisp_nil && listp(expr)) {
    Obj *fun = lst_elt((List*)expr, 0);
    if (symbolp(fun) && strcasecmp(sym_name(fun).c_str(), "nbr") == 0) {
      int off = *i;
      Obj *nexpr = lst_elt((List*)expr, 1);
      *i += 1;
      *nexprs = PAIR(nexpr, *nexprs);
      if (n == 1)
        return read_qq("e", lisp_nil);
      else
        return read_qq("(elt t $i)", qq_env("$i", new Number(off), NULL));
    } else {
      Obj *head = do_rewrite_fold_hood_star(fun, nexprs, i, n);
      Obj *tail = do_rewrite_fold_hood_star(lst_tail((List*)expr), nexprs, i, n);
      return PAIR(head, tail);
    }
  } else 
    return expr;
}

Obj* rewrite_fold_hood_star(Obj *expr, Obj **nexprs, int *n) {
  int counter = 0;
  *n = n_nbr_refs(expr);
  Obj *form = do_rewrite_fold_hood_star(expr, nexprs, &counter, *n);
  *nexprs = lst_rev((List*)*nexprs);
  return form;
}

Obj* hood_folder (const char *name, List *args, Obj *merge, Obj *cmp) {
  switch (lst_len(args)) {
  case 1:
    return merge;
  case 2:
    return read_qq("(fun (a b) (if ($cmp ($elt a) ($elt b)) a b))", 
                   qq_env("$elt", lst_elt(args, 1), "$cmp", cmp, NULL));
  default:
    clerror(name, /* args */ new list<AST*>(), "MAX-HOOD: WRONG NUM ARGS %d", lst_len(args));
    return NULL;
  }
}


Obj* rewrite_conds (List *args) {
  if (args == lisp_nil)
    return read_qq("(if-error)", lisp_nil);
  else {
    List *arg = (List*)lst_elt(args, 0);
    List *qqenv
      = qq_env("$pred", lst_elt(arg, 0),
               "$body", lst_elt(arg, 1),
               "$rest", rewrite_conds(lst_tail(args)),
               NULL);
    return read_qq("(if $pred $body $rest)", qqenv);
  }
}

extern AST *ast_op_call_check (AST_OP* op, list<AST*> *args );

// (cond (pa xa) (pb xb) (pc xc)) -> (if pa xa (if pb xb (if pc xc)))

Obj *body_from(List *forms) {
  if (lst_len(forms) == 1)
    return lst_elt(forms, 0);
  else {
    Obj *asym = new Symbol("all");
    return PAIR(asym, (Obj*)forms);
  }
}

AST* parse_special_form (const char *name, Obj *e, List *args, list<VAR*> *env) {
  int  i;
  if (strcasecmp(name, "let") == 0) {
    list<VAR*> *vars   = new list<VAR*>();
    list<AST*> *inits  = new list<AST*>();
    Obj  *body   = body_from(lst_tail(args));
    List *bargs  = (List*)lst_elt(args, 0);
    int  n       = lst_len(bargs);
    list<VAR*> *newenv = new list<VAR*>(*env);
    for (i = 0; i < n; i++) {
      AST  *init_val;
      List *arg    = (List*)lst_elt(bargs, i);
      char *name   = sym_elt(arg, 0);
      Obj  *init   = lst_elt(arg, 1);
      VAR  *var    = new VAR(name, &any_type);
      vars->push_front(var);
      init_val     = parse(init, env);
      var->ast     = init_val;
      var->type    = init_val->type;
      inits->push_front(init_val);
      newenv->push_front(var);
    }
    if (n == 0) {
      return parse(body, env);
    } else {
      return new_ast_let(vars, inits, parse(body, newenv), env);
    }
  } else if (strcasecmp(name, "let*") == 0) {
    Obj *let
      = rewrite_letstar((List*)lst_head(args), lst_tail(args));
    return parse(let, env);
    /*
  } else if (strcasecmp(name, "bind") == 0) {
    Obj *let
      = rewrite_bind((List*)lst_head(args), lst_tail(args));
    return parse(let, env);
    */
  } else if (strcasecmp(name, "fold-hood*") == 0) {
    // (fold-hood* folder init expr) 
    //    -> (fold-hood (fun (r t) (folder r (... (elt t i) ...))) init (tup ... ei ...))
    //    -> (fold-hood (fun (r e) (folder r (... e ...))) init e0)
    int n;
    Obj *expr   = lst_elt(args, 2);
    Obj *nexprs = lisp_nil;
    Obj *nexpr  = rewrite_fold_hood_star(expr, &nexprs, &n);
    List *qqenv
      = qq_env("$folder", lst_elt(args, 0),
               "$nexpr",  nexpr,
               "$nexprs", nexprs,
               "$init",   lst_elt(args, 1),
               NULL);
    char *str;
    if (n == 0) // no tup optimization
      str = (char*)"(fold-hood (fun (r e) ($folder r $nexpr)) $init 0)";
    else if (n == 1) // no tup optimization
      str = (char*)"(fold-hood (fun (r e) ($folder r $nexpr)) $init . $nexprs)";
    else
      str = (char*)"(fold-hood (fun (r t) ($folder r $nexpr)) $init (tup . $nexprs))";
    Obj *form = read_qq(str, qqenv);
    AST *ast  = parse(form, env);
    // if ((List*)nexprs == lisp_nil)
    //   cerror(ast, "FOLD-HOOD*: empty nbr expressions");
    return ast;
  } else if (strcasecmp(name, "fold-hood-plus*") == 0) {
    // (fold-hood-plus* folder expr) 
    //    -> (fold-hood-plus folder (fun (t) (... (elt t i) ...)) (tup ... ei ...))
    //    -> (fold-hood-plus folder (fun (e) (... e ...)) e0)
    int n;
    Obj *folder = lst_elt(args, 0);
    Obj *expr   = lst_elt(args, 1);
    Obj *nexprs = lisp_nil;
    Obj *nexpr  = rewrite_fold_hood_star(expr, &nexprs, &n);
    List *qqenv
      = qq_env("$folder", folder,
               "$nexpr",  nexpr,
               "$nexprs", nexprs,
               NULL);
    char *str;
    if (n == 0) // no tup optimization
      str = (char*)"(fold-hood-plus $folder (fun (e) $nexpr) 0)";
    else if (n == 1) { // no tup optimization
      str = (char*)"(fold-hood-plus $folder (fun (e) $nexpr) . $nexprs)";
    } else
      str = (char*)"(fold-hood-plus $folder (fun (t) $nexpr) (tup . $nexprs))";
    Obj *form = read_qq(str, qqenv);
    // post_form(form); post("YUK\n");
    AST *ast  = parse(form, env);
    return ast;
  } else if (strcasecmp(name, "min-hood+") == 0) {
    Obj *form = read_qq("(min-hood (if (is-self) (inf) $expr))", 
                        qq_env("$expr", lst_elt(args, 0), NULL));
    return parse(form, env);
  } else if (strcasecmp(name, "max-hood+") == 0) {
    Obj *form = read_qq("(max-hood (if (is-self) (neg (inf)) $expr))", 
                        qq_env("$expr", lst_elt(args, 0), NULL));
    return parse(form, env);
  } else if (strcasecmp(name, "any-hood+") == 0) {
    Obj *form = read_qq("(any-hood (if (is-self) 0 $expr))", 
                        qq_env("$expr", lst_elt(args, 0), NULL));
    return parse(form, env);
  } else if (strcasecmp(name, "all-hood+") == 0) {
    Obj *form = read_qq("(all-hood (if (is-self) 1 $expr))", 
                        qq_env("$expr", lst_elt(args, 0), NULL));
    return parse(form, env);
  } else if (strcasecmp(name, "max-hood") == 0) {
    Obj *folder = hood_folder("max-hood", args, read_qq("max", lisp_nil), read_qq(">", lisp_nil));
    Obj *form   = read_qq("(fold-hood-plus* $folder $expr)", 
                          qq_env("$expr", lst_elt(args, 0), "$folder", folder, NULL)); 
    return parse(form, env);
  } else if (strcasecmp(name, "min-hood") == 0) {
    Obj *folder = hood_folder("min-hood", args, read_qq("min", lisp_nil), read_qq("<", lisp_nil));
    Obj *form   = read_qq("(fold-hood-plus* $folder $expr)", 
                          qq_env("$expr", lst_elt(args, 0), "$folder", folder, NULL));
    return parse(form, env);
  } else if (strcasecmp(name, "all-hood") == 0) {
    Obj *folder = hood_folder("all-hood", args, read_qq("muxand", lisp_nil), read_qq("<", lisp_nil));
    Obj *form   = read_qq("(fold-hood-plus* $folder $expr)", 
                          qq_env("$expr", lst_elt(args, 0), "$folder", folder, NULL));
    return parse(form, env);
  } else if (strcasecmp(name, "any-hood") == 0) {
    Obj *folder = hood_folder("any-hood", args, read_qq("muxor", lisp_nil), read_qq(">", lisp_nil));
    Obj *form   = read_qq("(fold-hood-plus* $folder $expr)", 
                          qq_env("$expr", lst_elt(args, 0), "$folder", folder, NULL));
    return parse(form, env);
  } else if (strcasecmp(name, "sum-hood") == 0) {
    Obj *form = read_qq("(fold-hood-plus* + $expr)", 
                        qq_env("$expr", lst_elt(args, 0), NULL));
    return parse(form, env);
  } else if (strcasecmp(name, "int-hood") == 0) {
    Obj *form = read_qq("(fold-hood-plus* + (* (infinitesimal) $expr))", 
                        qq_env("$expr", lst_elt(args, 0), NULL));
    return parse(form, env);
  } else if (strcasecmp(name, "and") == 0) {
    List *qenv = qq_env("$arg1", lst_elt(args, 0), 
                        "$arg2", lst_elt(args, 1),
                        NULL);
    Obj *form = read_qq("(if $arg1 $arg2 0)", qenv);
    return parse(form, env);
  } else if (strcasecmp(name, "or") == 0) {
    List *qenv = qq_env("$arg1", lst_elt(args, 0), 
                        "$arg2", lst_elt(args, 1),
                        NULL);
    Obj *form = read_qq("(let ((~orval $arg1)) (if ~orval ~orval $arg2))", qenv);
    return parse(form, env);
  } else if (strcasecmp(name, "rep") == 0) {
    List *qenv = qq_env("$n", lst_elt(args, 0), 
                        "$i", lst_elt(args, 1), 
                        "$e", lst_elt(args, 2), 
                        NULL);
    Obj *form = read_qq("(letfed (($n $i $e)) $n)", qenv);
    return parse(form, env);
  } else if (strcasecmp(name, "case") == 0) {
    Obj *val   = lst_elt(args, 0);
    Obj *var   = Symbol::gensym("val");
    Obj *cases = rewrite_cases(var, lst_tail(args));
    Obj *let   = _list(new Symbol("let"),
                       _list(_list(var, val, NULL), NULL),
                       cases, NULL);
    return parse(let, env);
  } else if (strcasecmp(name, "cond") == 0) {
    Obj *form = rewrite_conds(args);
    return parse(form, env);
  } else if (strcasecmp(name, "all") == 0) {
    if (lst_len(args) == 1)
      return parse(lst_elt(args, 0), env);
    else {
      return ast_op_call_check(all_op, parse_args(args, env));
    }
  } else if (strcasecmp(name, "null") == 0) {
    AST *val = parse(lst_elt(args, 0), env);
    // post_form(lst_elt(args, 0));
    // post("BEF NULL TYPE %s\n", type_name(val->type));
    maybe_decr_n_refs(val);
    AST *res = real_null_of(val->type);
    // post("AFT NULL TYPE %s\n", type_name(res->type));
    return res;
  } else if (strcasecmp(name, "*") == 0) {
    Obj *call = rewrite_nary(new Symbol("B*"), args);
    return parse(call, env);
  } else if (strcasecmp(name, "+") == 0) {
    Obj *call = rewrite_nary(new Symbol("B+"), args);
    return parse(call, env);
  } else if (strcasecmp(name, "defstruct") == 0) {
    // TODO: ADD SUPPORT FOR INHERITANCE
    char *name   = sym_elt(args, 0);
    Obj *fields  = lst_tail(lst_tail(args));
    Obj *def_sym = new Symbol("def");
    Obj *all_sym = new Symbol("all");
    Obj *tup_sym = new Symbol("tup");
    Obj *getters = build_getters(name, 0, (List*)fields);
    Obj *ds      = _list(all_sym,
                         _list(def_sym,
                               cat_sym2((char*)"new-", name),
                                         fields,
                               PAIR(tup_sym, fields),
                               NULL),
                         PAIR(all_sym, getters),
                         NULL);
    return parse(ds, env);
  } else if (strcasecmp(name, "quote") == 0) {
    Obj *val = lst_head(args);
    if (symbolp(val)) {
      return parse_symbol(((Symbol*)val)->getName().c_str());
    } else if (numberp(val)) {
      return new AST_LIT(((Number*)val)->getValue());
    } else
      uerror("UNSUPPORTED QUOTE KIND %s", val->lispType());
  } else if (strcasecmp(name, "def") == 0) {
    Obj  *fsym   = new Symbol("fun");
    char *name   = sym_elt(args, 0);
    List *params = (List*)lst_elt(args, 1);
    Obj *body    = body_from(lst_tail(lst_tail(args)));
    List *form   = _list(fsym, params, body, NULL);
    AST_FUN *fun = (AST_FUN*)parse(form, env);
    // post("COMPILING DEF %s ", name); env_print(fun->vars); post("\n");
    fun->name    = name;
    globals->push_front(new VAR(name, new ONE_FUN_TYPE(fun)));
    // post("ADDED DEF %s\n", name);
    return new AST_LIT(0);
  } else if (strcasecmp(name, "letfed") == 0) { 
    // (letfed ((f (fill 0) (add (cam 0) f))) f)
    // (let ((f (feedback (fill 0)))) (let ((f (set-feedback f (add (cam 0) f)))) f))
    // (letfed ((f (add (cam 0) g)) (g (add f f))) f)
    // (let ((f (feedback)) (g (feedback))) 
    //   (let ((f (set-feedback f (add (cam 0) g))) (g (set-feedback g (add f f)))) f))
    List *bindings = (List*)lst_elt(args, 0);
    Obj  *body     = body_from(lst_tail(args));
    List *inis = lisp_nil;
    List *vals = lisp_nil;
    List *tbs  = lisp_nil;
    List *atbs = lisp_nil;
    List *cmbs = lisp_nil;
    List *bs;
    Obj *let_fed;
    for (bs = bindings; bs != lisp_nil; bs = lst_tail(bs)) {
      List *b = (List*)bs->getHead();
      Obj  *n = lst_elt(b, 0);
      if (lst_len(b) == 2) {
        cmbs = PAIR(b, cmbs);
      } else if (lst_len(b) == 3) {
        Obj  *i = read_qq("(fun () $b1)", qq_env("$b1", lst_elt(b, 1), NULL));
        Obj  *v = lst_elt(b, 2);
        List *tn = (List*)n;
        if (listp(n) && symbolp(lst_head(tn)) &&
            strcasecmp(sym_name(lst_head(tn)).c_str(), "TUP") == 0) {
          int i;
          List *tns;
          Obj *t = Symbol::gensym("t");
          for (tns = lst_tail(tn), i = 0; tns != lisp_nil; tns = lst_tail(tns), i++) {
            char form[100];
            sprintf(form, "($n (elt $t %d))", i);
            Obj *tb = read_qq(form, qq_env("$n", lst_head(tns), "$t", t, NULL));
            // post_form(tb);
            tbs  = PAIR(tb, tbs);
            atbs = PAIR(tb, atbs);
          }
          tbs = lst_rev(tbs);
          n = t;
        }
        atbs = lst_rev(atbs);
        inis = PAIR(read_qq("($n (init-feedback $i))", qq_env("$n", n, "$i", i, NULL)), inis);
        vals = PAIR(read_qq("($n (feedback $n (let $tbs $v)))", 
                            qq_env("$n", n, "$tbs", tbs, "$v", v, NULL)), vals);
      } else {
        // clerror("LETFED", args, "LETFED: MALFORMED BINDING %d", lst_len(b));
        uerror("LETFED: MALFORMED BINDING %d", lst_len(b));
      }
    }
    let_fed = read_qq("(let $inis (let* $cmbs (let $vals (let $atbs $body))))", 
		      qq_env("$inis", inis, "$cmbs", lst_rev(cmbs), "$vals", vals, 
			     "$atbs", atbs, "$body", body, NULL));
    return parse(let_fed, env);
  } else if (strcasecmp(name, "select") == 0) {
    Obj *form
      = read_select(0, lst_elt(args, 0), lst_elt(args, 1), lst_tail(args));
    // post_form(form);
    return parse(form, env);
  } else if (strcasecmp(name, "loop") == 0) { 
    Obj *form = read_qq("(elt (seq . $args) 0)", qq_env("$args", args, NULL));
    // post_form(form);
    return parse(form, env);
  } else if (strcasecmp(name, "seq") == 0) { 
    List *qqenv
      = qq_env("$1st-ss", lst_elt(args, 0),
               "$ss", args,
               "$len-ss", new Number(lst_len(args)), 
               NULL);
    char *str
      = (char*)"(letfed ((vei (tup (null $1st-ss) 0) \
                        (let* ((idx (elt vei 1)) \
                               (ve (select (mod idx $len-ss) . $ss))) \
                          (if (elt ve 1) (tup ve idx) \
                            (tup ve (+ idx 1)))))) \
            (tup (elt (elt vei 0) 0) (< (elt vei 1) $len-ss)))";
    Obj *form = read_qq(str, qqenv);
    // post_form(form);
    return parse(form, env);
  } else if (strcasecmp(name, "if") == 0) {
    list<AST*> *ast_args = parse_args(args, env);
    if (lst_len(args) != 3) 
      clerror("IF", ast_args, "IF: WRONG NUM ARGS %d", lst_len(args));
    else {
      AST* tst = list_nth(ast_args, 0);
      AST* con = list_nth(ast_args, 1);
      AST* alt = list_nth(ast_args, 2);

      AST* ast = new AST_OP_CALL(if_op, ast_args);
      if (!tst->type->subtype(NUMT))
        cerror(ast, "IF: BAD TST %s", tst->type->name());
      else {
        if (!is_same_type(con->type, alt->type)) {
          ast->print(); fprintf(error_log(),"\n");
          cerror(ast, "IF: REQUIRES SAME TYPE FOR BOTH BRANCHES CON %s ALT %s",
                 con->type->name(), alt->type->name());
        } else
          return ast;
      } 
    }
  } else if (strcasecmp(name, "mux") == 0) {
    list<AST*> *ast_args = parse_args(args, env);
    if (lst_len(args) != 3) 
      clerror("MUX", ast_args, "MUX: WRONG NUM ARGS %d", lst_len(args));
    else {
      AST* tst = list_nth(ast_args, 0);
      AST* con = list_nth(ast_args, 1);
      AST* alt = list_nth(ast_args, 2);

      int is_vec = con->type->kind == VEC_KIND || con->type->kind == TUP_KIND;
      AST* ast = new AST_OP_CALL(is_vec ? vmux_op : mux_op, ast_args);
      if (!tst->type->subtype(NUMT))
        cerror(ast, "MUX: BAD TST %s", tst->type->name());
      else {
        if (!is_same_type(con->type, alt->type))
          cerror(ast, "MUX: REQUIRE SAME TYPE FOR BOTH BRANCHES CON %s ALT %s",
                 con->type->name(), alt->type->name());
        else
          return ast;
      } 
    }
  } else if (strcasecmp(name, "map") == 0) {
    list<AST*> *ast_args = parse_args(args, env);
    if (lst_len(args) != 2) 
      clerror("MAP", ast_args, "MAP: WRONG NUM ARGS %d", lst_len(args));
    else {
      AST* rfun = list_nth(ast_args, 0);
      AST* tup  = list_nth(ast_args, 1);
      if (rfun->type->kind != ONE_FUN_KIND) 
        clerror("MAP", ast_args, "MAP: BAD FUN %s", rfun->type->name());
      else {
        AST_FUN* fun = ((ONE_FUN_TYPE*)rfun->type)->value;
        FUN_TYPE* type = (FUN_TYPE*)fun->fun_type;
        if (type->arity != 1) 
          clerror("MAP", ast_args, "MAP: WRONG NUM ARGS FOR FUN %d", type->arity);
        else if (tup->type->kind != VEC_KIND) 
          clerror("MAP", ast_args, "MAP: REQUIRES HOMOGENOUS TUP ARG %s", 
                  tup->type->name());
        else {
          TYPE* elt_type
            = ((VEC_TYPE*)tup->type)->elt_type;
          list<VAR*> *vars = new list<VAR*>();
          vars->push_back(new VAR(fun->vars->front()->name, elt_type));
          AST* body = parse_fun_body(vars, fun->body, env);
          AST* clone_fun
            = new AST_FUN("fun", vars, fun->body, body);
          maybe_decr_n_refs(rfun);
          list<AST*> *args = new list<AST*>();
          args->push_back(clone_fun);
          args->push_back(tup);
          return new AST_OP_CALL(map_op, args);
        }
      } 
    }
  } else if (strcasecmp(name, "apply") == 0) {
    list<AST*> *ast_args = parse_args(args, env);
    if (lst_len(args) != 2) 
      clerror("APPLY", ast_args, "APPLY: WRONG NUM ARGS %d", lst_len(args));
    else {
      AST*  rfun     = list_nth(ast_args, 0);
      AST*  tup      = list_nth(ast_args, 1);
      TYPE* tup_type = tup->type;
      if (tup_type->kind != TUP_KIND && tup_type->kind != VEC_KIND)
        clerror("APPLY", ast_args, 
                "APPLY: REQUIRES TUP/VEC ARG %s", tup_type->name());
      else if (rfun->type->kind != ONE_FUN_KIND) 
        clerror("APPLY", ast_args, "APPLY: BAD FUN %s", rfun->type->name());
      else {
        AST_FUN* fun = ((ONE_FUN_TYPE*)rfun->type)->value;
        FUN_TYPE* type = (FUN_TYPE*)fun->fun_type;
        if (type->arity != tup_type_len(tup_type)) 
          clerror("APPLY", ast_args, 
                  "APPLY: WRONG NUM ARGS FOR FUN EXPECTED %d GOT %d", 
                  type->arity, tup_type_len(tup_type));
        else {
          int i;
          AST* body;
          AST* clone_fun;
          list<VAR*> *vars = new list<VAR*>();
          for (i = 0; i < tup_type_len(tup_type); i++)
            vars->push_front(new VAR(list_nth(fun->vars, i)->name,
                                     tup_type_elt(tup_type, i)));
          body = parse_fun_body(vars, fun->body, env);
          clone_fun = new AST_FUN("fun", vars, fun->body, body);
          maybe_decr_n_refs(rfun);
          list<AST*> *args = new list<AST*>();
          args->push_back(clone_fun);
          args->push_back(tup);
          return new AST_OP_CALL(apply_op, args);
        }
      }
    }
  } else if (strcasecmp(name, "fold") == 0) {
    list<AST*> *ast_args = parse_args(args, env);
    if (lst_len(args) != 3) 
      clerror("FOLD", ast_args, "FOLD: WRONG NUM ARGS %d", lst_len(args));
    else {
      AST* rfun = list_nth(ast_args, 0);
      AST* init = list_nth(ast_args, 1);
      AST* tup  = list_nth(ast_args, 2);
      if (rfun->type->kind != ONE_FUN_KIND) 
        clerror("FOLD", ast_args, "FOLD: BAD FUN %s", rfun->type->name());
      else {
        AST_FUN* fun = ((ONE_FUN_TYPE*)rfun->type)->value;
        FUN_TYPE* type = (FUN_TYPE*)fun->fun_type;
        if (type->arity != 2) 
          clerror("FOLD", ast_args, "FOLD: WRONG NUM ARGS FOR FUN %d", type->arity);
        else if (tup->type->kind != VEC_KIND) 
          clerror("FOLD", ast_args, "FOLD: REQUIRES HOMOGENOUS TUP ARG %s", 
                  tup->type->name());
        else {
          TYPE* elt_type
            = ((VEC_TYPE*)tup->type)->elt_type;     
          if (!is_same_type(elt_type, init->type))
            clerror("FOLD", ast_args,
                    "FOLD: REQUIRES INIT TYPE %s SAME AS TUP ELT_TYPE %s",
                    init->type->name(), elt_type->name());
          else {
            list<VAR*> *vars = new list<VAR*>();
            vars->push_back(new VAR(list_nth(fun->vars, 0)->name, elt_type));
            vars->push_back(new VAR(list_nth(fun->vars, 1)->name, elt_type));
            AST* body = parse_fun_body(vars, fun->body, env);
            AST* clone_fun = new AST_FUN("fun", vars, fun->body, body);
            AST_OP* op
              = (elt_type->kind == TUP_KIND || elt_type->kind == VEC_KIND) ?
                vfold_op : fold_op;
            maybe_decr_n_refs(rfun);
            list<AST*> *args = new list<AST*>();
            args->push_back(clone_fun);
            args->push_back(init);
            args->push_back(tup);
            return new AST_OP_CALL(op, args);
          }
        }
      }
    } 
  } else if (strcasecmp(name, "tup") == 0) {
    if (lst_len(args) == 0)
      return new AST_OP_CALL(nul_tup_op, new list<AST*>());
    else
      return NULL;
  } else if (strcasecmp(name, "fold-hood") == 0) {
    list<AST*> *ast_args = parse_args(args, env);
    if (lst_len(args) != 3) 
      clerror("FOLD-HOOD", ast_args, "FOLD-HOOD: WRONG NUM ARGS %d", lst_len(args));
    else {
      AST* rfun = list_nth(ast_args, 0);
      AST* init = list_nth(ast_args, 1);
      AST* arg  = list_nth(ast_args, 2);
      if (rfun->type->kind != ONE_FUN_KIND) 
        clerror("FOLD-HOOD", ast_args, 
                "FOLD-HOOD: BAD FUN %s", rfun->type->name());
      else {
        AST_FUN* fun = ((ONE_FUN_TYPE*)rfun->type)->value;
        FUN_TYPE* type = (FUN_TYPE*)fun->fun_type;
        if (type->arity != 2) 
          clerror("FOLD-HOOD", ast_args, 
                  "FOLD-HOOD: WRONG NUM ARGS FOR FUN %d", type->arity);
        else {
          TYPE* elt_type = arg->type;
          list<VAR*> *vars = new list<VAR*>();
          vars->push_back(new VAR(list_nth(fun->vars, 0)->name, init->type));
          vars->push_back(new VAR(list_nth(fun->vars, 1)->name, elt_type));
          AST* body = parse_fun_body(vars, fun->body, env);
          AST* clone_fun = new AST_FUN("fun", vars, fun->body, body);
          AST_OP* op
            = (init->type->kind == TUP_KIND || init->type->kind == VEC_KIND) ?
              vfold_hood_op : fold_hood_op;
          list<AST*> *args = new list<AST*>();
          args->push_back(clone_fun);
          args->push_back(init);
          args->push_back(arg);
          AST* ast = new AST_OP_CALL(op, args);
          if (!is_same_type(body->type, init->type))
            cerror(ast, "FOLD-HOOD: REQUIRES INIT TYPE %s SAME AS FUN RET TYPE %s",
                   init->type->name(), body->type->name());
          maybe_decr_n_refs(rfun);
          return ast;
        }
      }
    } 
  } else if (strcasecmp(name, "fold-hood-plus") == 0) {
    list<AST*> *ast_args = parse_args(args, env);
    if (lst_len(args) != 3) 
      clerror("FOLD-HOOD-PLUS", ast_args, 
              "FOLD-HOOD-PLUS: WRONG NUM ARGS %d", lst_len(args));
    else {
      AST* fuse = list_nth(ast_args, 0);
      AST* mold = list_nth(ast_args, 1);
      AST* expr = list_nth(ast_args, 2);
      if (fuse->type->kind != ONE_FUN_KIND) 
        clerror("FOLD-HOOD-PLUS", ast_args, 
                "FOLD-HOOD+: BAD FUSER %s", fuse->type->name());
      if (mold->type->kind != ONE_FUN_KIND) 
        clerror("FOLD-HOOD-PLUS", ast_args, 
                "FOLD-HOOD-PLUS: BAD MOLDER %s", mold->type->name());
      AST_FUN* ffun = ((ONE_FUN_TYPE*)fuse->type)->value;
      FUN_TYPE* ftype = (FUN_TYPE*)ffun->fun_type;
      if (ftype->arity != 2) 
        clerror("FOLD-HOOD-PLUS", ast_args, 
                "FOLD-HOOD-PLUS: WRONG NUM ARGS FOR FUSER %d", ftype->arity);
      AST_FUN* mfun = ((ONE_FUN_TYPE*)mold->type)->value;
      FUN_TYPE* mtype = (FUN_TYPE*)mfun->fun_type;
      if (mtype->arity != 1) 
        clerror("FOLD-HOOD-PLUS", ast_args, 
                "FOLD-HOOD-PLUS: WRONG NUM ARGS FOR MOLDER %d", mtype->arity);
      TYPE* elt_type = expr->type;
      list<VAR*> *mvars = new list<VAR*>();
      mvars->push_back(new VAR(list_nth(mfun->vars, 0)->name, elt_type));
      AST* mbody = parse_fun_body(mvars, mfun->body, env);
      AST* clone_mfun = new AST_FUN("fun", mvars, mfun->body, mbody);
      list<VAR*> *fvars = new list<VAR*>;
      fvars->push_back(new VAR(list_nth(ffun->vars, 0)->name, mbody->type));
      fvars->push_back(new VAR(list_nth(ffun->vars, 1)->name, mbody->type));
      AST* fbody = parse_fun_body(fvars, ffun->body, env);
      AST* clone_ffun = new AST_FUN("fun", fvars, ffun->body, fbody);
      AST_OP* op 
        = (mbody->type->kind == TUP_KIND || mbody->type->kind == VEC_KIND) ?
        vfold_hood_plus_op : fold_hood_plus_op;
      list<AST*> *args = new list<AST*>();
      args->push_back(clone_ffun);
      args->push_back(clone_mfun);
      args->push_back(expr);
      AST* ast = new AST_OP_CALL(op, args);
      // if (!is_same_type(body->type, init->type))
      //   cerror(ast, "FOLD-HOOD-PLUS: REQUIRES INIT TYPE %s SAME AS FUN RET TYPE %s",
      //          type_name(init->type), type_name(body->type));
      maybe_decr_n_refs(fuse);
      maybe_decr_n_refs(mold);
      return ast;
    }
  } else if (strcasecmp(name, "fun") == 0) {
    List *params = (List*)lst_elt(args, 0);
    Obj  *body   = body_from(lst_tail(args));
    list<VAR*> *vars   = params_to_vars(params);
    AST  *abody  = vars->size() > 0 ? NULL : parse_fun_body(vars, body, env);
    return new AST_FUN("fun", vars, body, abody);
  } else {
    return NULL;
  }
  uerror("Unknown or unhandled special form");
  return NULL;
}

int type_check (AST *arg, TYPE *type) {
  return arg->type->subtype(type);
  // return 1;
}

int ast_op_call_type_check (FUN_TYPE *type, list<AST*> *args) {
  int i = 0;
  for (list<AST*>::iterator it = args->begin();
       it != args->end(); i++, it++) {
    AST *arg = *it;
    if (i < type->arity && !type_check(arg, type->param_types[i]))
      return i;
  }
  return -1;
}

AST *ast_op_call_check (AST_OP* op, list<AST*> *args ) {
  int i;
  int nargs = args->size();
  AST* ast;
  FUN_TYPE *type = (FUN_TYPE*)(op->type);
  ast = new AST_OP_CALL(op, args); 
  if (!(nargs == type->arity ||
        (type->is_nary && nargs >= type->arity)))
    cerror(ast, "WRONG NUMBER OF ARGS FOR %s GIVEN %d WANTED %d IS_NARY %d",
           op->name, nargs, type->arity, type->is_nary);
  if ((i = ast_op_call_type_check(type, args)) >= 0)
    cerror(ast, "TYPE ERROR FOR %s ON ARG %d GOT %s EXPECTED %s",
           op->name, i, 
           (list_nth(args, i))->type->name(),
           type->param_types[i]->name());
  // post("OP %s\n", op->name);
  return ast;
}

AST *ast_gop_call_check (AST_GOP *gop, list<AST*> *args) {
  int nargs = args->size();
  FUN_TYPE *gop_type = (FUN_TYPE*)(gop->type);
  if (!(nargs == gop_type->arity ||
        (gop_type->is_nary && nargs >= gop_type->arity)))
    clerror("GOP-CALL", args,
            "WRONG NUMBER OF ARGS FOR %s GIVEN %d WANTED %d IS_NARY %d", 
           gop->name, nargs, gop_type->arity, gop_type->is_nary);
  for (list<AST_OP*>::iterator it = gop->ops->begin();
       it != gop->ops->end(); it++) {
    AST_OP *op = *it;
    if (ast_op_call_type_check((FUN_TYPE*)(op->type), args) < 0) {
      AST *ast = new AST_OP_CALL(op, args); 
      // post("OP %s\n", op->name);
      return ast;
    }
  }
  //post_gop(gop);
  for (list<AST*>::iterator it = args->begin();
       it != args->end(); it++) {
    fprintf(error_log()," %s", (*it)->type->name());
  }
  fprintf(error_log(),": ");
  clerror("GOP-CALL", args, "NO APPLICABLE METHODS ERROR FOR %s", gop->name);
  return NULL;
}

AST* parse (Obj *e, list<VAR*> *env) {
  int i;
  // debug("E COMPILE\n");
  if (numberp(e)) {
    return new AST_LIT(((Number*)e)->getValue());
  } else if (symbolp(e)) {
    AST *res;
    const char *name = ((Symbol*)e)->getName().c_str();
    // debug("FOUND SYM %s\n", name);
    res = parse_reference(name, env);
    if (res != NULL) 
      return res;
    else {
      uerror("UNABLE TO FIND %s", name);
    }
  } else if (listp(e)) {
    List *le = (List*)e;
    int lstlen = lst_len(le);
    int  nargs = lstlen - 1;
    Obj  *fun_obj  = lst_head(le);
    List *args = lst_tail(le);
    AST  *ast_fun;
    TYPE *type;
    // debug("FOUND LST\n");
    if (lstlen == 0) 
      uerror("ILLEGAL EMPTY LIST");

    if (symbolp(fun_obj)) {
      const char *name = sym_name(fun_obj).c_str();
      AST  *res  = parse_special_form(name, fun_obj, args, env);
      if (res != NULL)
        return res;
      else {
        VAR *var = lookup(name, env);
        list<AST*> *ast_args = parse_args(args, env);
        if (var != NULL) {
          type = var->type;
          switch (type->kind) {
          case ONE_OP_KIND: 
            return ast_op_call_check(((ONE_OP_TYPE*)type)->op, ast_args); 
          case ONE_GOP_KIND: 
            return ast_gop_call_check(((ONE_GOP_TYPE*)type)->gop, ast_args); 
	  default:
	    break; // fall through on all other cases
          }
          var->n_refs -= 1;
        }
      }
    }
    ast_fun = parse(fun_obj, env);
    type = ast_fun->type;
    if (type->kind == ONE_FUN_KIND) {
      AST_FUN *fun = ((ONE_FUN_TYPE*)type)->value;
      FUN_TYPE *type = (FUN_TYPE*)(fun->fun_type);
      VAR *var;
      AST *body;
      list<VAR*> *vars = new list<VAR*>;
      list<AST*> *let_vals = new list<AST*>;
      list<AST*> *as = parse_args(args, env);

      if (!(nargs == type->arity ||
            (type->is_nary && nargs >= type->arity)))
        clerror("...", /* PAIR(fun, as) */ new list<AST*>(), 
                "WRONG NUMBER OF ARGS FOR %s GIVEN %d EXPECTED %d IS_NARY %d", 
               fun->name, nargs, type->arity, type->is_nary);
      i = 0;
      for (list<AST*>::iterator it = as->begin();
           it != as->end(); i++, it++) {
        AST* a = *it;
        var = new VAR(list_nth(fun->vars, i)->name, &any_type);
        var->ast = a;
        var->type = a->type;
        vars->push_back(var);
        let_vals->push_back(a);
      }
      maybe_decr_n_refs(ast_fun);
      body = parse_fun_body(vars, fun->body, env);
      return new_ast_let(vars, let_vals, body, env);
    } else {
      list<AST*> *as = parse_args(args, env);
      return new AST_CALL(ast_fun, as);
      // uerror("UNKNOWN CALL TYPE %d", type->kind);
    }
  }
  uerror("Unknown or unhandled form");
  return NULL;
}

Script* compile (Obj *e, int is_dump_ast) {
  globals = new list<VAR*>();
  // run the initial parse
  AST *res = parse(e, new list<VAR*>());
  res = ast_optimize_lets(res);
  // gather global data through lifting
  LIFT_DATA data = {new list<VAR*>(), new list<AST*>(), 0, 0, 0, false};
  res = ast_lift(res, &data);
  data.is_fun_lift = 1;
  res = ast_lift(res, &data);
  int n_glos  = data.glos->size()+1;
  int stk_siz = res->stack_size();
  int env_siz = res->env_size();
  if (is_dump_ast) {
    fprintf(error_log(),"// ");
    res->print();
    fprintf(error_log(),"\n");
  }
  // Finally, do the actual emission
  Script body; res->emit(&body);
  Script* script = new Script(9, DEF_VM_OP, data.export_len, data.n_exports, 
                             n_glos >> 8, n_glos & 255, data.n_state, 
                             stk_siz >> 8, stk_siz & 255, env_siz);
  for (list<AST*>::iterator i = data.glos->begin();
       i != data.glos->end(); i++)
    (*i)->emit(script);
  emit_def_fun_op(body.size()+1, script);
  script->append(&body);
  script->add(RET_OP,EXIT_OP);
  return script;
}

char *xlate_opname(char *dst, char *src) {
  int i;
  for (i = 0; i < strlen(src); i++) {
    
    dst[i] = src[i] == '-' ? '_' : toupper(src[i]);
  }
  dst[i] = 0;
  strcat(dst, "_OP");
  return dst;
}

#define MAX_DIRS 100

void init_compiler () {
  init_lisp();
  init_ops();
}

uint8_t *compile_script (const char *str, int *len, int is_dump_ast) {
  // post("SCRIPT %s\n", str);
  Obj* obj = read_from_str(str);
  Script* s = compile(obj, is_dump_ast);
  *len = s->size();
  uint8_t *dst = (uint8_t*)MALLOC(*len);
  for(int i=0;i<*len;i++) dst[i]=s->pop();
  return dst;
}

FILE* dump_target=stdout;
void dump_instructions (bool is_json_format, int n, uint8_t *bytes) {
  int j;
  if (is_json_format) { fprintf(dump_target,"{ \"script\" : \n  ["); }
  else { fprintf(dump_target,"uint8_t script[] = {"); }
  for (j = 0; j < n; j++) {
    int i;
    char *name, opname[100];
    uint8_t code = bytes[j];
    AST_OP* op = lookup_op_by_code(code, &name);
    if (op == NULL) uerror("NULL OP %d", code);
    if (j != 0) {
      if(is_json_format) { fprintf(dump_target,",\n"); }
      else { fprintf(dump_target,","); }
    }
    if(is_json_format) fprintf(dump_target," \"%s\"",xlate_opname(opname, name));
    else fprintf(dump_target," %s",xlate_opname(opname, name));
    if (op->arity < 0) {
      for (i = 0; i < (-op->arity)+op->is_nary;) {
        if (is_json_format) { fprintf(dump_target,",\n"); }
        else { fprintf(dump_target,","); }
        j += 1;
        if (is_json_format) { fprintf(dump_target," \"%d\"", bytes[j]); }
        else { fprintf(dump_target," %d", bytes[j]); }
        if (!(bytes[j] & 0x80)) i++;
      }
    } else {
      for (i = 0; i < op->arity+op->is_nary; i++) {
        if (is_json_format) { fprintf(dump_target,",\n"); }
        else { fprintf(dump_target,","); }
        j += 1;
        if (is_json_format) { fprintf(dump_target," \"%d\"", bytes[j]);}
        else { fprintf(dump_target," %d", bytes[j]); }
      }
    }
  }
  if (is_json_format) { fprintf(dump_target,"]\n}\n"); }
  else { 
    fprintf(dump_target," };\n"); 
    fprintf(dump_target,"uint16_t script_len = %d;", n);
  }
  fprintf(dump_target,"\n");
}

void dump_code (int n, uint8_t *bytes) {
  int j;
  for (j = 0; j < n; j++) {
    int i;
    char *name;
    uint8_t code = bytes[j];
    AST_OP* op = lookup_op_by_code(code, &name);
    if (op == NULL) uerror("NULL OP %d", code);
    fprintf(dump_target,"%3d: %s", j, name);
    if (op->arity < 0) {
      for (i = 0; i < (-op->arity)+op->is_nary; ) {
        j += 1;
        fprintf(dump_target," %d", bytes[j]);
        if (!(bytes[j] & 0x80)) i++;
      }
    } else {
      for (i = 0; i < op->arity+op->is_nary; i++) {
        j += 1;
        fprintf(dump_target," %d", bytes[j]);
      }
    }
  }
}

/***** PLATFORM-SPECIFIC OP DEFINITIONS *****/
#include <fstream>
#include "sexpr.h"

void downcase(string& str) 
{ for(int i=0;i<str.size();i++) { str[i]=tolower(str[i]); } }

// Invalid op name is -1
int op_num(string s) {
  for(int i=0;i<256;i++) { if(s==op_names[i]) return i; } 
  uerror("%s is not a valid operation name.",s.c_str());
  return -1;
}

int next_free_op() {
  for(int i=0;i<256;i++) {if(op_names[i]=="") return i; }
  uerror("No free opcodes available.");
  return -1;
}

TYPE* name_to_type (SExpr* ex) {
  if(ex->type_of()=="SE_List") {
    SE_List* l = (SE_List*)ex;
    if(l->len() == 2 && *l->children[0]=="vector" && *l->children[1]==3) {
      return VEC3T;
    } else {
      uerror("Compound type %s is not a (vector 3)",ex->to_str().c_str());
    }
  } else if(ex->isSymbol()) {
    if(*ex=="scalar") return NUMT;
    if(*ex=="boolean") return NUMT;
  }
  uerror("Unhandled or unknown type %s",ex->to_str().c_str());
  return NULL;
}

void
parse_external_defop(SE_List &list)
{
  assert(0 < list.len());
  assert(list[0]->isSymbol());
  assert(*list[0] == "defop");
  if (list.len() < 3)
    uerror("defop has too few arguments");

  int opcode;
  SExpr &opspec = *list[1];
  if (opspec.isSymbol()) {
    SE_Symbol &symbol = dynamic_cast<SE_Symbol &>(opspec);
    opcode = ((symbol == "?") ? next_free_op() : op_num(symbol.name));
  } else if (opspec.isScalar()) {
    SE_Scalar &scalar = dynamic_cast<SE_Scalar &>(opspec);
    opcode = static_cast<int>(scalar.value);
  } else {
    uerror("defop op not symbol or number");
  }

  SExpr &name_sexpr = *list[2];
  if (!name_sexpr.isSymbol())
    uerror("defop fn not symbol");
  const string &name_string = dynamic_cast<SE_Symbol &>(name_sexpr).name;
  size_t name_size = name_string.size();
  char *name_cstr = new char[name_size + 1];
  name_string.copy(name_cstr, name_size);
  name_cstr[name_size] = 0;

  vector<TYPE *> types;
  for (size_t i = 3; i < list.len(); i++)
    types.push_back(name_to_type(list[i]));

  add_op(name_cstr, opcode, 0, 0, new FUN_TYPE(&types));
}

void
parse_external_op(SE_List &list)
{
  assert(0 < list.len());
  assert(list[0]->isSymbol());

  SE_Symbol &definer = dynamic_cast<SE_Symbol &>(*list[0]);
  if (definer == "defop")
    parse_external_defop(list);
  else
    uerror("Unknown external operation definer: %s", definer.name.c_str());
}

bool
parse_opfile(SExpr &sexpr)
{
  if (!sexpr.isList())
    return false;

  SE_List &list = dynamic_cast<SE_List &>(sexpr);
  SExpr &first = *list[0];
  if (!first.isSymbol())
    return false;

  // FIXME: Implement != on symbols.  This is silly.
  if (! (first == "all"))
    parse_external_op(list);
  else
    for (int i = 1; i < list.len(); i++) {
      SExpr &item = *list[i];
      if (!item.isList())
        return false;
      SE_List &sublist = dynamic_cast<SE_List &>(item);
      if (sublist.len() <= 0)
        return false;
      if (!sublist[0]->isSymbol())
        return false;
      parse_external_op(sublist);
    }

  return true;
}

void
read_opfile(const string &filename)
{
  SExpr *sexpr;

  {
    ifstream opfile_stream(filename.c_str());
    if (!opfile_stream.good())
      uerror("Could not find opfile %s", filename.c_str());
    sexpr = read_sexpr(filename, &opfile_stream);
  }

  if (sexpr == 0)
    uerror("Could not read opfile %s", filename.c_str());
  if (!parse_opfile(*sexpr))
    uerror("Could not parse opfile %s", filename.c_str());
}

void
PaleoCompiler::setDefops(const string &defops)
{
  if (is_echo_defops)
    std::cout << "defops = " << defops.c_str() << "\n";

  SExpr *sexpr = read_sexpr("defops", defops);
  if (sexpr == 0)
    uerror("Could not read defops %s", defops.c_str());
  if (!parse_opfile(*sexpr))
    uerror("%s is not an opfile", defops.c_str());
}


/***** COMPILER WRAPPER CLASS *****/
PaleoCompiler::PaleoCompiler(Args* args) : Compiler(args) {
  SE_Symbol::case_insensitive = true;
  
  proto_path = new Path();
  if (args->extract_switch("--srcdir"))
    srcdir = args->pop_next();

  if (args->extract_switch("--basepath"))
    proto_path->add_to_path(args->pop_next());
  else
    proto_path->add_default_path(srcdir);
  
  while(args->extract_switch("-path",false)) // can extract multiple times
    proto_path->add_to_path(args->pop_next());
  
  is_show_code = args->extract_switch("-k");
  is_dump_code = args->extract_switch("--instructions");
  is_json_format = args->extract_switch("--json");
  is_dump_ast = args->extract_switch("--print-ast");
  is_echo_defops = args->extract_switch("--echo-defops");
  init_compiler();
  last_script=(char*)"";
}

PaleoCompiler::~PaleoCompiler() {
  delete proto_path;
  srcdir = "";
}

static int
os_mkdir(const char *pathname)
{
#ifdef _WIN32
  return mkdir(pathname);
#else
  return mkdir(pathname, ACCESSPERMS);
#endif
}

// When being run standalone, -D controls dumping (it's normally
// consumed by the simulator).  Likewise, if -dump-stem is present,
// then dumping goes to a file instead of stdout.
void
PaleoCompiler::init_standalone(Args *args)
{
  oldc_test_mode = args->extract_switch("--test-mode");
  is_dump_code |= args->extract_switch("-D");
  bool dump_to_stdout = true;
  const char *dump_dir = "dumps", *dump_stem = "dump";
  if (args->extract_switch("-dump-dir"))
    { dump_dir = args->pop_next(); dump_to_stdout = false; }
  if (args->extract_switch("-dump-stem"))
    { dump_stem = args->pop_next(); dump_to_stdout = false; }
  if(dump_to_stdout) {
    dump_target = stdout;
  } else {
    char buf[1000];
    // Ensure that the dump directory exists.  EEXIST is too coarse,
    // but it will do for now.
    if (0 != os_mkdir(dump_dir) && errno != EEXIST)
      uerror("Unable to create dump directory %s", dump_dir);
    snprintf(buf, sizeof buf, "%s/%s.log", dump_dir, dump_stem);
    dump_target = fopen(buf,"w");
    if (dump_target == 0)
      uerror("Unable to open dump file: %s", strerror(errno));
  }

  // Get any platform-specific additional opcodes
  const string &platform
    = args->extract_switch("--platform") ? args->pop_next() : "sim";
  const string &platform_directory
    = ProtoPluginManager::PLATFORM_DIR + "/" + platform + "/";
  read_opfile(platform_directory + ProtoPluginManager::PLATFORM_OPFILE);
  while (args->extract_switch("-L", false)) {
    string layer_name = args->pop_next();
    ensure_extension(layer_name, ".proto");
    read_opfile(platform_directory + layer_name);
  }
}

uint8_t* PaleoCompiler::compile(const char *str, int* len) {
  last_script=str;
  uint8_t* bytes = compile_script(str,len,is_dump_ast);
  if(is_dump_code) dump_instructions(is_json_format,*len,bytes);
  return bytes;
}

void PaleoCompiler::visualize() {
  if(is_show_code) {
    // "compiler should be able to show the code here, but doesn't yet"
  }
}

bool PaleoCompiler::handle_key(KeyEvent* key) {
  if(key->normal && !key->ctrl) {
    switch(key->key) {
    case 'k': is_show_code = !is_show_code; return true;
    }
  }
  return false;
}
