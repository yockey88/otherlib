import os
import subprocess
import sys
import argparse
import shutil

def regen_project():
  print("Regenerating the project files...")
  subprocess.call(["cmake", "-S", ".", "-B", "build"])
  if not os.path.exists("build/other.sln"):
    print("Solution file does not exist. Please try again.")
    sys.exit(1)

def copy_dlls(cfg, dll_cfg):
  print(f"Copying DLLs ({dll_cfg}) for configuration: {cfg}...")
  dlls = [
    f"extern/sdl/lib/{dll_cfg.lower()}/SDL3.dll",
    "extern/assimp/lib/assimp-vc143-mt.dll",
    f"extern/python312/python312.dll",
    "extern/sol2/lib/lua-5.4.4.dll",
  ]
  destinations = [
    f"build/development-drivers/{cfg}/",
    f"build/driver/{cfg}/",
    f"build/other-terminal/src/{cfg}/",
    f"build/scratch/{cfg}/",
    f"build/tests/{cfg}/",
    f"build/tools/{cfg}/",
  ]

  for dll in dlls:
    if os.path.exists(dll):
      for dest in destinations:
        if os.path.exists(dest):
          shutil.copy(dll, dest)
    else:
      print(f"Warning: {dll} does not exist.")


def run_subprocess(args):
  print(f"Running command: {' '.join(args)}")
  try:
    subprocess.run(args, check=True)
  except subprocess.CalledProcessError as e:
    print(f"Error running command: {e}")
    sys.exit(1)

def run_project(out_dir, cfg, name, config_file, args, verbose = False, extra_args=None):
  run_command = [f"build/{out_dir}/{cfg}/{name}.exe", f"resources/{config_file}"]
  # if verbose:
  run_command.append("--verbose")
  if extra_args:
    run_command.extend(extra_args)
  run_subprocess(run_command)
  
