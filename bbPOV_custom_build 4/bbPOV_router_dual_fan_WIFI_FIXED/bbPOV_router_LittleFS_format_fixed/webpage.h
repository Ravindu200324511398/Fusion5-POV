#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <pgmspace.h>

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Fusion 5 · Hologram Controller</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@500;600;700&family=Inter:wght@400;500;600&family=JetBrains+Mono:wght@500;600&display=swap" rel="stylesheet">
  <style>
    :root {
      --void: #05060c;
      --void-2: #0a0d18;
      --surface: rgba(16, 19, 32, 0.66);
      --surface-soft: rgba(255, 255, 255, 0.03);
      --line: rgba(255, 255, 255, 0.08);
      --line-soft: rgba(255, 255, 255, 0.05);
      --ink: #eef0fb;
      --ink-dim: #8a8fac;
      --ink-faint: #5b5f78;
      --cyan: #2fe6d8;
      --magenta: #ff4fb4;
      --amber: #ffb648;
      --trail: linear-gradient(115deg, var(--cyan) 0%, var(--magenta) 55%, var(--amber) 100%);
      --danger: #ff5c72;
      --success: #34dd92;
      --radius-lg: 22px;
      --radius-md: 14px;
      --radius-sm: 9px;
      --shadow-deep: 0 20px 60px -20px rgba(0, 0, 0, 0.6);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    html, body { background: var(--void); }
    body {
      font-family: 'Inter', sans-serif;
      color: var(--ink);
      min-height: 100vh;
      background:
        radial-gradient(900px 500px at 12% -10%, rgba(47, 230, 216, 0.10), transparent 60%),
        radial-gradient(900px 600px at 100% 10%, rgba(255, 79, 180, 0.08), transparent 55%),
        var(--void);
      padding: 0 0 60px;
      position: relative;
    }
    .scan-rail {
      position: fixed;
      top: 0; left: 0; right: 0;
      height: 3px;
      background: rgba(255, 255, 255, 0.04);
      overflow: hidden;
      z-index: 50;
    }
    .scan-rail::after {
      content: "";
      position: absolute;
      top: 0; bottom: 0;
      width: 30%;
      background: var(--trail);
      filter: blur(0.5px);
      animation: sweep 5.5s linear infinite;
    }
    @keyframes sweep {
      0% { left: -30%; }
      100% { left: 100%; }
    }
    .shell {
      max-width: 1180px;
      margin: 0 auto;
      padding: 48px 24px 0;
    }
    .hero {
      display: flex;
      align-items: center;
      gap: 20px;
      margin-bottom: 36px;
      flex-wrap: wrap;
    }
    .hero-mark {
      width: 56px;
      height: 56px;
      border-radius: 50%;
      position: relative;
      flex-shrink: 0;
      background: conic-gradient(from 0deg, var(--cyan), var(--magenta), var(--amber), var(--cyan));
      animation: spin 6s linear infinite;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .hero-mark::before {
      content: "";
      position: absolute;
      inset: 6px;
      border-radius: 50%;
      background: var(--void);
    }
    .hero-mark::after {
      content: "";
      position: absolute;
      width: 7px;
      height: 7px;
      border-radius: 50%;
      background: var(--ink);
      box-shadow: 0 0 10px rgba(255, 255, 255, 0.8);
    }
    @keyframes spin { to { transform: rotate(360deg); } }
    .hero-text { flex: 1; min-width: 200px; }
    .hero-text h1 {
      font-family: 'Space Grotesk', sans-serif;
      font-size: 28px;
      font-weight: 700;
      letter-spacing: -0.01em;
    }
    .hero-text p {
      color: var(--ink-dim);
      font-size: 13.5px;
      margin-top: 2px;
    }
    .status-pill {
      display: inline-flex;
      align-items: center;
      gap: 9px;
      background: var(--surface);
      border: 1px solid var(--line);
      padding: 9px 16px 9px 12px;
      border-radius: 999px;
      font-size: 13px;
      font-weight: 500;
      backdrop-filter: blur(10px);
    }
    .status-dot {
      width: 9px; height: 9px;
      border-radius: 50%;
      background: var(--success);
      box-shadow: 0 0 9px var(--success);
      animation: pulse 1.6s ease-in-out infinite;
      flex-shrink: 0;
    }
    .status-pill.offline .status-dot { background: var(--danger); box-shadow: 0 0 9px var(--danger); animation: none; }
    @keyframes pulse {
      0%, 100% { opacity: 0.55; transform: scale(0.85); }
      50% { opacity: 1; transform: scale(1.05); }
    }

    .grid {
      display: grid;
      grid-template-columns: 1.05fr 1fr 0.85fr;
      gap: 20px;
      align-items: start;
    }
    @media (max-width: 980px) { .grid { grid-template-columns: 1fr 1fr; } .panel-preview { grid-column: span 2; } }
    @media (max-width: 660px) { .grid { grid-template-columns: 1fr; } .panel-preview { grid-column: auto; } }

    .panel {
      background: var(--surface);
      border: 1px solid var(--line);
      border-radius: var(--radius-lg);
      padding: 24px;
      backdrop-filter: blur(18px);
      box-shadow: var(--shadow-deep);
      display: flex;
      flex-direction: column;
    }
    .panel-title {
      display: flex;
      align-items: center;
      gap: 10px;
      font-family: 'Space Grotesk', sans-serif;
      font-size: 14.5px;
      font-weight: 600;
      letter-spacing: 0.01em;
      margin-bottom: 18px;
      color: var(--ink);
    }
    .panel-title svg { width: 17px; height: 17px; stroke: var(--cyan); flex-shrink: 0; }

    .media-list {
      list-style: none;
      max-height: 230px;
      overflow-y: auto;
      border: 1px solid var(--line-soft);
      border-radius: var(--radius-md);
      background: rgba(0, 0, 0, 0.22);
      margin-bottom: 16px;
    }
    .media-list::-webkit-scrollbar { width: 6px; }
    .media-list::-webkit-scrollbar-thumb { background: var(--line); border-radius: 6px; }
    .media-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 11px 14px;
      border-bottom: 1px solid var(--line-soft);
      cursor: pointer;
      transition: background 0.15s ease;
    }
    .media-item:last-child { border-bottom: none; }
    .media-item:hover { background: var(--surface-soft); }
    .media-item.active {
      background: rgba(47, 230, 216, 0.08);
      box-shadow: inset 3px 0 0 var(--cyan);
    }
    .media-item-name {
      font-size: 13.5px;
      font-weight: 500;
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .media-item-name .dot {
      width: 5px; height: 5px; border-radius: 50%;
      background: var(--cyan);
      opacity: 0;
      flex-shrink: 0;
    }
    .media-item.active .media-item-name .dot { opacity: 1; box-shadow: 0 0 6px var(--cyan); }
    .media-empty { padding: 16px; font-size: 13px; color: var(--ink-faint); }
    .btn-icon {
      background: none; border: none; cursor: pointer;
      width: 26px; height: 26px;
      display: flex; align-items: center; justify-content: center;
      border-radius: 7px;
      color: var(--ink-faint);
      transition: all 0.15s ease;
    }
    .btn-icon:hover { background: rgba(255, 92, 114, 0.12); color: var(--danger); }
    .btn-icon svg { width: 15px; height: 15px; }

    .playback-row {
      display: flex;
      justify-content: center;
      align-items: center;
      gap: 22px;
      margin: 4px 0 20px;
    }
    .ctl {
      background: var(--surface-soft);
      border: 1px solid var(--line);
      width: 42px; height: 42px;
      border-radius: 50%;
      display: flex; align-items: center; justify-content: center;
      cursor: pointer;
      color: var(--ink);
      transition: all 0.18s ease;
    }
    .ctl:hover { border-color: var(--cyan); color: var(--cyan); transform: translateY(-1px); }
    .ctl svg { width: 16px; height: 16px; }
    .ctl.main { width: 50px; height: 50px; background: var(--trail); color: var(--void); border: none; }
    .ctl.main svg { width: 18px; height: 18px; }

    .stat-grid {
      display: grid;
      grid-template-columns: repeat(2, 1fr);
      gap: 10px;
      margin-bottom: 18px;
    }
    .stat {
      background: rgba(255, 255, 255, 0.025);
      border: 1px solid var(--line-soft);
      border-radius: var(--radius-sm);
      padding: 11px 12px;
    }
    .stat-label {
      font-size: 10.5px;
      color: var(--ink-faint);
      text-transform: uppercase;
      letter-spacing: 0.06em;
      font-weight: 600;
    }
    .stat-value {
      font-family: 'JetBrains Mono', monospace;
      font-size: 14.5px;
      font-weight: 600;
      margin-top: 3px;
      color: var(--ink);
    }
    .stat.clickable { cursor: pointer; }
    .stat.clickable:hover { border-color: var(--cyan); }

    .btn-row { display: flex; gap: 10px; margin-top: auto; }
    .btn {
      flex: 1;
      padding: 11px;
      border: none;
      border-radius: var(--radius-sm);
      font-family: inherit;
      font-size: 13px;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.18s ease;
      display: flex; align-items: center; justify-content: center; gap: 6px;
    }
    .btn-ghost {
      background: var(--surface-soft);
      border: 1px solid var(--line);
      color: var(--ink);
    }
    .btn-ghost:hover { border-color: var(--cyan); color: var(--cyan); }
    .btn-solid {
      background: var(--trail);
      color: #0a0a0f;
    }
    .btn-solid:hover { filter: brightness(1.08); }

    .dropzone {
      border: 1.5px dashed var(--line);
      border-radius: var(--radius-md);
      padding: 26px 18px;
      cursor: pointer;
      transition: all 0.2s ease;
      background: rgba(255, 255, 255, 0.015);
      margin-bottom: 18px;
      text-align: center;
    }
    .dropzone:hover, .dropzone.dragover {
      border-color: var(--cyan);
      background: rgba(47, 230, 216, 0.05);
    }
    .dropzone svg { width: 30px; height: 30px; stroke: var(--ink-faint); margin-bottom: 10px; transition: stroke 0.2s ease; }
    .dropzone:hover svg { stroke: var(--cyan); }
    .dropzone p { font-size: 13.5px; font-weight: 600; }
    .dropzone span { font-size: 11.5px; color: var(--ink-faint); display: block; margin-top: 4px; }
    input[type="file"] { display: none; }

    .field { margin-bottom: 15px; }
    .field label {
      display: flex; justify-content: space-between;
      font-size: 11px; color: var(--ink-faint);
      font-weight: 600; margin-bottom: 7px;
      text-transform: uppercase; letter-spacing: 0.05em;
    }
    .field label span.val { color: var(--cyan); font-family: 'JetBrains Mono', monospace; }
    .field input[type="text"] {
      width: 100%;
      background: rgba(0, 0, 0, 0.25);
      border: 1px solid var(--line);
      border-radius: 9px;
      padding: 10px 12px;
      color: var(--ink);
      font-family: inherit;
      font-size: 13.5px;
    }
    .field input[type="text"]:focus { outline: none; border-color: var(--cyan); }
    .field input[type="range"] {
      width: 100%;
      -webkit-appearance: none;
      height: 5px;
      background: var(--line);
      border-radius: 3px;
      outline: none;
    }
    .field input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 15px; height: 15px;
      border-radius: 50%;
      background: var(--cyan);
      box-shadow: 0 0 8px rgba(47, 230, 216, 0.7);
      cursor: pointer;
    }

    .progress-wrap { margin-top: 6px; display: none; }
    .progress-text {
      font-size: 12px; font-weight: 600;
      margin-bottom: 6px; color: var(--ink-dim);
      display: flex; justify-content: space-between;
    }
    .progress-bg { width: 100%; height: 7px; background: var(--line-soft); border-radius: 4px; overflow: hidden; }
    .progress-fill { width: 0%; height: 100%; background: var(--trail); border-radius: 4px; transition: width 0.15s ease; }

    .panel-preview { align-items: center; }
    .preview-ring {
      position: relative;
      width: 230px; height: 230px;
      border-radius: 50%;
      display: flex; align-items: center; justify-content: center;
      margin: 6px 0 18px;
    }
    .preview-ring::before {
      content: "";
      position: absolute;
      inset: -8px;
      border-radius: 50%;
      background: conic-gradient(from 90deg, transparent 0deg, var(--cyan) 40deg, transparent 90deg, var(--magenta) 200deg, transparent 250deg, var(--amber) 320deg, transparent 360deg);
      animation: spin 4s linear infinite;
      opacity: 0.85;
    }
    .preview-disc {
      position: relative;
      width: 210px; height: 210px;
      border-radius: 50%;
      background: rgba(0, 0, 0, 0.5);
      overflow: hidden;
      display: flex; align-items: center; justify-content: center;
      box-shadow: inset 0 0 26px rgba(0, 0, 0, 0.85);
    }
    .preview-disc canvas { width: 100%; height: 100%; border-radius: 50%; }
    .preview-caption {
      font-size: 11.5px;
      color: var(--ink-faint);
      text-align: center;
      letter-spacing: 0.03em;
    }
    .preview-caption b { color: var(--ink-dim); }

    .toast-stack {
      position: fixed;
      bottom: 22px; right: 22px;
      display: flex; flex-direction: column; gap: 10px;
      z-index: 100;
    }
    .toast {
      background: var(--void-2);
      border: 1px solid var(--line);
      border-radius: var(--radius-sm);
      padding: 12px 16px;
      font-size: 13px;
      font-weight: 500;
      box-shadow: var(--shadow-deep);
      display: flex; align-items: center; gap: 9px;
      min-width: 200px;
      animation: toast-in 0.22s ease;
    }
    .toast.error { border-color: rgba(255, 92, 114, 0.4); }
    .toast.success { border-color: rgba(52, 221, 146, 0.4); }
    .toast-dot { width: 7px; height: 7px; border-radius: 50%; flex-shrink: 0; background: var(--success); }
    .toast.error .toast-dot { background: var(--danger); }
    @keyframes toast-in { from { opacity: 0; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }

    .modal-backdrop {
      position: fixed; inset: 0;
      background: rgba(2, 3, 8, 0.7);
      backdrop-filter: blur(4px);
      display: none;
      align-items: center; justify-content: center;
      z-index: 99;
    }
    .modal-backdrop.open { display: flex; }
    .modal-card {
      background: var(--void-2);
      border: 1px solid var(--line);
      border-radius: var(--radius-md);
      padding: 24px;
      width: 320px;
      box-shadow: var(--shadow-deep);
    }
    .modal-card p { font-size: 14px; color: var(--ink); margin-bottom: 18px; line-height: 1.5; }
    .modal-actions { display: flex; gap: 10px; }

    /* ── Clock & Text Panel ── */
    .clock-text-panel { margin-top: 20px; }
    .clock-row { display: flex; gap: 20px; align-items: flex-start; flex-wrap: wrap; }
    .clock-box { flex: 1; min-width: 180px; display: flex; flex-direction: column; align-items: center; gap: 10px; }
    #analog-clock-canvas { border-radius: 50%; background: rgba(0,0,0,0.35); border: 1.5px solid var(--line); }
    .digital-clock-display {
      font-family: 'JetBrains Mono', monospace;
      font-size: 28px;
      font-weight: 600;
      letter-spacing: 0.08em;
      background: linear-gradient(115deg, var(--cyan), var(--magenta));
      -webkit-background-clip: text;
      -webkit-text-fill-color: transparent;
      background-clip: text;
      padding: 4px 0;
    }
    .clock-date-display { font-size: 12px; color: var(--ink-dim); letter-spacing: 0.04em; }
    .text-pov-box { flex: 1; min-width: 220px; display: flex; flex-direction: column; gap: 12px; }
    .text-pov-box label { font-size: 12.5px; color: var(--ink-dim); font-weight: 500; }
    .text-pov-box input[type=text], .text-pov-box select {
      background: rgba(0,0,0,0.3);
      border: 1px solid var(--line);
      border-radius: var(--radius-sm);
      color: var(--ink);
      padding: 9px 12px;
      font-family: 'Inter', sans-serif;
      font-size: 14px;
      width: 100%;
      outline: none;
      transition: border-color 0.2s;
    }
    .text-pov-box input[type=text]:focus, .text-pov-box select:focus { border-color: var(--cyan); }
    .color-row { display: flex; gap: 10px; align-items: center; }
    .color-row label { font-size: 12px; color: var(--ink-dim); }
    .color-row input[type=color] { width: 36px; height: 28px; border: none; border-radius: 6px; cursor: pointer; background: none; }
    .pov-preview-canvas { border-radius: 8px; border: 1px solid var(--line); background: #000; width: 100%; max-width: 280px; aspect-ratio: 1; display: block; margin: 0 auto; }
    .send-clock-options { display: flex; gap: 8px; flex-wrap: wrap; margin-top: 4px; }
  </style>
</head>
<body>
  <div class="scan-rail"></div>

  <div class="shell">
    <div class="hero">
      <div class="hero-mark"></div>
      <div class="hero-text">
        <h1>Fusion 5</h1>
        <p>Persistence-of-vision hologram controller · 200 LEDs</p>
      </div>
      <div class="status-pill" id="status-pill">
        <span class="status-dot"></span>
        <span id="fan-status-text">Connecting…</span>
      </div>
    </div>

    <div class="grid">
      <!-- Library -->
      <section class="panel">
        <div class="panel-title">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="9"></circle><circle cx="12" cy="12" r="2.5"></circle></svg>
          Library
        </div>

        <ul class="media-list" id="media-list">
          <li class="media-empty">Loading…</li>
        </ul>

        <div class="playback-row">
          <div class="ctl" onclick="prevMedia()" title="Previous">
            <svg viewBox="0 0 24 24" fill="currentColor"><path d="M6 6h2v12H6zm3.5 6l8.5 6V6z"></path></svg>
          </div>
          <div class="ctl main" id="play-pause-btn" onclick="togglePlay()" title="Play / Pause">
            <svg viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="5" width="4" height="14" rx="1"></rect><rect x="14" y="5" width="4" height="14" rx="1"></rect></svg>
          </div>
          <div class="ctl" onclick="nextMedia()" title="Next">
            <svg viewBox="0 0 24 24" fill="currentColor"><path d="M16 6h2v12h-2zM6 6l8.5 6L6 18z"></path></svg>
          </div>
        </div>

        <div class="stat-grid">
          <div class="stat">
            <div class="stat-label">RPM</div>
            <div class="stat-value" id="info-rpm">0</div>
          </div>
          <div class="stat clickable" id="info-autonext" onclick="toggleAutoNext()">
            <div class="stat-label">Auto-next</div>
            <div class="stat-value">Enabled</div>
          </div>
          <div class="stat">
            <div class="stat-label">Free heap</div>
            <div class="stat-value" id="info-heap">0 KB</div>
          </div>
          <div class="stat">
            <div class="stat-label">Flash free</div>
            <div class="stat-value" id="info-flash">0 / 0 MB</div>
          </div>
        </div>

        <div class="btn-row">
          <button class="btn btn-ghost" onclick="confirmAction('Reboot the POV fan now?', rebootDevice)">Reboot</button>
          <button class="btn btn-ghost" onclick="window.location.href='/update'">Firmware</button>
        </div>
      </section>

      <!-- Upload / Convert -->
      <section class="panel">
        <div class="panel-title">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path><polyline points="17 8 12 3 7 8"></polyline><line x1="12" y1="3" x2="12" y2="15"></line></svg>
          New animation
        </div>

        <div class="dropzone" id="dropzone" onclick="document.getElementById('file-input').click()">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path><polyline points="17 8 12 3 7 8"></polyline><line x1="12" y1="3" x2="12" y2="15"></line></svg>
          <p>Drag &amp; drop an image or video</p>
          <span>PNG · JPG · MP4 · WebM — converted automatically</span>
        </div>
        <input type="file" id="file-input" accept="image/*,video/*">

        <div class="field">
          <label>Animation name</label>
          <input type="text" id="upload-name" placeholder="e.g. spiral_cube">
        </div>

        <div class="field">
          <label><span>Edge brightness</span><span class="val" id="label-bright">60%</span></label>
          <input type="range" id="param-bright" min="0" max="100" value="60" oninput="document.getElementById('label-bright').innerText=this.value+'%'">
        </div>

        <div class="field">
          <label><span>Center brightness (LED 0)</span><span class="val" id="label-led0">15%</span></label>
          <input type="range" id="param-led0" min="0" max="100" value="15" oninput="document.getElementById('label-led0').innerText=this.value+'%'">
        </div>

        <div class="progress-wrap" id="progress-container">
          <div class="progress-text">
            <span id="progress-status">Converting…</span>
            <span id="progress-percent">0%</span>
          </div>
          <div class="progress-bg"><div class="progress-fill" id="progress-bar-fill"></div></div>
        </div>
      </section>

      <!-- Live preview -->
      <section class="panel panel-preview">
        <div class="panel-title">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M2 12s3.5-7 10-7 10 7 10 7-3.5 7-10 7-10-7-10-7z"></path><circle cx="12" cy="12" r="3"></circle></svg>
          Hologram preview
        </div>
        <div class="preview-ring">
          <div class="preview-disc">
            <canvas id="preview-canvas-360"></canvas>
          </div>
        </div>
        <div class="preview-caption">Simulated rotation &mdash; <b>what the blade will show</b></div>
      </section>
    </div>
  </div>

  <!-- Clock & Text Display Panel -->
  <div class="shell" style="padding-top:0;">
    <section class="panel clock-text-panel">
      <div class="panel-title">
        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg>
        Clock &amp; Text Display
      </div>
      <div class="clock-row">
        <!-- Clocks -->
        <div class="clock-box">
          <canvas id="analog-clock-canvas" width="160" height="160"></canvas>
          <div class="digital-clock-display" id="digital-clock-display">00:00:00</div>
          <div class="clock-date-display" id="clock-date-display"></div>
        </div>
        <!-- Text / Options -->
        <div class="text-pov-box">
          <div>
            <label>Custom text to display on fan</label>
            <input type="text" id="pov-text-input" placeholder="e.g. FUSION 5" value="FUSION 5" maxlength="24">
          </div>
          <div>
            <label>Font style</label>
            <select id="pov-font-select">
              <option value="bold 52px 'Space Grotesk', sans-serif">Space Grotesk (Bold)</option>
              <option value="bold 48px 'JetBrains Mono', monospace">JetBrains Mono</option>
              <option value="bold 56px Arial, sans-serif">Arial Bold</option>
              <option value="bold 52px Georgia, serif">Georgia</option>
            </select>
          </div>
          <div class="color-row">
            <label>Text color</label>
            <input type="color" id="pov-text-color" value="#2fe6d8">
            <label style="margin-left:12px;">Background</label>
            <input type="color" id="pov-bg-color" value="#000000">
          </div>
          <div>
            <label>What to send to the fan</label>
            <div class="send-clock-options">
              <button class="btn btn-solid" onclick="sendTextToPOV()">Send Text</button>
              <button class="btn btn-ghost" onclick="sendAnalogClockToPOV()">Send Analog Clock</button>
              <button class="btn btn-ghost" onclick="sendDigitalClockToPOV()">Send Digital Clock</button>
            </div>
          </div>
        </div>
        <!-- POV Preview -->
        <div class="clock-box" style="min-width:200px;">
          <div style="font-size:12px;color:var(--ink-dim);margin-bottom:6px;">POV preview</div>
          <canvas class="pov-preview-canvas" id="pov-preview-360" width="240" height="240"></canvas>
        </div>
      </div>
    </section>
  </div>

  <!-- Hidden canvases for polar conversion -->
  <canvas id="canvas-src" style="display:none;"></canvas>
  <canvas id="canvas-polar" style="display:none;" width="100" height="320"></canvas>

  <!-- Confirm modal -->
  <div class="modal-backdrop" id="modal-backdrop">
    <div class="modal-card">
      <p id="modal-message">Are you sure?</p>
      <div class="modal-actions">
        <button class="btn btn-ghost" onclick="closeModal()">Cancel</button>
        <button class="btn btn-solid" id="modal-confirm-btn">Confirm</button>
      </div>
    </div>
  </div>

  <div class="toast-stack" id="toast-stack"></div>

  <script>
    const NUMPIXELS = 100; // 100 LEDs per half (radial width)
    const DIV = 320;       // 320 angular divisions (polar height)
    let isPlaying = true;
    let autoNext = true;

    // ---------- UI helpers: toasts + confirm modal (replaces alert/confirm) ----------
    function showToast(message, type = 'success') {
      const stack = document.getElementById('toast-stack');
      const el = document.createElement('div');
      el.className = `toast ${type}`;
      el.innerHTML = `<span class="toast-dot"></span><span>${message}</span>`;
      stack.appendChild(el);
      setTimeout(() => el.remove(), 4200);
    }

    function confirmAction(message, onConfirm) {
      const backdrop = document.getElementById('modal-backdrop');
      document.getElementById('modal-message').innerText = message;
      const btn = document.getElementById('modal-confirm-btn');
      const handler = () => { closeModal(); onConfirm(); };
      btn.onclick = handler;
      backdrop.classList.add('open');
    }

    function closeModal() {
      document.getElementById('modal-backdrop').classList.remove('open');
    }

    // ---------- Status polling ----------
    async function getStatus() {
      try {
        let res = await fetch('/status');
        let status = await res.json();

        document.getElementById('status-pill').classList.remove('offline');
        document.getElementById('fan-status-text').innerText = "ESP32-S3 connected";
        document.getElementById('info-rpm').innerText = status.rpm;

        isPlaying = status.playing;
        document.getElementById('play-pause-btn').innerHTML = isPlaying
          ? '<svg viewBox="0 0 24 24" fill="currentColor"><rect x="6" y="5" width="4" height="14" rx="1"></rect><rect x="14" y="5" width="4" height="14" rx="1"></rect></svg>'
          : '<svg viewBox="0 0 24 24" fill="currentColor"><path d="M7 5l13 7-13 7z"></path></svg>';

        autoNext = status.auto_next;
        document.querySelector('#info-autonext .stat-value').innerText = autoNext ? "Enabled" : "Disabled";

        document.getElementById('info-heap').innerText = Math.round(status.free_heap / 1024) + " KB";
        document.getElementById('info-flash').innerText = Math.round(status.free_flash / (1024*1024)) + " / " + Math.round(status.total_flash / (1024*1024)) + " MB";

        let listRes = await fetch('/list');
        let list = await listRes.json();
        updateMediaListUI(list, status.current_media);
      } catch (err) {
        document.getElementById('status-pill').classList.add('offline');
        document.getElementById('fan-status-text').innerText = "Offline";
      }
    }

    function updateMediaListUI(list, currentMedia) {
      let listEl = document.getElementById('media-list');
      listEl.innerHTML = '';
      if (list.length === 0) {
        listEl.innerHTML = '<li class="media-empty">No animations yet — upload one to get started.</li>';
        return;
      }
      list.forEach(name => {
        let li = document.createElement('li');
        li.className = 'media-item' + (name === currentMedia ? ' active' : '');
        li.onclick = () => selectMedia(name);

        let nameDiv = document.createElement('div');
        nameDiv.className = 'media-item-name';
        nameDiv.innerHTML = '<span class="dot"></span>' + name;

        let delBtn = document.createElement('button');
        delBtn.className = 'btn-icon';
        delBtn.innerHTML = '<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><polyline points="3 6 5 6 21 6"></polyline><path d="M19 6l-1 14a2 2 0 0 1-2 2H8a2 2 0 0 1-2-2L5 6"></path><path d="M10 11v6M14 11v6"></path></svg>';
        delBtn.onclick = (e) => {
          e.stopPropagation();
          confirmAction(`Delete animation "${name}"? This can't be undone.`, () => deleteMedia(name));
        };

        li.appendChild(nameDiv);
        li.appendChild(delBtn);
        listEl.appendChild(li);
      });
    }

    async function selectMedia(name) {
      await fetch(`/select?name=${encodeURIComponent(name)}`);
      getStatus();
    }

    async function deleteMedia(name) {
      await fetch(`/delete?name=${encodeURIComponent(name)}`);
      showToast(`Deleted "${name}"`, 'success');
      getStatus();
    }

    async function togglePlay() {
      await fetch('/playpause');
      getStatus();
    }

    async function toggleAutoNext() {
      await fetch('/autonext');
      getStatus();
    }

    async function nextMedia() {
      await fetch('/next');
      getStatus();
    }

    async function prevMedia() {
      await fetch('/prev');
      getStatus();
    }

    async function rebootDevice() {
      await fetch('/reboot');
      showToast("Rebooting — reload the page in a few seconds.", 'success');
    }

    // Set up polling
    setInterval(getStatus, 4000);
    getStatus();

    // ---------- Drag-and-drop & upload handling ----------
    const dropzone = document.getElementById('dropzone');
    const fileInput = document.getElementById('file-input');

    dropzone.addEventListener('dragover', (e) => {
      e.preventDefault();
      dropzone.classList.add('dragover');
    });

    dropzone.addEventListener('dragleave', () => {
      dropzone.classList.remove('dragover');
    });

    dropzone.addEventListener('drop', (e) => {
      e.preventDefault();
      dropzone.classList.remove('dragover');
      if (e.dataTransfer.files.length) {
        handleFile(e.dataTransfer.files[0]);
      }
    });

    fileInput.addEventListener('change', (e) => {
      if (fileInput.files.length) {
        handleFile(fileInput.files[0]);
      }
    });

    function handleFile(file) {
      let cleanName = file.name.split('.')[0].replace(/[^a-zA-Z0-9_]/g, "_").toLowerCase();
      document.getElementById('upload-name').value = cleanName;

      if (file.type.startsWith('image/')) {
        processImage(file);
      } else if (file.type.startsWith('video/')) {
        processVideo(file);
      } else {
        showToast("Unsupported format — use JPEG, PNG, MP4, or WebM.", 'error');
      }
    }

    // ---------- Quality helpers: resize + contrast/saturation/gamma before polar conversion ----------
    function clamp255(v) { return Math.max(0, Math.min(255, Math.round(v))); }
    function enhanceRgb(r, g, b) {
      const contrast = 1.22;
      const saturation = 1.18;
      const gamma = 0.90;

      r = (r - 128) * contrast + 128;
      g = (g - 128) * contrast + 128;
      b = (b - 128) * contrast + 128;

      const gray = 0.299 * r + 0.587 * g + 0.114 * b;
      r = gray + (r - gray) * saturation;
      g = gray + (g - gray) * saturation;
      b = gray + (b - gray) * saturation;

      r = 255 * Math.pow(Math.max(0, Math.min(255, r)) / 255, gamma);
      g = 255 * Math.pow(Math.max(0, Math.min(255, g)) / 255, gamma);
      b = 255 * Math.pow(Math.max(0, Math.min(255, b)) / 255, gamma);
      return [clamp255(r), clamp255(g), clamp255(b)];
    }

    function sampleBilinear(pixels, w, h, x, y) {
      if (x < 0 || x >= w - 1 || y < 0 || y >= h - 1) return [0, 0, 0];
      const x0 = Math.floor(x), y0 = Math.floor(y);
      const x1 = x0 + 1, y1 = y0 + 1;
      const fx = x - x0, fy = y - y0;
      const i00 = (y0 * w + x0) * 4;
      const i10 = (y0 * w + x1) * 4;
      const i01 = (y1 * w + x0) * 4;
      const i11 = (y1 * w + x1) * 4;
      const out = [0, 1, 2].map(c => {
        const top = pixels[i00+c] * (1 - fx) + pixels[i10+c] * fx;
        const bot = pixels[i01+c] * (1 - fx) + pixels[i11+c] * fx;
        return top * (1 - fy) + bot * fy;
      });
      return out;
    }

    function applyUnsharpMask(ctx, w, h, amount = 0.35) {
      const src = ctx.getImageData(0, 0, w, h);
      const data = src.data;
      const copy = new Uint8ClampedArray(data);
      // Lightweight sharpen kernel, enough for small POV source images.
      for (let y = 1; y < h - 1; y++) {
        for (let x = 1; x < w - 1; x++) {
          const idx = (y * w + x) * 4;
          for (let c = 0; c < 3; c++) {
            const center = copy[idx+c] * 5;
            const near = copy[idx-4+c] + copy[idx+4+c] + copy[idx-w*4+c] + copy[idx+w*4+c];
            const sharp = center - near;
            data[idx+c] = clamp255(copy[idx+c] * (1 - amount) + sharp * amount);
          }
        }
      }
      ctx.putImageData(src, 0, 0);
    }

    // Polar warping: bilinear sampling + LED-oriented color enhancement.
    function polarWarp(srcCtx, destCtx, w, h, led0Bright, bright) {
      const srcData = srcCtx.getImageData(0, 0, w, h);
      const destData = destCtx.createImageData(NUMPIXELS, DIV);
      const srcPixels = srcData.data;
      const destPixels = destData.data;

      const wC = (w - 1) / 2;
      const hC = (h - 1) / 2;

      for (let j = 0; j < DIV; j++) {
        const angle = (2 * Math.PI * j) / DIV;
        const cosA = Math.cos(angle);
        const sinA = Math.sin(angle);

        for (let i = 0; i < NUMPIXELS; i++) {
          const r = i;
          const srcX = wC - r * sinA;
          const srcY = hC + r * cosA;

          let [rCol, gCol, bCol] = sampleBilinear(srcPixels, w, h, srcX, srcY);
          [rCol, gCol, bCol] = enhanceRgb(rCol, gCol, bCol);

          // Radial compensation: center is intentionally lower, edge is brighter.
          const radiusFactor = Math.pow(i / Math.max(1, NUMPIXELS - 1), 0.75);
          const scale = ((led0Bright / 100) + (bright / 100 - led0Bright / 100) * radiusFactor);

          const destIdx = (j * NUMPIXELS + i) * 4;
          destPixels[destIdx] = clamp255(rCol * scale);
          destPixels[destIdx + 1] = clamp255(gCol * scale);
          destPixels[destIdx + 2] = clamp255(bCol * scale);
          destPixels[destIdx + 3] = 255;
        }
      }
      destCtx.putImageData(destData, 0, 0);
      drawHologramSimulation(destData);
    }

    // Hologram simulation: reconstructs Cartesian from polar to show the rotating display.
    function drawHologramSimulation(polarData) {
      const simCanvas = document.getElementById('preview-canvas-360');
      simCanvas.width = 300;
      simCanvas.height = 300;
      const simCtx = simCanvas.getContext('2d');
      const simData = simCtx.createImageData(300, 300);
      const simPixels = simData.data;
      const polarPixels = polarData.data;

      const cX = 150;
      const cY = 150;

      for (let y = 0; y < 300; y++) {
        for (let x = 0; x < 300; x++) {
          const dx = x - cX;
          const dy = y - cY;
          const r = Math.sqrt(dx*dx + dy*dy);

          if (r < 148) {
            const i = Math.round((r / 148) * (NUMPIXELS - 1));
            let angle = Math.atan2(-dx, dy);
            if (angle < 0) angle += 2 * Math.PI;
            const j = Math.round((angle / (2 * Math.PI)) * (DIV - 1));

            const pIdx = (j * NUMPIXELS + i) * 4;
            const sIdx = (y * 300 + x) * 4;

            simPixels[sIdx] = polarPixels[pIdx];
            simPixels[sIdx + 1] = polarPixels[pIdx + 1];
            simPixels[sIdx + 2] = polarPixels[pIdx + 2];
            simPixels[sIdx + 3] = 255;
          }
        }
      }
      simCtx.putImageData(simData, 0, 0);
    }

    function showProgress(status, pct) {
      document.getElementById('progress-container').style.display = 'block';
      document.getElementById('progress-status').innerText = status;
      document.getElementById('progress-percent').innerText = Math.round(pct) + "%";
      document.getElementById('progress-bar-fill').style.width = pct + "%";
    }

    function hideProgress() {
      document.getElementById('progress-container').style.display = 'none';
    }

    async function processImage(file) {
      try {
        const img = new Image();
        img.src = URL.createObjectURL(file);
        await new Promise(r => img.onload = r);

        const srcCanvas = document.getElementById('canvas-src');
        const srcCtx = srcCanvas.getContext('2d');
        const polarCanvas = document.getElementById('canvas-polar');
        const polarCtx = polarCanvas.getContext('2d');

        const size = NUMPIXELS * 2 - 1;
        srcCanvas.width = size;
        srcCanvas.height = size;
        srcCtx.imageSmoothingEnabled = true;
        srcCtx.imageSmoothingQuality = "high";
        srcCtx.drawImage(img, 0, 0, size, size);
        applyUnsharpMask(srcCtx, size, size, 0.28);

        showProgress("Calibrating…", 50);
        const bright = document.getElementById('param-bright').value;
        const led0 = document.getElementById('param-led0').value;
        polarWarp(srcCtx, polarCtx, size, size, led0, bright);

        const animName = document.getElementById('upload-name').value || "unnamed";
        showProgress("Uploading…", 75);

        let blob = await new Promise(resolve => polarCanvas.toBlob(resolve, 'image/jpeg', 0.92));
        await uploadFrame(animName, 0, blob);

        showProgress("Finishing…", 95);
        await finishUpload(animName);
        showProgress("Done", 100);
        showToast(`"${animName}" uploaded`, 'success');
        setTimeout(hideProgress, 1200);
      } catch (err) {
        showToast("Image processing failed: " + err, 'error');
        hideProgress();
      }
    }

    async function processVideo(file) {
      try {
        const video = document.createElement('video');
        video.src = URL.createObjectURL(file);
        video.muted = true;
        video.playsInline = true;

        showProgress("Decoding video…", 5);
        await new Promise(r => video.onloadedmetadata = r);

        const srcCanvas = document.getElementById('canvas-src');
        const srcCtx = srcCanvas.getContext('2d');
        const polarCanvas = document.getElementById('canvas-polar');
        const polarCtx = polarCanvas.getContext('2d');

        const size = NUMPIXELS * 2 - 1;
        srcCanvas.width = size;
        srcCanvas.height = size;

        const duration = video.duration;
        const fps = 20; // Better smoothness for POV; keep below huge storage usage
        const interval = 1.0 / fps;
        const maxFrames = 160; // 8 seconds max at 20 FPS
        const totalFrames = Math.min(maxFrames, Math.floor(duration * fps));

        const animName = document.getElementById('upload-name').value || "unnamed";
        const bright = document.getElementById('param-bright').value;
        const led0 = document.getElementById('param-led0').value;

        for (let f = 0; f < totalFrames; f++) {
          const pct = (f / totalFrames) * 90 + 5;
          showProgress(`Processing frame ${f + 1}/${totalFrames}…`, pct);

          video.currentTime = f * interval;
          await new Promise(r => video.onseeked = r);

          srcCtx.imageSmoothingEnabled = true;
          srcCtx.imageSmoothingQuality = "high";
          srcCtx.drawImage(video, 0, 0, size, size);
          applyUnsharpMask(srcCtx, size, size, 0.20);
          polarWarp(srcCtx, polarCtx, size, size, led0, bright);

          let blob = await new Promise(resolve => polarCanvas.toBlob(resolve, 'image/jpeg', 0.92));
          await uploadFrame(animName, f, blob);
          await new Promise(r => setTimeout(r, 60)); // 60ms delay to prevent network starvation
        }

        showProgress("Saving animation…", 95);
        await finishUpload(animName);
        showProgress("Done", 100);
        showToast(`"${animName}" uploaded (${totalFrames} frames)`, 'success');
        setTimeout(hideProgress, 1200);
      } catch (err) {
        showToast("Video processing failed: " + err, 'error');
        hideProgress();
      }
    }

    function uploadFrame(animName, frameNum, blob) {
      return new Promise((resolve, reject) => {
        let xhr = new XMLHttpRequest();
        let path = `/bbPOV-P/${animName}/${frameNum}.jpg`;
        xhr.open('POST', `/upload_file?path=${encodeURIComponent(path)}`, true);

        // ESP32 WebServer upload handler needs multipart/form-data.
        // Sending the raw blob body causes timeout/no saved file.
        let form = new FormData();
        form.append('file', blob, `${frameNum}.jpg`);

        xhr.onload = () => {
          if (xhr.status === 200) resolve();
          else reject("Status " + xhr.status + ": " + xhr.responseText);
        };
        xhr.onerror = () => reject("Network error");
        xhr.ontimeout = () => reject("Upload timeout");
        xhr.timeout = 30000;
        xhr.send(form);
      });
    }

    async function finishUpload(animName) {
      await fetch(`/upload_end?name=${encodeURIComponent(animName)}&t=${Date.now()}`, { method: 'POST', cache: 'no-store' });
      getStatus();
    }

    // ── Analog Clock ──────────────────────────────────────────────────────────
    function drawAnalogClock() {
      const canvas = document.getElementById('analog-clock-canvas');
      const ctx = canvas.getContext('2d');
      const W = canvas.width, H = canvas.height;
      const cx = W / 2, cy = H / 2, R = W / 2 - 6;
      const now = new Date();
      const s = now.getSeconds(), m = now.getMinutes(), h = now.getHours() % 12;

      ctx.clearRect(0, 0, W, H);

      // Face
      ctx.beginPath();
      ctx.arc(cx, cy, R, 0, 2 * Math.PI);
      ctx.fillStyle = '#000';
      ctx.fill();
      ctx.strokeStyle = '#2fe6d8';
      ctx.lineWidth = 2;
      ctx.stroke();

      // Tick marks
      for (let i = 0; i < 60; i++) {
        const a = (i / 60) * 2 * Math.PI - Math.PI / 2;
        const isHour = i % 5 === 0;
        const len = isHour ? 10 : 5;
        ctx.beginPath();
        ctx.moveTo(cx + Math.cos(a) * (R - len), cy + Math.sin(a) * (R - len));
        ctx.lineTo(cx + Math.cos(a) * R, cy + Math.sin(a) * R);
        ctx.strokeStyle = isHour ? '#2fe6d8' : '#444';
        ctx.lineWidth = isHour ? 2 : 1;
        ctx.stroke();
      }

      // Hour hand
      const hAngle = ((h + m / 60) / 12) * 2 * Math.PI - Math.PI / 2;
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.lineTo(cx + Math.cos(hAngle) * R * 0.52, cy + Math.sin(hAngle) * R * 0.52);
      ctx.strokeStyle = '#eef0fb';
      ctx.lineWidth = 4;
      ctx.lineCap = 'round';
      ctx.stroke();

      // Minute hand
      const mAngle = ((m + s / 60) / 60) * 2 * Math.PI - Math.PI / 2;
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.lineTo(cx + Math.cos(mAngle) * R * 0.74, cy + Math.sin(mAngle) * R * 0.74);
      ctx.strokeStyle = '#2fe6d8';
      ctx.lineWidth = 3;
      ctx.lineCap = 'round';
      ctx.stroke();

      // Second hand
      const sAngle = (s / 60) * 2 * Math.PI - Math.PI / 2;
      ctx.beginPath();
      ctx.moveTo(cx, cy);
      ctx.lineTo(cx + Math.cos(sAngle) * R * 0.86, cy + Math.sin(sAngle) * R * 0.86);
      ctx.strokeStyle = '#ff4fb4';
      ctx.lineWidth = 1.5;
      ctx.lineCap = 'round';
      ctx.stroke();

      // Center dot
      ctx.beginPath();
      ctx.arc(cx, cy, 4, 0, 2 * Math.PI);
      ctx.fillStyle = '#ff4fb4';
      ctx.fill();
    }

    // ── Digital Clock ─────────────────────────────────────────────────────────
    function updateDigitalClock() {
      const now = new Date();
      const pad = n => String(n).padStart(2, '0');
      document.getElementById('digital-clock-display').innerText =
        `${pad(now.getHours())}:${pad(now.getMinutes())}:${pad(now.getSeconds())}`;
      const days = ['Sunday','Monday','Tuesday','Wednesday','Thursday','Friday','Saturday'];
      const months = ['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
      document.getElementById('clock-date-display').innerText =
        `${days[now.getDay()]}, ${months[now.getMonth()]} ${now.getDate()} ${now.getFullYear()}`;
    }

    setInterval(() => { drawAnalogClock(); updateDigitalClock(); }, 1000);
    drawAnalogClock();
    updateDigitalClock();

    // ── POV Polar Rendering Helpers ───────────────────────────────────────────
    function renderPolarFromCanvas(srcCanvas, animName, onDone) {
      const size = NUMPIXELS * 2 - 1;
      const polarCanvas = document.getElementById('canvas-polar');
      const polarCtx = polarCanvas.getContext('2d');
      const srcCtx = srcCanvas.getContext('2d');

      const srcData = srcCtx.getImageData(0, 0, size, size);
      const destData = polarCtx.createImageData(NUMPIXELS, DIV);
      const srcPx = srcData.data;
      const dstPx = destData.data;

      const wC = (size - 1) / 2, hC = (size - 1) / 2;
      const bright = 0.80, led0 = 0.15;

      for (let j = 0; j < DIV; j++) {
        const angle = (2 * Math.PI * j) / DIV;
        const cosA = Math.cos(angle), sinA = Math.sin(angle);
        for (let i = 0; i < NUMPIXELS; i++) {
          const srcX = wC - i * sinA;
          const srcY = hC + i * cosA;
          const x0 = Math.floor(srcX), y0 = Math.floor(srcY);
          let r = 0, g = 0, b = 0;
          if (x0 >= 0 && x0 < size - 1 && y0 >= 0 && y0 < size - 1) {
            const fx = srcX - x0, fy = srcY - y0;
            const i00 = (y0 * size + x0) * 4;
            const i10 = (y0 * size + x0 + 1) * 4;
            const i01 = ((y0 + 1) * size + x0) * 4;
            const i11 = ((y0 + 1) * size + x0 + 1) * 4;
            r = srcPx[i00] * (1-fx)*(1-fy) + srcPx[i10]*fx*(1-fy) + srcPx[i01]*(1-fx)*fy + srcPx[i11]*fx*fy;
            g = srcPx[i00+1]*(1-fx)*(1-fy)+srcPx[i10+1]*fx*(1-fy)+srcPx[i01+1]*(1-fx)*fy+srcPx[i11+1]*fx*fy;
            b = srcPx[i00+2]*(1-fx)*(1-fy)+srcPx[i10+2]*fx*(1-fy)+srcPx[i01+2]*(1-fx)*fy+srcPx[i11+2]*fx*fy;
          }
          const rf = Math.pow(i / Math.max(1, NUMPIXELS - 1), 0.75);
          const sc = led0 + (bright - led0) * rf;
          const di = (j * NUMPIXELS + i) * 4;
          dstPx[di] = Math.min(255, r * sc);
          dstPx[di+1] = Math.min(255, g * sc);
          dstPx[di+2] = Math.min(255, b * sc);
          dstPx[di+3] = 255;
        }
      }
      polarCtx.putImageData(destData, 0, 0);
      drawHologramSimulationTo('pov-preview-360', destData, 240);
      uploadPolarAndSend(animName, polarCanvas, onDone);
    }

    function drawHologramSimulationTo(canvasId, polarData, size) {
      const simCanvas = document.getElementById(canvasId);
      simCanvas.width = size; simCanvas.height = size;
      const simCtx = simCanvas.getContext('2d');
      const simData = simCtx.createImageData(size, size);
      const sp = simData.data, pp = polarData.data;
      const cx = size / 2, cy = size / 2, maxR = size / 2 - 2;
      for (let y = 0; y < size; y++) {
        for (let x = 0; x < size; x++) {
          const dx = x - cx, dy = y - cy;
          const r = Math.sqrt(dx*dx + dy*dy);
          if (r < maxR) {
            const i = Math.round((r / maxR) * (NUMPIXELS - 1));
            let angle = Math.atan2(-dx, dy);
            if (angle < 0) angle += 2 * Math.PI;
            const j = Math.round((angle / (2 * Math.PI)) * (DIV - 1));
            const pIdx = (j * NUMPIXELS + i) * 4;
            const sIdx = (y * size + x) * 4;
            sp[sIdx] = pp[pIdx]; sp[sIdx+1] = pp[pIdx+1]; sp[sIdx+2] = pp[pIdx+2]; sp[sIdx+3] = 255;
          }
        }
      }
      simCtx.putImageData(simData, 0, 0);
    }

    async function uploadPolarAndSend(animName, polarCanvas, onDone) {
      showProgress('Sending to fan…', 60);
      let blob = await new Promise(r => polarCanvas.toBlob(r, 'image/jpeg', 0.92));
      try {
        await uploadFrame(animName, 0, blob);
        showProgress('Finishing…', 90);
        await finishUpload(animName);
        showProgress('Done ✓', 100);
        showToast(`"${animName}" sent to fan`, 'success');
        setTimeout(hideProgress, 1400);
        if (onDone) onDone();
      } catch(e) {
        showToast('Send failed: ' + e, 'error');
        hideProgress();
      }
    }

    function makeSrcCanvas(drawFn) {
      const size = NUMPIXELS * 2 - 1;
      const c = document.getElementById('canvas-src');
      c.width = size; c.height = size;
      const ctx = c.getContext('2d');
      ctx.clearRect(0, 0, size, size);
      drawFn(ctx, size);
      return c;
    }

    // ── Send Text ─────────────────────────────────────────────────────────────
    function sendTextToPOV() {
      const text = document.getElementById('pov-text-input').value.trim() || 'FUSION 5';
      const fg = document.getElementById('pov-text-color').value;
      const bg = document.getElementById('pov-bg-color').value;
      const font = document.getElementById('pov-font-select').value;
      const src = makeSrcCanvas((ctx, size) => {
        ctx.fillStyle = bg;
        ctx.fillRect(0, 0, size, size);
        ctx.font = font;
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillStyle = fg;
        // Auto-scale text to fit
        let fs = parseInt(font);
        while (fs > 10) {
          ctx.font = font.replace(/\d+px/, fs + 'px');
          if (ctx.measureText(text).width < size * 0.88) break;
          fs -= 2;
        }
        ctx.fillText(text, size / 2, size / 2);
      });
      renderPolarFromCanvas(src, 'fusion5_text');
    }

    // ── Send Analog Clock ─────────────────────────────────────────────────────
    function sendAnalogClockToPOV() {
      const bg = document.getElementById('pov-bg-color').value;
      const src = makeSrcCanvas((ctx, size) => {
        const cx = size / 2, cy = size / 2, R = size / 2 - 4;
        const now = new Date();
        const sec = now.getSeconds(), min = now.getMinutes(), hr = now.getHours() % 12;

        ctx.fillStyle = bg;
        ctx.fillRect(0, 0, size, size);

        // Face
        ctx.beginPath();
        ctx.arc(cx, cy, R, 0, 2*Math.PI);
        ctx.fillStyle = '#05060c';
        ctx.fill();
        ctx.strokeStyle = '#2fe6d8';
        ctx.lineWidth = 3;
        ctx.stroke();

        // Ticks
        for (let i = 0; i < 60; i++) {
          const a = (i/60)*2*Math.PI - Math.PI/2;
          const isH = i%5===0;
          ctx.beginPath();
          ctx.moveTo(cx+Math.cos(a)*(R-(isH?14:7)), cy+Math.sin(a)*(R-(isH?14:7)));
          ctx.lineTo(cx+Math.cos(a)*R, cy+Math.sin(a)*R);
          ctx.strokeStyle = isH ? '#2fe6d8' : '#333';
          ctx.lineWidth = isH ? 3 : 1;
          ctx.stroke();
        }

        // Hour nums
        ctx.font = `bold ${Math.round(size*0.08)}px Arial`;
        ctx.textAlign='center'; ctx.textBaseline='middle'; ctx.fillStyle='#8a8fac';
        for (let i=1;i<=12;i++){
          const a=(i/12)*2*Math.PI-Math.PI/2;
          ctx.fillText(i, cx+Math.cos(a)*(R*0.76), cy+Math.sin(a)*(R*0.76));
        }

        // Hour hand
        const hA = ((hr+min/60)/12)*2*Math.PI-Math.PI/2;
        ctx.beginPath(); ctx.moveTo(cx,cy);
        ctx.lineTo(cx+Math.cos(hA)*R*0.50, cy+Math.sin(hA)*R*0.50);
        ctx.strokeStyle='#eef0fb'; ctx.lineWidth=5; ctx.lineCap='round'; ctx.stroke();

        // Minute hand
        const mA = ((min+sec/60)/60)*2*Math.PI-Math.PI/2;
        ctx.beginPath(); ctx.moveTo(cx,cy);
        ctx.lineTo(cx+Math.cos(mA)*R*0.72, cy+Math.sin(mA)*R*0.72);
        ctx.strokeStyle='#2fe6d8'; ctx.lineWidth=3; ctx.lineCap='round'; ctx.stroke();

        // Second hand
        const sA = (sec/60)*2*Math.PI-Math.PI/2;
        ctx.beginPath(); ctx.moveTo(cx,cy);
        ctx.lineTo(cx+Math.cos(sA)*R*0.84, cy+Math.sin(sA)*R*0.84);
        ctx.strokeStyle='#ff4fb4'; ctx.lineWidth=2; ctx.lineCap='round'; ctx.stroke();

        ctx.beginPath(); ctx.arc(cx,cy,6,0,2*Math.PI);
        ctx.fillStyle='#ff4fb4'; ctx.fill();

        // Label
        ctx.font=`bold ${Math.round(size*0.07)}px 'Space Grotesk', sans-serif`;
        ctx.fillStyle='#2fe6d8'; ctx.textAlign='center';
        ctx.fillText('FUSION 5', cx, cy + R*0.55);
      });
      renderPolarFromCanvas(src, 'fusion5_analog');
    }

    // ── Send Digital Clock ────────────────────────────────────────────────────
    function sendDigitalClockToPOV() {
      const bg = document.getElementById('pov-bg-color').value;
      const fg = document.getElementById('pov-text-color').value;
      const src = makeSrcCanvas((ctx, size) => {
        const now = new Date();
        const pad = n => String(n).padStart(2,'0');
        const timeStr = `${pad(now.getHours())}:${pad(now.getMinutes())}:${pad(now.getSeconds())}`;
        const days=['SUN','MON','TUE','WED','THU','FRI','SAT'];
        const months=['JAN','FEB','MAR','APR','MAY','JUN','JUL','AUG','SEP','OCT','NOV','DEC'];
        const dateStr = `${days[now.getDay()]} ${now.getDate()} ${months[now.getMonth()]}`;

        ctx.fillStyle = bg;
        ctx.fillRect(0,0,size,size);

        // Outer ring
        ctx.beginPath();
        ctx.arc(size/2,size/2,size/2-2,0,2*Math.PI);
        ctx.strokeStyle='#2fe6d8';
        ctx.lineWidth=3; ctx.stroke();

        // Glow time
        ctx.font=`bold ${Math.round(size*0.19)}px 'JetBrains Mono', monospace`;
        ctx.textAlign='center'; ctx.textBaseline='middle';
        ctx.shadowColor=fg; ctx.shadowBlur=18;
        ctx.fillStyle=fg;
        ctx.fillText(timeStr, size/2, size/2 - size*0.06);
        ctx.shadowBlur=0;

        // Date
        ctx.font=`bold ${Math.round(size*0.09)}px 'Space Grotesk', sans-serif`;
        ctx.fillStyle='#8a8fac';
        ctx.fillText(dateStr, size/2, size/2 + size*0.16);

        // Brand
        ctx.font=`bold ${Math.round(size*0.07)}px 'Space Grotesk', sans-serif`;
        ctx.fillStyle='#ff4fb4';
        ctx.fillText('FUSION 5', size/2, size/2 + size*0.31);
      });
      renderPolarFromCanvas(src, 'fusion5_digital');
    }
  </script>
</body>
</html>)rawliteral";

#endif
