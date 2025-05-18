import os
import subprocess
import sys
import argparse

def recursive_compile_flatbuffers(path, output_dir="network/fb_specs"):
  """
  Recursively compile all .fbs files in the given directory and its subdirectories.
  """
  for root, dirs, files in os.walk(path):
    for file in files:
      if file.endswith(".fbs"):
        fbs_file = os.path.join(root, file)
        subprocess.run(["extern/flatbuffers/flatc", "-o", output_dir, "--cpp", fbs_file], check=True)
    
    for dir in dirs:
      dir_path = os.path.join(root, dir)
      recursive_compile_flatbuffers(dir_path, output_dir)

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description="A simple CLI for a Python project.")
  # -rs <sandbox-name> OR --run-sandbox <sandbox-name>
  parser.add_argument("--build", "-b", action="store_true", help="Build the project.")
  parser.add_argument("--run", "-r", action="store_true", help="Run the main driver.")

  args = parser.parse_args()
  try:
    if not args.build and not args.run:
      print("No action specified. Use --build or --run.")
      parser.print_help()
      sys.exit(1)

    if args.build:
      print("Building the project...")
      # build fbs first
      recursive_compile_flatbuffers("network/fb_specs")
      subprocess.run(["ninja"], check=True, cwd="build")

    if args.run:
      subprocess.run(["build/otherenv_driver.exe"], check=True)
      
  except subprocess.CalledProcessError as e:
    print(f"Error: {e}")
    sys.exit(1)
