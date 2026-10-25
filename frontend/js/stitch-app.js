/**
 * ISHA AI - Google Stitch Client Application
 * Handles Real-time Streaming, Knowledge Base Ingestion, Online Live RAG,
 * Vision Diagnostics, Voice STT/TTS, and Watchdog Telemetry.
 */

let ws = null;
let currentTab = "chatTab";
let isOnlineRagActive = false;
let activePack = "all";
let chatHistory = [];
let isGenerating = false;
let currentStreamBubble = null;
let currentTraceContent = null;
let accumulatedMarkdown = "";
let audioContext = null;
let analyser = null;
let animationFrameId = null;

document.addEventListener("DOMContentLoaded", () => {
  initTheme();
  initPWA();
  initTabs();
  initWebSocket();
  initPromptDock();
  initQuickChips();
  initKnowledgeBase();
  initLiveRagExplorer();
  initVisionStudio();
  initWatchdog();
  startTelemetryPolling();
});

/* ==========================================================================
   THEME & UI SWITCHING
   ========================================================================== */
function initTheme() {
  const savedTheme = localStorage.getItem("isha_theme") || "dark";
  document.documentElement.setAttribute("data-theme", savedTheme);
  updateThemeIcon(savedTheme);

  document.getElementById("themeToggleBtn").addEventListener("click", () => {
    const current = document.documentElement.getAttribute("data-theme");
    const next = current === "dark" ? "light" : "dark";
    document.documentElement.setAttribute("data-theme", next);
    localStorage.setItem("isha_theme", next);
    updateThemeIcon(next);
  });
}

function updateThemeIcon(theme) {
  const icon = document.getElementById("themeIcon");
  if (icon) icon.textContent = theme === "dark" ? "☀️" : "🌙";
}

let deferredInstallPrompt = null;
function initPWA() {
  // Register Service Worker
  if ('serviceWorker' in navigator) {
    navigator.serviceWorker.register('/sw.js').catch(err => {
      console.warn('[PWA] ServiceWorker registration skipped:', err);
    });
  }

  const installBtn = document.getElementById("installAppBtn");
  window.addEventListener('beforeinstallprompt', (e) => {
    e.preventDefault();
    deferredInstallPrompt = e;
    if (installBtn) installBtn.style.display = "inline-flex";
  });

  if (installBtn) {
    installBtn.addEventListener('click', async () => {
      if (deferredInstallPrompt) {
        deferredInstallPrompt.prompt();
        const { outcome } = await deferredInstallPrompt.userChoice;
        if (outcome === 'accepted') {
          installBtn.style.display = 'none';
        }
        deferredInstallPrompt = null;
      } else {
        alert("To install ISHA AI on your device:\n- In Chrome/Edge: Click the install icon in the URL address bar.\n- On Android: Tap menu (⋮) -> 'Add to Home screen' or 'Install App'.\n- On iPhone/iPad: Tap Share -> 'Add to Home Screen'.");
      }
    });
  }
}

function initTabs() {
  const railBtns = document.querySelectorAll(".rail-btn");
  railBtns.forEach(btn => {
    btn.addEventListener("click", () => {
      const tabId = btn.getAttribute("data-tab");
      switchTab(tabId);
    });
  });
}

function switchTab(tabId) {
  document.querySelectorAll(".rail-btn").forEach(b => b.classList.remove("active"));
  document.querySelectorAll(".tab-pane").forEach(p => p.classList.remove("active"));

  const targetBtn = document.querySelector(`.rail-btn[data-tab="${tabId}"]`);
  const targetPane = document.getElementById(tabId);

  if (targetBtn) targetBtn.classList.add("active");
  if (targetPane) targetPane.classList.add("active");
  currentTab = tabId;

  if (tabId === "kbTab") loadPacks();
}

/* ==========================================================================
   WEBSOCKET & REAL-TIME STREAMING
   ========================================================================== */
