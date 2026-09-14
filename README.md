# Lynn

**Lynn** is a particle-life simulation combined with an audio visualizer: thousands of particles interact with each other under a variant of gravitational attraction, and their motion is driven in real time by whatever music is playing.

## Demo

**The Ocean** — `Stereo_FFT`, `NUM_TYPES = 2`

https://github.com/user-attachments/assets/c37befa8-85c6-476e-8fb8-c9df5d802fc5

**Way Back** — `Stereo_Left_FFT`, `NUM_TYPES = 2`

https://github.com/user-attachments/assets/b7d289e6-1de3-47f4-bfd3-f0a588850df4

**Escape From Heaven** — `Stereo_Alternating_FFT`, `NUM_TYPES = 4`

https://github.com/user-attachments/assets/ee7d2fcb-e1e9-4e7e-89f1-d8ec50129f51

**Why Do I** — `Stereo_Split_FFT`, `NUM_TYPES = 4`

https://github.com/user-attachments/assets/178ec9bf-4968-4558-b298-39f7b105293b

## The idea

This project started with me getting fascinated by the concept of [Particle Life](https://en.wikipedia.org/wiki/Particle_life) — simple particles following a few basic attraction/repulsion rules that, at scale, produce behavior that looks almost alive.

The force rule in Lynn is inspired by gravitational attraction, but with one twist: instead of the force vector always pointing straight at the other particle's center, it's rotated by an angle. That small angular offset stops particles from simply collapsing into a point — instead they start swirling, forming flows and structures that never quite repeat.

The audio side came later. I was watching a video on YouTube, saw the classic bouncing audio bars, and wanted to try building one myself. Halfway through, I realized it would be far more interesting if the music drove the particle-life simulation directly, instead of just a row of bars. Lynn is what came out of merging those two ideas.

## Tech stack

- **C++17**
- **[raylib](https://github.com/raysan5/raylib)** — rendering and the main render loop
- **[SDL2](https://github.com/libsdl-org/SDL)** — input handling and audio (real-time audio analysis driving the particles)
- **CMake** (FetchContent) — cross-platform build system, automatically fetches raylib and SDL2 on configure

## Build

Requirements: CMake ≥ 3.15, a C++17 compiler (MSVC / MinGW-w64 / GCC / Clang), and Git (so CMake can fetch raylib and SDL2).
 
```bash
git clone https://github.com/Lavanda-Aspen/Lynn.git
cd Lynn
cmake -B build
cmake --build build
```
 
On Windows, if you're using **MinGW-w64** (e.g. via MSYS2) rather than Visual Studio, specify the generator explicitly — otherwise CMake may default to a Visual Studio generator that doesn't match your compiler:
 
```powershell
cmake -B build -G "MinGW Makefiles"
cmake --build build
```
 
If you have Visual Studio installed and want to use it instead, the plain `cmake -B build` command above is enough — CMake will pick the VS generator automatically.
 
The executable ends up at `build/Lynn` (Linux/macOS) or `build/Lynn.exe` / `build/Release/Lynn.exe` (Windows), depending on the generator.
 
The initial `cmake -B build` step (which fetches and configures raylib/SDL2) only needs to be run once. After that, for any rebuild — after pulling changes or editing the code — just run `cmake --build build` again.

## Usage

Run the executable with the path to an audio file:

```bash
build/Lynn.exe "file_path"
```

Example:

```bash
build/Lynn.exe "Assets/The Ocean.wav"
```

## Configuration

A few constants near the top of `main.cpp` (`#define`) control the simulation:

- **`NUM_TYPES`** — should be set to **2 or 4**. Other values aren't guaranteed to behave correctly.
- **`MAX_PARTICLES`** — setting this too high can cause noticeable lag/stutter; tune it to your machine.

A handful of functions for "steering" how a particle's force angle gets rotated are already declared around **lines 30–34**. To try a different one, go to **line 121** and swap in the name of one of those functions — leave the surrounding variables as they are.

## Known limitations

- Only **`.wav`** audio files are supported for now.
- File paths/names should use **plain ASCII characters, no accents/diacritics** — non-ASCII filenames may fail to load correctly.

## License

Released under the [MIT License](LICENSE).

## Credits

- [raylib](https://www.raylib.com/) — by Ramon Santamaria (raysan5) and contributors, zlib/libpng license.
- [SDL2](https://libsdl.org/) — by Sam Lantinga and contributors, zlib license.
