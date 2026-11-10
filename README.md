# SampleRealm: Meter

An output meter with a spectrum, a stereo field scope, and loudness with true peak, made to look good on screen

## Features

The meter only listens: the audio goes out exactly as it came in.

```
in ──────────────────────────────────────────────→ out
 │
 └─→ loudness + true peak ──┐
 └─→ spectrum ──────────────┼─→ frame ─→ editor
 └─→ stereo field + beat ───┘
```

### Spectrum

- 96 bands spaced evenly in pitch, 20 Hz to 20 kHz
- 4096-point FFT at 44.1 and 48 kHz, longer at higher rates so the window covers about the same time
- Tilted up 4.5 dB/oct around 1 kHz, so most music reads about level
- Rises fast and falls 24 dB a second, with a thin line holding each band's highest level

### Stereo Field

- Goniometer with a fading trace: mono is an upright line, out of phase lies flat
- Scaled to the signal's recent peak, so quiet material fills the scope too
- Correlation, -1 to +1, turning red below 0
- Left/right balance

### Loudness

- ITU-R BS.1770-4 and EBU R 128: momentary (400 ms), short-term (3 s) and gated integrated loudness, in LUFS
- Loudness range (EBU Tech 3342), in LU
- Bars for momentary and short-term, with the integrated loudness as a line across them

### True Peak

- 4× oversampled, as BS.1770-4, so peaks between samples are caught
- A falling bar per channel, and the highest peak held in dBTP
- A clip counter: each time the signal goes over 0 dBTP, with overs within 100 ms counted once

### Play and Reset

- Starting playback starts the integrated loudness, the range, the held peaks and the clip count again
- While the host is stopped they wait, and the integrated loudness shows PAUSED
- If the host stops sending audio altogether, the display falls away as it would through silence
- With no host transport, as in the Standalone, they always run
- **RESET** starts them again by hand, along with the spectrum's held line

### Views and Size

- One dashboard with all three panels
- A panel's corner button, or a double-click, expands it over the whole editor, and the same goes back
- The editor is resizable from 800 × 500 to 1920 × 1200. Its size is saved with the session, not with presets

### Look

- Five colour themes: Neon, Ember, Phosphor, Ultraviolet and Ice
- **FX** sets how much the display moves with the beat: Full, Subtle or Off
- **CAPTURE** hides the header and the small labels for a clean screen recording, leaving a small wordmark.
  Move the mouse and press **EXIT CAPTURE** to go back
- Presets store the look (theme, FX and view), with six factory looks in `Assets/Presets/`

## System Requirements

**macOS**

- macOS 12.0 (Monterey) or later
- Universal 2 - runs natively on Intel and Apple Silicon
- VST3, AU, or Standalone

**Windows**

- Windows 10 or later, 64-bit
- VST3 or Standalone

On both, mono or stereo, and the input layout must match the output.

## Build Requirements

- CMake 3.25+
- A C++23 capable compiler
  - **macOS**: Xcode or its Command Line Tools (the `xcode` preset needs the full Xcode app)
  - **Windows**: Visual Studio 2022 or later with the "Desktop development with C++" workload
- Git

JUCE is fetched automatically at configure time via CPM into `.cpm-cache/`, shared by every preset and by `wasm/` -
there is no submodule to initialize. A build folder made before the cache needs its `_deps/juce-*` removed first.

The web demo also needs Emscripten (`brew install emscripten`) and Node 24+.

## Commands

Every command is the same on both platforms, except the rows marked **macOS** or **Windows**. Each
workflow configures, builds and, for tests, runs in one go.

| Task | Command |
|---|---|
| Build the plugin (Debug) | `cmake --workflow --preset debug` |
| Build the plugin (Release) | `cmake --workflow --preset release` |
| Run the unit tests | `cmake --workflow --preset unit` |
| Run the unit tests and pluginval | `cmake --workflow --preset test` |
| Run all tests against a Release build | `cmake --workflow --preset test-release` |
| Build the plugin with the WebView editor (Release) | `cmake --workflow --preset webview` |
| Launch the Standalone app (Debug) | `cmake --build --preset run` |
| Install for your DAW: **macOS** | Automatic on every build |
| Install for your DAW: **Windows** | `cmake --build --preset install-windows`, from an administrator prompt |
| Open in an IDE for debugging: **macOS** | `cmake --preset xcode`, then open `build-xcode/Meter.xcodeproj` |
| Open in an IDE for debugging: **Windows** | Open `build-debug\Meter.sln` in Visual Studio after a Debug workflow |

`run` and `install-windows` use the Debug build, so run `cmake --workflow --preset debug` once first.
Windows installs on demand rather than after every build because its VST3 folder needs
administrator rights; `-DPLUGIN_COPY_AFTER_BUILD=ON` opts back in.

The individual steps still work on their own, for example to pass a `-D` option:

```bash
cmake --preset debug -DPLUGINVAL_STRICTNESS=5   # configure
cmake --build --preset test                     # build
ctest --preset debug                            # test
```

`cmake --list-presets=all` lists the presets for your platform: `xcode` only appears on macOS, and
`install-windows` only on Windows.

## Unit Tests

