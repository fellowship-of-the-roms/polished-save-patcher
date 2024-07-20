# Polished Crystal Save Patcher

This project is a WebAssembly-based tool that patches Polished Crystal save files from Version 7 (3.0.0-beta) to Version 8 (9bit). It allows you to update your save files to ensure compatibility with the latest version of the game.

## Features

- Patches save files from Version 7 (3.0.0-beta) to Version 8 (9bit).
- Provides a user-friendly web interface for patching save files.
- Outputs detailed logs for troubleshooting.

## Usage

1. **Save your game inside a Pokémon Center.**
2. **Back up your original save file somewhere safe.**
3. Open the [Polished Crystal Save Patcher](https://vulcandth.github.io/polished-save-patcher/).
4. Click the "Choose File" button to select your old save file.
5. Click the "Patch Save" button to start the patching process.
6. Once the patching is complete, a new save file will be downloaded automatically.
7. After loading your patched save for the first time, promptly exit the Pokémon Center before interacting with anything.

## Building from Source

To build this project from source, you need to have [Emscripten](https://emscripten.org/) installed. Follow these steps:

1. **Clone the repository:**
   ```sh
   git clone https://github.com/vulcandth/polished-save-patcher.git
   cd polished-save-patcher
   ```

2. **Build the project:**
   `make`

3. Run a local server to test:
   ```sh
   cd build
   python3 -m http.server
   ```
