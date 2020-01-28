#include "DynamicDeque.hpp"
#include <iostream>

int main() {

  DynamicDeque<int> test;
  test.reserve(10);

  test.push_back(1);
  test.push_back(2);
  test.push_back(3);

  test.flush();

  test.push_back(4);
  test.push_front(5);

  std::cout << test[1] << std::endl;
  
}
