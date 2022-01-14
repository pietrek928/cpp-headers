#ifndef __MANAGER_H_
#define __MANAGER_H_

#include <algorithm>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <thread>
#include <tuple>

#include <mem/lock.h>
#include <mem/object_container.h>
#include <utils/utils.h>

#include "change_monitor.h"
#include "compile.h"
#include "dependency_tracker.h"
#include "targets.h"
#include "translate.h"

template <class... Ttarget_types> class TargetManager : LockObject {
  volatile bool run = false;

  CompileDependencyTracker dependency_tracker;
  ObjectContainer<Ttarget_types...> tgs;
  File2Number file_mapper;
  std::vector<std::thread> compile_workers;

  template <class T> auto translate(T v) { return file_mapper.get(v); }

  void init_objects() {
    auto l = lock();

    std::unordered_map<int, std::vector<int>> deps;
    int i = 0;
    file_mapper.clear();
    tgs.for_each([&](const auto &t) {
      file_mapper.add(t.target, i);
      i++;
    });

    dependency_tracker.clear();
    tgs.for_each([&](const auto &t) {
      auto obj_n = translate(t.target);
      auto str_deps = t.find_deps();
      deps[obj_n] = translate(str_deps);
      dependency_tracker.update_deps(obj_n, {}, deps[obj_n]);
    });
  }

public:
  TargetManager() {}

  ~TargetManager() { stop_workers(); }

  template <class To> void push_target(const To &o) {
    tgs.template insert<To>(o);
  }

  template <class To> void push_target(To &&o) { tgs.template insert<To>(o); }

#ifdef USE_PYTHON
  void push_source_extension(boost::python::object ext) {
    auto t = FromSourceTarget();
    t.target = boost::python::extract<std::string>(ext.attr("name"));
    std::replace(t.target.begin(), t.target.end(), '.', '/');
    t.target += ".so";
    t.source = boost::python::extract<std::string>(
        ext.attr("sources")[0]); // TODO: more sources ?
    t.include_dirs_extend(ext.attr("include_dirs"));
    t.libs_extend(ext.attr("libraries"));
    t.options_extend(ext.attr("extra_compile_args"));
    push_target(t);
  }
#endif /* USE_PYTHON */

  void compiler_worker() {
    while (run) {
      auto compile_n = dependency_tracker.compile_pop(&run);
      tgs.call_for(compile_n, [&](auto &t) {
        try {
          t.compile();
          dependency_tracker.compile_success(compile_n);
        } catch (const compile_error &e) {
          dependency_tracker.compile_fail(compile_n);
        }
      });
    }
  }

  void run_watcher(const boost::python::object &watched_dir_,
                   bool compile_on_start) {
    std::vector<std::string> watched_dirs;
    extend_vector(watched_dirs, watched_dir_);

    init_objects();

    if (!compile_on_start) {
      dependency_tracker.clear_state();
    }

    ChangeMonitor watcher;

    for (auto &d : watched_dirs) {
      watcher.add(d);
    }

    DBG(files) << "Change monitor started." << LOG_ENDL;
    watcher.listen([&](const std::string &obj_name) {
      DBG(files) << "Object changed " << obj_name << DBG_ENDL;
      auto obj_n = translate(obj_name);
      dependency_tracker.changed(obj_n);
    });
  }

  void start_workers(int count) {
    stop_workers();

    auto l = lock();
    run = true;
    compile_workers.reserve(count);
    for (int i = 0; i < count; i++) {
      compile_workers.emplace_back([&]() {
        try {
          this->compiler_worker();
        } catch (const std::runtime_error &e) {
        }
      });
    }
  }

  void stop_workers() {
    auto l = lock();

    run = false;
    dependency_tracker.notifyAll();
    for (auto &t : compile_workers) {
      t.join();
    }
    compile_workers.clear();
  }
};

template <class Ttargets_tuple> auto init_targets(Ttargets_tuple tgs) {
  return TargetManager<Ttargets_tuple>(tgs, false);
}

void restart(char **argv) {
  INFO(compile) << "Restarting program..." << DBG_ENDL;
  ASSERT_SYS(execv(argv[0], argv), "Restarting failed.");
}

#endif /* __MANAGER_H_ */
