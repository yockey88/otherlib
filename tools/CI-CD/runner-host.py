import subprocess
from aiohttp import web

def _print_json_data(data, indent=0):
  indent_str = "  " * indent
  if isinstance(data, dict):
    for key, value in data.items():
      print(f"{indent_str}{key}:")
      _print_json_data(value, indent + 1)
  elif isinstance(data, list):
    for index, item in enumerate(data):
      print(f"{indent_str}[{index}]:")
      _print_json_data(item, indent + 1)
  else:
    print(f"{indent_str}{data}")

class TestRunner:
  def __init__(self):
    ...
    
  def _validate_run_request(self, request, request_payload):
    # check it is push event
    if not request_payload:
      print("Invalid request: payload is empty")
      return False
    
    if request.headers.get("X-GitHub-Event") != "push":
      print("Invalid event type: expected 'push', got '{}'".format(request.headers.get("X-GitHub-Event")))
      return False
    
    if request.headers.get("Content-Type") != "application/json":
      print("Invalid content type: expected 'application/json', got '{}'".format(request.headers.get("Content-Type")))
      return False  
    
    return True
  
  async def _get_data(self, request):
    data = await request.json()
    if not self._validate_run_request(request, data):
      raise ValueError("Invalid request")
    else:
      for key, value in request.headers.items():
        print(f"Header: {key} = {value}")
    return data
  
  async def _handle_run_test_get(self, data):
    print(f"Handling GET request with data: {data}")
    return web.Response(status=200, text="Request validated successfully")
  
  async def _handle_run_test_post(self, data):
    for key, value in data.items():
      _print_json_data({key: value})
    return web.Response(status=200, text="Request validated successfully")
    
  async def get_run_tests(self, request):
    try:
      data = await self._get_data(request)
      return await self._handle_run_test_get(data)
      
    except ValueError as e:
      print(f"Validation error: {e}")
      return web.Response(status=400, text=str(e))
    
    except subprocess.CalledProcessError as e: 
      print(f"Error executing CI/CD pipeline: {e}")
      return web.Response(status=500, text=f"Error executing CI/CD pipeline: {e}")
    
    except Exception as e:
      print(f"Unexpected error: {e}")
      return web.Response(status=500, text=f"Unexpected error: {e}")
    
  async def post_run_tests(self, request):
    try:
      data = await self._get_data(request)
      return await self._handle_run_test_post(data)
    
    except subprocess.CalledProcessError as e: 
      print(f"Error executing CI/CD pipeline: {e}")
      return web.Response(status=500, text=f"Error executing CI/CD pipeline: {e}")
  
    except Exception as e:
      print(f"Unexpected error: {e}")
      return web.Response(status=500, text=f"Unexpected error: {e}")

# async def main():
Tester = TestRunner()
app = web.Application()
app.add_routes([
  web.get('/run-test', Tester.get_run_tests),
  web.post('/run-test', Tester.post_run_tests)
])
web.run_app(app)

# if __name__ == "__main__":
#   try:
#     import asyncio
#     asyncio.run(main())
#   except KeyboardInterrupt:
#     print("Runner host shutting down...")
#   print("Runner host has stopped.")