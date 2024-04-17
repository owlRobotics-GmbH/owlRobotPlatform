# tasks.py
import invoke
import sys
import os


os.chdir('../build')
print(os.getcwd())


def print_banner(msg):
    print("==================================================")
    print("= {} ".format(msg))



def compile_python_module(cpp_name, extension_name):
    print('compile_python_module')
    invoke.run(
        "g++ -O3 -Wall -Werror -shared -std=c++11 -fPIC "
        "`python3-config --includes` "
        "{0} "
        #"-o {1}`python3-config --extension-suffix` "
        "-o {1}.so "
        "-Wl,-rpath,.".format(cpp_name, extension_name)
    )
    

@invoke.task()
def build_swig(c):
    """ Build the cython extension module """
    print_banner("Building Python Module")
    # Run cython on the pyx file to create a .cpp file
    invoke.run("swig -python -c++ -outdir ../build -o ../build/mylib_wrap.cxx ../python/mylib_swig.i")

    # Compile and link the cython wrapper library
    compile_python_module("mylib_wrap.cxx", "_mylib")
    print("* Complete")


@invoke.task(
    build_swig,
)
def all(c):
    """Build and run all tests"""
    pass