## TODO: this is ugly, fix this
def validate_args(args, parser):
  if not args.build and not args.regen_project \
      and not args.run and not args.run_scratch \
      and not args.run_terminal and not args.run_tests \
      and not args.compile_serialization_schema \
      and not args.compile_object \
      and not args.run_test_suite and not args.run_server \
      and not args.install:
    parser.print_help()
    sys.exit(1)

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='''Other Environment CLI Tool.\n
                                   This tool helps manage the Other Environment project, including building, running, and testing various components, as well as providing user interfaces for Other Environment projects''')
  parser.add_argument("--verbose", "-v", action="store_true", help="Enable verbose output.")
  parser.add_argument("--regen-project", "-rg", action="store_true", help="Regenerate the project files.")
  parser.add_argument("--build", "-b", action="store_true", help="Build the project.")
  parser.add_argument("--run", "-r", action="store_true", help="Run the main driver.")
  parser.add_argument("--run-server", "-srv", action="store_true", help="Run the server driver.")
  parser.add_argument("--run-scratch", "-rs", action="store_true", help="Run the scratch application.")
  parser.add_argument("--run-terminal", "-rt", action="store_true", help="Run the other terminal application.")
  parser.add_argument("--run-tests", "-t", action="store_true", help="Run the collection of other environment test suites.")
  parser.add_argument("--run-test-suite", "-ts", nargs=1, type=str, metavar="TEST_FILTER", help="Runs the test suites by passing the argument to GTests's --gtest-filter=<arg> flag.")
  parser.add_argument("--compile-serialization-schema", "-css", type=str, help="Compile the serialization schema.")
  parser.add_argument("--compile-object", "-co", nargs = 2, type=str, metavar=("SCHEMA_FILE", "OBJECT_FILE"), help="Compile a binary object using the <object_file> and the <schema_file>")
  parser.add_argument("--cfg", "-c", type=str, default="Debug", choices=["Debug", "Release", "Profile", "ProfileD"])
  # parser.add_argument("--generate-cs-bindings", "-gcb", action="store_true", help="Generate C# bindings.")
  parser.add_argument("--install", "-i", action="store_true", help="Install Other Environment to the system.")

  args = parser.parse_args()
  try:
    validate_args(args, parser)

    cfg = args.cfg

    if args.compile_serialization_schema is not None and os.path.exists(args.compile_serialization_schema):
      if not args.compile_serialization_schema.endswith(".fbs"):
        print(f"Error: The file {args.compile_serialization_schema} is not a valid FlatBuffers schema file.")
        sys.exit(1)

      if not os.path.exists("resources/simulation-configs/"):
        os.makedirs("resources/simulation-configs/")

      print(f"Compiling serialization schema: {args.compile_serialization_schema}")
      run_subprocess(["tools/flatc.exe", "--cpp",
                      "-o", "resources/simulation-configs/", 
                      args.compile_serialization_schema])
                      #  "--gen-object-api", "--gen-mutable", "--gen-all", 
      print("Serialization schema compiled successfully.")

    if args.install:
      # remove if installation folder exists, this only works locally for dev testing (and only on windows)
      if os.path.exists("C:/OtherEnvironment/"):
        shutil.rmtree("C:/OtherEnvironment/")
      run_subprocess(["cmake", "-S", ".", "-B", "build"])
      run_subprocess(["cmake", "--build", "build", "--config", cfg])
      run_subprocess(["cmake", "--install", "build", "--config", cfg])
      print("Other Environment installed successfully.")
      sys.exit(0)

    if args.compile_object is not None and len(args.compile_object) == 2:
      schema_file, object_file = args.compile_object
      if not os.path.exists(object_file) or not os.path.exists(schema_file):
        print(f"Error: The object file {object_file} or schema file {schema_file} does not exist.")
        sys.exit(1)

      print(f"Compiling object file: {object_file} with schema: {schema_file}")
      run_subprocess(["tools/flatc.exe", "--binary", schema_file, object_file])
      print("Object file compiled successfully.")
    elif args.compile_object is not None and len(args.compile_object) != 2:
      print("Error: --compile-object requires two arguments: <object_file> and <schema_file>.")
      sys.exit(1)

    if args.regen_project:
      regen_project()

    if args.build:
      run_subprocess(["cmake", "--build", "build", "--config", cfg])

      dll_cfg = "Release"
      if cfg == "Debug" or cfg == "ProfileD":
        dll_cfg = "Debug"
      copy_dlls(cfg, dll_cfg)

      
    if args.run:
      print(f"Running Other-Driver [{cfg}]")
      run_project("development-drivers", cfg, "runtime_dev", "runtime-dev.toml", args, args.verbose)
      # run_project("development-drivers", cfg, "rendering_dev", "renderer-dev.toml", args, args.verbose)
      # run_project("scratch", cfg, "gl-testing", "dev-config.toml", args, args.verbose)
      # run_project("scratch", cfg, "behavior-node", "behavior-node-dev.toml", args, args.verbose)
      # run_project("scratch", cfg, "action-dev", "action-dev.toml", args, args.verbose)

    elif args.run_server:
      run_project("development-drivers", cfg, "server_dev", "server-config.toml", args, args.verbose)
    elif args.run_scratch:
      print(f"Running Other-Scratch [{cfg}]")
      run_project("scratch" , cfg, "gl-testing", "gl-test-config.toml", args, args.verbose)
    elif args.run_terminal:
      print(f"Running Other-Terminal [{cfg}]")
      run_project("other-terminal", cfg, "other_terminal", "dev-config.toml", args, args.verbose)
    elif args.run_tests:
      print("Running tests...")
      extra_args = [ "--gtest_shuffle" ]
      
      ## TODO: fix platform specific output paths
      if cfg == "Debug" or cfg == "ProfileD":
        extra_args.append("--gtest_output=xml:other_test_results.windows.debug.xml")
      else:
        extra_args.append("--gtest_output=xml:other_test_results.windows.release.xml")

      run_project("tests", cfg, "other_tests", "dev-test-config.toml", args, args.verbose, extra_args=extra_args)
    elif args.run_test_suite is not None and len(args.run_test_suite) == 1:
      test_filter = args.run_test_suite[0]
      print(f"Running test suite with filter: {test_filter}")
      run_project("tests", cfg, "other_tests", "dev-test-config.toml", args, args.verbose, extra_args=[f"--gtest_filter={test_filter}", "--gtest_shuffle"])
      
  except subprocess.CalledProcessError as e:
    print(f"Error: {e}")
    sys.exit(1)