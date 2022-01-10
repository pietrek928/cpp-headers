#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <iostream>

void test() { std::cout << "ELO xD" << std::endl; }

BOOST_PYTHON_MODULE(test_lib) {
  using namespace boost;
  using namespace python;

  def("test", test);
}
