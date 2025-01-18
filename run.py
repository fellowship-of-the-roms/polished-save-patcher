import os
import subprocess

# Change to the build directory
os.chdir('build')

# Start the HTTP server
try:
    subprocess.run(['python3', '-m', 'http.server'], check=True)
except KeyboardInterrupt:
    print("\nServer stopped.")
