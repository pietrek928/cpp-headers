from setuptools import setup, find_packages

from cpp_utils.manage import get_extensions, LiveTestCommand

print(list(get_extensions('cpp_utils')))

setup(
    name='cpp_utils',
    description='C++ utils',
    author='pietrek xD',
    packages=find_packages(exclude=('docs',)),
    # ext_package='cpp_utils',
    ext_modules=list(get_extensions('cpp_utils')),
    package_data={
        'cpp_utils': [
            'include/**/*.h',
        ]
    },
    include_package_data=False,
    version='0.1.0',
    install_requires=(),
    cmdclass={
        'live_test': LiveTestCommand,
    },
)
