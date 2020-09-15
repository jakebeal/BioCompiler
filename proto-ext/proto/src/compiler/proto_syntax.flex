/* a flex scanner for Proto
Copyright (C) 2005-2008, Jonathan Bachrach, Jacob Beal, and contributors 
listed in the AUTHORS file in the MIT Proto distribution's top directory.

This file is part of MIT Proto, and is distributed under the terms of
the GNU General Public License, with a linking exception, as described
in the file LICENSE in the MIT Proto distribution's top directory.

   Used: alphanumeric, *+-./<=>?_&:
   Special: ;'(),`@|
   Reserved & Used: ~
   Reserved & Unused: !"#$%[\]^{}
*/

%option yylineno

CONSTITUENT   [*+\-./<=>?_&:]
RESERVED     [!"#$%\[\\\]^{}~]
SPECIAL      [;'(),`@|]

%{
#define YY_BREAK { if(cur->error) return 1; } break;
#include <stack>
#include "sexpr.h"
#include "utils.h"
using namespace std;
extern "C" int yywrap() { return 1; }

int yylex();

struct SExprLexer {
  SE_List* base; // the implicit wrapping "all" for compilation
  stack<SE_List*> enclosure; // what SExprs enclose the current parse?
  stack<bool> wraps; // are these SExprs normal or character macro wrappers?
  bool error;
  string name; // name of code source (e.g. file, "command-line")
  
  void compile_error(Attribute* context,string msg) {
    *cperr << context->to_str() << ": " << msg << endl;
    error=true;
  }
  
  string ibuf;
  SExprLexer(string name, istream* in=0, ostream* out=0) {
    error   = false; this->name=name;
    base = new SE_List();  base->add(new SE_Symbol("all"));
    enclosure.push(base); wraps.push(false);
    // setup input stream; output is discarded
    yyout = tmpfile();
    ibuf = "";
    while(in->good()) ibuf+=in->get();
    ibuf[ibuf.size()-1]=0;
    yy_scan_string(ibuf.c_str());
  }

  virtual ~SExprLexer() {} // nothing to clean: SExprs are a problem of others
  
  // returns NULL on error
  SExpr* tokenize() {
    yylex();
    if(enclosure.top()!=base) { 
      compile_error(enclosure.top()->attributes["CONTEXT"],"Missing right parenthesis");
    }
    if(error) { delete base; return NULL; 
    } else if(base->len()==2) { return base->children[1]; // single SEXpr
    } else { return base; }
  }
  
  Context* context() { return new Context(name,yylineno); }
  
  // start a new sexpr, contained within the current context
  void start_compound_sexpr() {
    SE_List *e = new SE_List(); e->attributes["CONTEXT"]=context();
    enclosure.top()->add(e);
    enclosure.push(e); wraps.push(false);
  }

  void end_compound_sexpr() { 
    if(wraps.top() && enclosure.top()->children.size()<=1) { 
      compile_error(enclosure.top()->attributes["CONTEXT"],
                    "Wrapper macro " + enclosure.top()->op()->to_str() + 
		    " is not applied to anything");
    } else if(enclosure.top()==base) { 
      compile_error(context(),"Too many right parentheses");
    } else {
      enclosure.pop(); wraps.pop();
      if(wraps.top()) { // if the next layer is a macro wrapper, finish it too
        end_compound_sexpr();
      }
    }
  }

  // single character macros like ' create wrappers around the next SExpr
  void wrap_next_sexpr(SE_Symbol* symbol) {
    symbol->attributes["CONTEXT"]=context();
    SE_List* s = new SE_List();
    s->add(symbol); enclosure.top()->add(s);
    enclosure.push(s); wraps.push(true);
  }

  void add_sexpr(SExpr* s) { 
    s->attributes["CONTEXT"]=context();
    enclosure.top()->add(s);
    if(wraps.top()) { end_compound_sexpr(); }
  }
};

SExprLexer* cur;

%}


%%

[[:space:]]+	/* consume whitespace */
;.*$		/* consume comments */

"("		cur->start_compound_sexpr();
")"		cur->end_compound_sexpr();

`		cur->wrap_next_sexpr(new SE_Symbol("quasiquote"));
'		cur->wrap_next_sexpr(new SE_Symbol("quote"));
,		cur->wrap_next_sexpr(new SE_Symbol("comma"));
,@		cur->wrap_next_sexpr(new SE_Symbol("comma-splice"));
"|"		cur->add_sexpr(new SE_Symbol("|"));

-?[[:digit:]]+(\.[[:digit:]]*)?(e-?[[:digit:]]+)? |
-?\.[[:digit:]]+(e-?[[:digit:]]+)?		cur->add_sexpr(new SE_Scalar(atof(yytext)));

[I|i]nf		cur->add_sexpr(new SE_Scalar(INFINITY));
-[I|i]nf	cur->add_sexpr(new SE_Scalar(-INFINITY));
NaN		cur->add_sexpr(new SE_Scalar(NAN));
nan		cur->add_sexpr(new SE_Scalar(NAN));

[[:alnum:]*+\-./<=>?_&:]+		 cur->add_sexpr(new SE_Symbol(yytext));

{RESERVED}  cur->compile_error(cur->context(),"Illegal use of reserved character '"+string(yytext)+"'");
[^[:alnum:][:space:]{RESERVED}{CONSTITUENT}{SPECIAL}]           cur->compile_error(cur->context(),"Unrecognized character: '"+string(yytext)+"'");

%%

SExpr* read_sexpr(const string &name, const string &in)
{ return read_sexpr(name,new istringstream(in)); }
SExpr* read_sexpr(const string &name, istream* in, ostream* out) { 
  SExprLexer lex(name,in,out); cur = &lex;
  SExpr* sexp = lex.tokenize();
  yylex_destroy(); // reset state
  return sexp;
}
