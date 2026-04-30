import os
import regex as re

def get_failed_test_names(content):
  pattern = re.compile(r'<testcase name="([^"]*)"')
  failure_pattern = re.compile(rf'<testcase name="{pattern}".*?<failure')
  failed_test_names = content.split('<testcase name="')[1:]
  return [name.split('"')[0] for name in failed_test_names if failure_pattern.search(content)]

def validate_test_success(results_file) -> bool:
  if not os.path.exists(result_file):
    print(f"Test result file {result_file} does not exist.")
    return False

  pattern = re.compile(r'failures="[1-9][0-9]*"')
  with open(result_file, "r") as f:
    content = f.read()
    matches = len(pattern.findall(content))
    if matches > 0:
      print(f"Found {matches} failed test(s) in the results.")
      test_names = get_failed_test_names(content)
      print("Failed test names:")
      for name in test_names:
        print(f" - {name}")
      return False
    
  
  print("All tests passed successfully.")
  return True


if __name__ == "__main__":
  # get first arg as test result file path
  import sys
  if len(sys.argv) > 1:
    result_file = sys.argv[1]
  else:
    result_file = "other_test_results.xml"

  if validate_test_success(result_file):
    sys.exit(0)
  else:
    sys.exit(1)