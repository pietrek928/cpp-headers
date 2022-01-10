#include <string>
#include <thread>
#include <vector>

#include <mem/lock.h>
#include <utils/utils.h>

#include "change_monitor.h"
#include "manager.h"
#include "targets.h"

int main(int argc, char **argv) {
  std::vector<std::string> compile_opts = {"-std=c++17", "-O2", "-s"};
  std::vector<std::string> common_include_dirs = {".", "..", "../mem"};
  auto mgr = init_targets(std::make_tuple(
      FromSourceTarget("run.e", "run_config.cc", compile_opts,
                       common_include_dirs, {"pthread"}),
      ActionTarget("run.e", "!compiler", [&]() { restart(argv); })));

  std::thread worker1([&]() { mgr.run_compiler(); });

  mgr.run_watcher(common_include_dirs);

  return 0;
}
