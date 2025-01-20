# Polished Crystal Save Patcher

This project is a WebAssembly-based tool that patches Polished Crystal save files between the following versions:

| Save Version | Canonical Name |
|--------------|----------------|
| 7            | 3.0.0-beta     |
| 8            | 3.0.0          |
| 9            | 3.1.0          |

## Features

- Provides a user-friendly web interface for patching save files.
- Outputs detailed logs for troubleshooting.

## Usage

1. **Save your game inside a Pokémon Center.**
2. **Back up your original save file somewhere safe.**
3. Open the [Polished Crystal Save Patcher](https://fellowship-of-the-roms.github.io/polished-save-patcher/).
4. Click the "Choose File" button to select your old save file.
5. Click the "Patch Save" button to start the patching process.
6. Once the patching is complete, a new save file will be downloaded automatically.
7. After loading your patched save for the first time, promptly exit the Pokémon Center before interacting with anything.

---

## Building From Source

### Requirements

- [Emscripten](https://emscripten.org/docs/getting_started/index.html) (Ensure you can run `emcc` in your environment.)
- [Python 3](https://www.python.org/) (for serving files locally if you want to test in a browser)

You can either build natively on Linux/macOS or use Windows with Emscripten. If using Windows Subsystem for Linux (WSL), see the note below about serving files locally.

### Steps

1. **Clone the repository**:

   ```sh
   git clone https://github.com/fellowship-of-the-roms/polished-save-patcher.git
   cd polished-save-patcher
   ```

2. **Build**:

   ```sh
   # Normal build (unoptimized)
   make
   # OR for a release (optimized) build
   # make release
   ```

   The build artifacts will appear in the `build` directory

3. **Serve the build locally**:
   Note on WSL: If you are using WSL, running `python3 -m http.server` directly inside WSL will start the server on WSL's localhost. To access it from a Windows browser, use http://<WSLIP>:8000. Use `ip addr` to find the <WSLIP> address.
   ```sh
   python3 -m http.server
   ```
   Open [http://localhost:8000](http://localhost:8000) in your browser.

### Windows Users
* If you prefer a full Windows approach without WSL, you can:
   1. Install and activate Emscripten on Windows (using `emsdk`).
   2. Open a command prompt or PowerShell with Emscripten environment set (`emsdk_env.bat`).
   3. Run `make release` (or just `make`) from there.
   4. Serve the files (using `python3 -m http.server`) similarly from the `build` directory.

## Running From a Release
If you prefer to use a pre-built release instead of compiling from source:
   1. Download the build package from the [Releases](https://github.com/fellowship-of-the-roms/polished-save-patcher/releases) page.
   2. Extract it into a folder of your choice.
   3. Star a local server from inside that folder:
      ```sh
      python3 -m http.server
      ```
   4. Open [http://localhost:8000](http://localhost:8000) in your browser.
