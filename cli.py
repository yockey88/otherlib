"""Bootstrap wrapper around oecli, the Other Environment CLI.

Every development workflow lives in oecli itself (see other-cli/), so developers use
the exact tool that ships with a release. oecli is a build artifact though, so
something outside the build has to exist to create it the first time; that something
is this script.

  python cli.py <anything>            -> forwarded verbatim to the built oecli
  python cli.py build --tests -c Debug
  python cli.py test -c Debug
  python cli.py bootstrap [-c CFG] [--tests] [--regen]   (only build oecli itself)

When no oecli build exists yet, the script configures cmake, builds the oecli target,
stages the runtime DLLs oecli itself needs, and then forwards the command.
"""
import os
import subprocess
import sys

REPO_ROOT = os.path.dirname(os.path.abspath(__file__))
BUILD_DIR = os.path.join(REPO_ROOT, "build")
CONFIGS = ["Debug", "Release", "Profile", "ProfileD"]


def run(args, cwd=None):
  print(f"[cli.py] {' '.join(args)}", flush=True)
  return subprocess.call(args, cwd=cwd)


def oecli_path(cfg):
  return os.path.join(BUILD_DIR, "other-cli", cfg, "oecli.exe")


def find_oecli(preferred=None):
  order = ([preferred] if preferred else []) + [c for c in CONFIGS if c != preferred]
  for cfg in order:
    path = oecli_path(cfg)
    if os.path.exists(path):
      return path
  return None


def stage_oecli_dlls(cfg):
  ## mirror of the staging inside `oecli build`, trimmed to the DLLs oecli itself
  ##  loads; the full staging pass for every application runs inside the build tool
  import shutil
  family = "Debug" if cfg in ("Debug", "ProfileD") else "Release"
  extern = os.path.join(REPO_ROOT, "extern")
  dlls = [
    os.path.join(extern, "sdl", "lib", family.lower(), "SDL3.dll"),
    os.path.join(extern, "assimp", "lib", "assimp-vc143-mt.dll"),
    os.path.join(extern, "python312", "python312.dll"),
    os.path.join(extern, "sol2", "lib", "lua-5.4.4.dll"),
    os.path.join(extern, "jolt", "bin", family, "Jolt.dll"),
  ]
  physx = ["PhysX_64", "PhysXCommon_64", "PhysXCooking_64", "PhysXFoundation_64", "PhysXGpu_64"]
  if family == "Debug":
    physx.append("PVDRuntime_64")
  dlls.extend(os.path.join(extern, "physx", "bin", family, f"{name}.dll") for name in physx)

  destination = os.path.join(BUILD_DIR, "other-cli", cfg)
  for dll in dlls:
    if os.path.exists(dll):
      shutil.copy(dll, destination)
    else:
      print(f"[cli.py] warning: {dll} does not exist")


def bootstrap(cfg, with_tests=False, regen=False):
  have_project_files = any(os.path.exists(os.path.join(BUILD_DIR, name)) for name in ("other.sln", "other.slnx"))
  if regen or with_tests or not have_project_files:
    configure = ["cmake", "-S", REPO_ROOT, "-B", BUILD_DIR]
    if with_tests:
      configure.append("-DBUILD_OTHER_TESTS=ON")
    if run(configure) != 0:
      print("[cli.py] cmake project generation failed")
      sys.exit(1)

  if run(["cmake", "--build", BUILD_DIR, "--config", cfg, "--parallel", "--target", "oecli"]) != 0:
    print("[cli.py] failed to build oecli")
    sys.exit(1)

  stage_oecli_dlls(cfg)
  return oecli_path(cfg)


def infer_config(args):
  for i, arg in enumerate(args):
    if arg in ("-c", "--config") and i + 1 < len(args):
      return args[i + 1]
  return None


def main(argv):
  if argv and argv[0] == "bootstrap":
    cfg = infer_config(argv[1:]) or "Debug"
    if cfg not in CONFIGS:
      print(f"[cli.py] invalid config '{cfg}' (expected one of {', '.join(CONFIGS)})")
      return 1
    path = bootstrap(cfg, with_tests="--tests" in argv, regen="--regen" in argv)
    print(f"[cli.py] oecli ready at {path}")
    return 0

  cfg = infer_config(argv)
  if cfg is not None and cfg not in CONFIGS:
    print(f"[cli.py] invalid config '{cfg}' (expected one of {', '.join(CONFIGS)})")
    return 1

  oecli = find_oecli(cfg)
  if oecli is None:
    print("[cli.py] no oecli build found, bootstrapping...")
    oecli = bootstrap(cfg or "Debug", with_tests="--tests" in argv)

  ## forwarded from the caller's cwd so project tools (create, open) resolve
  ##  relative paths the way a direct oecli invocation would
  return subprocess.call([oecli] + argv)


if __name__ == "__main__":
  sys.exit(main(sys.argv[1:]))
