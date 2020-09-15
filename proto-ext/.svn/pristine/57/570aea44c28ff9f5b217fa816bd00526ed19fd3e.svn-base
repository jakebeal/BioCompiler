/* Proto compiler
Copyright (C) 2009, Jacob Beal, and contributors
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory. */

// During experimental development, the NeoCompiler will be worked on
// "off to the side" of the original, not replacing it.

#ifndef PROTO_COMPILER_NEOCOMPILER_H
#define PROTO_COMPILER_NEOCOMPILER_H

#include <stdint.h>

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "config.h"

#include "analyzer.h"
#include "ir.h"
#include "reader.h"
#include "utils.h"

struct ProtoInterpreter; class NeoCompiler;

/*****************************************************************************
 *  INTERPRETER                                                              *
 *****************************************************************************/

/**
 * Binding environments: tracks name/object associations during interpretation
 */
struct Env {
  Env* parent; ProtoInterpreter* cp;
  std::map<std::string,CompilationElement*> bindings;

  Env(ProtoInterpreter* cp) { parent=NULL; this->cp = cp; }
  Env(Env* parent) { this->parent=parent; cp = parent->cp; }
  void bind(std::string name, CompilationElement* value);
  void force_bind(std::string name, CompilationElement* value);

  /**
   * Lookups: w/o type, returns NULL on failure; w. type, checks, returns dummy
   */
  CompilationElement* lookup(std::string name, bool recursed=false);
  CompilationElement* lookup(SE_Symbol* sym, std::string type);

  /**
   * Operators needed to be accessed unshadowed by compiler. These are gathered
   * after initialization, but before user code is loaded.
   */
  static std::map<std::string,Operator*> core_ops;
  static void record_core_ops(Env* toplevel);
  static Operator* core_op(std::string name);
};

class ProtoInterpreter {
 public:
  friend class Env;
  Env* toplevel;
  AM* allspace;
  DFG* dfg;
  NeoCompiler* parent;
  int verbosity;

  ProtoInterpreter(NeoCompiler* parent, Args* args);
  ~ProtoInterpreter();

  void interpret(SExpr* sexpr);
  static bool sexp_is_type(SExpr* s);
  static ProtoType* sexp_to_type(SExpr* s);
  static Signature* sexp_to_sig(SExpr* s,Env* bindloc=NULL,CompoundOp* op=NULL,AM* space=NULL);
  static std::pair<std::string,ProtoType*> parse_argument(SE_List_iter* i, int n, Signature* sig, bool anonymous_ok=true);

 private:
  /**
   * FOR INTERNAL USE ONLY
   */
  void interpret(SExpr* sexpr, bool recursed);
  void interpret_file(std::string name);

  Operator* sexp_to_op(SExpr* s, Env *env);
  Macro* sexp_to_macro(SE_List* s, Env *env);
  Signature* sexp_to_macro_sig(SExpr* s);
  Field* sexp_to_graph(SExpr* s, AM* space, Env *env);
  SExpr* expand_macro(MacroOperator* m, SE_List* call);
  SExpr* macro_substitute(SExpr* src,Env* e,SE_List* wrapper=NULL);
  ProtoType* symbolic_literal(std::string name);

  /// compiler special-form handlers
  Field* let_to_graph(SE_List* s, AM* space, Env *env,bool incremental);
  /// compiler special-form handlers
  Field* letfed_to_graph(SE_List* s, AM* space, Env *env,bool init);
  /// compiler special-form handlers
  Field* restrict_to_graph(SE_List* s, AM* space, Env *env);
};

/**
 * RULE-BASED GRAPH TRANSFORMATION
 */
class DFGTransformer {
 public:
  std::vector<IRPropagator*> rules;
  bool paranoid;
  int max_loops, verbosity;

  DFGTransformer() {}
  ~DFGTransformer() {}

  virtual void transform(DFG* g);
};

class ProtoAnalyzer : public DFGTransformer {
 public:
  ProtoAnalyzer(NeoCompiler* parent, Args* args);
  ~ProtoAnalyzer() {}
};

class GlobalToLocal : public DFGTransformer {
 public:
  GlobalToLocal(NeoCompiler* parent, Args* args);
  ~GlobalToLocal() {}
};

/**
 * CODE EMITTER.
 *  this is a framework class: we'll have multiple instantiations
 */
class CodeEmitter { public: reflection_base(CodeEmitter);
  public:
    /**
     * If an emitter wants to change some of the core operators, it must
     * call Env::record_core_ops(parent->interpreter->toplevel)
     * after it has loaded its modified definitions
     */
    virtual uint8_t *emit_from(DFG *g, int *len) = 0;

    virtual void print(std::ostream *out = cpout) { *cpout << "CodeEmitter"; }
};

class InstructionPropagator;
class Instruction;
class Block;

class ProtoKernelEmitter : public CodeEmitter { public: reflection_sub(ProtoKernelEmitter, CodeEmitter);
 public:
  bool is_dump_hex, paranoid;
  static bool op_debug;
  int max_loops, verbosity, print_compact;
  NeoCompiler *parent;

