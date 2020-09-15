/* Base class for XML emission kludges
Copyright (C) 2009-2013, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

#ifndef __CHEAPO_XML__
#define __CHEAPO_XML__

#include "biocompiler.h"

/*************** XML CLASSES ***************/
struct XMLitem {
  string name;
  map<string,string> attributes;
  list<XMLitem*> elements;

  XMLitem(string name) { this->name = name; }
  XMLitem(string name,string about) { 
    this->name = name; 
    attributes["rdf:about"] = about;
  }
  void addElt(XMLitem* elt) { elements.push_back(elt); }
  void addEltToFront(XMLitem* elt) { elements.push_front(elt); }
  void linkElt(string relation,XMLitem* elt) {
    XMLitem* rel = new XMLitem(relation);
    rel->addElt(elt); addElt(rel);
  }
  
  void addComment(string comment);
  void addBreak(int n=1);

  virtual void emit_prefix(ostream* out) {
    ierror("XML library misconfigured: header required but not defined");
  }

  virtual void emit(ostream* out, bool root=true) {
    if(root) { emit_prefix(out); }
    *out << pp_indent() << "<" << name << attr2str(); 
    if(elements.empty()) { *out << "/>" << endl;
    } else {
      *out << ">\n";
      pp_push(2);
      list<XMLitem*>::iterator i = elements.begin();
      while(i!=elements.end()) { (*i)->emit(out,false); i++; }
      pp_pop();
      *out << pp_indent() << "</" << name << ">" << endl;
    }
    if(root) {
      *out << "</rdf:RDF>" << endl;
    }
  }
  
  static void smoke_test() {
    XMLitem foo("Foo"), bar("BarItem");
    foo.attributes["water"] = "salt";
    foo.attributes["Element"] = "fire";
    foo.elements.push_back(&bar);
    bar.attributes["country"] = "Bahrain";
    bar.attributes["frobcount"] = i2s(15);
    foo.emit(&cout);
    // should produce:
    // <?xml version="1.0" encoding="ISO-8859-1"?>
    // <Foo Element="fire" water="salt" xmlns="urn:bbn.com:biocompiler:grn:">
    //   <BarItem country="Bahrain" frobcount="15"/>
    // </Foo>
  }


private:
  string attr2str() {
    pp_push(2);
    string s = "";
    map<string,string>::iterator i = attributes.begin();
    while(i!=attributes.end()){s+=" "+(*i).first+"=\""+(*i).second+"\""; i++;}
    pp_pop();
    return s;
  }
};

// a comment
struct XMLcomment : public XMLitem {
  XMLcomment(string contents) : XMLitem(contents) {}
  virtual void emit(ostream* out, bool root=true) {
    *out << pp_indent() << "<!-- " << name << " -->" << endl;
  }
};
// an n-line whitespace
struct XMLbreak : public XMLitem {
  int n;
  XMLbreak(int n=1) : XMLitem("") { this->n = n; }
  virtual void emit(ostream* out, bool root=true) {
    for(int i=0;i<n;i++) { *out << pp_indent() << endl; }
  }
};
// 
struct TrivialItem : public XMLitem {
  string contents;
  TrivialItem(string name, string contents) : XMLitem(name) {
    this->contents = contents;
  }
  
  virtual void emit(ostream* out, bool root=true) {
    *out << pp_indent() << "<" << name << ">" << contents;
    *out << "</" << name << ">" << endl;
  }
};

#endif // __CHEAPO_XML__
