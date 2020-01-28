#include <memory.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include "shared.hpp"

template <typename T, int row_size, int column_size>
class Matrix {
protected:
  T buffer[row_size][column_size];
  int row, col;
public:
  Matrix () : row(row_size), col(column_size) {
    memset(buffer, 0, sizeof(buffer));
  };
  Matrix (T m[row_size][column_size]) : row(row_size), col(column_size) {
    for(int i = 0; i < row_size; i++) 
      for(int j = 0; j < column_size; j++)
	buffer[i][j] = m[i][j];
  };
  bool put(T const & x, int n, int m) {
    buffer[n][m] = x;
    return (buffer[n][m] == x ? true : false);
  }
  T get(int n, int m) {
    if(n > row_size || m > column_size) {
      // Error: getting out of matrix
      // ... Put an error message here or return empty matrix! - bjarke, 10th Oct 2019.
      printf("Error: Tried to get value outside of matrix scope\n");
      exit(1);
    }
    return buffer[n][m];
  }
};

template <typename T, int n, int m>
Matrix<T,n,m> floydWarshall(Matrix<T,n,m> W) {
  auto D = W; // Distance Matrix
  auto P = W; // Predecessor Matrix

  // Construct the predecessor matrix
  for(int i = 0; i < n; i++) {
    for(int j = 0; j < n; j++) {
      if(i == j || P.get(i,j) == INT_MAX) {
	P.put(INT_MAX,i,j);
      }
      if(i != j && P.get(i,j) < INT_MAX) {
	P.put(i,i,j);
      }
    }
  }
  
  for(int k = 0; k < n; k++) {
    for(int i = 0; i < n; i++) {
      for(int j = 0; j < n; j++) {
	if(D.get(i,k) != INT_MAX && D.get(k,j) != INT_MAX && D.get(i,k) + D.get(k,j) < D.get(i,j)) {
	  D.put(D.get(i,k) + D.get(k,j),i,j);
	  P.put(P.get(k,j),i,j);
	}
      }
    }
  }
	     
  return P;
}

// I.E from vertex 1 to 8 prints like:
// 2 -> 3 -> 4 -> 5 -> 6 -> 7 ->
// so the source/dest nodes are implicit. 
template <typename T, int n, int m>
void print_path(Matrix<T,n,m> P, int i, int j) {
  if(P.get(i,j) == i)
    return;
  
  print_path(P, i, P.get(i,j));
  printf(" %d -> ", P.get(i,j));
}

template <typename T, int n, int m>
void get_path(Matrix<T,n,m> P,
	      DynamicDeque<int>* path_tree_buffer,
	      int source, int dest) {
  if(P.get(source, dest) == source)
    return;

  get_path(P, path_tree_buffer, source, P.get(source, dest));
  path_tree_buffer->push_back(P.get(source, dest));
}
