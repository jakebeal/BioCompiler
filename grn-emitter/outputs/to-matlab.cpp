/* Matlab model file emission
Copyright (C) 2009-2011, Raytheon BBN Technologies and contributors listed 
in the AUTHORS file in TASBE BioCompiler distribution's top directory.

This file is part of the TASBE BioCompiler, and is distributed under
the terms of the GNU General Public License, with a linking exception,
as described in the file LICENSE in the TASBE BioCompiler
distribution's top directory. */

// Emit an abstract GRN in SBOL format
#include "biocompiler.h"
#include "grn_utilities.h"

using namespace grn;

/* Names:
 Per chemical:
   Signal concentration: Chem, Chem_eff for regulated chems 
   Signal change: dChem
   Hill coefficient: H_Chem
   Halflife: t_Chem
 Per functional unit:
   RR strength: K_Chem[consumer#]
   RR dissociation constant: D_Chem[consumer#]
   Base transcriptional rate: R_[FU#]
 */

void generate_matlab_equations(GRNEmitter* emitter, ostream* vars, ostream* eqns,vector<string> *paramnames,vector<string> *varnames) {

  map<Chemical*,string,CompilationElement_cmp> productions; // string grows w. each addition
  
  int n=0;
  for(set<DNAComponent*>::iterator i=emitter->grn.dnacomponents.begin();i!=emitter->grn.dnacomponents.end();i++) {
    n++;
    FunctionalUnit* f = (FunctionalUnit*)(*i);
    string rate = "";
    for(int j=0;j<f->sequence.size();j++) {
      DNAComponent* dc = f->sequence[j];
      if(dc->isA("Promoter")) { 
        string R = "R_"+i2s(n); paramnames->push_back(R);
        *vars << R << " = " << f2s(force_promoter_rate((Promoter*)dc),5) << ";\n";
        rate += R;
      }
      for_set(ExpressionRegulation*,dc->regulators,er) {
        string C = sanitized_name((*er)->signal->name); // record later
        string H = ("H_"+C); // chemical parameter; record later
        string K = ("K_"+C); paramnames->push_back(K);
        string D = ("D_"+C); paramnames->push_back(D);
        if((*er)->signal->regulatedBy.size()) C += "_eff";
        // Note that multi-consumer variables currently assume no sequestration
        *vars << K << " = " << f2s((*er)->strength) << ";\n";
        *vars << D << " = " << f2s((*er)->dissociation) << ";\n";
        rate += ".*(1+"+K+((*er)->repressor?".^(-1)":"")+".*("+C+"./"+D+").^"+H+")./(1+("+C+"./"+D+").^"+H+")";
        //} else if(dc->isA("RBS")) { // ignore
      }
      if(dc->isA("Promoter")) {
        // already handled
      } else if(dc->isA("CodingSequence")) {
        Chemical* c = ((CodingSequence*)dc)->product;
        if(productions[c]!="") productions[c] += " + ";
        productions[c] += rate;
      } else if(dc->isA("Terminator")) { // ignore
      } else {
        ierror("Don't know how to output to Matlab: "+dc->to_str());
      }
    }
  }

  set<RegulatoryReaction*>::iterator ri=emitter->grn.reactions.begin();
  for( ;ri!=emitter->grn.reactions.end();ri++) {
    string Cr = sanitized_name((*ri)->regulator->name), Cs = sanitized_name((*ri)->substrate->name);
    if((*ri)->substrate->regulatedBy.size()>1)
      compile_warn("Multiply regulated chemicals are not currently modeled in Matlab: model will be incorrect");
    string Cs_eff = (Cs+"_eff"); // derived name: doesn't need recording
    string H = ("H_"+Cr); // chemical parameter; record later
    string K = ("K_"+Cr+Cs); paramnames->push_back(K);
    string D = ("D_"+Cr+Cs); paramnames->push_back(D);
    *vars << K << " = " << f2s(1.0) << ";\n";
    *vars << D << " = " << f2s(DEFAULT_DISSOCIATION) << ";\n";
    string eff_eqn = Cs+"*"+K+"*("+((*ri)->repressor?"":"1-")+
      "(1/(1+("+Cr+"./"+D+").^"+H+")));\n";
    *eqns << Cs_eff << " = " << eff_eqn;
  }

  map<string,Chemical*>::iterator i=emitter->grn.chemicals.begin();
  for(;i!=emitter->grn.chemicals.end();i++) {
    Chemical* c = (*i).second; string CN = sanitized_name(c->name);
    varnames->push_back(CN);
    string dC = ("d"+CN); // derived name: doesn't need recording
    string H = "H_"+CN; paramnames->push_back(H);
    string half = "t_"+(CN); paramnames->push_back(half);
    *vars << H << " = " << c->hill_coefficient << ";\n";
    *vars << half << " = " << c->halflife << ";\n";
    *eqns << dC << " = " << productions[c] << " - log(2)./" << half
          << ".*" << CN << ";\n";
  }
}


