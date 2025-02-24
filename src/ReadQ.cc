/*****************************************************************************/
/*    This file is part of RAReQS.                                           */
/*                                                                           */
/*    rareqs is free software: you can redistribute it and/or modify         */
/*    it under the terms of the GNU General Public License as published by   */
/*    the Free Software Foundation, either version 3 of the License, or      */
/*    (at your option) any later version.                                    */
/*                                                                           */
/*    rareqs is distributed in the hope that it will be useful,              */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of         */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          */
/*    GNU General Public License for more details.                           */
/*                                                                           */
/*    You should have received a copy of the GNU General Public License      */
/*    along with rareqs.  If not, see <http://www.gnu.org/licenses/>.        */
/*****************************************************************************/
/*
 * File:   Read2Q.cc
 * Author: mikolas
 *
 * Created on Tue Jan 11 15:08:14
 */
#include "ReadQ.hh"
#include "ReadException.hh"
#include <stdio.h>
#include <algorithm>
#include "parse_utils.hh"
using std::max;
using SATSPC::mkLit;

ReadQ::ReadQ(Reader& r, bool _qube_input)
: unsatisfy( false )
, r(r) 
, qube_input(_qube_input)
, qube_output(-1)
, max_id(0)
, _header_read(false)
{}

ReadQ::~ReadQ() {
}

bool ReadQ::get_header_read() const  {return _header_read;}
Var ReadQ::get_max_id() const {return max_id;}
const vector<Quantification>& ReadQ::get_prefix() const {return quantifications;}
const vector<LitSet>& ReadQ::get_clauses() const          {return clause_vector;}
const vector<double>& ReadQ::get_prob() const { return probs; }
const vector<double>& ReadQ::get_thres() const { return thres; }
int ReadQ::get_qube_output() const { assert(qube_input); return qube_output; }

void ReadQ::read_cnf_clause(Reader& in, vector<Lit>& lits) {
  int parsed_lit;
  lits.clear();
  for (;;){
      skipWhitespace(in);
      parsed_lit = parse_lit(in);
      if (parsed_lit == 0) break;
      const Var v = abs(parsed_lit);
      max_id = max(max_id, v);
      if (!contains(quantified_variables,v)) unquantified_variables.insert(v);
      lits.push_back(parsed_lit>0 ? mkLit(v): ~mkLit(v));
  }
}

void ReadQ::read_quantification(Reader& in, Quantification& quantification)  {
  const char qchar = *in;
  ++in;
  if (qchar == 'a') quantification.first=UNIVERSAL;
  else if (qchar == 'e') quantification.first=EXISTENTIAL;
  else if (qchar == 'r') quantification.first=RANDOM; // Perry
  else if (qchar == 't') quantification.first=THRESHOLD;
  else throw ReadException("unexpected quantifier");

  // [Perry] Read prob
  float prob = -1;
  if (qchar == 'r' || qchar == 't') {
    skipWhitespace(in);
    prob = parseFloat(in);
    if (prob > 1 || prob < 0 && prob!=-1) 
      throw ReadException("Probability neither between 0 and 1 nor -1");
    ++in;
    if(qchar == 't'){
      thres.push_back(prob);
      skipLine(in);
      return;
    }
  }

  vector<Var> variables;
  while (true) {
    skipWhitespace(in);
    if (*in==EOF) throw ReadException("quantifier not terminated by 0");
    const Var v = parse_variable(in);
    if (v==0) {
      do { skipLine(in); } while (*in=='c');
      if (*in!=qchar) break;
      
      // [Perry] Read new prob
      if (*in == 'r') {
        ++in;
        skipWhitespace(in);
        prob = parseFloat(in);
        if (prob > 1 || prob < 0 && prob!=-1) 
          throw ReadException("Probability neither between 0 and 1 nor -1");
      }

      ++in;
    } else {
      max_id = max(max_id,v);
      
      // [Perry] Store prob of variable
      if (max_id >= (int)probs.size()) probs.resize(max_id+1, -1);
      probs[v] = prob;

      variables.push_back(v);
      quantified_variables.insert(v);
    }
  }
  quantification.second=VarVector(variables);
  thres.push_back(-1);
}

Var ReadQ::parse_variable(Reader& in) {
    if (*in < '0' || *in > '9') {
       string s("unexpected char in place of a variable: ");  
       s+=*in;
       throw ReadException(s);
    }
    Var return_value = 0;
    while (*in >= '0' && *in <= '9') {
        return_value = return_value*10 + (*in - '0');
        ++in;
    }
    return return_value;
}

