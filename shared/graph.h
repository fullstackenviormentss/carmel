#ifndef GRAPH_H
#define GRAPH_H 1
#include "config.h"

#include <iostream>
#include <vector>

#include "2heap.h"
#include "list.h"
#include "weight.h"


struct GraphArc {
  int source;
  int dest;
  float weight;
  void *data;
};

std::ostream & operator << (std::ostream &out, const GraphArc &a);

struct GraphState {
  List<GraphArc> arcs;
};

struct Graph {
  GraphState *states;
  int nStates;
};


Graph reverseGraph(Graph g) ;

extern Graph dfsGraph;
extern bool *dfsVis;

void dfsRec(int state, int pred);

void depthFirstSearch(Graph graph, int startState, bool* visited, void (*func)(int state, int pred));

extern List<int> *topSort;

void pushTopo(int state, int pred);

List<int> *topologicalSort(Graph g);

void countNoCyclePaths(Graph g, Weight *nPaths, int source);


struct DistToState {
  int state;
  static DistToState **stateLocations;
  static float *weights;
  static float unreachable;
  operator float() const { return weights[state]; }
  void operator = (DistToState rhs) { 
    stateLocations[rhs.state] = this;
    state = rhs.state;
  }
};


inline bool operator < (DistToState lhs, DistToState rhs);

inline bool operator == (DistToState lhs, DistToState rhs);

inline bool operator == (DistToState lhs, float rhs);

Graph shortestPathTree(Graph g, int dest, float *dist);

Graph removeStates(Graph g, bool marked[]); // not tested

void printGraph(Graph g, std::ostream &out);

#endif
