/**
 * ISHA AI - Google Stitch Client Application
 * Consumer-Facing & Human-Centric Architecture:
 * - Real-time grounded streaming
 * - Instant Persona Switching (Farming, Health, Legal, STEM)
 * - Live Web Search toggle
 * - Document Ingestion & Chat
 * - Voice STT & TTS with Speech Waveform
 * - Multimodal Camera & Vision Analysis
 * - Hidden Slide-Over Developer Diagnostics Drawer
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
let animationFrameId = null;

const packTitles = {
  "all": "✨ All Knowledge Topics",
  "kisan_saathi": "🌾 Kisan Crop & Farm Doctor",
  "swasthya": "🩺 Health & Family First Aid",
  "bharat_core": "⚖️ Citizen Rights & Legal",
  "shiksha": "🎓 Study & STEM Helper",
  "user_library": "📁 My Uploaded Documents"
};

document.addEventListener("DOMContentLoaded", () => {
  initTheme();
  initPWA();
  initTabs();
  initPersonas();
  initWebSocket();
  initPromptDock();
  initHeroCards();
  initDeveloperDrawer();
  initKnowledgeBase();
  initLiveRagExplorer();
  initVisionStudio();
  initWatchdog();
  startTelemetryPolling();
});

/* ==========================================================================
   THEME & PWA
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
  if ('serviceWorker' in navigator) {
    navigator.serviceWorker.register('/sw.js').catch(err => {
      console.warn('[PWA] ServiceWorker:', err);
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
        alert("To install ISHA AI:\n- On Desktop: Click the install icon in the URL bar.\n- On Android: Tap menu (⋮) -> 'Add to Home screen' or 'Install App'.\n- On iPhone/iPad: Tap Share -> 'Add to Home Screen'.");
      }
    });
  }
}

/* ==========================================================================
   NAVIGATION & PERSONA SWITCHING
   ========================================================================== */
