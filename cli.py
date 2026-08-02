import os
import subprocess
import sys
import argparse
import shutil
import json

def get_physx_dlls(dll_cfg):
  physx_base_path = "extern/physx/bin/"
  dlls = [
    f"{physx_base_path}{dll_cfg}/PhysX_64.dll",
    f"{physx_base_path}{dll_cfg}/PhysXCommon_64.dll",
    f"{physx_base_path}{dll_cfg}/PhysXCooking_64.dll",
    f"{physx_base_path}{dll_cfg}/PhysXFoundation_64.dll",
    f"{physx_base_path}{dll_cfg}/PhysXGpu_64.dll",
  ]
  if dll_cfg == "Debug":
    dlls.append(f"{physx_base_path}{dll_cfg}/PVDRuntime_64.dll")
  return dlls

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
    f"extern/jolt/bin/{dll_cfg}/Jolt.dll",
  ]
  dlls.extend(get_physx_dlls(dll_cfg))

  destinations = [
    f"build/development-drivers/{cfg}/",
    f"build/driver/{cfg}/",
    f"build/other-terminal/src/{cfg}/",
    f"build/scratch/{cfg}/",
    f"build/tests/{cfg}/",
    f"build/tests/harness/{cfg}/",
    f"build/other-cli/{cfg}/",
    f"build/other-editor/{cfg}/",
    f"build/other-server/{cfg}/",
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

def run_project(out_dir, cfg, name, config_file, verbose = False, extra_args=None, project_path=None):
  ## normpath so the exe token contains backslashes; otherwise CreateProcess PATH-searches
  ##  the forward-slash name and misses it in shells that exclude the CWD from PATH
  run_command = [os.path.normpath(f"build/{out_dir}/{cfg}/{name}.exe"), f"{config_file}"]
  
  if verbose:  
    run_command.append("--verbose")
  if project_path is not None:
    run_command.append("-f")
    run_command.append(project_path)
  
  if extra_args:
    run_command.extend(extra_args)
  run_subprocess(run_command)
  
def validate_args(args, parser):
  ## every flag except --verbose/--cfg is an action; require at least one
  actions = {k: v for k, v in vars(args).items() if k not in ("verbose", "cfg")}
  if not any(actions.values()):
    parser.print_help()
    sys.exit(1)

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='''Other Environment CLI Tool.\n
                                   This tool helps manage the Other Environment project, including building, running, and testing various components, as well as providing user interfaces for Other Environment projects''')
  parser.add_argument("--verbose", "-v", action="store_true", help="Enable verbose output.")
  parser.add_argument("--regen-project", "-rg", action="store_true", help="Regenerate the project files.")
  parser.add_argument("--build", "-b", action="store_true", help="Build the project.")
  parser.add_argument("--run", "-r", action="store_true", help="Run the main driver.")
  parser.add_argument("--run-project", "-rp", nargs=1, type=str, metavar="PROJECT_PATH", help="Run the main driver with a specific project file.")
  parser.add_argument("--run-server", "-srv", action="store_true", help="Run the server driver.")
  parser.add_argument("--run-scratch", "-rs", action="store_true", help="Run the scratch application.")
  parser.add_argument("--run-terminal", "-rt", action="store_true", help="Run the other terminal application.")
  parser.add_argument("--run-tests", "-t", action="store_true", help="Run the collection of other environment test suites.")
  parser.add_argument("--run-test-suite", "-ts", nargs=1, type=str, metavar="TEST_FILTER", help="Runs the test suites by passing the argument to GTests's --gtest-filter=<arg> flag.")
  parser.add_argument("--run-soak", "-soak", action="store_true", help="Run the soak test harness against the test project and validate its report.")
  parser.add_argument("--cfg", "-c", type=str, default="Debug", choices=["Debug", "Release", "Profile", "ProfileD"])
  # parser.add_argument("--generate-cs-bindings", "-gcb", action="store_true", help="Generate C# bindings.")
  parser.add_argument("--install", "-i", action="store_true", help="Install Other Environment to the system.")
  parser.add_argument("--daemon-server", "-dsrv", action="store_true", help="Run the Other Environment Daemon Server.")

  args = parser.parse_args()
  try:
    validate_args(args, parser)

    cfg = args.cfg
    if cfg != "Debug" and cfg != "Release" and cfg != "Profile" and cfg != "ProfileD":
      print(f"Error: Invalid configuration '{cfg}'. Valid options are: Debug, Release, Profile, ProfileD.")
      sys.exit(1)

    if args.install:
      run_subprocess(["cmake", "--install", "build", "--config", cfg])
      print("Other Environment installed successfully.")
      sys.exit(0)

    if args.regen_project:
      regen_project()

    if args.build:
      ## why on God's green earth would microsoft change the fucking file extension
      if not os.path.exists("build/other.sln") and not os.path.exists("build/other.slnx"):
        run_subprocess(["cmake", "-S", ".", "-B", "build"])
      run_subprocess(["cmake", "--build", "build", "--config", cfg, "--parallel"])

      dll_cfg = "Release"
      if cfg == "Debug" or cfg == "ProfileD":
        dll_cfg = "Debug"
      copy_dlls(cfg, dll_cfg)
      
    if args.run:
      print(f"Running Other-Driver [{cfg}]")
      run_project("other-editor", cfg, "other_editor", "resources/editor-config.toml", args.verbose)
    
    elif args.run_project is not None and len(args.run_project) == 1:
      project_path = args.run_project[0]
      if not os.path.exists(project_path):
        print(f"Error: The specified project file {project_path} does not exist.")
        sys.exit(1)
      print(f"Running Other-Driver [{cfg}] with project file: {project_path}")
      run_project("other-editor", cfg, "other_editor", "resources/editor-config.toml", args.verbose, project_path=project_path)
    
    elif args.run_server:
      run_project("other-server", cfg, "other_server", "server-config.toml", args.verbose, extra_args=["--cwd", "other-server"])

    elif args.run_scratch:
      print(f"Running Other-Scratch [{cfg}]")
      run_project("scratch" , cfg, "gl-testing", "resources/gl-test-config.toml", args.verbose)
    
    elif args.run_terminal:
      print(f"Running Other-Terminal [{cfg}]")
      run_project("other-terminal", cfg, "other_terminal", "resources/dev-config.toml", args.verbose)
      
    elif args.run_tests:
      print("Running tests...")
      extra_args = [ "--gtest_shuffle" ]
      ## TODO: fix platform specific output paths
      if cfg == "Debug" or cfg == "ProfileD":
        extra_args.append("--gtest_output=xml:other_test_results.windows.debug.xml")
      else:
        extra_args.append("--gtest_output=xml:other_test_results.windows.release.xml")
      run_project("tests", cfg, "other_tests", "resources/dev-test-config.toml", args.verbose, extra_args=extra_args)

    elif args.run_test_suite is not None and len(args.run_test_suite) == 1:
      test_filter = args.run_test_suite[0]
      print(f"Running test suite with filter: {test_filter}")
      extra_args=[f"--gtest_filter={test_filter}", "--gtest_shuffle"]
      ## TODO: fix platform specific output paths
      if cfg == "Debug" or cfg == "ProfileD":
        extra_args.append("--gtest_output=xml:other_test_results.windows.debug.xml")
      else:
        extra_args.append("--gtest_output=xml:other_test_results.windows.release.xml")
      run_project("tests", cfg, "other_tests", "resources/dev-test-config.toml", args.verbose, extra_args=extra_args)
    
    elif args.run_soak:
      print(f"Running Soak Harness [{cfg}]")
      report_path = "logs/soak-report.json"
      if os.path.exists(report_path):
        os.remove(report_path)
      run_project("tests/harness", cfg, "other_soak", "tests/harness/soak-config.toml", args.verbose)

      ## the driver exits 0 for any clean run; the verdict lives in the report
      if not os.path.exists(report_path):
        print(f"Soak FAILED: no report written to {report_path} (harness crashed or never finalized).")
        sys.exit(1)
      with open(report_path, "r") as f:
        report = json.load(f)
      print(f"Soak result: {'PASS' if report.get('pass') else 'FAIL'} - {report.get('reason')}")
      if not report.get("pass"):
        sys.exit(1)

    elif args.daemon_server:
      run_subprocess(["pwsh.exe", "-File", "tools/daemon-server.ps1"])
      
  except subprocess.CalledProcessError as e:
    print(f"Error: {e}")
    sys.exit(1)