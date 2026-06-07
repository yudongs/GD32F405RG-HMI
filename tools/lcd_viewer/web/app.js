// MKLink LCD Viewer — frontend logic.
// Connects to ws://<host>/ws, draws 4800-byte binary frames onto a 240x160
// canvas at native 1:1, and wires up the period / pause / save controls.

(() => {
  "use strict";

  // ---------- DOM ----------
  const canvas    = document.getElementById("lcd");
  const ctx       = canvas.getContext("2d");
  const statusDot = document.getElementById("status-dot");
  const statusTxt = document.getElementById("status-text");
  const fpsEl     = document.getElementById("fps");
  const framesEl  = document.getElementById("frames");
  const tsEl      = document.getElementById("last-ts");
  const btnPause  = document.getElementById("btn-pause");
  const inPeriod  = document.getElementById("period");
  const btnSave   = document.getElementById("btn-save");

  const FB_W = 240, FB_H = 160, FB_SIZE = FB_W * FB_H / 8;  // 4800

  // Pre-allocate the ImageData buffer (240*160*4 bytes).
  const img = ctx.createImageData(FB_W, FB_H);

  // ---------- State ----------
  let paused = false;
  let totalFrames = 0;
  let fpsFrames = 0;
  let lastFpsTs = performance.now();
  let lastFrameTs = 0;        // device ts_us of most recent rendered frame
  let helloPeriodMs = 50;
  let pendingSaveBlobUrl = null;

  // ---------- WebSocket ----------
  function setStatus(state, msg) {
    statusDot.className = "status-dot " + (state || "");
    statusTxt.textContent = msg || state || "unknown";
  }

  function connect() {
    setStatus("", "connecting…");
    const proto = location.protocol === "https:" ? "wss" : "ws";
    const url = `${proto}://${location.host}/ws`;
    const ws = new WebSocket(url);
    ws.binaryType = "arraybuffer";

    ws.onopen = () => {
      setStatus("starting", "等待首帧…");
    };

    // Single unified onmessage handler. Disambiguates "frame" vs "PNG response"
    // purely by payload length, so we never need a `waitingForPng` flag and
    // a framebuffer frame arriving in any race window is never misread as PNG.
    //
    //   Frame payload : exactly FB_SIZE (4800) bytes
    //   PNG response  : 4B LE uint32 length (N) + N bytes of PNG  →  total 4 + N
    ws.onmessage = (ev) => {
      if (typeof ev.data === "string") {
        handleText(JSON.parse(ev.data));
        return;
      }
      const bytes = new Uint8Array(ev.data);
      if (bytes.length === FB_SIZE) {
        handleBinary(bytes);
      } else if (bytes.length > 4) {
        const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        const pngLen = view.getUint32(0, true);
        if (bytes.length === 4 + pngLen) {
          handlePngResponse(bytes.subarray(4));
        } else {
          console.warn("unknown binary payload length:", bytes.length);
        }
      } else {
        console.warn("unexpected short binary message:", bytes.length);
      }
    };

    ws.onerror = () => {
      setStatus("error", "ws error");
    };

    ws.onclose = () => {
      setStatus("", "disconnected, retrying in 2s…");
      setTimeout(connect, 2000);
    };

    window._ws = ws;  // for debugging from devtools
  }

  function handleText(msg) {
    if (msg.type === "hello") {
      helloPeriodMs = msg.period_ms;
      inPeriod.value = helloPeriodMs;
    } else if (msg.type === "status") {
      setStatus(msg.state, msg.msg || msg.state);
    }
  }

  function handleBinary(bytes) {
    if (paused) return;
    if (bytes.length !== FB_SIZE) {
      console.warn("frame size mismatch:", bytes.length, "expected", FB_SIZE);
      return;
    }
    renderFrame(bytes);
    // Device ts is not currently pushed by the server (it only sends raw
    // bytes); we synthesize one from the local clock for display purposes.
    lastFrameTs = performance.now() * 1000;  // µs, monotonic
    totalFrames += 1;
    fpsFrames += 1;
    const now = performance.now();
    if (now - lastFpsTs >= 1000) {
      fpsEl.textContent = (fpsFrames * 1000 / (now - lastFpsTs)).toFixed(1);
      framesEl.textContent = totalFrames.toString();
      tsEl.textContent = Math.floor(lastFrameTs).toString();
      fpsFrames = 0;
      lastFpsTs = now;
    }
  }

  function handlePngResponse(pngBytes) {
    const blob = new Blob([pngBytes], { type: "image/png" });
    if (pendingSaveBlobUrl) URL.revokeObjectURL(pendingSaveBlobUrl);
    pendingSaveBlobUrl = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = pendingSaveBlobUrl;
    a.download = `frame_${Date.now()}.png`;
    document.body.appendChild(a);
    a.click();
    a.remove();
    btnSave.textContent = "💾 保存当前帧";
    btnSave.disabled = false;
  }

  // ---------- Canvas rendering ----------
  // 1bpp MSB-first → 32-bit RGBA. bit=1 → bright green, bit=0 → black.
  function renderFrame(buf) {
    const px = img.data;
    let p = 0;
    for (let y = 0; y < FB_H; y++) {
      for (let xByte = 0; xByte < FB_W / 8; xByte++) {
        const b = buf[y * (FB_W / 8) + xByte];
        for (let bit = 7; bit >= 0; bit--) {
          const on = (b >> bit) & 1;
          px[p]     = on ? 0x50 : 0x00;
          px[p + 1] = on ? 0xff : 0x00;
          px[p + 2] = on ? 0x78 : 0x00;
          px[p + 3] = 0xff;
          p += 4;
        }
      }
    }
    ctx.putImageData(img, 0, 0);
  }

  // ---------- Controls ----------
  btnPause.addEventListener("click", () => {
    paused = !paused;
    btnPause.textContent = paused ? "▶ 继续" : "⏸ 暂停";
    if (window._ws && window._ws.readyState === 1) {
      window._ws.send(JSON.stringify({ type: paused ? "pause" : "resume" }));
    }
  });

  inPeriod.addEventListener("change", () => {
    let v = parseInt(inPeriod.value, 10);
    if (isNaN(v)) v = 50;
    v = Math.max(10, Math.min(200, v));
    inPeriod.value = v;
    if (window._ws && window._ws.readyState === 1) {
      window._ws.send(JSON.stringify({ type: "set_period", ms: v }));
    }
  });

  btnSave.addEventListener("click", () => {
    if (window._ws && window._ws.readyState === 1) {
      window._ws.send(JSON.stringify({ type: "save_png" }));
      btnSave.textContent = "⏳ 生成中…";
      btnSave.disabled = true;
    }
  });

  // ---------- Go ----------
  connect();
})();