function initWebSocket() {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  const wsUrl = `${protocol}//${window.location.host}/ws/chat`;

  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    console.log("[ISHA WS] Connected to cognitive runtime.");
  };

  ws.onmessage = (event) => {
    try {
      const msg = JSON.parse(event.data);
      handleStreamEvent(msg);
    } catch (e) {
      console.error("[ISHA WS] Parse error:", e);
    }
  };

  ws.onclose = () => {
    console.warn("[ISHA WS] Disconnected. Reconnecting in 2s...");
    setTimeout(initWebSocket, 2000);
  };

  ws.onerror = (err) => {
    console.error("[ISHA WS] Error:", err);
  };
}

function handleStreamEvent(msg) {
  if (msg.type === "retrieval_meta") {
    renderRetrievalTrace(msg.data);
  } else if (msg.type === "token") {
    accumulatedMarkdown += msg.token;
    if (currentStreamBubble) {
      currentStreamBubble.innerHTML = marked.parse(accumulatedMarkdown);
      scrollChatToBottom();
    }
  } else if (msg.type === "complete") {
    isGenerating = false;
    updateSendBtnState(false);
    if (currentStreamBubble) {
      appendTTSButton(currentStreamBubble, accumulatedMarkdown);
    }
    chatHistory.push({ role: "assistant", content: accumulatedMarkdown });
  } else if (msg.type === "error") {
    isGenerating = false;
    updateSendBtnState(false);
    if (currentStreamBubble) {
      currentStreamBubble.innerHTML += `<p style="color: var(--accent-rose); margin-top: 8px;"><strong>Error:</strong> ${msg.error}</p>`;
    }
  }
}

/* ==========================================================================
   CHAT CANVAS & PROMPT DOCK
   ========================================================================== */
function initPromptDock() {
  const promptInput = document.getElementById("promptInput");
  const sendBtn = document.getElementById("sendBtn");
  const onlineRagToggle = document.getElementById("onlineRagToggle");
  const packSelector = document.getElementById("packSelector");

  // Auto-resize textarea
  promptInput.addEventListener("input", () => {
    promptInput.style.height = "auto";
    promptInput.style.height = Math.min(promptInput.scrollHeight, 120) + "px";
  });

  promptInput.addEventListener("keydown", (e) => {
    if (e.key === "Enter" && !e.shiftKey) {
      e.preventDefault();
      dispatchQuery();
    }
  });

  sendBtn.addEventListener("click", () => {
    dispatchQuery();
  });

  // Online Live RAG Toggle
  onlineRagToggle.addEventListener("click", () => {
    isOnlineRagActive = !isOnlineRagActive;
    updateOnlineRagUI();
  });

  // Pack selector
  packSelector.addEventListener("change", (e) => {
    activePack = e.target.value;
  });

  // Attach Image Button in Prompt Dock
  document.getElementById("attachImageBtn").addEventListener("click", () => {
    document.getElementById("imageFileInput").click();
  });

  document.getElementById("imageFileInput").addEventListener("change", (e) => {
    const file = e.target.files[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = (evt) => {
        switchTab("visionTab");
        loadVisionImage(evt.target.result, file.name);
      };
      reader.readAsDataURL(file);
    }
  });

  // Voice Microphone button
  initVoiceSTT();
}

function updateOnlineRagUI() {
  const toggle = document.getElementById("onlineRagToggle");
  const toggleDot = document.getElementById("ragToggleDot");
  const toggleText = document.getElementById("ragToggleText");
  const modeDot = document.getElementById("modeDot");
  const modeText = document.getElementById("modeText");

  if (isOnlineRagActive) {
    toggle.classList.add("active");
    toggleDot.className = "pill-dot cyan";
    toggleText.textContent = "🌐 Online Live RAG: ON";
    modeDot.className = "pill-dot cyan";
    modeText.textContent = "Online Live Mode (Live RAG)";
  } else {
    toggle.classList.remove("active");
    toggleDot.className = "pill-dot";
    toggleText.textContent = "🌐 Online Live RAG: OFF";
    modeDot.className = "pill-dot saffron";
    modeText.textContent = "Device Offline Mode";
  }
}

