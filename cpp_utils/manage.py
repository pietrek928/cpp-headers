from distutils.extension import Extension
from importlib import import_module
from os import listdir, getcwd, walk
from os.path import isdir, join, dirname
from subprocess import run
from sys import path, version_info
from types import ModuleType
from typing import Dict, Iterable

from setuptools import Command


def _make_unique(items):
    got_items = set()
    for item in items:
        if item not in got_items and item is not None:
            yield item


def _gather_all(configs: Iterable[Dict], key: str):
    for config in configs:
        if config.get(key):
            for item in config[key]:
                yield item


def _gather(configs: Iterable[Dict], key: str, unique: bool = True):
    items = _gather_all(configs, key)
    return tuple(
        _make_unique(items) if unique else items
    )


def get_module_config(m: ModuleType):
    try:
        config_module = import_module(f'{m.__name__}.config')
        config_vars = dict(vars(config_module))

        include_dir = f'{m.__path__[0]}/include'
        if not isdir(include_dir):
            include_dir = None

        deps = config_vars.get('DEPENDENCIES') or ()
        deps_configs = tuple(
            map(get_module_config, deps)
        ) + (config_vars,)

        config_vars['INCLUDES'] = tuple(
            config_vars.get('INCLUDES') or ()) + (include_dir,)

        return dict(
            INCLUDES=_gather(deps_configs, 'INCLUDES'),
            LIBS=_gather(deps_configs, 'LIBS'),
            OPTS=_gather(deps_configs, 'OPTS', unique=False),
            TEST_OPTS=config_vars.get('TEST_OPTS') or (),
        )
    except ImportError:
        return {}


def _include_opts(cfg):
    return tuple(
        f'-I{include_dir}' for include_dir in cfg['INCLUDES']
    )


def _lib_opts(cfg):
    return tuple(
        f'-l{lib}' for lib in cfg['LIBS']
    )


def _compile_test_cmd(cfg, fname):
    return (
            ('g++',) + cfg['OPTS'] +
            _include_opts(cfg) + _lib_opts(cfg) + cfg['TEST_OPTS'] +
            (fname, '-o', f'{fname}.e')
    )


def _run_test(cfg, fname):
    compile_cmd = _compile_test_cmd(cfg, fname)
    result = run(compile_cmd)
    if result.returncode:
        raise RuntimeError(f'Compilation failed {result}')

    result = run([f'{fname}.e'])
    if result.returncode:
        raise RuntimeError(f'Running test failed {result}')


def run_tests(module, test_dir):
    cfg = get_module_config(module)
    for fname in listdir(test_dir):
        if fname.startswith('test_') and fname.endswith('.cc'):
            _run_test(cfg, join(test_dir, fname))


def add_local_path():
    path.append(getcwd())


def get_python_includes():
    a, b = version_info[:2]
    return f'/usr/include/python{a}.{b}',


def get_python_libs():
    a, b = version_info[:2]
    return f'python{a}.{b}',


def get_boost_libs():
    a, b = version_info[:2]
    return f'boost_python{a}{b}',


def get_extensions(*module_paths, lib_dir='lib'):
    add_local_path()

    for module_path in module_paths:
        lib_dir = join(module_path.replace('.', '/'), lib_dir)
        try:
            lib_files = tuple(
                fname for fname in listdir(lib_dir)
                if fname.endswith('.cc')
            )
        except FileNotFoundError:
            continue
        if not lib_files:
            continue

        module = import_module(module_path)
        cfg = get_module_config(module)

        for lib_file in lib_files:
            lib_name = lib_file.split('.')[0]
            yield Extension(
                f'{module_path}.{lib_name}', [join(lib_dir, lib_file)],
                define_macros=[],
                include_dirs=list(cfg['INCLUDES'] + (lib_dir,)),
                libraries=list(cfg['LIBS']),
                extra_compile_args=list(
                    cfg['TEST_OPTS']) + ['-shared', '-fPIC'],
            )


def get_file_directories(files):
    d = set()
    for f in files:
        dd = dirname(f)
        if dd:
            d.add(dd)
        else:
            d.add('.')
    return tuple(sorted(d))


def get_extensions_files(extensions: Iterable[Extension]):
    for e in extensions:
        yield from e.sources
        for d in e.include_dirs:
            for root, dirs, files in walk(d):
                for dd in dirs:
                    yield f'{root}/{dd}/'


def run_live_tests(test_extensions):
    test_extensions = tuple(test_extensions)
    from cpp_utils.python_runner import TargetManager
    m = TargetManager()
    for e in test_extensions:
        m.push_source_extension(e)
    m.start_workers(4)
    m.run_watcher(get_file_directories(
        get_extensions_files(test_extensions)
    ), True)


class LiveTestCommand(Command):
    """A custom command to run Pylint on all Python source files."""

    test_modules = ()

    description = 'run python tests live'
    user_options = [
        # The format is (long option, short option, description).
        ('test-modules=', 'm', 'Modules to run tests'),
    ]

    def initialize_options(self):
        """Set default values for options."""
        pass

    def finalize_options(self):
        """Post-process options."""
        if isinstance(self.test_modules, str):
            self.test_modules = tuple(self.test_modules.split(','))

    def run(self):
        run_live_tests(get_extensions(*self.test_modules, lib_dir='test'))
