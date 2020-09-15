/* GraphViz dot emitter hack
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

// Emit an abstract GRN in SBOLv format
#include "biocompiler.h"
#include "grn_utilities.h"

using namespace grn;

string dotfile_header() {
  string s;
  s += "";
  s += "digraph GeneticRegulatoryNetwork {\n";
  s += "  node [shape=plaintext penwidth=\"2\"]\n";
  s += "  edge [penwidth=\"2\" arrowsize=\"1\"]\n";
  s += "  rankdir = LR\n";
  return s;
}

string dotfile_footer() { return "}\n"; }

string declare_chemical(Chemical* c) {
  return "  " + sanitized_name(c->name) +
    " [width=\"0.1\" height=\"0.1\" margin=\"0.04,0.02\"];\n";
}

string fu_elt_name(DNAComponent* dc) { 
  return "\"" + dc->container->nicename() + "~" + dc->nicename() + "\"";
}

string declare_functional_unit(FunctionalUnit* fu) {
  string features, orderlinks = "    ";
  for(int i=0;i<fu->sequence.size();i++) {
    string name = fu_elt_name(fu->sequence[i]);
    if(fu->sequence[i]->isA("Promoter")) {
      features += "    "+name+" [shape=promoter labelloc=\"b\" label=\"\"];\n";
    } else if(fu->sequence[i]->isA("CodingSequence")) {
      string chemname = sanitized_name(((CodingSequence*)fu->sequence[i])->product->name);
      features += "    "+name+" [shape=cds label=\""+chemname+"\"];\n";
    } else if(fu->sequence[i]->isA("Terminator")) {
      features += "    "+name+" [shape=terminator labelloc=\"b\" label=\"\"];\n";
    }
    if(i>0) orderlinks += " -> ";
    orderlinks += name;
  }
  
  string s;
  s += "  subgraph cluster_" + fu->nicename() + " {\n";
  s += "    color=none;\n";
  s += "    edge [arrowhead=none];\n";
  s += features;
  s += orderlinks + ";\n";
  s += "  };\n";

  return s;
}

string regulation_properties(bool repress) {
  if(repress)
    return "arrowhead=\"tee\" color=\"red\"";
  else
    return "color=\"green\"";
}

string declare_regulation(Chemical* c) {
  string s;
  bool is_repressor = false, is_activator = false;
  
  // First do the consumers, which will determine color of producer edges
  for_set(ExpressionRegulation*,c->consumers,er) {
    if((*er)->repressor) is_repressor = true; else is_activator = true;

    string cname = sanitized_name((*er)->signal->name);
    string pname = fu_elt_name((*er)->target);
    s += "  " + cname + " -> " + pname; // declare edge
    s += " [headport=nw " + regulation_properties((*er)->repressor) + "]\n"; // decorate
  }
  
  // Now create the producer edges
  string color = (is_repressor ? (is_activator ? "darkorange" : "red")
                  : (is_activator ? "green" : "blue"));
  for_set(CodingSequence*,c->producers,pcs) {
    string cname = sanitized_name((*pcs)->product->name);
    string pcsname = fu_elt_name(*pcs);
    s += "  " + pcsname + " -> " + cname; // declare edge
    s += " [tailport=n arrowhead=\"none\" color=\"" + color + "\"]\n";
  }
  
  // Finally, create regulatory reaction edges
  for_set(RegulatoryReaction*,c->regulatorFor,rr) {
    string rname = sanitized_name((*rr)->regulator->name);
    string sname = sanitized_name((*rr)->substrate->name);
    s += "  " + rname + " -> " + sname; // declare edge
    s += " [" + regulation_properties((*rr)->repressor) + "]\n"; // decorate
  }

  return s;
}

/*************** ENTRY POINT ***************/
void GRNEmitter::to_dot(ostream* out) {
  // Declare the graph and header information
  *out << "// Genetic regulatory network for Proto: "
    + string(parent->last_script) << endl;
  *out << "// Produced by BioCompiler version " << BIOCOMPILER_VERSION << endl;
  *out << "// Command to make graph:  dot -Tsvg [name].dot > [name].svg" <<endl;
  *out << dotfile_header() << endl;

  // Declare Chemicals
  for_map(string, Chemical*, grn.chemicals, i)
    { *out << declare_chemical(i->second); }
  *out << endl;

  // Declare FunctionalUnits
  for_set(DNAComponent*, grn.dnacomponents, i) 
    { *out << declare_functional_unit((FunctionalUnit*)*i); }
  *out << endl;
  
  // Declare production & regulation relations
  for_map(string, Chemical*, grn.chemicals, i)
    { *out << declare_regulation(i->second); }
   *out << endl;
  
  // Close the graph
  *out << dotfile_footer(); *out << endl;

  *out << flush;
}
