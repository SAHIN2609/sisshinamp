# SishhinDSP — Native JUCE VST3 + Standalone Source Package

**This is a native C++/JUCE plugin project, not an HTML/WebView wrapper.**

The original uploaded HTML is included, unchanged, under
`DesignReference/Original-SishhinDSP.html`. The runtime UI is recreated with
JUCE graphics, knobs, buttons, menus, panels, and a Japanese logo extracted
from that HTML. The design is similar to the reference, **not pixel-identical**.

## Building with GitHub Actions (Windows + macOS)

1. Download this ZIP, extract it, and upload **the contents of the top-level
   folder** (not a folder nested inside the repository) to your own GitHub repo.
   The repository root must contain `CMakeLists.txt`, `Source/`, `Assets/`,
   `tests/`, and `.github/workflows/build.yml`.
2. Open the repository's **Actions** tab and select
   **Build SishhinDSP VST3 and Standalone**.
3. Choose **Run workflow**, or push to `main` / `master` to trigger a build.
4. When the runs finish, download `SishhinDSP-Windows` and `SishhinDSP-macOS`
   from the workflow's **Artifacts** section.
5. Install the `.vst3` into your DAW (on Windows, typically
   `C:\Program Files\Common Files\VST3`; on macOS, typically
   `/Library/Audio/Plug-Ins/VST3`). Run the standalone binary directly.

No token, login, licensing server, GitHub API key, or WebView is needed at
plugin runtime. The workflows **download JUCE** from its official GitHub repo
while building; an internet connection is required in Actions.

For local builds with CMake 3.22+, C++17, and suitable MSVC/Xcode tools:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

## Controls / audio path

**Input → gate → pitch transpose → optional optical compression / drive /
low boost → Sakura/Taki/Take amp → tone stack → cabinet model → dual mic
mix → optional loaded IR → 9-band EQ → optional stereo doubler → tape echo
→ reverb → master/output.**

- Sakura Clean: high-headroom, gentle soft-knee saturation.
- Taki Metal: tight two-stage high-gain distortion.
- Take Thick: low-mid-heavy asymmetric saturation.
- Amp Gain, Bass, Middle, Treble, Presence, Master, Output, input/output gains
  are fully parameterized and automatable.
- Three cabinets: 1x12 open, 4x12 modern, 2x12 oversized.
- Two microphone sections: each has position, distance, level, pan, and on/off.
- Five effects: Bloom compressor, Edge distortion, Weight low boost, Echo
  delay, Halo reverb. Separate knob parameters and on/off automation.
- 9-band graphic EQ; noise gate; stereo/mono input; transpose; doubler.
- Cabinet IR loader accepts WAV and AIFF and stores IR data in plugin state.
  **IRs are limited to the first 768 resampled taps** (a short cabinet FIR).
  This is not a full-length partitioned convolution implementation.
- 72 named factory presets with settings, plus user `.sdsp` presets saved in
  Documents/SishhinDSP/Presets. Presets include the loaded short IR taps.
- MIDI Program Change loads factory programs (0–71), even if editor is closed.
- VST3 and standalone application, stereo and mono bus compatibility.

## Tests & limitations

The **portable DSP engine** can be compiled and run without JUCE, using:

```sh
cmake -S . -B test-build -DSDSP_BUILD_PLUGIN=OFF -DSDSP_BUILD_TESTS=ON
cmake --build test-build --config Release
ctest --test-dir test-build -C Release --output-on-failure
```

That test checks audible-path differences numerically and verifies output is
finite and non-silent, including transpose and IR. It is **not** a substitute
for a real VST3 compilation and DAW test. There is no local Windows or macOS
JUCE toolchain in the authoring environment, so those platform builds and
host compatibility are **not yet verified**. If GitHub Actions reports a
compiler error, use the first compiler error in the workflow log for debugging.

A limitation of this initial implementation is that the cab IR uses a short
768-tap time-domain FIR and the reverb is a compact delay-based effect, not
studio-quality partitioned convolution. The DSP is native and functional, but
sonic refinement and latency/alias testing in your DAW may still be needed.

JUCE is © Raw Material Software. Check the JUCE license for the obligations
that apply to your intended distribution; JUCE is not bundled in this ZIP.
