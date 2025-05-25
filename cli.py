import os
import subprocess
import sys
import argparse

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

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="A simple CLI for a Python project.")
  parser.add_argument("--verbose", "-v", action="store_true", help="Enable verbose output.")
  parser.add_argument("--regen-project", "-rg", action="store_true", help="Regenerate the project files.")
  parser.add_argument("--build", "-b", action="store_true", help="Build the project.")
  parser.add_argument("--run", "-r", action="store_true", help="Run the main driver.")
  parser.add_argument("--cfg", "-c", type=str, default="Debug", choices=["Debug", "Release", "Debug-AS", "Profile"])

  args = parser.parse_args()
  try:
    if not args.build and not args.run and not args.regen_project:
      parser.print_help()
      sys.exit(1)

    cfg = args.cfg
      
    if args.regen_project:
      regen_project()

    if args.build:
      # build fbs first
      filename = "build/other.sln"
      if not os.path.exists(filename):
        print(f"Solution file {filename} does not exist. Please regenerate the project files first.")
        sys.exit(1)
      build_sln_file(filename, cfg)
      

    if args.run:
      print(f"Running Other-Driver [{cfg}]")
      run_command = [f"build/driver/{cfg}/other_driver.exe", "resources/dev-config.toml"] 
      if args.verbose is not None and args.verbose:
        run_command.append("--verbose")

      subprocess.run(run_command, check=True)
      
  except subprocess.CalledProcessError as e:
    print(f"Error: {e}")
    sys.exit(1)
