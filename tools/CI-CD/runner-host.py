import os
import subprocess
import sys
from aiohttp import web

routes = web.RouteTableDef()

def validate_run_request(request):
  # Implement validation logic for the incoming request here
  # For example, you might check for specific query parameters, headers, or authentication tokens
  return True

@routes.get('/run')
async def handle_run_request(request):
  try:
    if not validate_run_request(request):
      return web.Response(status=400, text="Invalid request")
    else:
      print("Received request to run CI/CD pipeline")
      print(f"Request query: {request.query_string}")
      print(f"Request method: {request.method}")
      print(f"Request path: {request.path}")
      for key, value in request.headers.items():
        print(f"Header: {key} = {value}")
      return web.Response(status=200, text="Request validated successfully")
    
  except subprocess.CalledProcessError as e: 
    print(f"Error executing CI/CD pipeline: {e}")
    return web.Response(status=500, text=f"Error executing CI/CD pipeline: {e}")
  
  except Exception as e:
    print(f"Unexpected error: {e}")
    return web.Response(status=500, text=f"Unexpected error: {e}")

# async def main():
app = web.Application()
app.add_routes(routes)
web.run_app(app)

# if __name__ == "__main__":
#   try:
#     import asyncio
#     asyncio.run(main())
#   except KeyboardInterrupt:
#     print("Runner host shutting down...")
#   print("Runner host has stopped.")