void print_matlab_ode_file(GRNEmitter* emitter, string& script, ostream* out, string eqns, string network,vector<string> *paramnames,vector<string> *varnames) {

  vector<pair<string,int> > inputvar; // chem name, index in output
  for(int i=0;i<varnames->size();i++) {
    Chemical* c = emitter->grn.chemicals[(*varnames)[i]];
    if(c && c->attributes[":motif-constant"]) {
      pair<string,int> entry = make_pair((*varnames)[i],i+1);
      if(c->producers.size()==0) inputvar.push_back(entry);
    }
  }

  *out<<"function dy = " << emitter->circuit_name << "(t, y)" << endl;
  *out<<"% Genetic circuit for Proto: " << script << endl;
  *out<<"% Ordinary differential equations describing the evolution of"<<endl;
  *out<<"% a genetic circuit.  Variable t is the current time (in "<<endl;
  *out<<"% y is the current concentrations of species (nanomolar)"<<endl;
  *out<<"% Function is designed to be called from accompanying file"<<endl;
  *out<<"% "<<emitter->circuit_name<<"_system.m, which defines constants."<<endl;
  *out<<"%{"<<endl;
  *out<<"Circuit equations represent network: "<<endl;
  *out<<network;
  *out<<"%}"<<endl;
  *out<<endl;
  *out<<"% make sure system parameters are accessible"<<endl;
  *out<<"global";
  for(int i=0;i<paramnames->size();i++) *out<<" "<<(*paramnames)[i];
  *out<<";"<<endl;
  *out<<"global input_values n_stages;"<<endl;
  *out<<endl;
  *out<<"% unpack y into named concentrations"<<endl;
  for(int i=0;i<varnames->size();i++)
    { *out<<(*varnames)[i]<<" = y("<<(i+1)<<");"<<endl; }
  *out<<endl;
  *out<<"% compute derivative of concentrations"<<endl;
  *out<<eqns;
  *out<<endl;
  *out<<"% drive inputs to match specified value waveform"<<endl;
  *out<<"stage = floor(t/input_values.stage_length)+1;"<<endl;
  *out<<"if(stage>n_stages), stage=n_stages; end;"<<endl;
  for(int i=0;i<inputvar.size();i++) {
    string vname = inputvar[i].first;
    *out<<"d"<<vname<<" = d"<<vname<<" + 0.01*((1+499*input_values."<<vname
        <<"(stage))-"<<vname<<");"<<endl;
  }
  *out<<endl;
  *out<<"% pack derivatives into dy column vector"<<endl;
  *out<<"dy = ["; 
  for(int i=0;i<varnames->size();i++)
    { if(i) { *out<<"; "; } *out<<"d"<<(*varnames)[i]; }
  *out<<"];"<<endl;
}


