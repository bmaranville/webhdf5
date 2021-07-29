#!/usr/bin/python3

from __future__ import print_function

import os
import subprocess
import pprint
from pathlib import Path
import tempfile
from shutil import copy, rmtree

cleanup_dirs = []


def exported():
    with open("exported.txt") as f:
        return "EXPORTED_FUNCTIONS=" \
            + pprint.pformat([f"_{line.strip()}" for line in f]) \
                    .replace("\n", "") \
                    .replace("'", '\"')


def check_emscripten():
    try:
        path = Path(os.environ["EMSDK"])
        return [path.exists(),
                subprocess.run(["emcc", "--version"], text=True, capture_output=True).stdout.split(' ')[4]]

    except KeyError:
        return [False, None]


def setup_build(prefix="_build"):
    def gen_directories():
        cache = tempfile.mkdtemp(prefix="cache", dir=os.getcwd())
        print(cache)
        cleanup_dirs.append(cache)

        while True:
            build = tempfile.mkdtemp(prefix=prefix, dir=os.getcwd())
            print(build)
            cleanup_dirs.append(build)
            yield [build, cache]

    if not hasattr(setup_build, "gen"):
        setup_build.gen = gen_directories()

    return setup_build.gen.__next__()


def configure():
    print("Checking for Emscripten...", end=' ')
    exists, version = check_emscripten()
    print(version, "Found")

    if not exists:
        exit(1)


def cleanup():
    for d in cleanup_dirs:
        rmtree(d)


def make_executable(path):
    import stat
    st = os.stat(path)
    os.chmod(path, st.st_mode | stat.S_IEXEC)

def execute(command, in_dir):
    cwd = os.getcwd()
    os.chdir(in_dir)
    print(f"Running [{str(in_dir)}]...", " ".join(command))
    try:
        subprocess.run(command, stdout=subprocess.PIPE, text=True, check=True)
    except subprocess.CalledProcessError as err:
        print(err.__cause__)

    os.chdir(cwd)


def run_configure_script(build_dir, runner=["bash"]):
    configure_script = Path("..") / Path("libhdf5") / Path("configure")
    execute(runner + [str(configure_script),
                      #"CFLAGS=-m32",
                      #"CXXFLAGS=-m32",
                      #"LDFLAGS=-m32",
                      #"CFLAGS=-lz",
                      "LIBS=-lz",
                      "--disable-tests",
                      "--enable-cxx",
                      "--enable-build-mode=production",
                      "--disable-tools", "--disable-shared",
                      "--disable-deprecated-symbols"],
            build_dir)

def run_cmake_script(build_dir, runner=[""]):
    lib_path = Path("..") / Path("libhdf5")
    execute(runner + ["cmake"] + [str(lib_path),
                      #"CFLAGS=-m32",
                      #"CXXFLAGS=-m32",
                      #"LDFLAGS=-m32"
                      ],
            build_dir)

def run_make(build_dir, runner=[]):
    execute(runner + ["make", "-j8"], build_dir)


def finalise(build_dir, exported_functions):
    print('finalising: ', build_dir, ", current working dir: ", os.getcwd())
    libs = [str(build_dir / Path("src") / Path(".libs") / Path("libhdf5.a")),
            str(build_dir / Path("hl") / Path("src") / Path(".libs")
                / Path("libhdf5_hl.a")),
            str(build_dir / Path("c++") / Path("src") / Path(".libs")
                / Path("libhdf5_cpp.a"))]

    execute(["emcc", "-O3"]
            + libs
            + ["--bind", "../test.cpp"]
            + ["-I../libhdf5/src/", "-I./src"]
            + ["-I../libhdf5/c++/src/"]
            + ["-I../libhdf5/hl/src/"]
            + ["-o", "webhdf5.js", 
               "-s", "WASM_BIGINT",
               "-s", "EXPORT_ES6=1",
               "-s", "MODULARIZE=1",
               "-s", "FORCE_FILESYSTEM=1",
               "-s", "USE_ZLIB=1",
               #"-s", "WASM_ASYNC_COMPILATION=0",
               "-s", "EXTRA_EXPORTED_RUNTIME_METHODS=[\"ccall\", \"cwrap\", \"FS\"]",
               #"-s", "EXPORTED_FUNCTIONS=[\"_free\"]"],
               "-s", exported_functions],
            build_dir)
    return [build_dir / Path("webhdf5.wasm"), build_dir / Path("webhdf5.js")]


def native_build():
    build_dir, cache_dir = setup_build()
    libhdf5_dir = Path("libhdf5")
    autogen_script = Path("..") / Path("libhdf5") / Path("autogen.sh")

    execute([str(autogen_script)], libhdf5_dir)
    run_configure_script(build_dir)
    #run_cmake_script(build_dir)
    run_make(build_dir)
    copy(Path(build_dir) / Path("src") / Path("H5detect"), cache_dir)
    copy(Path(build_dir) / Path("src") / Path("H5make_libsettings"), cache_dir)

def finalise_and_copy(build_dir, exported_functions=exported()):
    wasm, js = finalise(Path(build_dir), exported_functions)
    copy(wasm, Path("webhdf5.wasm"))
    copy(js, Path("webhdf5.js"))

def wasm_build(exported_functions):
    build_dir, cache_dir = setup_build()
    src_dir = Path(build_dir) / Path("src")
    run_configure_script(build_dir, ["emconfigure"])
    run_make(build_dir, ["emmake"])

    copy(Path(cache_dir) / Path("H5detect"), src_dir)
    #make_executable(src_dir / Path("H5detect") )
    execute(["touch", "H5detect"], in_dir=src_dir)
    run_make(build_dir, ["emmake"])

    copy(Path(cache_dir) / Path("H5make_libsettings"), src_dir)
    execute(["touch", "H5make_libsettings"], in_dir=src_dir)
    run_make(build_dir, ["emmake"])

    finalise_and_copy(build_dir)
    #wasm, js = finalise(Path(build_dir), exported_functions)
    #copy(wasm, Path("webhdf5.wasm"))
    #copy(js, Path("webhdf5.js"))

    execute(["brotli", "-9", "webhdf5.wasm"], Path())


if __name__ == "__main__":
    configure()
    try:
        native_build()
        wasm_build(exported())
    except BaseException as err:
        print(err)

    # cleanup()