function initQuickChips() {
  const chips = document.querySelectorAll(".quick-chip");
  chips.forEach(chip => {
    chip.addEventListener("click", () => {
      const query = chip.getAttribute("data-query");
      const requiresOnline = chip.getAttribute("data-online") === "true";
      if (requiresOnline && !isOnlineRagActive) {
        isOnlineRagActive = true;
        updateOnlineRagUI();
      }
      document.getElementById("promptInput").value = query;
      dispatchQuery();
    });
  });
}

function dispatchQuery() {
  if (isGenerating) return;

  const promptInput = document.getElementById("promptInput");
  const query = promptInput.value.trim();
  if (!query) return;

  promptInput.value = "";
  promptInput.style.height = "28px";

  // Append user message
  appendUserMessage(query);
  chatHistory.push({ role: "user", content: query });

  // Prepare Assistant message container
  prepareAssistantStreamBubble();

  // Send over WebSocket or fallback to HTTP
  const payload = {
    query: query,
    pack_id: activePack,
    use_online_rag: isOnlineRagActive,
    history: chatHistory.slice(-4)
  };

  if (ws && ws.readyState === WebSocket.OPEN) {
    isGenerating = true;
    updateSendBtnState(true);
    ws.send(JSON.stringify(payload));
  } else {
    console.warn("[ISHA] WebSocket not connected; using REST fallback");
    fallbackHttpQuery(payload);
  }
}

function appendUserMessage(text) {
  const timeline = document.getElementById("chatTimeline");
  const msgEl = document.createElement("div");
  msgEl.className = "chat-message user";
  msgEl.innerHTML = `
    <div class="avatar-badge">U</div>
    <div class="message-bubble">${escapeHtml(text)}</div>
  `;
  timeline.appendChild(msgEl);
  scrollChatToBottom();
}

function prepareAssistantStreamBubble() {
  const timeline = document.getElementById("chatTimeline");
  const msgEl = document.createElement("div");
  msgEl.className = "chat-message assistant";
  msgEl.innerHTML = `
    <div class="avatar-badge">I</div>
    <div class="message-bubble">
      <div class="stream-content" style="min-height: 20px;">
        <span style="color: var(--text-muted); font-size: 0.85rem;">Thinking & retrieving grounded context...</span>
      </div>
      <div class="grounding-trace" style="display: none;">
        <div class="trace-header">
          <span>🔍 Grounding Context & Citations</span>
          <span class="trace-toggle-arrow">▼</span>
        </div>
        <div class="trace-content" style="display: none;"></div>
      </div>
    </div>
  `;
  timeline.appendChild(msgEl);

  currentStreamBubble = msgEl.querySelector(".stream-content");
  currentTraceContent = msgEl.querySelector(".grounding-trace");
  accumulatedMarkdown = "";
  scrollChatToBottom();
}

function renderRetrievalTrace(data) {
  if (!currentTraceContent) return;

  const traceBox = currentTraceContent;
  const traceBody = traceBox.querySelector(".trace-content");
  traceBox.style.display = "block";

  let citationsHtml = "";
  (data.citations || []).forEach(c => {
    const isWeb = c.type === "web";
    const badgeColor = isWeb ? "var(--accent-cyan)" : "var(--accent-saffron)";
    citationsHtml += `
      <div style="background: rgba(255,255,255,0.02); border: 1px solid var(--border-subtle); border-radius: var(--radius-sm); padding: 8px 12px; margin-top: 4px;">
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 4px;">
          <strong style="color: ${badgeColor}; font-size: 0.8rem;">[${c.citation_id}] ${escapeHtml(c.title)}</strong>
          <span style="font-size: 0.72rem; color: var(--text-muted);">Relevance: ${Math.round(c.score * 100)}%</span>
        </div>
        <p style="font-size: 0.75rem; color: var(--text-secondary);">${escapeHtml(c.snippet)}</p>
        ${c.url ? `<a href="${c.url}" target="_blank" style="color: var(--accent-cyan); font-size: 0.72rem; text-decoration: none; margin-top: 4px; display: inline-block;">Open Source URL ↗</a>` : ""}
      </div>
    `;
  });

  traceBody.innerHTML = `
    <div style="font-size: 0.75rem; color: var(--text-muted); margin-bottom: 6px;">
      ⚡ Retrieval Latency: <strong>${data.retrieval_time_ms} ms</strong> | Local Chunks: <strong>${data.local_matches}</strong> | Web Sources: <strong>${data.web_matches}</strong>
    </div>
    ${citationsHtml || "<p style='font-size: 0.75rem;'>No external grounding citations required for this query.</p>"}
  `;

  // Toggle trace collapse
  const header = traceBox.querySelector(".trace-header");
  header.addEventListener("click", () => {
    const isVisible = traceBody.style.display !== "none";
    traceBody.style.display = isVisible ? "none" : "flex";
    traceBox.querySelector(".trace-toggle-arrow").textContent = isVisible ? "▼" : "▲";
  });
}

