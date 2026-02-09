#!/usr/bin/env python
import os
import sys

#godot_root = os.path.abspath(os.path.join("..", "..", "godot"))
godot_root = os.path.abspath("godot")
godot_cpp_path = os.path.abspath("godot-cpp")

env = SConscript(os.path.join(godot_cpp_path, "SConstruct"))

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=[
    "src",
    os.path.join(godot_cpp_path, "include"),
    os.path.join(godot_cpp_path, "include", "godot_cpp"),
    os.path.join(godot_cpp_path, "gen", "include"),
    os.path.join(godot_cpp_path, "gen", "include", "godot_cpp")
])

modules_path = os.path.join(godot_root, "modules")
if os.path.isdir(modules_path):
    env.Append(CPPPATH=[modules_path])

sources = Glob("src/*.cpp")

if env["target"] in ["editor", "template_debug", "template_release"]:
    try:
        doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "bin/softref.{}.{}.framework/softref.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "bin/softref.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "bin/softref.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "bin/softref{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