  ProtoKernelEmitter(NeoCompiler *parent, Args *args);
  void init_standalone(Args *args);
  uint8_t *emit_from(DFG *g, int *len);
  void setDefops(const std::string &defops);
  virtual void print(std::ostream *out = cpout) { *cpout << "ProtoKernelEmitter"; }

  /// Map of compound ops -> instructions (in global mem).
  std::map<CompoundOp *, Block *> globalNameMap;
  /// Map of dchange OI -> InitFeedback instruction
  ///  Used to get the proper init feedback to match with a feedback op
  std::map<OI*, Instruction*> dchangeMap;
  /// Map of dchange OI -> "read" Reference
  /// Used to fix the read reference to be the init feedback's let, since the read is resolved first
  std::map<OI*, std::set<Instruction*> > dchangeReadMap;

 private:
  std::vector<InstructionPropagator *> rules;
  std::vector<IRPropagator *> preemitter_rules;

  /// Global & env storage.
  std::map<Field *,CompilationElement *, CompilationElement_cmp> memory;

  /// Fragments floating up to find a home.
  std::map<OI *,CompilationElement *, CompilationElement_cmp> fragments;

  /// List of scalar/vector ops.
  std::map<std::string, std::pair<int, int> > sv_ops;

  /// List of folding ops, which are also scalar/vector pairs.
  std::map<std::string, std::pair<int, int> > fold_ops;

  /// List of Feedback ops, dchange, delay
  std::map<std::string, std::pair<int, int> > feedback_ops;

  Instruction *start, *end;

  void load_ops(const std::string &name);
  void read_extension_ops(std::istream *stream);
  void load_extension_ops(const std::string &name);
  void process_extension_ops(SExpr *sexpr);
  void process_extension_op(SExpr *sexpr);
  Instruction *tree2instructions(Field *f);
  Instruction *primitive_to_instruction(OperatorInstance *oi);
  Instruction *standard_primitive_instruction(OperatorInstance *oi);
  Instruction *vector_primitive_instruction(OperatorInstance *oi);
  Instruction *fold_primitive_instruction(OperatorInstance *oi);
  Instruction *init_feedback_instruction(OperatorInstance *oi);
  Instruction *ref_instruction(OperatorInstance *oi);
  OperatorInstance *find_dchange(OperatorInstance *oi);
  Instruction *let_instruction(OperatorInstance *oi);
  Instruction *feedback_instruction(OperatorInstance *oi);
  Instruction *divide_primitive_instruction(OperatorInstance *oi);
  Instruction *tuple_primitive_instruction(OperatorInstance *oi);
  Instruction *branch_primitive_instruction(OperatorInstance *oi);
  Instruction *literal_to_instruction(ProtoType *l, OperatorInstance *context);
  Instruction *scalar_literal_instruction(ProtoScalar *scalar);
  Instruction *integer_literal_instruction(unsigned int value);
  Instruction *float_literal_instruction(float value);
  Instruction *tuple_literal_instruction(ProtoTuple *tuple,
      OperatorInstance *context);
  Instruction *lambda_literal_instruction(ProtoLambda *lambda,
      OperatorInstance *context);
  Instruction *parameter_to_instruction(Parameter *param);
  Instruction *dfg2instructions(AM *g);

  /// allocates globals for vector ops
  Instruction *vec_op_store(ProtoType *t);
};

/*****************************************************************************
 *  TOP-LEVEL COMPILER                                                       *
 *****************************************************************************/

/**
 * Interface class (shared w. PaleoCompiler)
 */
class Compiler : public EventConsumer {
 public:
  Compiler(Args* args) {}
  ~Compiler() {}
  // setup output files as standalone app
  virtual void init_standalone(Args* args) = 0;
  // compile expression str; len is filled in w. output length
  virtual uint8_t* compile(const char *str, int* len) = 0;
  virtual void set_platform(const std::string &path) = 0;
  virtual void setDefops(const std::string &defops) = 0;
};

/**
 * NeoCompiler implementation
 */
class NeoCompiler : public Compiler {
 public:
  Path proto_path;
  bool is_dump_code, is_dump_all, is_dump_analyzed, is_dump_interpreted,
    is_dump_raw_localized, is_dump_localized, is_dump_dotfiles,is_dotfields;
  std::string dotstem;
  int is_early_terminate;
  bool paranoid; int verbosity;
  std::string infile;
  ProtoInterpreter* interpreter;
  DFGTransformer *analyzer, *localizer;
  CodeEmitter* emitter;
  /// the last piece of text fed to start the compiler
  const char* last_script;

 public:
  NeoCompiler(Args* args);
  ~NeoCompiler();
  void init_standalone(Args* args);

  uint8_t* compile(const char *str, int* len);
  void set_platform(const std::string &path);
  void setDefops(const std::string &defops);
};

/// list of internal tests:
void type_system_tests();

#endif // PROTO_COMPILER_NEOCOMPILER_H
