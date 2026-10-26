# SishhinDSP — HTML WebView VST3 and Standalone (JUCE 8)

## Live tuner and DAW MIDI mappings

The WebView's **TUNER** button is enabled. It opens a tuner overlay which
reads the **raw guitar DI before any cabinet, amplifier or pedals** and shows
a detected note, frequency in Hz, and cents relative to A440. The native
processor uses a lock-free, decimated input capture and a YIN-style pitch
detector on the UI thread; the tuner never records audio or sends it to a
network service. It requires an input signal in the DAW.

**LOAD LOGO** and its hidden image-upload input have been removed from the
plugin interface. The bundled Japanese amp logo remains in place.

Click **MIDI MAPPINGS** for a dialog resembling the familiar amp-plugin MIDI
assignment workflow. Map CC Absolute values to real JUCE parameters, assign
note-on/off messages, and assign specific MIDI Program Changes to factory
presets. Channel 0 (Omni) or channels 1–16 are supported, and unassigned
Program Changes 0–71 still select the corresponding factory preset. Right
click a knob, slider, amp choice, pedal switch or preset menu to choose
**Enable MIDI Learn** or **Open MIDI Mappings**. The standard browser right-click
menu stays suppressed. Mapping rows are saved in the DAW plugin state.

MIDI messages must be routed from the DAW to SishhinDSP; the plugin does not
request browser USB/Web MIDI permissions. CC learning and assignments are
applied on JUCE's message thread, not inside the real-time audio callback.
The MIDI event bridge uses a single latest-event slot, so extremely fast
simultaneous MIDI streams can drop intermediate events; regular footswitch
and expression-control messages are supported.

## Cleaner Sakura and Take voicings

Sakura CLEAN now has a largely linear, high-headroom preamp with only gentle
soft limiting on large peaks; Take THICK uses low-mid body shaping, mild
compression and substantial dry blend rather than multiple fuzz-like
distortion stages. WEIGHT's pre-amp boost is limited to avoid unexpected
overdrive. Taki METAL retains its tighter, distinct multi-stage saturation.
Expanded DSP regression tests check that Clean and Thick stay below
fuzz-like third-harmonic thresholds on a reference guitar test signal.

## Updated amp selector and modern Taki Metal voicing

The AMP page's three selector cards now display miniature amplifier heads
that match the main visual designs: Sakura cream woven/grilled cabinet, Taki
black metal with amber backlighting, and Take deep-green tweed. Each includes
a small nameplate, Japanese identity, miniature faceplate, input jack,
controls, power light and handle. This updates only the visual selectors.

Taki Metal's audio path now adds a dedicated high-pass before distortion,
filtered two-stage saturation, low-mid contouring, a mild pick-attack blend,
a restrained EDGE boost (when enabled), and post-clip low-pass / upper-mid
presence shaping. Factory metal presets reduce unnecessary overdrive and
rebalance bass/mid/presence for tighter riffs and palm mutes. Sakura Clean and
Take Thick keep their separate processing paths.

The portable DSP tests additionally compare third-harmonic content of the
clean and metal circuits to catch accidental regression toward identical
sounds. These checks cannot replace listening to a real guitar DI in a
DAW, and changing an amp simulation's tone is subjective.

## UI polish and right-click menu

The plugin intentionally suppresses the embedded browser's standard right-click
menu (including **Save As**, **Print**, and **More tools**). The restriction is
installed both in the HTML's capture-phase contextmenu handler and JUCE's
early WebView user script, so it applies to Windows WebView2 and macOS WKWebView.
Normal knob dragging, plugin presets, and file-import controls remain enabled.

All three amplifier views (Sakura Clean, Taki Metal, Take Thick) now use richer
HTML/CSS visual materials: mesh and woven speaker grilles, amp-casing textures,
glowing tubes, cast metal faceplates, screws, model plaques, chrome control caps,
and case hardware. These are **rendered materials**, not photographic captures,
and the original logo and interaction layout remain in place.

The Windows/macOS Actions workflow also runs `tools/validate_web_ui.py` to
check inline JavaScript syntax, contextmenu suppression, native bridge, model
markup and assets before compilation. A passing browser UI test does not
establish DAW runtime correctness; check the platform VST3 builds separately.

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
- The original browser Web MIDI permission popup is gone. Use the native
  **MIDI MAPPINGS** editor and right-click MIDI Learn; all incoming MIDI must
  be routed through the DAW.
- TUNER now uses native pitch detection on the raw guitar input; see above.

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