int ReadQ::parse_lit(Reader& in) {
    Var  return_value = 0;
    bool neg = false;
    if (*in=='-') { neg=true; ++in; }
    else if (*in == '+') ++in;
    if ((*in < '0') || (*in > '9')) {
        string s("unexpected char in place of a literal: ");  s+=*in;
        throw ReadException(s);
    }
    while (*in >= '0' && *in <= '9') {
        return_value = return_value*10 + (*in - '0');
        ++in;
    }
    if (neg) return_value=-return_value;
    return return_value;
}


void  ReadQ::read_header() {
    while (*r == 'c') skipLine(r);      
    if (*r == 'p') {
      _header_read = true;
      skipLine(r);
    }
}

void  ReadQ::read_quantifiers() {
  for (;;){
    if (*r == 'c') {
      skipLine(r);
      continue;
    }      
    if (*r != 'e' && *r != 'a' && *r != 'r' && *r != 't') break;
    Quantification quantification;
    read_quantification(r, quantification);
    quantifications.push_back(quantification);
  }
}

void ReadQ::read_clauses() {
  Reader& in=r;  
  vector<Lit> ls;
  for (;;){
    skipWhitespace(in);
    if (*in == EOF) break;
    if (*r == 'c') {
      skipLine(in);
      continue;
    }      
    ls.clear();
    read_cnf_clause(in, ls);
    if( ls.empty() ) {
		unsatisfy = true;
    	clause_vector.push_back(LitSet(ls));
		continue;
	}
	bool alwaysSatisfied = false;
	for( size_t i = 0; i < ls.size() - 1; ++i ) for( size_t j = i+1; j < ls.size(); ++j ) if(ls[i] == ~ls[j]) alwaysSatisfied = true;
    if( !alwaysSatisfied ) clause_vector.push_back(LitSet(ls));
  }
}


void ReadQ::read_word(const char* word, size_t length) {
  while (length) {
    if (word[0] != *r) {
        string s("unexpected char in place of: ");  s+=word[0];
        throw ReadException(s);
    }
    ++r; 
    --length;
    ++word;
  }
}

// compact the prefix by removing thrshold quantiication
void ReadQ::compact_prefix(){
  double thr_prob = 0;
  bool   record_thres = false;
  size_t moving_offset = 0;
  for(size_t i=0; i<quantifications.size(); ++i){
    Quantification q = quantifications[i];
    if (q.first == THRESHOLD){
      assert(q.second.empty());
      assert( 0 <= thres[i] && thres[i] <= 1);
      thr_prob = thres[i];
      record_thres = true;
      moving_offset++;
    }
    else{
      if(moving_offset){
        quantifications[i-moving_offset] = q;
        if(record_thres)  
          thres[i-moving_offset] = thr_prob;
        else             
          thres[i-moving_offset] = -1;
        record_thres = false;
      }
    }
  }
  quantifications.resize(quantifications.size()-moving_offset);
  thres.resize(thres.size()-moving_offset);
}

void ReadQ::print_prefix(){
  assert(thres.size()==quantifications.size());
  cout << "Prefix:\n";
  for(size_t i=0; i<quantifications.size(); ++i){
    cout << quantifications[i].first << " " << thres[i] << ", "; 
  }
  cout << endl;
}


void ReadQ::read() {
  read_header();

  if (qube_input && (*r == 's'))  {
    ++r;
    skipWhitespace(r);
    read_word("cnf", 3);
    skipWhitespace(r);
    std::cerr << "code: " << *r << std::endl;
    if (*r == '0') qube_output = 0;
    else if (*r == '1') qube_output = 1;
    else {
      string s("unexpected char in place of 0/1");
      throw ReadException(s);
    }   
    return; 
  }

  read_quantifiers();
  compact_prefix();
  read_clauses();

  // print_prefix();
  

  if (!unquantified_variables.empty ()) {
    if (!quantifications.empty() && quantifications[0].first==EXISTENTIAL) {
      vector<Var> variables;
      FOR_EACH(vi,quantifications[0].second) variables.push_back(*vi);
      FOR_EACH(vi,unquantified_variables) variables.push_back(*vi);
      quantifications[0].second = VarVector(variables);
    } else {
      Quantification quantification;
      quantification.first=EXISTENTIAL;
      vector<Var> variables;
      FOR_EACH(vi,unquantified_variables) {
        cout << "Unquantified variable found!! " << *vi << endl;
        variables.push_back(*vi);
      }
      quantification.second=VarVector(variables);
      quantifications.insert(quantifications.begin(), quantification);
    }
  }
}
