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

  // A user preset is saved in both WebView localStorage (for the HTML list)
  // and native Documents/SishhinDSP/Presets/*.sdsp (for durable plugin presets).
  document.addEventListener("keydown", e => {
    if (e.target && e.target.id === "nm" && e.key === "Enter" && US.length)
      send("sdspSavePreset", { name: US[US.length - 1].name });
  });
  document.addEventListener("click", e => {
    if (e.target.closest("#dl") && cur >= 72 && US[cur-72])
      send("sdspDeletePreset", { name: US[cur-72].name });
  }, true);

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
    if (e.target.id === "irbanklist") {
      const index = Number(e.target.value);
      if(Number.isInteger(index) && index>=0 && index<BANK_NAMES.length) {
        S.bankIndex=index;
        S.on.ir=true;
        IR={name:BANK_NAMES[index],dur:5,sr:96000,ch:1,peaks:[]};
        refreshIR();
        send("sdspFactoryIR",{index});
      }
    }
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
    } else if (el("#sv")) {
      if (cur >= 72 && US[cur-72])
        send("sdspSavePreset", { name: US[cur-72].name });
    } else if (el("#irx")) {
      S.bankIndex = -1;
      send("sdspClearIR", {});
    } else if (el("#eqr")) {
      for (let n=0;n<9;n++) send("sdspParameter", { id: "e" + n, value: 0 });
    }
  });

  // Native tuner + DAW MIDI mapping UI. Neither feature requests browser MIDI
  // permissions; audio/MIDI are read by the JUCE processor in the host.
  const dialogStyle = document.createElement("style");
  dialogStyle.textContent = String.raw`
    .sdsp-modal[hidden],.sdsp-context[hidden]{display:none!important}
    .sdsp-modal{position:fixed;inset:0;display:flex;align-items:center;justify-content:center;
      background:#05060bc9;backdrop-filter:blur(6px);z-index:200000;padding:24px;color:#f5f3ee;
      font:500 12px Inter,system-ui,sans-serif}
    .sdsp-dialog{width:min(780px,96vw);max-height:calc(100vh - 48px);overflow:auto;background:#12141b;
      border:1px solid #454653;border-radius:11px;box-shadow:0 25px 80px #000b}
    .sdsp-dialog header{display:flex;align-items:center;justify-content:center;padding:19px 24px;
      border-bottom:1px solid #333744;position:relative}
    .sdsp-dialog header h3{margin:0;font-size:15px;letter-spacing:.04em}
    .sdsp-modal button{color:#eee;font:inherit;cursor:pointer}
    .sdsp-modal .sdsp-close{position:absolute;right:16px;top:9px;background:transparent;border:0;
      padding:8px;font-size:25px;line-height:1}
    .sdsp-context{position:fixed;z-index:300000;width:204px;background:#14161c;color:#f1f0ef;
      padding:5px;border-radius:7px;border:1px solid #383941;box-shadow:0 10px 30px #0009}
    .sdsp-context button{display:block;width:100%;border:0;background:transparent;color:inherit;
      padding:9px 10px;text-align:left;font:500 12px Inter,system-ui,sans-serif;cursor:pointer}
    .sdsp-context button:hover{background:#8b4642}
    .sdsp-midi-content{padding:19px 18px 24px}
    .sdsp-midi-grid{border-collapse:collapse;width:100%;font-size:11px}
    .sdsp-midi-grid th{text-align:left;color:#d1d0d1;font-size:10px;white-space:nowrap;font-weight:600;
      padding:0 4px 10px}
    .sdsp-midi-grid td{background:#1d2028;border-bottom:5px solid #12141b;padding:3px 4px}
    .sdsp-midi-grid select,.sdsp-midi-grid input{width:100%;min-width:0;border:0;color:#e5e5e9;
      border-radius:2px;background:#292c34;padding:7px 3px;font:500 11px Inter,system-ui,sans-serif}
    .sdsp-midi-grid select option{background:#222630;color:#fff}
    .sdsp-midi-grid td:nth-child(1){width:18%}
    .sdsp-midi-grid td:nth-child(2){width:26%}
    .sdsp-midi-grid td:nth-child(3){width:14%}
    .sdsp-midi-grid td:nth-child(4){width:17%}
    .sdsp-midi-grid td:nth-child(5){width:16%}
    .sdsp-midi-grid .remove{background:transparent;color:#fff;border:0;font-size:17px;padding:5px}
    .sdsp-midi-buttons{display:flex;align-items:center;gap:12px;margin-top:12px}
    .sdsp-midi-buttons button{border:1px solid #676a76;background:#222630;border-radius:5px;padding:8px 13px}
    .sdsp-midi-learn{margin-left:auto;color:#9fa7b8;font-size:10px;min-height:20px}
    .sdsp-tuner-dialog{width:min(510px,96vw)}
    .sdsp-tuner-body{text-align:center;padding:36px 27px 40px}
    .sdsp-tuner-pitch{font-size:76px;font-weight:700;line-height:1.16;letter-spacing:-.04em}
    .sdsp-tuner-frequency{font-size:15px;color:#aeb3c2;margin:5px 0 21px}
    .sdsp-tuner-track{height:45px;margin:0 auto;max-width:410px;position:relative;
      background:linear-gradient(90deg,#7c252d12,#405a4a44 47%,#405a4a44 53%,#7c252d12);
      border-bottom:2px solid #444955}
    .sdsp-tuner-track::before{content:"";height:36px;position:absolute;left:50%;
      width:2px;top:5px;background:#62c99c80}
    .sdsp-tuner-needle{position:absolute;top:4px;left:50%;width:3px;height:38px;
      transform:translateX(-50%);background:#e2a85a;border-radius:2px;box-shadow:0 0 8px #e2a85a}
    .sdsp-tuner-needle.intune{background:#77e8b8;box-shadow:0 0 14px #59e2aa}
    .sdsp-tuner-cents{color:#bfc4d0;margin-top:17px}
    .sdsp-tuner-hint{margin-top:19px;font-size:11px;color:#878fa1}
    .sdsp-tuner-labels{display:flex;justify-content:space-between;color:#9a9fae;margin-top:6px}
    .sdsp-foot-button{padding:7px 11px!important;border-radius:6px!important;background:#e6e9f2!important;
      color:#232b3c!important}
    @media(max-width:650px){
      .sdsp-midi-grid{font-size:9px}.sdsp-midi-grid select,.sdsp-midi-grid input{font-size:9px;padding:6px 2px}
      .sdsp-midi-content{padding:12px 6px}.sdsp-dialog header{padding:13px}
      .sdsp-midi-grid th{font-size:9px}
    }
  `;
  document.head.appendChild(dialogStyle);

  const tunerDialog = document.createElement("div");
  tunerDialog.id = "sdspTunerDialog";
  tunerDialog.className = "sdsp-modal";
  tunerDialog.hidden = true;
  tunerDialog.innerHTML = '<section class="sdsp-dialog sdsp-tuner-dialog" role="dialog" aria-modal="true" aria-label="Guitar tuner">' +
    '<header><h3>GUITAR TUNER · A440</h3><button class="sdsp-close" id="sdspTunerClose" aria-label="Close tuner">×</button></header>' +
    '<div class="sdsp-tuner-body"><div class="sdsp-tuner-pitch" id="sdspTunerNote">—</div>' +
    '<div class="sdsp-tuner-frequency" id="sdspTunerFrequency">PLAY A SINGLE STRING</div>' +
    '<div class="sdsp-tuner-track"><i class="sdsp-tuner-needle" id="sdspTunerNeedle"></i></div>' +
    '<div class="sdsp-tuner-labels"><span>−50 cents</span><span>IN TUNE</span><span>+50 cents</span></div>' +
    '<div class="sdsp-tuner-cents" id="sdspTunerCents">WAITING FOR GUITAR INPUT</div>' +
    '<div class="sdsp-tuner-hint">Reads clean DI before the amplifier or pedals · A = 440 Hz</div></div></section>';
  document.body.appendChild(tunerDialog);
  // If the DAW has no guitar audio routed, the tuner explicitly shows NO INPUT.
  let smoothTunerHz = 0;
  const updateTunerUI = raw => {
    if (tunerDialog.hidden) return;
    const noteNode = document.querySelector("#sdspTunerNote");
    const hzNode = document.querySelector("#sdspTunerFrequency");
    const centsNode = document.querySelector("#sdspTunerCents");
    const needle = document.querySelector("#sdspTunerNeedle");
    if (!(raw >= 35 && raw <= 1100)) {
      smoothTunerHz = 0;
      noteNode.textContent = "—";
      hzNode.textContent = "PLAY A SINGLE STRING";
      centsNode.textContent = "WAITING FOR GUITAR INPUT";
      needle.style.left = "50%";
      needle.classList.remove("intune");
      return;
    }
    smoothTunerHz = smoothTunerHz > 0 && Math.abs(12*Math.log2(raw/smoothTunerHz)) < 1.6
      ? smoothTunerHz*.70 + raw*.30 : raw;
    const pitch=69+12*Math.log2(smoothTunerHz/440);
    const nearest=Math.round(pitch);
    const names=["C","C♯","D","D♯","E","F","F♯","G","G♯","A","A♯","B"];
    const note=names[(nearest%12+12)%12]+String(Math.floor(nearest/12)-1);
    const cents=Math.round((pitch-nearest)*100);
    noteNode.textContent=note;
    hzNode.textContent=smoothTunerHz.toFixed(1)+" Hz";
    centsNode.textContent=(cents>0?"+":"")+cents+" cents"+(Math.abs(cents)<=4?" · IN TUNE":"");
    needle.style.left=String(Math.max(5,Math.min(95,50+cents*.9)))+"%";
    needle.classList.toggle("intune",Math.abs(cents)<=4);
  };
  const toggleTuner = open => {
    tunerDialog.hidden = !open;
    if(open) midiDialog.hidden=true;
    smoothTunerHz=0;
    send("sdspTunerToggle",{enabled:open});
  };
  const tunerButton=document.querySelector("#tuner");
  if(tunerButton) {
    tunerButton.classList.add("sdsp-foot-button");
    tunerButton.addEventListener("click",()=>toggleTuner(tunerDialog.hidden));
  }
  tunerDialog.querySelector("#sdspTunerClose").addEventListener("click",()=>toggleTuner(false));
  tunerDialog.addEventListener("click",e=>{if(e.target===tunerDialog)toggleTuner(false)});

  const midiDialog=document.createElement("div");
  midiDialog.id="sdspMidiDialog";
  midiDialog.className="sdsp-modal";
  midiDialog.hidden=true;
  midiDialog.innerHTML='<section class="sdsp-dialog" role="dialog" aria-modal="true" aria-label="MIDI mappings">' +
    '<header><h3>MIDI Mappings</h3><button id="sdspMidiClose" class="sdsp-close" aria-label="Close MIDI mappings">×</button></header>' +
    '<div class="sdsp-midi-content"><table class="sdsp-midi-grid"><thead><tr>' +
    '<th>Type</th><th>Parameter / Preset</th><th>Channel</th><th>Note / CC / PC</th><th>Preset #</th><th></th>' +
    '</tr></thead><tbody id="sdspMidiRows"></tbody></table>' +
    '<div class="sdsp-midi-buttons"><button id="sdspMidiAdd">＋ Add mapping</button>' +
    '<button id="sdspMidiCancel">Cancel MIDI Learn</button>' +
    '<span class="sdsp-midi-learn" id="sdspMidiLearnStatus"></span></div></div></section>';
  document.body.appendChild(midiDialog);

  const context=document.createElement("div");
  context.className="sdsp-context";
  context.id="sdspMidiContext";
  context.hidden=true;
  context.innerHTML='<button id="sdspContextLearn">Enable MIDI Learn</button>' +
                    '<button id="sdspContextOpen">Open MIDI Mappings…</button>';
  document.body.appendChild(context);
  const nameFor = id => id==="preset" ? "Factory Preset" :
    id==="amp" ? "Amplifier Model" : id==="cab" ? "Cabinet Model" :
    id==="ma" ? "Mic A Power" : id==="mb" ? "Mic B Power" :
    id==="gateOn" ? "Noise Gate" : id==="eqOn" ? "Graphic EQ Power" :
    id==="power" ? "Amplifier Power" : id==="doubler" ? "Doubler" :
    id==="stereo" ? "Stereo Input" : id==="irOn" ? "IR Power" :
    id==="raOn" ? "Mic A Room Send" : id==="rbOn" ? "Mic B Room Send" :
    /^p[0-4]On$/.test(id) ? ["Bloom", "Edge", "Weight", "Echo", "Halo"][Number(id[1])]+" On/Off" :
    id==="micaType" ? "Mic A Model" : id==="micbType" ? "Mic B Model" :
    R[id] ? R[id][4]+" · "+id.toUpperCase() : id;
  const validParameters=["preset","amp","cab","input","gate","trans","dbl","out",
      "power","gateOn","eqOn","irOn","ma","mb","doubler","stereo",
      "micaType","micbType","raOn","rbOn"]
      .concat(["p0On","p1On","p2On","p3On","p4On"])
      .concat(Object.keys(R));
  const midiTargets=[...new Set(validParameters)];
  const midiTypes=["CC Absolute","Program Change","Note On/Off"];
  let midiRows=[];
  let midiLearnTarget="";
  let midiMappingHash="";
  let menuTarget="preset";
  const fieldOptions=(values,chosen)=>values.map(pair=>'<option value="'+esc(pair[0])+'"'+
    (String(pair[0])===String(chosen)?" selected":"")+'>'+esc(pair[1])+'</option>').join("");
  const targetOptions=midiTargets.map(id=>[id,nameFor(id)]);
  const typeOptions=midiTypes.map((name,i)=>[i,name]);
  const channelOptions=[["0","Omni"]].concat(Array.from({length:16},(_,i)=>[i+1,"Ch "+(i+1)]));
  const normalizeMap = row => ({
    target:midiTargets.includes(String(row.target))?String(row.target):"preset",
    type:Math.max(0,Math.min(2,Number(row.type)||0)),
    channel:Math.max(0,Math.min(16,Number(row.channel)||0)),
    number:Math.max(0,Math.min(127,Number(row.number)||0)),
    preset:Math.max(0,Math.min(71,Number(row.preset)||0))
  });
  const renderMidiRows=()=>{
    const body=midiDialog.querySelector("#sdspMidiRows");
    body.innerHTML=midiRows.map((row,index)=>{
      const id=' data-map-index="'+index+'"';
      return '<tr'+id+'><td><select data-field="type">'+fieldOptions(typeOptions,row.type)+'</select></td>'+
        '<td><select data-field="target">'+fieldOptions(targetOptions,row.target)+'</select></td>'+
        '<td><select data-field="channel">'+fieldOptions(channelOptions,row.channel)+'</select></td>'+
        '<td><input type="number" data-field="number" min="0" max="127" value="'+row.number+'"></td>'+
        '<td><input type="number" data-field="preset" min="0" max="71" value="'+row.preset+'"'+
          (row.target!=="preset"?" disabled":"")+'></td>'+
        '<td><button class="remove" data-remove="'+index+'" aria-label="Remove mapping">×</button></td></tr>';
    }).join("");
    midiDialog.querySelector("#sdspMidiCancel").disabled = !midiLearnTarget;
    midiDialog.querySelector("#sdspMidiLearnStatus").textContent =
      midiLearnTarget ? "LEARNING: "+nameFor(midiLearnTarget)+" · Send a MIDI message…" :
                        midiRows.length+" mapping"+(midiRows.length===1?"":"s")+" · MIDI from DAW";
  };
  const saveMidiRows=()=>{midiRows=midiRows.map(normalizeMap);send("sdspMidiMappingsSet",midiRows);};
  const openMidiPanel=()=>{
    context.hidden=true;
    midiDialog.hidden=false;
    if(!tunerDialog.hidden)toggleTuner(false);
    renderMidiRows();
  };
  const midiButton=document.querySelector("#midi");
  if(midiButton){
    midiButton.classList.add("sdsp-foot-button");
    midiButton.addEventListener("click",e=>{
      e.preventDefault();
      e.stopImmediatePropagation();
      openMidiPanel();
    },true);
  }
  midiDialog.querySelector("#sdspMidiClose").addEventListener("click",()=>midiDialog.hidden=true);
  midiDialog.addEventListener("click",e=>{
    if(e.target===midiDialog)midiDialog.hidden=true;
    const rem=e.target.closest("[data-remove]");
    if(rem) {
      midiRows.splice(Number(rem.dataset.remove),1);
      saveMidiRows();
      renderMidiRows();
    }
  });
  midiDialog.querySelector("#sdspMidiAdd").addEventListener("click",()=>{
    midiRows.push({type:0,target:"p0On",channel:0,number:10,preset:0});
    saveMidiRows();renderMidiRows();
  });
  midiDialog.querySelector("#sdspMidiCancel").addEventListener("click",()=>{
    send("sdspMidiCancelLearn",{});
    midiLearnTarget="";renderMidiRows();
  });
  midiDialog.querySelector("#sdspMidiRows").addEventListener("change",e=>{
    const row=e.target.closest("[data-map-index]");
    const field=e.target.dataset.field;
    if(!row || !field)return;
    const index=Number(row.dataset.mapIndex);
    const value=field==="target" ? e.target.value : Number(e.target.value);
    if(!midiRows[index])return;
    midiRows[index][field]=value;
    saveMidiRows();renderMidiRows();
  });
  const updateMidiUI=(rows,learn)=>{
    const next=Array.isArray(rows)?rows.map(normalizeMap):[];
    const serialized=JSON.stringify(next);
    const incomingLearn=String(learn||"");
    // Host state remains authoritative, except while the user edits a field.
    if(serialized!==midiMappingHash || incomingLearn!==midiLearnTarget) {
      const editing=midiDialog.contains(document.activeElement) &&
        /^(SELECT|INPUT)$/.test(document.activeElement.tagName);
      if(!editing) {
        midiRows=next;
        midiMappingHash=serialized;
        midiLearnTarget=incomingLearn;
        if(!midiDialog.hidden)renderMidiRows();
      }
    }
  };
  const closeContext=()=>{context.hidden=true;};
  document.addEventListener("click",e=>{
    if(!e.target.closest("#sdspMidiContext"))closeContext();
  },true);
  const resolveTarget=element=>{
    if(!element)return null;
    if(element.hasAttribute("data-k"))return element.dataset.k;
    if(element.hasAttribute("data-e"))return "e"+element.dataset.e;
    if(element.hasAttribute("data-t")){
      const key=element.dataset.t;
      if(key==="gate")return "gateOn";
      if(key==="dbl")return "doubler";
      if(key==="mode")return "stereo";
      if(key==="eq")return "eqOn";
      if(key==="voice")return "power";
      if(key==="ir")return "irOn";
      if(key==="ra"||key==="rb")return key+"On";
      if(/^p[0-4]$/.test(key))return key+"On";
      if(key==="ma"||key==="mb")return key;
    }
    if(element.hasAttribute("data-amp"))return "amp";
    if(element.hasAttribute("data-cab"))return "cab";
    if(element.hasAttribute("data-ms"))return "mic"+element.dataset.ms+"Type";
    if(element.id==="ps")return "preset";
    return null;
  };
  document.addEventListener("contextmenu",e=>{
    e.preventDefault();
    if(!midiDialog.hidden || !tunerDialog.hidden)return;
    const control=e.target.closest(
      "[data-k],[data-e],[data-t],[data-amp],[data-cab],[data-ms],#ps");
    const target=resolveTarget(control);
    if(!target || !midiTargets.includes(target)){closeContext();return;}
    menuTarget=target;
    const button=context.querySelector("#sdspContextLearn");
    button.textContent="Enable MIDI Learn · "+nameFor(target);
    context.hidden=false;
    context.style.left=Math.max(8,Math.min(window.innerWidth-215,e.clientX))+"px";
    context.style.top=Math.max(8,Math.min(window.innerHeight-89,e.clientY))+"px";
  },true);
  context.querySelector("#sdspContextLearn").addEventListener("click",e=>{
    e.stopPropagation();closeContext();
    midiLearnTarget=menuTarget;
    send("sdspMidiLearn",{target:menuTarget});
    openMidiPanel();
  });
  context.querySelector("#sdspContextOpen").addEventListener("click",e=>{
    e.stopPropagation();openMidiPanel();
  });
  document.addEventListener("keydown",e=>{
    if(e.key!=="Escape")return;
    if(!tunerDialog.hidden)toggleTuner(false);
    if(!midiDialog.hidden)midiDialog.hidden=true;
    closeContext();
  });

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
      S.bankIndex=typeof state.irBankIndex === "number" ? state.irBankIndex : -1;
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
          const bankList=document.querySelector("#irbanklist");
          if (bankList) bankList.value=String(S.bankIndex);
          const info=document.querySelector(".irm");
          if (info && irName) info.textContent=irName+" · NATIVE CAB IR";
        }
      }
      updateTunerUI(Number(state.tunerHz||0));
      updateMidiUI(state.midiMappings,state.midiLearnTarget);
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