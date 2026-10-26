# SishhinDSP — HTML WebView VST3 and Standalone (JUCE 8)

## Updated UI and 100+ MiB installed plugin

The red/yellow/green fake window buttons were removed from the original-looking
HTML interface. The build now ships **72 distinct synthetic 96 kHz / 32-bit /
5-second cabinet and room impulse-response WAV files**, arranged by three
cabinets, four microphones, and six positions each. They are actual WAV audio,
not empty padding or copies of unrelated files.

The complete library lives inside the installed plugin bundle at
`SishhinDSP.vst3/Contents/Resources/CabinetIRLibrary`. Its uncompressed size
is approximately **132 MiB** and the build **fails** if the complete plugin
bundle is not larger than 100 MiB. The GitHub Actions ZIP may be smaller
because it compresses the files. This is the *installed VST3 bundle size*,
not the C++ plugin binary's size.

The CAB panel has a built-in bank selector. Each selection loads the relevant
768-tap early impulse into the real-time audio engine. The full five-second
hybrid cabinet/room IR WAV files can also be loaded by third-party long-IR
convolution plugins; the SishhinDSP real-time convolution stage currently uses
**only the first 768 taps**, so its internal engine does not reproduce the full
five-second room tail. The IRs are algorithmically synthesized, not recordings
of physical amplifiers or microphones, and adding them does not itself improve
amp modeling quality.

The waveform library is produced reproducibly by
`tools/generate_ir_bank.py` during GitHub Actions builds. The library is
bundled with the VST3, not checked into Git as 132 MiB of binary files.

SishhinDSP is a **native audio plugin** whose interface displays the original
SishhinDSP HTML inside JUCE's embedded browser. Guitar audio is processed by the
separate C++ `AmpEngine`, **not** by JavaScript or the browser. The original
design reference is unchanged under
`DesignReference/Original-SishhinDSP.html`; the embedded runtime version is
`Assets/interface.html`, with nonvisual event handling in `Assets/bridge.js`.

## Platforms and requirements

- Windows: Windows 10/11 with the **Microsoft Edge WebView2 Runtime** installed.
  GitHub Actions downloads the WebView2 SDK via NuGet to compile the VST3 and
  standalone versions. The end user's machine still needs the Edge WebView2
  runtime (normally installed with Edge).
- macOS: JUCE uses the system WKWebView / WebKit component.
- The production UI does not require a local HTTP server or external audio
  engine. Web fonts requested by the original HTML may use fallback fonts
  when offline. Embedded Japanese logo/artwork is self-contained.

## Build with GitHub Actions

1. Open **Actions** in the GitHub repository.
2. Choose **Build SishhinDSP VST3 and Standalone** and **Run workflow**.
3. Download the Windows and/or macOS workflow artifacts after both jobs succeed.
4. In a DAW, load the **SishhinDSP** VST3 onto an **audio effect/insert track**.
   Choose the correct guitar interface input. Your DAW may require you to enable
   input monitoring.
5. Watch the **INPUT** and **OUTPUT** dBFS readouts under the interface. If
   INPUT remains blank, check audio-device and DAW input routing before adjusting
   gain. If input moves but output does not, check VOICE/power bypass, gate, and
   master/output.

GitHub Actions builds use the pinned JUCE revision in `CMakeLists.txt`,
CMake, MSVC on Windows and Clang/Xcode on macOS.

For a local portable-only audio DSP test:

```sh
cmake -S . -B test-build -DSDSP_BUILD_PLUGIN=OFF -DSDSP_BUILD_TESTS=ON
cmake --build test-build --config Release
ctest --test-dir test-build -C Release --output-on-failure
```

## HTML ↔ native bridge

- Every amp knob, top-bar control, 9-band EQ slider, pedal switch, amp/cab
  selection, microphone type and room-send controls reports JUCE host parameters
  through `window.__JUCE__.backend.emitEvent`.
- Native JUCE sends DAW session state and host automation back to the HTML
  through `sdspState` events. Saving/restoring DAW plugin state is handled by
  `AudioProcessorValueTreeState`.
- Factory presets 0–71 are held in the JUCE processor and selected from the
  original browser preset list. User presets in the HTML use its local browser
  storage; the resulting audio parameters are also committed to the host's
  current plugin state.
- Selected WAV/AIFF impulses (up to 8 MB of source data) are decoded in the
  **native** engine and saved in the host plugin state as a short IR. JS only
  renders the waveform preview. For low CPU cost, only the first **768**
  resampled FIR taps are used (not a full-length convolution reverb).
- The original browser MIDI-access popup is replaced with a help message:
  MIDI Program Change 0–71 works through MIDI sent to the plugin from the DAW.
- The reference HTML has a disabled TUNER control. It remains disabled here;
  a native tuner algorithm is **not** implemented.

## Audio pipeline

Guitar input → gate → transpose → Bloom / Edge / Weight pedals → selected
Sakura Clean / Taki Metal / Take Thick amp → tone stack → chosen cabinet and
microphones → optional native impulse response → graphic EQ → doubler →
Echo / Halo with room sends → output.

**Validation status:** portable DSP tests check non-silent finite outputs,
distinct amp responses, IR and other DSP features; these do **not** guarantee
a successful Windows/macOS VST3 build, DAW compatibility, or artifact usability.
Use the current GitHub Actions run for the actual platform build result.

JUCE is © Raw Material Software; check its licensing requirements before
commercial distribution.
