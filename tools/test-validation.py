import os
import regex as re

def validate_test_success(results_file) -> bool:
  if not os.path.exists(result_file):
    print(f"Test result file {result_file} does not exist.")
    return False

  pattern = re.compile(r'failures="[1-9][0-9]*"')
  with open(result_file, "r") as f:
    content = f.read()
    matches = len(pattern.findall(content))
    if matches > 0:
      return False
  return True


if __name__ == "__main__":
  # get first arg as test result file path
  import sys
  if len(sys.argv) > 1:
    result_file = sys.argv[1]
  else:
    result_file = "other_test_results.xml"

  if validate_test_success(result_file):
    print("All tests passed successfully.")
    sys.exit(0)
  else:
    print("Some tests failed.")
    sys.exit(1)