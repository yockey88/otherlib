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
  dlls = [
    f"extern/sdl/lib/{dll_cfg.lower()}/SDL3.dll",
  ]
  
  assimp_debug = "extern/assimp/lib/debug/assimp-vc143-mtd.dll"
  assimp_release = "extern/assimp/lib/release/assimp-vc143-mt.dll"
  if cfg == "Debug" or cfg == "ProfileD":
    if os.path.exists(assimp_debug):
      dlls.append(assimp_debug)
  else:
    if os.path.exists(assimp_release):
      dlls.append(assimp_release)

  for dll in dlls:
    if os.path.exists(dll):
      dest = f"build/development-drivers/{cfg}/"
      shutil.copy(dll, dest)

      dest = f"build/driver/{cfg}/"
      shutil.copy(dll, dest)
      
      # dest = f"build/other-terminal/{cfg}/"
      # shutil.copy(dll, dest)
      
      dest = f"build/scratch/{cfg}/"
      shutil.copy(dll, dest)

      dest = f"build/tests/{cfg}/"
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

def run_project(out_dir, cfg, name, config_file, args, verbose = False):
  run_command = [f"build/{out_dir}/{cfg}/{name}.exe", f"resources/{config_file}"]
  if verbose:
    run_command.append("--verbose")
  run_subprocess(run_command)
  
def validate_args(args, parser):
  if not args.build and not args.regen_project \
      and not args.run and not args.run_scratch and not args.run_terminal and not args.run_tests:
    parser.print_help()
    sys.exit(1)

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="A simple CLI for a Python project.")
  parser.add_argument("--verbose", "-v", action="store_true", help="Enable verbose output.")
  parser.add_argument("--regen-project", "-rg", action="store_true", help="Regenerate the project files.")
  parser.add_argument("--build", "-b", action="store_true", help="Build the project.")
  parser.add_argument("--run", "-r", action="store_true", help="Run the main driver.")
  parser.add_argument("--run-scratch", "-rs", action="store_true", help="Run the scratch application.")
  parser.add_argument("--run-terminal", "-rt", action="store_true", help="Run the other terminal application.")
  parser.add_argument("--run-tests", "-t", action="store_true", help="Run the collection of other environment test suites.")
  parser.add_argument("--cfg", "-c", type=str, default="Debug", choices=["Debug", "Release", "Profile", "ProfileD"])
  # parser.add_argument("--regen-compile-commands", "-rcc", action="store_true", help="Regenerate the compile_commands.json file.")

  args = parser.parse_args()
  try:
    validate_args(args, parser)

    cfg = args.cfg
      
    if args.regen_project:
      regen_project()

    if args.build:
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
      run_project("development-drivers", cfg, "rendering_dev", "dev-config.toml", args, args.verbose)
    elif args.run_scratch:
      print(f"Running Other-Scratch [{cfg}]")
      run_project("scratch" , cfg, "gl-testing", "gl-test-config.toml", args, args.verbose)
    elif args.run_terminal:
      print(f"Running Other-Terminal [{cfg}]")
      run_project("other-terminal", cfg, "other_terminal", "dev-config.toml", args, args.verbose)
    elif args.run_tests:
      print("Running tests...")
      run_project("tests", cfg, "other_tests", "dev-test-config.toml", args, args.verbose)
      
  except subprocess.CalledProcessError as e:
    print(f"Error: {e}")
    sys.exit(1)