function appendTTSButton(bubbleEl, textToSpeak) {
  const ttsBtn = document.createElement("button");
  ttsBtn.className = "citation-chip";
  ttsBtn.style.marginTop = "12px";
  ttsBtn.style.display = "inline-flex";
  ttsBtn.innerHTML = `🔊 Listen to ISHA`;
  ttsBtn.addEventListener("click", () => {
    playSpeech(textToSpeak);
  });
  bubbleEl.appendChild(ttsBtn);
}

function playSpeech(text) {
  // Strip markdown formatting for cleaner speech synthesis
  const cleanText = text.replace(/[#*`_\[\]()]/g, " ").substring(0, 400);

  if ('speechSynthesis' in window) {
    window.speechSynthesis.cancel();
    const utterance = new SpeechSynthesisUtterance(cleanText);
    utterance.rate = 1.05;
    utterance.pitch = 1.0;
    window.speechSynthesis.speak(utterance);
    startAudioWaveAnimation();
    utterance.onend = () => stopAudioWaveAnimation();
  }
}

function updateSendBtnState(loading) {
  const sendBtn = document.getElementById("sendBtn");
  if (sendBtn) {
    sendBtn.disabled = loading;
    sendBtn.innerHTML = loading ? "⏳" : "➤";
  }
}

function scrollChatToBottom() {
  const timeline = document.getElementById("chatTimeline");
  timeline.scrollTop = timeline.scrollHeight;
}

function escapeHtml(str) {
  if (!str) return "";
  return str.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

/* ==========================================================================
   VOICE STT & AUDIO WAVE CANVAS
   ========================================================================== */
function initVoiceSTT() {
  const micBtn = document.getElementById("voiceMicBtn");
  const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;

  if (!SpeechRecognition) {
    micBtn.title = "Web Speech API not supported in this browser";
    return;
  }

  const recognition = new SpeechRecognition();
  recognition.continuous = false;
  recognition.interimResults = true;
  recognition.lang = "en-IN";

  let isListening = false;

  micBtn.addEventListener("click", () => {
    if (!isListening) {
      try {
        recognition.start();
        isListening = true;
        micBtn.style.background = "var(--accent-rose)";
        micBtn.style.color = "#fff";
        startAudioWaveAnimation();
      } catch (err) {
        console.error("Speech recognition error:", err);
      }
    } else {
      recognition.stop();
      isListening = false;
      micBtn.style.background = "var(--bg-card)";
      micBtn.style.color = "var(--text-secondary)";
      stopAudioWaveAnimation();
    }
  });

  recognition.onresult = (event) => {
    let transcript = "";
    for (let i = event.resultIndex; i < event.results.length; ++i) {
      transcript += event.results[i][0].transcript;
    }
    document.getElementById("promptInput").value = transcript;
  };

  recognition.onend = () => {
    isListening = false;
    micBtn.style.background = "var(--bg-card)";
    micBtn.style.color = "var(--text-secondary)";
    stopAudioWaveAnimation();
  };
}

function startAudioWaveAnimation() {
  const canvas = document.getElementById("audioWaveCanvas");
  if (!canvas) return;
  const ctx = canvas.getContext("2d");
  let phase = 0;

  function renderWave() {
    ctx.clearRect(0, 0, canvas.width, canvas.height);
    ctx.beginPath();
    ctx.strokeStyle = "#06b6d4";
    ctx.lineWidth = 2;

    for (let x = 0; x < canvas.width; x++) {
      const y = canvas.height / 2 + Math.sin(x * 0.15 + phase) * (canvas.height / 3);
      if (x === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
    phase += 0.2;
    animationFrameId = requestAnimationFrame(renderWave);
  }

  cancelAnimationFrame(animationFrameId);
  renderWave();
}

function stopAudioWaveAnimation() {
  cancelAnimationFrame(animationFrameId);
  const canvas = document.getElementById("audioWaveCanvas");
  if (canvas) {
    const ctx = canvas.getContext("2d");
    ctx.clearRect(0, 0, canvas.width, canvas.height);
  }
}

/* ==========================================================================
   KNOWLEDGE BASE STUDIO
   ========================================================================== */
function initKnowledgeBase() {
  loadPacks();
  loadAllChunks();

  const dropzone = document.getElementById("kbDropzone");
  const fileInput = document.getElementById("kbFileInput");

  dropzone.addEventListener("dragover", (e) => {
    e.preventDefault();
    dropzone.classList.add("dragover");
  });

  dropzone.addEventListener("dragleave", () => {
    dropzone.classList.remove("dragover");
  });

  dropzone.addEventListener("drop", (e) => {
    e.preventDefault();
    dropzone.classList.remove("dragover");
    if (e.dataTransfer.files && e.dataTransfer.files[0]) {
      handleFileUpload(e.dataTransfer.files[0]);
    }
  });

  fileInput.addEventListener("change", (e) => {
    if (e.target.files && e.target.files[0]) {
      handleFileUpload(e.target.files[0]);
    }
  });

  document.getElementById("chunkSearchInput").addEventListener("input", (e) => {
    filterChunksTable(e.target.value);
  });
}

async function loadPacks() {
  try {
    const res = await fetch("/api/packs");
    const packs = await res.json();
    renderPacksList(packs);

    let totalChunks = packs.reduce((acc, p) => acc + p.chunks_count, 0);
    document.getElementById("totalChunksDisplay").textContent = `${totalChunks} Indexed Chunks`;
  } catch (err) {
    console.error("Failed to load packs:", err);
  }
}

function renderPacksList(packs) {
  const container = document.getElementById("packsListContainer");
  container.innerHTML = "";

  packs.forEach(pack => {
    const div = document.createElement("div");
    div.className = "stitch-card";
    div.style.padding = "12px 16px";
    div.style.flexDirection = "row";
    div.style.alignItems = "center";
    div.style.justifyContent = "space-between";

    div.innerHTML = `
      <div style="display: flex; align-items: center; gap: 12px;">
        <span style="font-size: 1.5rem;">${pack.icon}</span>
        <div>
          <strong style="font-size: 0.9rem;">${escapeHtml(pack.title)}</strong>
          <p style="font-size: 0.74rem; color: var(--text-muted);">${escapeHtml(pack.description)}</p>
        </div>
      </div>
      <div class="stitch-pill">
        <span>${pack.chunks_count} chunks</span>
      </div>
    `;
    container.appendChild(div);
  });
}

async function handleFileUpload(file) {
  const progressBox = document.getElementById("ingestProgressBox");
  const progressBar = document.getElementById("ingestProgressBar");
  const statusMsg = document.getElementById("ingestStatusMsg");
  const pctText = document.getElementById("ingestPct");

  progressBox.style.display = "block";
  statusMsg.textContent = `Ingesting ${file.name}...`;
  progressBar.style.width = "40%";
  pctText.textContent = "40%";

  const formData = new FormData();
  formData.append("file", file);
  formData.append("pack_id", "user_library");

  try {
    const res = await fetch("/api/packs/ingest", {
      method: "POST",
      body: formData
    });
    const result = await res.json();

    progressBar.style.width = "100%";
    pctText.textContent = "100%";
    statusMsg.textContent = result.message || "Ingestion complete!";

    setTimeout(() => {
      progressBox.style.display = "none";
      progressBar.style.width = "0%";
      loadPacks();
      loadAllChunks();
    }, 1800);
  } catch (err) {
    statusMsg.textContent = "Ingestion failed: " + err.message;
    progressBar.style.background = "var(--accent-rose)";
  }
}

let cachedChunks = [];
async function loadAllChunks() {
  try {
    const res = await fetch("/api/packs/all/chunks");
    cachedChunks = await res.json();
    renderChunksTable(cachedChunks);
  } catch (err) {
    console.error("Failed to load chunks:", err);
  }
}

function renderChunksTable(chunks) {
  const tbody = document.getElementById("chunksTableBody");
  tbody.innerHTML = "";

  chunks.forEach(c => {
    const tr = document.createElement("tr");
    tr.innerHTML = `
      <td><span class="citation-chip" style="margin: 0;">${escapeHtml(c.chunk_id)}</span></td>
      <td><strong style="color: var(--accent-cyan); font-size: 0.78rem;">${escapeHtml(c.pack_id)}</strong></td>
      <td style="color: var(--text-secondary); line-height: 1.4;">${escapeHtml(c.text)}</td>
    `;
    tbody.appendChild(tr);
  });
}

function filterChunksTable(query) {
  if (!query) {
    renderChunksTable(cachedChunks);
    return;
  }
  const q = query.toLowerCase();
  const filtered = cachedChunks.filter(c => 
    c.text.toLowerCase().includes(q) || c.chunk_id.toLowerCase().includes(q)
  );
  renderChunksTable(filtered);
}

/* ==========================================================================
   ONLINE LIVE RAG & MCP EXPLORER
   ========================================================================== */
function initLiveRagExplorer() {
  const execBtn = document.getElementById("execLiveSearchBtn");
  const searchInput = document.getElementById("liveSearchInput");

  execBtn.addEventListener("click", () => {
    executeLiveRagSearch(searchInput.value.trim());
  });

  searchInput.addEventListener("keydown", (e) => {
    if (e.key === "Enter") {
      executeLiveRagSearch(searchInput.value.trim());
    }
  });
}

async function executeLiveRagSearch(query) {
  if (!query) return;

  const sanitizedDisplay = document.getElementById("sanitizedQueryDisplay");
  const resultsContainer = document.getElementById("liveSearchResultsContainer");

  resultsContainer.innerHTML = `<div style="color: var(--accent-cyan); font-size: 0.85rem;">Scraping live internet and querying Wikipedia...</div>`;

  try {
    const res = await fetch("/api/rag/online-search", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ query: query, max_results: 4 })
    });
    const data = await res.json();

    sanitizedDisplay.textContent = `Sanitized Query: "${data.query}" • PII Checked: Clean`;
    resultsContainer.innerHTML = "";

    if (!data.sources || data.sources.length === 0) {
      resultsContainer.innerHTML = `<div style="color: var(--text-muted); font-size: 0.85rem;">No web sources returned for this query.</div>`;
      return;
    }

    data.sources.forEach((src, idx) => {
      const card = document.createElement("div");
      card.className = "stitch-card";
      card.style.padding = "16px";
      card.innerHTML = `
        <div style="display: flex; justify-content: space-between; align-items: center;">
          <strong style="color: var(--accent-cyan); font-size: 0.95rem;">[W${idx + 1}] ${escapeHtml(src.title)}</strong>
          <span class="stitch-pill">${src.provider}</span>
        </div>
        <p style="font-size: 0.82rem; color: var(--text-secondary); line-height: 1.5; margin-top: 6px;">${escapeHtml(src.snippet)}</p>
        <a href="${src.url}" target="_blank" style="color: var(--accent-saffron); font-size: 0.78rem; text-decoration: none; margin-top: 6px; display: inline-block;">View Source Article ↗</a>
      `;
      resultsContainer.appendChild(card);
    });

  } catch (err) {
    resultsContainer.innerHTML = `<div style="color: var(--accent-rose); font-size: 0.85rem;">Search failed: ${err.message}</div>`;
  }
}

/* ==========================================================================
   VISION & MULTIMODAL STUDIO
   ========================================================================== */
let activeVisionBase64 = "";

function initVisionStudio() {
  const uploadInput = document.getElementById("visionUploadInput");
  const runBtn = document.getElementById("runVisionAnalysisBtn");

  uploadInput.addEventListener("change", (e) => {
    const file = e.target.files[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = (evt) => {
        loadVisionImage(evt.target.result, file.name);
      };
      reader.readAsDataURL(file);
    }
  });

  // Pre-loaded samples
  document.getElementById("loadSampleWheatRustBtn").addEventListener("click", () => {
    loadSamplePlaceholder("Wheat Stripe Rust Foliage", "yellow rust stripe pustules wheat leaf");
  });

  document.getElementById("loadSampleRiceBlastBtn").addEventListener("click", () => {
    loadSamplePlaceholder("Rice Blast Lesion Sample", "rice blast diamond lesions collar rot");
  });

  document.getElementById("loadSamplePrescriptionBtn").addEventListener("click", () => {
    loadSamplePlaceholder("Doctor Medical Prescription", "rx tab paracetamol 500mg syrup doctor prescription");
  });

  runBtn.addEventListener("click", () => {
    runVisionDiagnosis();
  });
}

function loadVisionImage(base64Data, label) {
  activeVisionBase64 = base64Data;
  const img = document.getElementById("visionPreviewImg");
  const prompt = document.getElementById("visionDropPrompt");
  img.src = base64Data;
  img.style.display = "block";
  prompt.style.display = "none";
}

function loadSamplePlaceholder(label, contextHint) {
  // Generate high-resolution colored canvas placeholder
  const canvas = document.createElement("canvas");
  canvas.width = 480;
  canvas.height = 320;
  const ctx = canvas.getContext("2d");
  
  // Gradient fill
  const grad = ctx.createLinearGradient(0, 0, 480, 320);
  grad.addColorStop(0, "#1e293b");
  grad.addColorStop(1, "#0f172a");
  ctx.fillStyle = grad;
  ctx.fillRect(0, 0, 480, 320);

  ctx.fillStyle = "#f59e0b";
  ctx.font = "bold 20px Inter, sans-serif";
  ctx.textAlign = "center";
  ctx.fillText(label, 240, 150);

  ctx.fillStyle = "#94a3b8";
  ctx.font = "14px Inter, sans-serif";
  ctx.fillText("Simulated Multimodal Optical Input", 240, 180);

  const dataUrl = canvas.toDataURL("image/jpeg");
  loadVisionImage(dataUrl, label);
  activeVisionContextHint = contextHint;
}

let activeVisionContextHint = "";
async function runVisionDiagnosis() {
  if (!activeVisionBase64) {
    alert("Please select or drop an image first.");
    return;
  }

  const resultArea = document.getElementById("visionResultArea");
  resultArea.innerHTML = `<div style="color: var(--accent-cyan); font-size: 0.85rem;">Running on-device frame analysis & feature extraction...</div>`;

  try {
    const res = await fetch("/api/vision/analyze", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        image_base64: activeVisionBase64,
        context_hint: activeVisionContextHint
      })
    });
    const data = await res.json();

    resultArea.innerHTML = `
      <div style="background: rgba(16, 185, 129, 0.1); border: 1px solid rgba(16, 185, 129, 0.3); border-radius: var(--radius-sm); padding: 16px;">
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px;">
          <strong style="color: var(--accent-emerald); font-size: 1rem;">${escapeHtml(data.diagnosis)}</strong>
          <span class="stitch-pill" style="border-color: var(--accent-emerald); color: var(--accent-emerald);">Confidence: ${Math.round(data.confidence * 100)}%</span>
        </div>
        <p style="font-size: 0.85rem; color: var(--text-primary); margin-top: 8px; line-height: 1.5;">${escapeHtml(data.remedy)}</p>
      </div>

      <div style="display: flex; gap: 10px; font-size: 0.78rem; color: var(--text-muted);">
        <span>Resolution: ${data.image_metadata.width}x${data.image_metadata.height}</span>
        <span>Format: ${data.image_metadata.format}</span>
        <span>Frame Governor: 5 FPS Cap (Safe)</span>
      </div>
    `;
  } catch (err) {
    resultArea.innerHTML = `<div style="color: var(--accent-rose); font-size: 0.85rem;">Vision analysis failed: ${err.message}</div>`;
  }
}

