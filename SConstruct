#!/usr/bin/env python
import os
import sys

from methods import print_error


libname = "laberinto"
projectdir = "project"

localEnv = Environment(tools=["default"], PLATFORM="")

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)
opts.Add(BoolVariable("tests", "Compila y ejecuta los tests unitarios (doctest) en lugar de la extensión", False))
opts.Update(localEnv)

Help(opts.GenerateHelpText(localEnv))

# ---------------------------------------------------------------------------
# Tests unitarios: `scons tests=yes`
# Compilan solo la lógica pura (src/core) con doctest, sin depender de Godot.
# ---------------------------------------------------------------------------
if localEnv["tests"]:
    test_env = Environment(tools=["default"], ENV=os.environ)
    test_env.Append(CPPPATH=["src/", "thirdparty/"])
    if test_env["CC"] == "cl":
        test_env.Append(CXXFLAGS=["/std:c++17", "/EHsc", "/utf-8", "/W3"])
    else:
        test_env.Append(CXXFLAGS=["-std=c++17", "-Wall"])
    test_sources = Glob("tests/*.cpp") + Glob("src/core/*.cpp")
    test_program = test_env.Program("bin/tests/laberinto_tests", source=test_sources)
    run_tests = test_env.Command("run_tests", test_program, test_program[0].abspath)
    test_env.AlwaysBuild(run_tests)
    Default(run_tests)
    Return()

# ---------------------------------------------------------------------------
# Extensión GDExtension (por defecto)
# ---------------------------------------------------------------------------
env = localEnv.Clone()

if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

env = SConscript("godot-cpp/SConstruct", {"api_version": 4.3, "env": env, "customs": customs})

env.Append(CPPPATH=["src/"])
# src/*.cpp (registro) + src/<módulo>/*.cpp (core, game, ...)
sources = Glob("src/*.cpp") + Glob("src/*/*.cpp")

# .dev doesn't inhibit compatibility, so we don't need to key it.
# .universal just means "compatible with all relevant arches" so we don't need to key it.
suffix = env['suffix'].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(env.subst('$SHLIBPREFIX'), libname, suffix, env.subst('$SHLIBSUFFIX'))

library = env.SharedLibrary(
    "bin/{}/{}".format(env['platform'], lib_filename),
    source=sources,
)

copy = env.Install("{}/bin/{}/".format(projectdir, env["platform"]), library)

default_args = [library, copy]
Default(*default_args)
