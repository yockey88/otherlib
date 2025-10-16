import os
import subprocess
import sys
import argparse
import shutil

def find_msbuild():
  # Check if MSBuild is in the PATH
  msbuild_path = subprocess.run(["where", "msbuild"], capture_output=True, text=True)
  if msbuild_path.returncode == 0:
    return msbuild_path.stdout.strip()
  
  # If not found, check the default installation path
  default_paths = [
    "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
    "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
    "C:\\Program Files\\Microsoft Visual Studio\\2019\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\MSBuild\\Current\\Bin\\MSBuild.exe",

    "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",
    "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",
    "C:\\Program Files\\Microsoft Visual Studio\\2019\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",
    "C:\\Program Files\\Microsoft Visual Studio\\2022\\Enterprise\\MSBuild\\Current\\Bin\\MSBuild.exe",
  ]
  
  for path in default_paths:
    if os.path.exists(path):
      return path
  
  print("MSBuild not found. Please install Visual Studio (2019 or 2022) with MSBuild.")
  sys.exit(1)

def regen_project():
  print("Regenerating the project files...")
  subprocess.call(["cmake", "-S", ".", "-B", "build"])
  if not os.path.exists("build/other.sln"):
    print("Solution file does not exist. Please try again.")
    sys.exit(1)

def build_sln_file(sln_file, cfg=None):
  print(f"Building the project : {sln_file}...")
  msbuild_path = find_msbuild()
  msargs = [
    msbuild_path,
    sln_file,
    f"/p:Configuration={cfg}" if cfg else "/p:Configuration=Release",
  ]
  try:
    subprocess.run(msargs, check=True)
  except subprocess.CalledProcessError as e:
    print(f"Error: {e}")
    sys.exit(1)

def copy_dlls(cfg, dll_cfg):
  print(f"Copying DLLs ({dll_cfg}) for configuration: {cfg}...")
  assimp_debug = "extern/assimp/lib/debug/assimp-vc143-mtd.dll"
  assimp_release = "extern/assimp/lib/release/assimp-vc143-mt.dll"
  dlls = [
    f"extern/sdl/lib/{dll_cfg.lower()}/SDL3.dll",
    assimp_debug if cfg == "Debug" else assimp_release,
    f"extern/python312/python312.dll",
  ]
  destinations = [
    f"build/development-drivers/{cfg}/",
    f"build/driver/{cfg}/",
    f"build/other-terminal/src/{cfg}/",
    f"build/scratch/{cfg}/",
    f"build/tests/{cfg}/",
    f"build/tools/{cfg}/",
  ]
  
  if cfg == "Debug" or cfg == "ProfileD":
    if os.path.exists(assimp_debug):
      dlls.append(assimp_debug)
  else:
    if os.path.exists(assimp_release):
      dlls.append(assimp_release)

  for dll in dlls:
    if os.path.exists(dll):
      for dest in destinations:
        if os.path.exists(dest):
          shutil.copy(dll, dest)
          # print(f"Copied {dll} to {dest}")

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
  if extra_args:
    run_command.extend(extra_args)
  if verbose:
    run_command.append("--verbose")
  run_subprocess(run_command)
  
## TODO: this is ugly, fix this
def validate_args(args, parser):
  if not args.build and not args.regen_project \
      and not args.run and not args.run_scratch \
      and not args.run_terminal and not args.run_tests \
      and not args.compile_serialization_schema \
      and not args.compile_object and not args.generate_cs_bindings \
      and not args.run_test_suite and not args.run_server:
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
  parser.add_argument("--generate-cs-bindings", "-gcb", action="store_true", help="Generate C# bindings.")
  # parser.add_argument("--regen-compile-commands", "-rcc", action="store_true", help="Regenerate the compile_commands.json file.")

  args = parser.parse_args()
  try:
    validate_args(args, parser)

    cfg = args.cfg

    if args.generate_cs_bindings:
      print("Generating C# bindings...")
      run_subprocess([f"build/code-generator/{cfg}/OtherCsBindingsGenerator.exe"])

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
      ### run dotnet restore on solution file to restore nuget packages
      # this has to happen before build step cause bulding dotnet projects
      # requires the *.project.json files to be present
      print("Restoring .NET packages...")
      run_subprocess(["dotnet", "restore", "build/other.sln"])

      filename = "build/other.sln"
      if not os.path.exists(filename):
        print(f"Solution file {filename} does not exist. Please regenerate the project files first.")
        sys.exit(1)
      build_sln_file(filename, cfg)

      dll_cfg = "Release"
      if cfg == "Debug" or cfg == "ProfileD":
        dll_cfg = "Debug"
      copy_dlls(cfg, dll_cfg)

      
    if args.run:
      print(f"Running Other-Driver [{cfg}]")
      # run_subprocess(["build/driver/" + cfg + "/other_driver.exe", "script-config.toml"])
      # run_subprocess(["build/scratch/" + cfg + "/coro-testing.exe", "resources/dev-test-config.toml"])
      run_project("development-drivers", cfg, "runtime_dev", "dev-config.toml", args, args.verbose)
      
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
      extra_args=["--gtest_shuffle" ]
      
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