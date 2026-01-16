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

## Architecture Overview

The patcher ships as a WebAssembly module compiled from the C++ sources in `src/`. The browser-hosted UI in `build/polished_save_patcher.html` loads the compiled module, feeds it the uploaded save file, and streams progress back to JavaScript for display. Each patch lives in `src/patching` where it consumes helpers from `src/core` to read and modify binary structures.

The patcher embeds the per-version symbol databases as compressed blobs in the compiled binary (WASM/CLI). They are decompressed and parsed at runtime; the compressed payloads are small, but the in-memory symbol map will use more memory than the compressed size.

### Data Flow
- User uploads a save file through the web UI.
- The JavaScript shim validates file size and forwards the bytes to the WebAssembly entry point.
- C++ parsing logic in `SaveBinary` maps the byte stream into versioned structures.
- Selected patch routines mutate the in-memory representation using utilities in `CommonPatchFunctions` and constants in `PatcherConstants.h`.
- The patched save is serialized back to bytes and returned to JavaScript for download alongside log output written through `Logging`.

## Usage

1. **Save your game near the stairs on the second floor of a Pokémon Center.**
2. **Back up your original save file somewhere safe.**
3. Open the [Polished Crystal Save Patcher](https://fellowship-of-the-roms.github.io/polished-save-patcher/).
4. Click the "Choose File" button to select your old save file.
5. Click the "Patch Save" button to start the patching process.
6. Once the patching is complete, a new save file will be downloaded automatically.
7. After loading your patched save for the first time, promptly exit the Pokémon Center before interacting with anything.

---

## Platform-Specific Setup & Usage Examples

### Windows (PowerShell)
1. Install the latest [emsdk](https://emscripten.org/docs/getting_started/downloads.html) and run `emsdk_env.ps1` to populate environment variables.
2. Confirm the toolchain: `emcc --version`.
3. Clone the repository and build:
   ```powershell
   git clone https://github.com/fellowship-of-the-roms/polished-save-patcher.git
   cd polished-save-patcher
   make release
   ```
4. Serve the build for local testing:
   ```powershell
   cd build
   python -m http.server
   ```
5. Open `http://localhost:8000` in a browser and load a save file as described above.

### macOS (zsh)
1. Install emsdk via Homebrew or the official installer, then `source path/to/emsdk_env.sh`.
2. Validate the compiler: `emcc --version`.
3. Build and run:
   ```zsh
   git clone https://github.com/fellowship-of-the-roms/polished-save-patcher.git
   cd polished-save-patcher
   make release
   python3 -m http.server --directory build
   ```
4. Visit `http://localhost:8000` and patch your save file.

### Linux (bash)
1. Install emsdk following the official docs and activate it with `source ./emsdk_env.sh`.
2. Ensure dependencies: `which emcc python3`.
3. Build and serve:
   ```bash
   git clone https://github.com/fellowship-of-the-roms/polished-save-patcher.git
   cd polished-save-patcher
   make
   python3 -m http.server --directory build
   ```
4. Navigate to `http://localhost:8000` in your preferred browser to apply patches.

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

## Troubleshooting
- **`ReferenceError: FS` or missing WASM file**: Re-run `make` to regenerate `polished_save_patcher.js` and ensure the `build` directory is served.
- **Patch reports "unsupported version"**: Confirm the save originates from versions 7, 8, or 9 and that you selected the correct file (.sav, not .sgm).
- **Browser refuses to download output**: Check for pop-up blockers or try a different browser; the download is triggered via JavaScript `Blob`.
- **`emcc` not found**: Verify the Emscripten environment script was sourced (`emsdk_env` on Windows, `emsdk_env.sh` on Unix-based systems).

## Developer Onboarding
- Install clang-format and clang-tidy to keep code style consistent; default configurations live in the project root when added.
- Build locally with `make` before pushing to ensure the WebAssembly artifacts compile.
- Run `python tools/map_events.py` and related helpers to regenerate derived data when touching event maps.
- Prefer adding new patches under `src/patching` with matching headers in `include/patching` and wire them up in the patch orchestration code in `main.cpp`.
- Document notable save-structure discoveries in `include/core` headers so future romhack support can reuse them.

## Running From a Release
If you prefer to use a pre-built release instead of compiling from source:
   1. Download the build package from the [Releases](https://github.com/fellowship-of-the-roms/polished-save-patcher/releases) page.
   2. Extract it into a folder of your choice.
   3. Star a local server from inside that folder:
      ```sh
      python3 -m http.server
      ```
   4. Open [http://localhost:8000](http://localhost:8000) in your browser.