/* ==========================================================================
   WATCHDOG & HARDWARE TELEMETRY
   ========================================================================== */
function initWatchdog() {
  document.getElementById("tierBtn1").addEventListener("click", () => setTier(1));
  document.getElementById("tierBtn2").addEventListener("click", () => setTier(2));
  document.getElementById("tierBtn3").addEventListener("click", () => setTier(3));

  document.getElementById("thermalBtn1").addEventListener("click", () => setThermal(1));
  document.getElementById("thermalBtn2").addEventListener("click", () => setThermal(2));
  document.getElementById("thermalBtn3").addEventListener("click", () => setThermal(3));
  document.getElementById("thermalBtn4").addEventListener("click", () => setThermal(4));

  document.getElementById("tierSelectorBtn").addEventListener("click", () => switchTab("watchdogTab"));
  document.getElementById("thermalIndicatorBtn").addEventListener("click", () => switchTab("watchdogTab"));
}

async function setTier(tier) {
  try {
    const res = await fetch("/api/telemetry/tier", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ tier })
    });
    const data = await res.json();
    updateTelemetryUI(data);
  } catch (e) {
    console.error("Failed to set tier:", e);
  }
}

async function setThermal(zone) {
  try {
    const res = await fetch("/api/telemetry/thermal", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ zone })
    });
    const data = await res.json();
    updateTelemetryUI(data);
  } catch (e) {
    console.error("Failed to set thermal:", e);
  }
}