Unit tests live in `Tests/` and use [Catch2](https://github.com/catchorg/Catch2). They build into a
`Meter_Tests` executable that links the plugin's code and drives the processor directly, checking
a snapshot of every parameter as hosts see it, bus layouts, state save and restore, that the audio
passes through bit for bit, the loudness against the signals and tolerances of EBU Tech 3341 and 3342
at 44.1, 48 and 96 kHz, the true peak, the spectrum, the stereo field, the transport's resets, and
the editor's layout, views, themes and capture mode.

Add a `.cpp` file to `Tests/` and reconfigure to pick it up. `Tests/TestHelpers.h` has helpers for
setting parameters and processing buffers. Set `-DENABLE_UNIT_TESTS=OFF` to skip them entirely.

Like pluginval, the tests are not part of a plain build: the `debug` workflow builds only the plugin.

## Validating the Plugin

[pluginval](https://github.com/Tracktion/pluginval) loads the built plugin as a host would and
tests it for stability. It is built from source on demand, so there is nothing to install.

```bash
cmake --build --preset debug --target validate   # builds, then validates
ctest --preset debug                             # validates an existing build
```

Logs land in `build-debug/pluginval-logs/`. Strictness defaults to 10; use
`-DPLUGINVAL_STRICTNESS=5` (range 1 to 10) for a faster run, or `-DENABLE_PLUGINVAL=OFF` to skip
pluginval entirely.

On macOS the AU test validates the installed component in `~/Library/Audio/Plug-Ins/Components`,
since macOS resolves Audio Units through its registry rather than by path, so it needs
`PLUGIN_COPY_AFTER_BUILD` left on. Steinberg's VST3 conformance validator is off by default, as it
pulls ~300MB of SDK for one extra test; enable it with `-DPLUGINVAL_VST3_VALIDATOR=ON`.

## CI

[`.github/workflows/ci.yml`](.github/workflows/ci.yml) builds the Release plugin on macOS and
Windows and runs the unit tests. It only runs when started by hand: **Actions → CI → Run workflow**
on GitHub, or `gh workflow run ci.yml`. Tick **Also run pluginval** (or pass `-f pluginval=true`) to
validate the plugin too, which takes longer.

Each run's page lists the plugins under **Artifacts** for 30 days: `Meter-macOS.tar.gz` (VST3 and
AU) and `Meter-Windows.tar.gz` (VST3). They are unsigned.

## Parameters

`Source/shared/ParameterSpecs.h` is the only parameter list, read by the plugin, the wasm engine and
the web UI. The snapshot test in `Tests/ProcessorTests.cpp` pins what hosts and saved sessions see.
The three parameters (theme, reactivity, view) only change the look, and hosts can't automate them.

## Engine and Editors

- `Source/dsp/MeterEngine` measures everything. `process()` takes the audio on the audio thread, and
  `updateFrame()` turns what it heard into one flat array of floats, a frame, about 60 times a second.
  All the smoothing and ballistics happen there, so the editors only draw.
- `Source/shared/MeterFrame.h` is the frame's layout, `MeterScales.h` the display scales and `Themes.h` the colour themes.
  The web UI gets all three from the engine rather than keeping a copy.
- The native editor draws the frame in `Source/ui/meter/` and the React editor in `webui/src/meter/`.
  The files mirror each other, and their layout, scale and text tests share the same cases, so change both together.
- The engine uses only `juce_audio_basics`, as the wasm build has no other JUCE module.
- The K-weighting filter's frequencies, gain and Qs in `Source/dsp/LoudnessMeter.cpp` are the ones
  [libebur128](https://github.com/jiixyj/libebur128) (MIT licence) uses to make BS.1770's filters at any sample rate.

## Web Demo and WebView Editor

| Task | Command |
|---|---|
| Build and test the demo (engine and UI tests) | `cd wasm && cmake --workflow --preset wasm` |
| Serve the demo with live reload at http://localhost:8099 | `cd webui && npm run serve` |
| Typecheck and test the UI on their own | `cd webui && npm run typecheck && npm test` |
| Build the plugin with the WebView editor | `cmake --workflow --preset webview` |

- The demo builds to `wasm/build-wasm/SampleRealm-Meter-<version>/`, named from `COMPANY_NAME`,
  `project()` and `VERSION` in `CMakeLists.txt`. Upload that folder; the site embeds its `index.html` in an iframe.
  [`wasm.yml`](.github/workflows/wasm.yml) builds the same folder on GitHub as an artifact of that name, run by hand:
  **Actions → Web demo → Run workflow**, or `gh workflow run wasm.yml`.
- The demo's input is made by `webui/src/demo/loop.ts`. For a recording, add it to
  `webui/public/demo/` as AAC (`.m4a`) so Safari can decode it, and decode it in `startAudio`.
- The demo runs the same engine as the plugin, so its readings are the plugin's. It is a fixed 960 × 600, scaled to fit the page.
- The `webview` build installs over the native one; run the `release` workflow to switch back.
- Delete `wasm/patches/juce-wasm.patch` once JUCE fixes its wasm support.

## Debugging in Xcode

### 1. Generate Xcode Project

```bash
cmake --preset xcode
open build-xcode/Meter.xcodeproj
```

### 2. Configure Debugging

1. Select your plugin target from the scheme dropdown
2. Go to **Product → Scheme → Edit Scheme**
3. Click **Run** on the left sidebar
4. Under **Executable**, choose **Other** and navigate to executable.

### 3. Build and Run

1. Press **Cmd+B** to build the plugin
2. Press **Cmd+R** to run with AudioPluginHost
3. Load your plugin in AudioPluginHost
