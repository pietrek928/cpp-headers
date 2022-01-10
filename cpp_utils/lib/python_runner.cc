#include <boost/core/noncopyable.hpp>
#include <boost/python.hpp>

#include <runner/manager.h>
#include <runner/targets.h>

using namespace boost::python;

BOOST_PYTHON_MODULE(python_runner) {
  class_<FromSourceTarget>("FromSourceTarget")
      .def_readwrite("target", &FromSourceTarget::target)
      .def_readwrite("source", &FromSourceTarget::source)
      .def("options_push", &FromSourceTarget::options_push)
      .def("options_extend", &FromSourceTarget::options_extend)
      .def("libs_push", &FromSourceTarget::libs_push)
      .def("libs_extend", &FromSourceTarget::libs_extend)
      .def("include_dirs_push", &FromSourceTarget::include_dirs_push)
      .def("include_dirs_extend", &FromSourceTarget::include_dirs_extend);

  class_<TargetManager<FromSourceTarget>, boost::noncopyable>("TargetManager")
      .def("push_source_extension",
           &TargetManager<FromSourceTarget>::push_source_extension)
      .def("run_watcher", &TargetManager<FromSourceTarget>::run_watcher)
      .def("start_workers", &TargetManager<FromSourceTarget>::start_workers)
      .def("stop_workers", &TargetManager<FromSourceTarget>::stop_workers);
}
