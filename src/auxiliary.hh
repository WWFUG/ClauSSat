/*
 * File:   auxiliary.hh
 * Author: mikolas
 *
 * Created on October 12, 2011
 */
#ifndef AUXILIARY_HH
#define	AUXILIARY_HH
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <assert.h>
#define __PL (std::cerr << __FILE__ << ":" << __LINE__ << std::endl).flush();
#define FOR_EACH(index,iterated)\
  for (auto index = (iterated).begin(); index != (iterated).end();++index)
#ifdef USE_GL
#define SATSPC Glucose
#else
#define SATSPC Minisat
#endif

static inline double read_cpu_time() {
  struct rusage ru; getrusage(RUSAGE_SELF, &ru);
  return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000;
}

//struct sizePairHash {
namespace std {
  template<>
    struct hash<std::pair<size_t, size_t> > {
      inline size_t operator() (const std::pair<size_t,size_t>& p) const {
        return p.first ^ p.second;
      }
    };
}

struct cstrHash {
  inline size_t operator() (const char* s) const {
    size_t hash = 5381;
    char c;
    while (c = *s++)
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
  }
};

struct cstrEq {
  inline size_t operator() (const char* s1,const char* s2) const {
    return strcmp(s1,s2)==0; };
};

template <class K,class V>
bool contains(const std::unordered_map<K,V>& es, const K& e) {
  return es.find(e) != es.end();
}

template <class K>
bool contains(const std::unordered_set<K>& es, const K& e) {
  return es.find(e) != es.end();
}

inline std::ostream& operator <<(std::ostream& outs, const std::vector<bool>& ns){
  outs<<'<';
  bool f=true;
  for(size_t i=0;i<ns.size();++i) {
    if(!ns[i]) continue;
    if(!f) outs<<' ';
    f=false;
    outs<<i;
  }
  return outs<<'>';
}

inline std::ostream& operator <<(std::ostream& outs, const std::vector<size_t>& ns){
  outs<<'<';
  for(size_t i=0;i<ns.size();++i) {
    if(i) outs<<' ';
    outs<<ns[i];
  }
  return outs<<'>';
}

double luby(double y, int x);
#endif	/* AUXILIARY_HH */

