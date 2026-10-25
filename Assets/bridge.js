/* SishhinDSP JUCE 8 bridge. The original HTML/CSS/artwork are deliberately retained. */
(() => {
  "use strict";
  const backend = window.__JUCE__ && window.__JUCE__.backend;
  if (!backend || typeof backend.emitEvent !== "function") {
    const s = document.querySelector(".cap");
    if (s) s.textContent = "WEB PREVIEW · Run inside SishhinDSP VST3 for processed audio";
    return;
  }
  let receiving = false;
  let lastProgram = -1;
  let activeDrag = null;
  const send = (name, payload) => {
    if (!receiving) backend.emitEvent(name, payload);
  };
  const toggleParameter = (name) => {
    if (name === "gate") return ["gateOn", !!S.on.gate];
    if (name === "dbl") return ["doubler", !!S.on.dbl];
    if (name === "mode") return ["stereo", S.mode === "stereo"];
    if (name === "eq") return ["eqOn", !!S.on.eq];
    if (name === "ir") return ["irOn", !!S.on.ir];
    if (name === "voice") return ["power", !!S.on.voice];
    if (name === "ra" || name === "rb") return [name + "On", !!S.on[name]];
    if (/^p[0-4]$/.test(name)) return [name + "On", !!S.on[name]];
    if (name === "ma" || name === "mb") return [name, !!S.on[name]];
    return null;
  };
  const values = () => {
    const v = { ...S.p, amp: S.amp, cab: S.cab,
                micaType: S.mica, micbType: S.micb };
    for (const name of ["gate","dbl","mode","eq","ir","voice","ma","mb",
                        "ra","rb","p0","p1","p2","p3","p4"]) {
      const pair = toggleParameter(name);
      if (pair) v[pair[0]] = Number(pair[1]);
    }
    return v;
  };
  const originalSet = set;
  set = function(key, value) {
    originalSet(key, value);
    send("sdspParameter", { id: key, value: S.p[key] });
  };
  const originalGo = go;
  go = function(index) {
    originalGo(index);
    if (index < FAC.length)
      send("sdspProgram", { index });
    else
      send("sdspBatch", { values: values() });
  };

  document.addEventListener("pointerdown", e => {
    const knob = e.target.closest(".knob");
    if (knob) activeDrag = knob.dataset.k;
  });
  document.addEventListener("pointerup", () => { activeDrag = null; });
  document.addEventListener("pointercancel", () => { activeDrag = null; });
  document.addEventListener("change", e => {
    const mic = e.target.dataset && e.target.dataset.ms;
    if (mic === "a" || mic === "b")
      send("sdspParameter", { id: "mic" + mic + "Type", value: S["mic" + mic] });
  });
  document.addEventListener("click", e => {
    const el = selector => e.target.closest(selector);
    let node;
    if ((node = el("[data-amp]"))) {
      send("sdspParameter", { id: "amp", value: S.amp });
    } else if ((node = el("[data-cab]"))) {
      send("sdspParameter", { id: "cab", value: S.cab });
    } else if ((node = el("[data-t]"))) {
      const pair = toggleParameter(node.dataset.t);
      if (pair) send("sdspParameter", { id: pair[0], value: Number(pair[1]) });
    } else if ((node = el("[data-mp]"))) {
      const mic = node.dataset.mp.split(",")[0];
      send("sdspParameter", { id: "mic" + mic + "Type", value: S["mic" + mic] });
    } else if (el("#irx")) {
      send("sdspClearIR", {});
    } else if (el("#eqr")) {
      for (let n=0;n<9;n++) send("sdspParameter", { id: "e" + n, value: 0 });
    }
  });

  // The browser's MIDI permission dialog does not apply to MIDI arriving in the DAW.
  const midiButton = document.querySelector("#midi");
  if (midiButton) {
    midiButton.addEventListener("click", e => {
      e.preventDefault(); e.stopImmediatePropagation();
      const msg = document.querySelector("#ms");
      if (msg) msg.textContent = "DAW MIDI PROGRAM CHANGE 0–71";
    }, true);
  }

  // Keep the waveform display, and also send the actual audio file to the native FIR loader.
  // This is a user-selected local file; no network upload occurs.
  const originalLoadIR = loadIR;
  loadIR = function(file) {
    if (file && file.size > 0 && file.size <= 8 * 1024 * 1024) {
      const reader = new FileReader();
      reader.onload = () => {
        const uri = String(reader.result || "");
        const split = uri.indexOf(",");
        if (split >= 0)
          send("sdspIR", { filename: file.name, base64: uri.slice(split+1) });
      };
      reader.onerror = () => {
        const el = document.querySelector(".irm");
        if (el) el.textContent = "Cannot read the selected IR file";
      };
      reader.readAsDataURL(file);
    } else if (file) {
      const el = document.querySelector(".irm");
      if (el) el.textContent = "Choose a WAV/AIFF IR up to 8 MB";
    }
    return originalLoadIR(file);
  };

  backend.addEventListener("sdspState", state => {
    if (!state || !state.values) return;
    receiving = true;
    try {
      if (typeof state.program === "number" && state.program !== lastProgram) {
        lastProgram = state.program;
        if (state.program >= 0 && state.program < FAC.length && cur !== state.program)
          originalGo(state.program);
      }
      const v = state.values;
      const oldAmp=S.amp, oldCab=S.cab;
      for (const [key, raw] of Object.entries(v)) {
        const number = Number(raw);
        if (key === "amp") S.amp = number|0;
        else if (key === "cab") S.cab = number|0;
        else if (key === "micaType") S.mica = number|0;
        else if (key === "micbType") S.micb = number|0;
        else if (key === "stereo") S.mode = number>=.5 ? "stereo" : "mono";
        else if (key === "gateOn") S.on.gate = number>=.5;
        else if (key === "doubler") S.on.dbl = number>=.5;
        else if (key === "eqOn") S.on.eq = number>=.5;
        else if (key === "irOn") S.on.ir = number>=.5;
        else if (key === "power") S.on.voice = number>=.5;
        else if (key === "raOn") S.on.ra = number>=.5;
        else if (key === "rbOn") S.on.rb = number>=.5;
        else if (key === "ma" || key === "mb") S.on[key] = number>=.5;
        else if (/^p[0-4]On$/.test(key)) S.on[key.slice(0,2)] = number>=.5;
        else if (Object.prototype.hasOwnProperty.call(R, key) && key !== activeDrag)
          S.p[key] = number;
      }
      const irName=String(state.irName || "");
      if (irName && (!IR || IR.name !== irName)) {
        IR = { name:irName, dur:0, sr:48000, ch:1, peaks:[] };
      } else if (!irName) {
        IR = null;
      }
      if (S.amp!==oldAmp || S.cab!==oldCab) view();
      else {
        sync();
        if (T === "cab") {
          const a=document.querySelector("[data-ms=a]"),b=document.querySelector("[data-ms=b]");
          if (a) a.value=String(S.mica);
          if (b) b.value=String(S.micb);
          const info=document.querySelector(".irm");
          if (info && irName) info.textContent=irName+" · NATIVE CAB IR";
        }
      }
      const footer=document.querySelector(".cap");
      if (footer) {
        const inDb = Number(state.inputPeak || 0);
        const outDb = Number(state.outputPeak || 0);
        footer.textContent = "NATIVE ENGINE · INPUT " +
          (inDb > 0.00001 ? Math.round(20*Math.log10(inDb))+" dBFS" : "—") +
          " · OUTPUT " +
          (outDb > 0.00001 ? Math.round(20*Math.log10(outDb))+" dBFS" : "—");
      }
    } finally { receiving = false; }
  });
  backend.addEventListener("sdspError", result => {
    const text=String(result && result.message || "Native engine error");
    const irInfo=document.querySelector(".irm");
    if (irInfo) irInfo.textContent=text;
  });
  // The backend is authoritative on session load: request its saved DAW state.
  S.on.voice = true;
  sync();
  // Wait until the whole page is loaded before asking JUCE to emit frontend events.
  const notifyReady = () => setTimeout(() => send("sdspReady", {}), 0);
  if (document.readyState === "complete") notifyReady();
  else window.addEventListener("load", notifyReady, { once: true });
})();