void print_matlab_system_file(GRNEmitter* emitter, string& script, ostream* out,string var,vector<string> *paramnames,vector<string> *varnames) {
  int layers = 1+estimate_diameter(&emitter->grn);

  // Assume motif-constant chemicals are the ones of interest
  vector<pair<string,int> > inputvar; // chem name, index in output
  vector<pair<string,int> > outputvar; // chem name, index in output
  vector<pair<string,int> > figvar; // chem name, index in output
  for(int i=0;i<varnames->size();i++) {
    Chemical* c = emitter->grn.chemicals[(*varnames)[i]];
    if(c && c->attributes[":motif-constant"]) {
      pair<string,int> entry = make_pair((*varnames)[i],i+1);
      if(emitter->plot_all_motif_constants && !emitter->plot_all_chemicals) 
        figvar.push_back(entry);
      if(c->consumers.size()==0 && c->regulatorFor.size()==0) outputvar.push_back(entry);
      if(c->producers.size()==0) inputvar.push_back(entry);
    }
  }
  if(emitter->plot_all_chemicals) {
    for(int i=0;i<varnames->size();i++) {
      Chemical* c = emitter->grn.chemicals[(*varnames)[i]];
      if(c) figvar.push_back(make_pair((*varnames)[i],i+1));
    }
  } else if(!emitter->plot_all_motif_constants) {
    for(int i=0;i<inputvar.size();i++) figvar.push_back(inputvar[i]);
    for(int i=0;i<outputvar.size();i++) figvar.push_back(outputvar[i]);
  }

  // Header
  *out<<"function [output_values,T,Y] = "<< emitter->circuit_name<<"_system(input_waveform,plot_figures)"<<endl;
  *out<<"% Simulates genetic circuit for Proto: "<<script<<endl;
  *out<<"% 1. Global parameters are set at the top"<<endl;
  *out<<"% 2. Simulation runs for interval tspan = [initial,final]"<<endl;
  *out<<"%    interval [will be] computed from propagation delay"<<endl;
  *out<<"% 3. Simulation length and inputs may be set by optional input_waveform argument"<<endl;
  *out<<"% 4. Plots are (optionally) made for each sensor & actuator"<<endl;
  // Process arguments
  *out<<endl;
  *out<<"v = ver(); is_octave = strcmpi(v(1).Name,'octave');"<<endl;
  *out<<endl;
  *out<<"if nargin < 2"<<endl;
  *out<<"  plot_figures=1;"<<endl;
  *out<<"end"<<endl;
  *out<<endl;
  *out<<"% make sure system parameters are shared"<<endl;
  *out<<"global";
  for(int i=0;i<paramnames->size();i++) *out<<" "<<(*paramnames)[i];
  *out<<";"<<endl;
  *out<<"global input_values n_stages;"<<endl;
  *out<<"% declare parameter values"<<endl;
  *out<<var;
  *out<<endl;
  *out<<"% system initial condition, packed into y_0"<<endl;
  for(int i=0;i<varnames->size();i++) { *out<<(*varnames)[i]<<"_0 = 0;"<<endl; }
  *out<<"y_0 = [";
  for(int i=0;i<varnames->size();i++)
    { if(i) { *out<<"; "; } *out<<(*varnames)[i]<<"_0"; }
  *out<<"];"<<endl;
  *out<<endl;
  *out<<"% simulation time interval (propagation delay)"<<endl;
  *out<<"if nargin > 0"<<endl;
  *out<<"    input_values = input_waveform;"<<endl;
  if(inputvar.size()>0) {
    *out<<"    n_stages = numel(input_waveform."<<inputvar[0].first<<");"<<endl;
  } else {
    *out<<"    n_stages = 1;"<<endl;
  }
  *out<<"else % choose defaults"<<endl;
  *out<<"    n_stages = 1;"<<endl;
  *out<<"    input_values.stage_length = "<<DEFAULT_LAYERTIME<<" * "<<layers<<";"<<endl;
  *out<<"    input_values.time_step = 1e3;"<<endl;
  for(int i=0;i<inputvar.size();i++) {
    string vname = inputvar[i].first;
    *out<<"    input_values."<<vname<<" = "<<vname<<"_0;"<<endl;
  }
  *out<<"end"<<endl;
  *out<<"endtime = input_values.stage_length*n_stages;"<<endl;
  *out<<"tspan = 0:input_values.time_step:endtime;"<<endl;
  *out<<endl;
  *out<<"% Run ODE numerical integrator"<<endl;
  *out<<"if is_octave, ode_tspan = [0 endtime]; else ode_tspan = tspan; end;"<<endl;
  // Might prefer to use stiff solver ode15s, but Octave does not
  // have it, so stick with ode23s instead
  *out<<"[Ttmp,Ytmp] = ode23s(@"<<emitter->circuit_name<<",ode_tspan,y_0,odeset('MaxStep',input_values.time_step));"<<endl;
  *out<<"Y = interp1 (Ttmp, Ytmp, tspan)';"<<endl;
  *out<<"T = tspan;"<<endl;
  *out<<endl;
  *out<<endl;
  *out<<"% Make plots"<<endl;
  *out<<"if plot_figures"<<endl;
  *out<<"  h = figure();"<<endl;
  *out<<"  set(h,'visible','off');"<<endl;
  int n = figvar.size();
  int fx = ceil(sqrt(n)); int fy = (fx*(fx-1)<n) ? fx : fx-1; // # of subfigures
  for(int i=0;i<n;i++) {
    if(i) *out<<endl;
    *out<<"  subplot("<<fx<<","<<fy<<","<<(i+1)<<");"<<endl;
    *out<<"  semilogy(T,Y(:,"<<figvar[i].second<<"),'b-');"<<endl;
    *out<<"  axis([min(T),max(T),1e0,1e3]);"<<endl;
    *out<<"  xlabel('Time (s)'); ylabel('["<<figvar[i].first<<"] (nM)');"<<endl;
  }
  *out<<endl;
  *out<<"  if (is_octave)"<<endl;
  *out<<"    FN = findall(h,'-property','FontName');"<<endl;
  *out<<"    set(FN,'FontName','Helvetica');"<<endl;
  *out<<"    print(h,'-dpdfwrite','"<<emitter->circuit_name<<"_simulation');"<<endl;
  *out<<"  else"<<endl;
  *out<<"    print(h,'-depsc2','"<<emitter->circuit_name<<"_simulation');"<<endl;
  *out<<"  end"<<endl;
  *out<<"  print(h,'-dpng','"<<emitter->circuit_name<<"_simulation');"<<endl;
  *out<<"  saveas(h,'"<<emitter->circuit_name<<"_simulation.fig');"<<endl;
  *out<<"end"<<endl;
  *out<<endl;
  *out<<"% Note: assumption that time_step divides evently into stage_length"<<endl;
  *out<<"phase_ends = find(mod(tspan(2:end),input_values.stage_length)==0)-1;"<<endl;
  for(int i=0;i<outputvar.size();i++) {
    *out<<"output_values."<<outputvar[i].first<<" = Y(phase_ends,"<<outputvar[i].second<<")>1e2;"<<endl;
  }
}