function startTelemetryPolling() {
  fetchTelemetry();
  setInterval(fetchTelemetry, 3000);
}

async function fetchTelemetry() {
  try {
    const res = await fetch("/api/telemetry");
    const data = await res.json();
    updateTelemetryUI(data);
  } catch (e) {}
}

function updateTelemetryUI(data) {
  // Header badges
  const headerTier = document.getElementById("headerTierText");
  const headerThermal = document.getElementById("headerThermalText");
  const thermalDot = document.getElementById("thermalDot");
  const headerRam = document.getElementById("headerRamText");

  if (headerTier) headerTier.textContent = `Tier ${data.tier}: ${data.device_class}`;
  if (headerThermal) headerThermal.textContent = data.thermal_status;
  if (headerRam) headerRam.textContent = `${data.process_rss_mb} MB`;

  if (thermalDot) {
    const dotClasses = { 1: "emerald", 2: "saffron", 3: "rose", 4: "rose" };
    thermalDot.className = `pill-dot ${dotClasses[data.thermal_zone] || "emerald"}`;
  }

  // Watchdog tab details
  const watchdogRss = document.getElementById("watchdogRss");
  const watchdogSysRam = document.getElementById("watchdogSysRam");
  const watchdogCpu = document.getElementById("watchdogCpu");
  const watchdogUptime = document.getElementById("watchdogUptime");
  const watchdogKvCache = document.getElementById("watchdogKvCache");
  const watchdogTotalTokens = document.getElementById("watchdogTotalTokens");
  const watchdogThrottlePolicy = document.getElementById("watchdogThrottlePolicy");

  if (watchdogRss) watchdogRss.textContent = `${data.process_rss_mb} MB`;
  if (watchdogSysRam) watchdogSysRam.textContent = `${data.system_ram_pct} %`;
  if (watchdogCpu) watchdogCpu.textContent = `${data.cpu_percent} %`;
  if (watchdogUptime) watchdogUptime.textContent = `${data.uptime_seconds} s`;
  if (watchdogKvCache) watchdogKvCache.textContent = `${data.kv_cache_tokens} / ${data.max_context} Tokens`;
  if (watchdogTotalTokens) watchdogTotalTokens.textContent = data.total_tokens;
  if (watchdogThrottlePolicy) {
    watchdogThrottlePolicy.textContent = data.thermal_throttle;
    watchdogThrottlePolicy.style.color = data.thermal_color;
  }
}