function initTabs() {
  const railBtns = document.querySelectorAll(".rail-btn");
  railBtns.forEach(btn => {
    btn.addEventListener("click", () => {
      const tabId = btn.getAttribute("data-tab");
      switchTab(tabId);
    });
  });

  document.getElementById("brandHomeBtn").addEventListener("click", () => {
    switchTab("chatTab");
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

function initPersonas() {
  const pills = document.querySelectorAll("#personaPillsBar .persona-pill");
  pills.forEach(pill => {
    pill.addEventListener("click", () => {
      pills.forEach(p => p.classList.remove("active"));
      pill.classList.add("active");
      activePack = pill.getAttribute("data-pack");
      document.getElementById("activeTopicBadge").textContent = packTitles[activePack] || "✨ Active Topic";
    });
  });
}

/* ==========================================================================
   DEVELOPER SLIDE-OVER DRAWER
   ========================================================================== */
function initDeveloperDrawer() {
  const openBtn = document.getElementById("openDevDrawerBtn");
  const closeBtn = document.getElementById("closeDevDrawerBtn");
  const drawer = document.getElementById("devDrawer");
  const backdrop = document.getElementById("devDrawerBackdrop");

  const openDrawer = () => {
    drawer.classList.add("active");
    backdrop.classList.add("active");
  };

  const closeDrawer = () => {
    drawer.classList.remove("active");
    backdrop.classList.remove("active");
  };

  if (openBtn) openBtn.addEventListener("click", openDrawer);
  if (closeBtn) closeBtn.addEventListener("click", closeDrawer);
  if (backdrop) backdrop.addEventListener("click", closeDrawer);
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
      appendMessageActions(currentStreamBubble.parentElement, accumulatedMarkdown);
    }
    chatHistory.push({ role: "assistant", content: accumulatedMarkdown });
  } else if (msg.type === "error") {
    isGenerating = false;
    updateSendBtnState(false);
    if (currentStreamBubble) {
      currentStreamBubble.innerHTML += `<p style="color: var(--accent-rose); margin-top: 8px;"><strong>Notice:</strong> ${msg.error}</p>`;
    }
  }
}

/* ==========================================================================
   PROMPT DOCK & HERO SCREEN
   ========================================================================== */
function initPromptDock() {
  const promptInput = document.getElementById("promptInput");
  const sendBtn = document.getElementById("sendBtn");
  const headerRagToggle = document.getElementById("onlineRagHeaderToggle");

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

  // Header Live Web Toggle Button
  headerRagToggle.addEventListener("click", () => {
    isOnlineRagActive = !isOnlineRagActive;
    updateOnlineRagUI();
  });

  // Attach Document Button in Prompt Dock
  const attachDocBtn = document.getElementById("attachDocBtn");
  const promptDocInput = document.getElementById("promptDocInput");
  attachDocBtn.addEventListener("click", () => {
    promptDocInput.click();
  });

  promptDocInput.addEventListener("change", async (e) => {
    const file = e.target.files[0];
    if (file) {
      appendUserMessage(`📎 Uploaded document: ${file.name}`);
      await handleFileUpload(file);
      appendAssistantSimpleMessage(`I have read and indexed **${file.name}**. What would you like to know or extract from it?`);
    }
  });

  // Photo / Camera Button in Prompt Dock
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

  initVoiceSTT();
}

function updateOnlineRagUI() {
  const headerBtn = document.getElementById("onlineRagHeaderToggle");
  const dot = document.getElementById("headerRagDot");
  const text = document.getElementById("headerRagText");
  const statusDot = document.getElementById("headerStatusDot");
  const statusText = document.getElementById("headerStatusText");

  if (isOnlineRagActive) {
    headerBtn.classList.add("online");
    dot.className = "pill-dot cyan";
    text.textContent = "🌐 Live Web: ON";
    statusDot.className = "pill-dot cyan";
    statusText.textContent = "Live Internet Active";
  } else {
    headerBtn.classList.remove("online");
    dot.className = "pill-dot";
    text.textContent = "🌐 Live Web: OFF";
    statusDot.className = "pill-dot emerald";
    statusText.textContent = "100% On-Device Private";
  }
}

function initHeroCards() {
  const cards = document.querySelectorAll(".hero-card");
  cards.forEach(card => {
    card.addEventListener("click", () => {
      const query = card.getAttribute("data-query");
      const pack = card.getAttribute("data-pack");
      const online = card.getAttribute("data-online") === "true";

      if (pack) {
        activePack = pack;
        const pill = document.querySelector(`.persona-pill[data-pack="${pack}"]`);
        if (pill) {
          document.querySelectorAll(".persona-pill").forEach(p => p.classList.remove("active"));
          pill.classList.add("active");
        }
        document.getElementById("activeTopicBadge").textContent = packTitles[activePack] || "✨ Active Topic";
      }

      if (online && !isOnlineRagActive) {
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
  promptInput.style.height = "32px";

  // Hide hero screen on first message
  const hero = document.getElementById("heroWelcome");
  if (hero) hero.style.display = "none";

  // Append user message
  appendUserMessage(query);
  chatHistory.push({ role: "user", content: query });

  // Prepare Assistant message container
  prepareAssistantStreamBubble();

  // Auto-detect latest/current queries to ensure Live Web RAG is engaged
  const isLatest = /latest|news|today|current|price|weather|2026|who is|what happened|online|internet/i.test(query);
  const effectiveOnline = isOnlineRagActive || isLatest;
  if (isLatest && !isOnlineRagActive) {
    isOnlineRagActive = true;
    updateOnlineRagUI();
  }

  // Send over WebSocket or fallback to HTTP
  const payload = {
    query: query,
    pack_id: activePack,
    use_online_rag: effectiveOnline,
    history: chatHistory.slice(-4)
  };

  if (ws && ws.readyState === WebSocket.OPEN) {
    isGenerating = true;
    updateSendBtnState(true);
    ws.send(JSON.stringify(payload));
  } else {
    console.warn("[ISHA] Using HTTP fallback");
    fallbackHttpQuery(payload);
  }
}

async function fallbackHttpQuery(payload) {
  isGenerating = true;
  updateSendBtnState(true);
  try {
    const res = await fetch("/api/chat", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload)
    });
    const data = await res.json();
    if (data.retrieval_meta) {
      renderRetrievalTrace(data.retrieval_meta);
    }
    accumulatedMarkdown = data.response || "I could not find an answer.";
    if (currentStreamBubble) {
      currentStreamBubble.innerHTML = marked.parse(accumulatedMarkdown);
      appendMessageActions(currentStreamBubble.parentElement, accumulatedMarkdown);
      scrollChatToBottom();
    }
    chatHistory.push({ role: "assistant", content: accumulatedMarkdown });
  } catch (err) {
    if (currentStreamBubble) {
      currentStreamBubble.innerHTML = `<span style="color:#ef4444">Error: ${escapeHtml(err.message)}</span>`;
    }
  } finally {
    isGenerating = false;
    updateSendBtnState(false);
  }
}

function appendUserMessage(text) {
  const timeline = document.getElementById("chatTimeline");
  const msgEl = document.createElement("div");
  msgEl.className = "chat-message user";
  msgEl.innerHTML = `
    <div class="avatar-badge">👤</div>
    <div class="message-bubble">${escapeHtml(text)}</div>
  `;
  timeline.appendChild(msgEl);
  scrollChatToBottom();
}

function appendAssistantSimpleMessage(text) {
  const timeline = document.getElementById("chatTimeline");
  const msgEl = document.createElement("div");
  msgEl.className = "chat-message assistant";
  msgEl.innerHTML = `
    <div class="avatar-badge">
      <img src="/app_dp.png" alt="ISHA">
    </div>
    <div class="message-bubble">${marked.parse(text)}</div>
  `;
  timeline.appendChild(msgEl);
  scrollChatToBottom();
}

function prepareAssistantStreamBubble() {
  const timeline = document.getElementById("chatTimeline");
  const msgEl = document.createElement("div");
  msgEl.className = "chat-message assistant";
  msgEl.innerHTML = `
    <div class="avatar-badge">
      <img src="/app_dp.png" alt="ISHA">
    </div>
    <div class="message-bubble">
      <div class="stream-content" style="min-height: 20px;">
        <span style="color: var(--text-muted); font-size: 0.9rem;">Consulting verified knowledge...</span>
      </div>
      <div class="grounding-trace" style="display: none;">
        <div class="trace-header">
          <span>🌿 Sources & Grounded Evidence</span>
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

  let imageHtml = "";
  if (data.image_url) {
    imageHtml = `
      <div style="margin-top: 8px; margin-bottom: 8px; border-radius: var(--radius-sm); overflow: hidden; border: 1px solid var(--border-subtle); background: rgba(0,0,0,0.25);">
        <img src="${escapeHtml(data.image_url)}" alt="Live Entity Image" style="width: 100%; max-height: 220px; object-fit: cover; display: block;" onerror="this.parentElement.style.display='none';" />
        <div style="padding: 4px 8px; font-size: 0.72rem; color: var(--accent-cyan); background: rgba(255,255,255,0.02);">🌐 Live Web Verified Visual</div>
      </div>
    `;
  }

  let citationsHtml = "";
  (data.citations || []).forEach(c => {
    const isWeb = c.type === "web";
    const badgeColor = isWeb ? "var(--accent-cyan)" : "var(--accent-saffron)";
    const icon = isWeb ? "🌐 Google / Web" : "🌿 Verified Knowledge";
    citationsHtml += `
      <div style="background: rgba(255,255,255,0.02); border: 1px solid var(--border-subtle); border-radius: var(--radius-sm); padding: 8px 12px; margin-top: 4px;">
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 4px;">
          <strong style="color: ${badgeColor}; font-size: 0.8rem;">[${c.citation_id}] ${escapeHtml(c.title)}</strong>
          <span style="font-size: 0.72rem; color: var(--text-muted);">${icon}</span>
        </div>
        <p style="font-size: 0.78rem; color: var(--text-secondary);">${escapeHtml(c.snippet)}</p>
        ${c.url ? `<a href="${c.url}" target="_blank" rel="noopener noreferrer" style="color: var(--accent-cyan); font-size: 0.74rem; text-decoration: none; margin-top: 4px; display: inline-block;">Open Direct Link ↗</a>` : ""}
      </div>
    `;
  });

  traceBody.innerHTML = `
    <div style="font-size: 0.76rem; color: var(--text-muted); margin-bottom: 6px;">
      Grounded in <strong>${data.local_matches}</strong> offline facts • <strong>${data.web_matches}</strong> web results
    </div>
    ${imageHtml}
    ${citationsHtml || "<p style='font-size: 0.78rem;'>Answered directly from foundational understanding.</p>"}
  `;

  const header = traceBox.querySelector(".trace-header");
  header.addEventListener("click", () => {
    const isVisible = traceBody.style.display !== "none";
    traceBody.style.display = isVisible ? "none" : "flex";
    traceBox.querySelector(".trace-toggle-arrow").textContent = isVisible ? "▼" : "▲";
  });
}

function appendMessageActions(bubbleEl, text) {
  const actionsDiv = document.createElement("div");
  actionsDiv.className = "message-actions";

  // TTS Listen button
  const listenBtn = document.createElement("button");
  listenBtn.className = "action-chip-btn";
  listenBtn.innerHTML = `🔊 Listen`;
  listenBtn.addEventListener("click", () => playSpeech(text));

  // Copy button
  const copyBtn = document.createElement("button");
  copyBtn.className = "action-chip-btn";
  copyBtn.innerHTML = `📋 Copy`;
  copyBtn.addEventListener("click", () => {
    navigator.clipboard.writeText(text);
    copyBtn.innerHTML = `✓ Copied`;
    setTimeout(() => { copyBtn.innerHTML = `📋 Copy`; }, 2000);
  });

  actionsDiv.appendChild(listenBtn);
  actionsDiv.appendChild(copyBtn);
  bubbleEl.appendChild(actionsDiv);
}

function playSpeech(text) {
  const cleanText = text.replace(/[#*`_\[\]()]/g, " ").substring(0, 450);
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
      const y = canvas.height / 2 + Math.sin(x * 0.18 + phase) * (canvas.height / 3);
      if (x === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
    phase += 0.25;
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
    document.getElementById("totalChunksDisplay").textContent = `${totalChunks} Knowledge Points`;
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
    div.style.padding = "14px 18px";
    div.style.flexDirection = "row";
    div.style.alignItems = "center";
    div.style.justifyContent = "space-between";

    div.innerHTML = `
      <div style="display: flex; align-items: center; gap: 12px;">
        <span style="font-size: 1.6rem;">${pack.icon}</span>
        <div>
          <strong style="font-size: 0.92rem;">${escapeHtml(pack.title)}</strong>
          <p style="font-size: 0.78rem; color: var(--text-muted);">${escapeHtml(pack.description)}</p>
        </div>
      </div>
      <div class="stitch-pill">
        <span>${pack.chunks_count} facts</span>
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

  if (progressBox) {
    progressBox.style.display = "block";
    statusMsg.textContent = `Reading ${file.name}...`;
    progressBar.style.width = "40%";
    pctText.textContent = "40%";
  }

  const formData = new FormData();
  formData.append("file", file);
  formData.append("pack_id", "user_library");

  try {
    const res = await fetch("/api/packs/ingest", {
      method: "POST",
      body: formData
    });
    const result = await res.json();

    if (progressBox) {
      progressBar.style.width = "100%";
      pctText.textContent = "100%";
      statusMsg.textContent = result.message || "Document indexed!";
      setTimeout(() => {
        progressBox.style.display = "none";
        progressBar.style.width = "0%";
      }, 1800);
    }

    loadPacks();
    loadAllChunks();
    return result;
  } catch (err) {
    if (statusMsg) statusMsg.textContent = "Upload error: " + err.message;
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
      <td><strong style="color: var(--accent-cyan); font-size: 0.8rem;">${escapeHtml(c.pack_id)}</strong></td>
      <td style="color: var(--text-secondary); line-height: 1.5;">${escapeHtml(c.text)}</td>
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
   ONLINE LIVE RAG
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

  resultsContainer.innerHTML = `<div style="color: var(--accent-cyan); font-size: 0.88rem;">Crawling live internet articles & Wikipedia...</div>`;

  try {
    const res = await fetch("/api/rag/online-search", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ query: query, max_results: 4 })
    });
    const data = await res.json();

    sanitizedDisplay.textContent = `Sanitized Query: "${data.query}" • Personal data scrubbed: Clean`;
    resultsContainer.innerHTML = "";

    if (!data.sources || data.sources.length === 0) {
      resultsContainer.innerHTML = `<div style="color: var(--text-muted); font-size: 0.88rem;">No web sources returned for this query.</div>`;
      return;
    }

    data.sources.forEach((src, idx) => {
      const card = document.createElement("div");
      card.className = "stitch-card";
      card.style.padding = "16px";
      card.innerHTML = `
        <div style="display: flex; justify-content: space-between; align-items: center;">
          <strong style="color: var(--accent-cyan); font-size: 0.98rem;">${escapeHtml(src.title)}</strong>
          <span class="stitch-pill">${src.provider}</span>
        </div>
        <p style="font-size: 0.84rem; color: var(--text-secondary); line-height: 1.5; margin-top: 6px;">${escapeHtml(src.snippet)}</p>
        <a href="${src.url}" target="_blank" style="color: var(--accent-saffron); font-size: 0.8rem; text-decoration: none; margin-top: 6px; display: inline-block;">Open Web Article ↗</a>
      `;
      resultsContainer.appendChild(card);
    });

  } catch (err) {
    resultsContainer.innerHTML = `<div style="color: var(--accent-rose); font-size: 0.88rem;">Search failed: ${err.message}</div>`;
  }
}

/* ==========================================================================
   VISION & MULTIMODAL STUDIO
   ========================================================================== */
let activeVisionBase64 = "";
let activeVisionContextHint = "";

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
  const canvas = document.createElement("canvas");
  canvas.width = 480;
  canvas.height = 320;
  const ctx = canvas.getContext("2d");
  
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
  ctx.fillText("Optical Diagnostic Sample", 240, 180);

  const dataUrl = canvas.toDataURL("image/jpeg");
  loadVisionImage(dataUrl, label);
  activeVisionContextHint = contextHint;
}

async function runVisionDiagnosis() {
  if (!activeVisionBase64) {
    alert("Please select or drop an image first.");
    return;
  }

  const resultArea = document.getElementById("visionResultArea");
  resultArea.innerHTML = `<div style="color: var(--accent-cyan); font-size: 0.88rem;">Analyzing visual symptoms and optical features...</div>`;

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
      <div style="background: rgba(16, 185, 129, 0.12); border: 1px solid rgba(16, 185, 129, 0.35); border-radius: var(--radius-sm); padding: 18px;">
        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px;">
          <strong style="color: var(--accent-emerald); font-size: 1.05rem;">${escapeHtml(data.diagnosis)}</strong>
          <span class="stitch-pill" style="border-color: var(--accent-emerald); color: var(--accent-emerald);">Confidence: ${Math.round(data.confidence * 100)}%</span>
        </div>
        <p style="font-size: 0.9rem; color: var(--text-primary); margin-top: 8px; line-height: 1.6;">${escapeHtml(data.remedy)}</p>
      </div>

      <div style="display: flex; gap: 10px; font-size: 0.8rem; color: var(--text-muted);">
        <span>Resolution: ${data.image_metadata.width}x${data.image_metadata.height}</span>
        <span>Format: ${data.image_metadata.format}</span>
        <span>Inspection: Instant On-Device</span>
      </div>
    `;
  } catch (err) {
    resultArea.innerHTML = `<div style="color: var(--accent-rose); font-size: 0.88rem;">Diagnosis error: ${err.message}</div>`;
  }
}

/* ==========================================================================
   WATCHDOG & HARDWARE TELEMETRY (SLIDE OVER DRAWER)
   ========================================================================== */
function initWatchdog() {
  document.getElementById("tierBtn1").addEventListener("click", () => setTier(1));
  document.getElementById("tierBtn2").addEventListener("click", () => setTier(2));
  document.getElementById("tierBtn3").addEventListener("click", () => setTier(3));

  document.getElementById("thermalBtn1").addEventListener("click", () => setThermal(1));
  document.getElementById("thermalBtn2").addEventListener("click", () => setThermal(2));
  document.getElementById("thermalBtn3").addEventListener("click", () => setThermal(3));
  document.getElementById("thermalBtn4").addEventListener("click", () => setThermal(4));
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
  } catch (e) {}
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
  } catch (e) {}
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
  const watchdogRss = document.getElementById("watchdogRss");
  const watchdogSysRam = document.getElementById("watchdogSysRam");
  const watchdogCpu = document.getElementById("watchdogCpu");
  const watchdogKvCache = document.getElementById("watchdogKvCache");
  const watchdogTotalTokens = document.getElementById("watchdogTotalTokens");

  if (watchdogRss) watchdogRss.textContent = `${data.process_rss_mb} MB`;
  if (watchdogSysRam) watchdogSysRam.textContent = `${data.system_ram_pct} %`;
  if (watchdogCpu) watchdogCpu.textContent = `${data.cpu_percent} %`;
  if (watchdogKvCache) watchdogKvCache.textContent = `${data.kv_cache_tokens} / ${data.max_context}`;
  if (watchdogTotalTokens) watchdogTotalTokens.textContent = data.total_tokens;
}
