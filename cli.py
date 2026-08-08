"""Bootstrap wrapper around oecli, the Other Environment CLI.

Every development workflow lives in oecli itself (see other-cli/), so developers use
the exact tool that ships with a release. oecli is a build artifact though, so
something outside the build has to exist to create it the first time; that something
is this script.

  python cli.py <anything>            -> forwarded verbatim to the built oecli
  python cli.py build --tests -c Debug
  python cli.py test -c Debug
  python cli.py bootstrap [-c CFG] [--tests] [--regen]   (only build oecli itself)
  python cli.py bootstrap --user                         (build the user cli instead)
  python cli.py cloc                                     (capped-LOC ledger measure)

When no oecli build exists yet, the script configures cmake, builds the oecli target,
stages the runtime DLLs oecli itself needs, and then forwards the command.

The build tree carries two cli flavors: `oecli` (the developer cli with the source-tree
workflow tools; what this script builds and forwards to) and `oecli_user` (the
project-workflow-only oecli.exe that ships with the SDK, built into other-cli/user/).
`bootstrap --user` builds the user flavor for trying the shipped experience locally;
forwarding always targets the developer cli.
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


def oecli_path(cfg, user=False):
  ## the user cli builds into other-cli/user/<cfg>/ so the two flavors never collide
  parts = [BUILD_DIR, "other-cli"] + (["user"] if user else []) + [cfg, "oecli.exe"]
  return os.path.join(*parts)


def find_oecli(preferred=None):
  order = ([preferred] if preferred else []) + [c for c in CONFIGS if c != preferred]
  for cfg in order:
    path = oecli_path(cfg)
    if os.path.exists(path):
      return path
  return None


def stage_oecli_dlls(cfg, user=False):
  ## mirror of the staging inside `oecli build`, trimmed to the DLLs oecli itself
  ##  loads; the full staging pass for every application runs inside the build tool
  import shutil
  family = "Debug" if cfg in ("Debug", "ProfileD") else "Release"
  extern = os.path.join(REPO_ROOT, "extern")
  dlls = [
    os.path.join(extern, "sdl", "lib", family.lower(), "SDL3.dll"),
    os.path.join(extern, "assimp", "lib", "assimp-vc143-mt.dll"),
    os.path.join(extern, "sol2", "lib", "lua-5.4.4.dll"),
    os.path.join(extern, "jolt", "bin", family.lower(), "Jolt.dll"),
    os.path.join(extern, "steam", "redistributable_bin", "win64", "steam_api64.dll"),
  ]

  destination = os.path.dirname(oecli_path(cfg, user))
  for dll in dlls:
    if os.path.exists(dll):
      shutil.copy(dll, destination)
    else:
      print(f"[cli.py] warning: {dll} does not exist")


def bootstrap(cfg, with_tests=False, regen=False, user=False):
  have_project_files = any(os.path.exists(os.path.join(BUILD_DIR, name)) for name in ("other.sln", "other.slnx"))
  if regen or with_tests or not have_project_files:
    configure = ["cmake", "-S", REPO_ROOT, "-B", BUILD_DIR]
    if with_tests:
      configure.append("-DBUILD_OTHER_TESTS=ON")
    if run(configure) != 0:
      print("[cli.py] cmake project generation failed")
      sys.exit(1)

  target = "oecli_user" if user else "oecli"
  if run(["cmake", "--build", BUILD_DIR, "--config", cfg, "--parallel", "--target", target]) != 0:
    print(f"[cli.py] failed to build {target}")
    sys.exit(1)

  stage_oecli_dlls(cfg, user)
  return oecli_path(cfg, user)


LEDGER_UNCAPPED = ("other-editor", "other-cli", "other-server",
                   "other-csharp", "other-csharp-interop", "other-lua-interop")
LEDGER_EXTENSIONS = (".cpp", ".hpp", ".h", ".inl")


def capped_loc():
  ## the roadmap LOC ledger: raw lines of C++ sources under each capped module's src/;
  ##  the applications (editor/cli/server), the C# side, and the interops are uncapped
  rows = []
  for entry in sorted(os.listdir(REPO_ROOT)):
    if not (entry == "otherlib" or entry.startswith("other-")) or entry in LEDGER_UNCAPPED:
      continue
    src = os.path.join(REPO_ROOT, entry, "src")
    if not os.path.isdir(src):
      continue
    lines = 0
    for root, _dirs, files in os.walk(src):
      for name in files:
        if name.endswith(LEDGER_EXTENSIONS):
          with open(os.path.join(root, name), "rb") as source:
            lines += source.read().count(b"\n")
    rows.append((entry, lines))

  for entry, lines in rows:
    print(f"{entry:<20}{lines:>8}")
  print(f"{'total':<20}{sum(lines for _, lines in rows):>8}")
  return 0


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
    user = "--user" in argv
    path = bootstrap(cfg, with_tests="--tests" in argv, regen="--regen" in argv, user=user)
    print(f"[cli.py] {'user' if user else 'developer'} oecli ready at {path}")
    return 0
  elif argv and argv[0] == "cloc":
    return capped_loc()

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
