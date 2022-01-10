from cpp_utils.manage import get_python_includes, get_boost_libs, get_python_libs

DEPENDENCIES = ()
LIBS = get_boost_libs() + get_python_libs()
OPTS = ('-O2', '-s', '-std=c++17')
TEST_OPTS = ('-lpthread',)
INCLUDES = get_python_includes()