void print_matlab_simulation_file(GRNEmitter* emitter, string& script, ostream* out,string var,vector<string> *paramnames,vector<string> *varnames) {
  int layers = 1+estimate_diameter(&emitter->grn);
  
  // Assume motif-constant chemicals are the ones of interest
  vector<pair<string,int> > inputvar; // chem name, index in output
  vector<pair<string,int> > outputvar; // chem name, index in output
  for(int i=0;i<varnames->size();i++) {
    Chemical* c = emitter->grn.chemicals[(*varnames)[i]];
    if(c && c->attributes[":motif-constant"]) {
      pair<string,int> entry = make_pair((*varnames)[i],i+1);
      if(c->consumers.size()==0 && c->regulatorFor.size()==0) outputvar.push_back(entry);
      if(c->producers.size()==0) inputvar.push_back(entry);
    }
  }

  // Simulation explanatory comments
  *out<<"% Simulates genetic circuit for Proto: "<<script<<endl;
  *out<<"% with all combinations of binary inputs"<<endl;
  *out<<endl;
  *out<<"v = ver(); is_octave = strcmpi(v(1).Name,'octave');"<<endl;
  *out<<"if is_octave"<<endl;
  *out<<"  pkg load odepkg"<<endl;
  *out<<"end"<<endl;
  *out<<endl;
  // Setup of simulation
  *out<<"input_waveform.stage_length = "<<DEFAULT_LAYERTIME<<" * "<<layers<<";"<<endl;
  *out<<"input_waveform.time_step = 1e3;"<<endl;
  int numstages = 1<<inputvar.size();
  for(int i=0;i<inputvar.size();i++) {
    *out<<"input_waveform."<<inputvar[i].first<<" = [";
    for(int j=0;j<numstages;j++) {
      if(j>0) *out<<" ";
      *out<<i2s(!(0==(j&(1<<i))));
    }
    *out<<"];"<<endl;
  }
  // Execute simulation
  *out<<"[out,T,Y] = "<<emitter->circuit_name<<"_system(input_waveform);"<<endl;
  *out<<endl;
  // Output CSV file with full timeseries
  *out<<"f = fopen('"<<emitter->circuit_name<<"_timeseries.csv','w');"<<endl;
  *out<<"fprintf(f,'Time";
  for(int i=0;i<varnames->size();i++) { *out<<","<<(*varnames)[i]; }
  *out<<"\\n');"<<endl;
  *out<<"for i=1:numel(T),"<<endl;
  *out<<"  fprintf(f,'%.2f";
  for(int i=0;i<varnames->size();i++) { *out<<",%.2f"; }
  *out<<"\\n',T(i)";
  for(int i=0;i<varnames->size();i++) { *out<<",Y(i,"<<(i+1)<<")"; }
  *out<<");"<<endl;
  *out<<"end"<<endl;
  *out<<"fclose(f);"<<endl;
  *out<<endl;
  // Output CSV file with truth table only
  *out<<"f = fopen('"<<emitter->circuit_name<<"_truth_table.csv','w');"<<endl;
  *out<<"fprintf(f,'";
  bool first = true;
  for(int i=0;i<inputvar.size();i++) { 
    if(!first) { *out<<","; } else { first=false; }
    *out<<inputvar[i].first; 
  }
  for(int i=0;i<outputvar.size();i++) { 
    if(!first) { *out<<","; } else { first=false; }
    *out<<outputvar[i].first; 
  }
  if(first) *out<<"No input or output variables"<<endl;
  *out<<"\\n');"<<endl;
  *out<<"for i=1:"<<numstages<<endl;
  *out<<"  fprintf(f,'";
  first = true;
  for(int i=0;i<inputvar.size()+outputvar.size();i++) {
    if(!first) { *out<<","; } else { first=false; }
    *out<<"%i";
  }
  *out<<"\\n'";
  for(int i=0;i<inputvar.size();i++) 
    { *out<<",input_waveform."<<inputvar[i].first<<"(i)"; }
  for(int i=0;i<outputvar.size();i++) 
    { *out<<",out."<<outputvar[i].first<<"(i)"; }
  *out<<");"<<endl;
  *out<<"end"<<endl;
  *out<<"fclose(f);"<<endl;
}

void GRNEmitter::to_matlab(string net_description) {
  string script = string(parent->last_script);
  ostringstream var, eqn;
  vector<string> paramnames,varnames;
  generate_matlab_equations(this, &var, &eqn,&paramnames,&varnames);
  ostream *circuit,*system,*sim;
  if(emit_to_stdout) {
    circuit = system = sim = cpout;
  } else {
    ofstream *circuitf = new ofstream((outstem+".m").c_str());
    ofstream *systemf = new ofstream((outstem+"_system.m").c_str());
    ofstream *simf = new ofstream((outstem+"_sim.m").c_str());
    if(!circuitf->is_open()||!systemf->is_open() || !simf->is_open())
      {compile_error("Can't open matlab output files");terminate_on_error();}
    else
      {circuit = circuitf; system = systemf;sim=simf;}
  }
  print_matlab_ode_file(this,script,circuit,eqn.str(),net_description,&paramnames,&varnames);
  print_matlab_system_file(this,script,system,var.str(),&paramnames,&varnames);
  print_matlab_simulation_file(this,script,sim,var.str(),&paramnames,&varnames);
  if(!emit_to_stdout) { delete circuit; delete system; }
}

