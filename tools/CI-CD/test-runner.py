import subprocess
from aiohttp import web

def validate_run_request(request):
  # Implement validation logic for the incoming request here
  # For example, you might check for specific query parameters, headers, or authentication tokens
  return True

@routes.get('/run-test')
async def handle_run_test_request(request):
  try:
    if not validate_run_request(request):
      return web.Response(status=400, text="Invalid request")
    else:
      for key, value in request.headers.items():
        print(f"Header: {key} = {value}")
      return web.Response(status=200, text="Request validated successfully")
    
  except subprocess.CalledProcessError as e: 
    print(f"Error executing CI/CD pipeline: {e}")
    return web.Response(status=500, text=f"Error executing CI/CD pipeline: {e}")
  
  except Exception as e:
    print(f"Unexpected error: {e}")
    return web.Response(status=500, text=f"Unexpected error: {e}")

@routes.post('/run-test')
async def handle_run_test_post_request(request):
  try:
    if not validate_run_request(request):
      return web.Response(status=400, text="Invalid request")
    else:
      for key, value in request.headers.items():
        print(f"Header: {key} = {value}")
      return web.Response(status=200, text="Request validated successfully")
    
  except subprocess.CalledProcessError as e: 
    print(f"Error executing CI/CD pipeline: {e}")
    return web.Response(status=500, text=f"Error executing CI/CD pipeline: {e}")
  
  except Exception as e:
    print(f"Unexpected error: {e}")
    return web.Response(status=500, text=f"Unexpected error: {e}")