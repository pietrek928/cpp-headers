#ifndef __DEPENDENCY_TRACKER_H_
#define __DEPENDENCY_TRACKER_H_

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <mem/lock.h>

constexpr auto CHANNEL_LOG_LEVEL_files = LogLevel::DEBUG;

class CompileDependencyTracker : public LockObject {
  std::unordered_map<int, std::vector<int>> reverse_dependency_graph;
  std::unordered_set<int> generated_obj;

  std::unordered_map<int, int> dep_cnt;
  std::unordered_set<int> to_compile;
  std::unordered_set<int> failed_obj;

  void add_to_compile(int v) {
    to_compile.insert(v);
    notify();
  }

  template <bool from_source> void inc_obj_dep(int v) {
    auto f = dep_cnt.find(v);
    if (f == dep_cnt.end()) {
      dep_cnt[v] = from_source ? 1 : 2;
      if (from_source) {
        add_to_compile(v);
      }
    } else if (!f->second) {
      f->second = from_source ? 1 : 2;
      if (from_source) {
        add_to_compile(v);
      }
    } else {
      if (!from_source) {
        f->second++;
      }
      if (f->second <= 1 && failed_obj.find(v) != failed_obj.end()) {
        add_to_compile(v);
      }
      return;
    }
    for (auto &vv : reverse_dependency_graph[v]) {
      inc_obj_dep<false>(vv);
    }
  }

  template <bool start> void dec_dep(int v) {
    auto f = dep_cnt.find(v);
    if (f == dep_cnt.end() || !f->second) {
      if (!start) {
        add_to_compile(v);
      }
      return;
    }
    f->second--;
    if (f->second <= 0) {
      dep_cnt.erase(f);
      for (auto &vv : reverse_dependency_graph[v]) {
        dec_dep<false>(vv);
      }
    } else if (f->second == 1) {
      add_to_compile(v);
    }
  }

public:
  void changed(int v) {
    auto l = lock();

    if (generated_obj.find(v) == generated_obj.end()) {
      for (auto &vv : reverse_dependency_graph[v]) {
        inc_obj_dep<true>(vv);
      }
    } else {
      // TODO: do sth ? recompile ???
    }
  }

  int compile_pop(volatile bool *run = NULL) {
    auto l = lock();

    auto b = to_compile.begin();

    while (b == to_compile.end() && (!run || *run)) {
      wait();
      b = to_compile.begin();
    }

    if (run && !*run) {
      throw std::runtime_error("Stop condition set");
    }

    auto r = *b;
    to_compile.erase(b);
    return r;
  }

  void compile_fail(int v) {
    auto l = lock();

    failed_obj.insert(v);
  }

  void compile_success(int v) {
    auto l = lock();

    failed_obj.erase(v);
    dec_dep<true>(v);
  }

  void clear_state() {
    auto l = lock();

    dep_cnt.clear();
    to_compile.clear();
    failed_obj.clear();
  }

  void update_deps(int v, std::vector<int> old_deps,
                   std::vector<int> new_deps) {
    auto l = lock();

    if (new_deps.size()) {
      generated_obj.insert(v);
    } else {
      generated_obj.erase(v);
    }

    std::sort(old_deps.begin(), old_deps.end());
    std::sort(new_deps.begin(), new_deps.end());

    std::vector<int> add_deps(std::max(old_deps.size(), new_deps.size()));
    auto it =
        std::set_difference(new_deps.begin(), new_deps.end(), old_deps.begin(),
                            old_deps.end(), add_deps.begin());
    add_deps.resize(it - add_deps.begin());
    for (auto &d : add_deps) {
      reverse_dependency_graph[d].push_back(v);
      inc_obj_dep<true>(v);
    }

    std::vector<int> rm_deps(std::max(old_deps.size(), new_deps.size()));
    it = std::set_difference(old_deps.begin(), old_deps.end(), new_deps.begin(),
                             new_deps.end(), rm_deps.begin());
    rm_deps.resize(it - rm_deps.begin());
    for (auto &d : rm_deps) {
      auto &deps_vec = reverse_dependency_graph[d];
      deps_vec.erase(std::remove(deps_vec.begin(), deps_vec.end(), v),
                     deps_vec.end());

      auto f = dep_cnt.find(d);
      if (f != dep_cnt.end() && f->second) {
        dec_dep<false>(v);
      }
    }
  }

  void clear() {
    auto l = lock();

    reverse_dependency_graph.clear();
    generated_obj.clear();
    dep_cnt.clear();
    to_compile.clear();
    failed_obj.clear();
  }
};

#endif /* __DEPENDENCY_TRACKER_H_ */
