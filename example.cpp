#define DEBUG
#include "edn.hpp"
#include <iostream>
#include <istream>
#include <iterator>

using edn::EdnNode;
using edn::read;
using edn::pprint;

int main() {	
  std::string someEdn;
  try {
    std::cin >> std::noskipws;
    std::istream_iterator<char> it(std::cin);
    std::istream_iterator<char> end;
    someEdn = std::string(it, end);

    EdnNode someMap = read(someEdn);
    std::cout << pprint(someMap) << std::endl;
  } catch (const char* e) {
    std::cout << "Error parsing: " << e << " full: " << someEdn << std::endl;
  }
}
