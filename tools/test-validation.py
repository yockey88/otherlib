import os
import regex as re

def _get_test_suites(results_content):
  pattern = re.compile(r'<testsuite.*?name="(.*?)".*?tests="(\d+)".*?failures="(\d+)".*?>(.*?)</testsuite>', re.DOTALL)
  return pattern.findall(results_content)

def _print_failure_details(test_suites, test_failures):
  for suite in test_suites:
    suite_name = suite[0]
    failures = int(suite[2])
    if failures > 0:
      print(f" - Test suite '{suite_name}' had {failures} failures.")
      
  print("")
  for suite in test_suites:
    suite_name = suite[0]
    failures = int(suite[2])
    test_suite_content = suite[3]
    if failures > 0:
      # passing unskipped test:
      #     <testcase name="..." file="..." line="..." status="..." time="..." timestamp="..." classname="..." />
      # skipped test:
      #     <testcase name="..." file="..." line="..." status="skipped" time="..." timestamp="..." classname="...">
      #       <skipped message="..."><![CDATA[...]]></skipped>
      #     </testcase>
      # failed test:
      #     <testcase name="..." file="..." line="..." status="..." time="..." timestamp="..." classname="...">
      #       <failure message="..."><![CDATA[...]]></failure>
      #     </testcase>
      failure_pattern = re.compile(
        r'<testcase\b[^>]*\bname="([^"]+)"[^>]*>' r'(?:(?!<testcase\b|</testcase>).)*?'
        r'<failure\b[^>]*(?:>(.*?)</failure>|/>)' r'(?:(?!<testcase\b|</testcase>).)*?</testcase>',
        re.DOTALL
      )
      for test_name, failure_message in failure_pattern.findall(test_suite_content):
        failure_output = failure_message.strip() if failure_message else "(no failure body provided)"
        print(f"[FAILED TEST: {suite_name}.{test_name}]:\n{failure_output}")

def validate_test_success(result_file) -> bool:
  if not os.path.exists(result_file):
    print(f"Test result file {result_file} does not exist.")
    return False
  
  test_suites = _get_test_suites(open(result_file, 'r').read())
  total_tests = sum(int(suite[1]) for suite in test_suites)
  total_failures = sum(int(suite[2]) for suite in test_suites)

  if total_failures > 0:
    print(f"Test suite failed with {total_failures} failures out of {total_tests} tests.")
    _print_failure_details(test_suites, total_failures)
    return False
  else:
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