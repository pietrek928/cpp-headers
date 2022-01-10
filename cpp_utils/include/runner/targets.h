#ifndef __TARGETS_H_
#define __TARGETS_H_

#include <string>
#include <vector>

#include <unistd.h>

#include <utils/utils.h>

#include "compile.h"

class FromSourceTarget {
public:
  std::string target;
  std::string source;
  std::vector<std::string> options;
  std::vector<std::string> include_dirs;
  std::vector<std::string> libs;

  PY_VECTOR_METHODS(options, std::string)
  PY_VECTOR_METHODS(libs, std::string)
  PY_VECTOR_METHODS(include_dirs, std::string)

  FromSourceTarget(std::string target, std::string source,
                   std::vector<std::string> options,
                   std::vector<std::string> include_dirs,
                   std::vector<std::string> libs)
      : target(target), source(source), options(options),
        include_dirs(include_dirs), libs(libs) {}

  void add_compile_opts(std::vector<std::string> &v) const {
    v.insert(v.end(), options.begin(), options.end());
  }

  void add_libs(std::vector<std::string> &v) const {
    for (auto &l : libs) {
      v.push_back("-l" + l);
    }
  }

  void add_include_dirs(std::vector<std::string> &v) const {
    for (auto &i : include_dirs) {
      v.push_back("-I" + i);
      //   v.push_back(i);
    }
  }

  void add_show_deps_opts(std::vector<std::string> &v) const {
    v.push_back("-MM");
    v.push_back("-MF");
    v.push_back("-");
  }

  void add_sources(std::vector<std::string> &v) const { v.push_back(source); }

  FromSourceTarget() {}

  auto find_deps() const {
    std::vector<std::string> cmd = {"g++"};
    add_compile_opts(cmd);
    add_sources(cmd);
    add_show_deps_opts(cmd);
    add_include_dirs(cmd);

    auto result = run_compilation(cmd);
    auto deps = parse_compile_deps(result);

    std::string dbg_msg = "Dependencies of " + target + ": ";
    for (auto &d : deps) {
      dbg_msg += "'" + d + "', ";
    }
    DBG(compile) << dbg_msg << DBG_ENDL;

    return deps;
  }

  auto compile() {
    std::vector<std::string> cmd = {"g++"};
    add_compile_opts(cmd);
    add_libs(cmd);
    add_sources(cmd);
    add_include_dirs(cmd);

    cmd.push_back("-o");
    cmd.push_back(target);

    return run_compilation(cmd);
  }

  void clear() { unlink(target.c_str()); }
};

template <class Taction> class ActionTarget {
public:
  Taction action;
  std::string source;
  std::string target;

  ActionTarget(std::string source, std::string target, Taction action)
      : source(source), target(target), action(action) {}

  std::vector<std::string> find_deps() { return {source}; }

  auto compile() { return action(); }

  void clear() {}
};

#endif /* __TARGETS_H_ */
