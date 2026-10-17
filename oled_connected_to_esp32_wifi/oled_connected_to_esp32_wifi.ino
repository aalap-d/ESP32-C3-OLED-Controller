
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>

// ---------------- ESP32-C3 PINS ----------------
#define OLED_SDA 6
#define OLED_SCL 7

// ---------------- WIFI SETTINGS -----------------
const char* AP_SSID = "C3-OLED";
const char* AP_PASSWORD = "oled12345";

// ---------------- OLED --------------------------
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
  U8G2_R0,
  U8X8_PIN_NONE
);

WebServer server(80);
uint8_t frameBuffer[1024];

// ---------------- WEBPAGE -----------------------
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1">
<title>ESP32 OLED Video Player</title>

<style>
* { box-sizing: border-box; }

body {
  margin: 0;
  padding: 18px;
  background: #10131a;
  color: #f1f5f9;
  font-family: Arial, sans-serif;
  text-align: center;
}

h2 {
  margin-top: 8px;
  color: #60a5fa;
}

.panel {
  max-width: 480px;
  margin: auto;
  padding: 16px;
  background: #1c2230;
  border-radius: 14px;
}

input[type=file] {
  display: block;
  width: 100%;
  margin: 16px 0;
  padding: 10px;
  background: #293244;
  color: white;
  border-radius: 8px;
}

video {
  display: block;
  width: 100%;
  max-height: 260px;
  background: black;
  border-radius: 8px;
  margin: 12px auto;
}

.buttons {
  display: flex;
  gap: 10px;
  margin-top: 14px;
}

button {
  flex: 1;
  padding: 13px 8px;
  border: none;
  border-radius: 8px;
  font-size: 16px;
  font-weight: bold;
  cursor: pointer;
  touch-action: manipulation;
}

#playBtn {
  background: #22c55e;
  color: #07130a;
}

#stopBtn {
  background: #ef4444;
  color: white;
}

#clearBtn {
  background: #475569;
  color: white;
  margin-top: 10px;
  width: 100%;
}

#status {
  margin-top: 14px;
  padding: 12px;
  background: #111827;
  border-radius: 8px;
  text-align: left;
  font-size: 14px;
  line-height: 1.5;
  overflow-wrap: anywhere;
}

.note {
  color: #cbd5e1;
  font-size: 13px;
  line-height: 1.5;
}

.section {
  margin-top: 18px;
  padding-top: 14px;
  border-top: 1px solid #374151;
  text-align: left;
}

textarea {
  display: block;
  width: 100%;
  resize: vertical;
  padding: 12px;
  border: 0;
  border-radius: 8px;
  background: #293244;
  color: white;
  font: 16px Arial, sans-serif;
}

#drawingCanvas {
  display: block;
  width: 100%;
  max-width: 384px;
  height: auto;
  aspect-ratio: 2 / 1;
  margin: 12px auto;
  background: #000;
  border: 2px solid #64748b;
  border-radius: 8px;
  touch-action: none;
  cursor: crosshair;
}
</style>
</head>

<body>
<div class="panel">

  <h2>ESP32 OLED Video Player</h2>

  <p class="note">
    Select a video, wait for it to load, and press Play.
  </p>

  <input id="fileInput" type="file" accept="video/*">

  <video id="video"
         playsinline
         webkit-playsinline
         muted
         loop
         preload="metadata"></video>

  <div class="buttons">
    <button id="playBtn" type="button">▶ Play</button>
    <button id="stopBtn" type="button">■ Stop</button>
  </div>

  <button id="clearBtn" type="button">Clear OLED</button>

  <div id="status">Select a video to begin.</div>

  <p class="note">
    The OLED displays a black-and-white version of your video.
  </p>

  <!-- TEXT DISPLAY SECTION -->
  <div class="section">
    <h2 style="text-align:center;">Display Text</h2>

    <p class="note">Type a message to display on the OLED.</p>

    <textarea id="oledText"
              rows="3"
              maxlength="300"
              placeholder="Type text here..."></textarea>

    <div class="buttons">
      <button id="displayTextBtn"
              style="background:#60a5fa;color:#08111f;"
              type="button">Display Text</button>

      <button id="clearTextBtn"
              style="background:#475569;color:white;"
              type="button">Clear Text</button>
    </div>
  </div>

  <!-- DRAWING SECTION -->
  <div class="section">
    <h2 style="text-align:center;">Draw on OLED</h2>

    <p class="note">
      Draw with your finger or mouse, then send it to the OLED.
    </p>

    <canvas id="drawingCanvas" width="128" height="64"></canvas>

    <div class="buttons">
      <button id="displayDrawingBtn"
              style="background:#60a5fa;color:#08111f;"
              type="button">Display Drawing</button>

      <button id="clearDrawingBtn"
              style="background:#475569;color:white;"
              type="button">Clear Drawing</button>
    </div>

    <p class="note">
      Drawings appear in black and white. Stop video streaming first
      if you want text or drawings to remain visible.
    </p>
  </div>

</div>

<script>
"use strict";

// ---------------- ELEMENTS ----------------
const fileInput = document.getElementById("fileInput");
const video = document.getElementById("video");
const playBtn = document.getElementById("playBtn");
const stopBtn = document.getElementById("stopBtn");
const clearBtn = document.getElementById("clearBtn");
const statusBox = document.getElementById("status");

const oledText = document.getElementById("oledText");
const displayTextBtn = document.getElementById("displayTextBtn");
const clearTextBtn = document.getElementById("clearTextBtn");

const drawingCanvas = document.getElementById("drawingCanvas");
const drawingCtx = drawingCanvas.getContext("2d");
const displayDrawingBtn = document.getElementById("displayDrawingBtn");
const clearDrawingBtn = document.getElementById("clearDrawingBtn");

// Canvas used for video frames
const canvas = document.createElement("canvas");
canvas.width = 128;
canvas.height = 64;

const ctx = canvas.getContext("2d", {
  alpha: false,
  willReadFrequently: true
});

let selectedURL = null;
let playing = false;
let session = 0;
let sending = false;

function setStatus(message) {
  statusBox.textContent = message;
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// ---------------- DRAWING CANVAS ----------------
drawingCtx.fillStyle = "#000000";
drawingCtx.fillRect(0, 0, 128, 64);

drawingCtx.strokeStyle = "#ffffff";
drawingCtx.lineWidth = 2;
drawingCtx.lineCap = "round";
drawingCtx.lineJoin = "round";

let drawingActive = false;

function drawingPosition(event) {
  const rect = drawingCanvas.getBoundingClientRect();

  return {
    x: (event.clientX - rect.left) * 128 / rect.width,
    y: (event.clientY - rect.top) * 64 / rect.height
  };
}

drawingCanvas.addEventListener("pointerdown", function(event) {
  event.preventDefault();
  drawingActive = true;

  drawingCanvas.setPointerCapture(event.pointerId);

  const pos = drawingPosition(event);

  drawingCtx.beginPath();
  drawingCtx.moveTo(pos.x, pos.y);
  drawingCtx.lineTo(pos.x + 0.1, pos.y + 0.1);
  drawingCtx.stroke();
});

drawingCanvas.addEventListener("pointermove", function(event) {
  if (!drawingActive) return;

  event.preventDefault();

  const pos = drawingPosition(event);
  drawingCtx.lineTo(pos.x, pos.y);
  drawingCtx.stroke();
});

function stopDrawing() {
  drawingActive = false;
  drawingCtx.closePath();
}

drawingCanvas.addEventListener("pointerup", stopDrawing);
drawingCanvas.addEventListener("pointercancel", stopDrawing);
drawingCanvas.addEventListener("lostpointercapture", stopDrawing);

clearDrawingBtn.addEventListener("click", function() {
  drawingCtx.fillStyle = "#000000";
  drawingCtx.fillRect(0, 0, 128, 64);
});

displayDrawingBtn.addEventListener("click", async function() {
  try {
    const image = drawingCtx.getImageData(0, 0, 128, 64);
    const pixels = image.data;
    const bytes = new Uint8Array(1024);

    for (let y = 0; y < 64; y++) {
      for (let x = 0; x < 128; x++) {
        const p = (y * 128 + x) * 4;

        const gray =
          (pixels[p] * 299 +
           pixels[p + 1] * 587 +
           pixels[p + 2] * 114) / 1000;

        if (gray >= 128) {
          const index = y * 16 + Math.floor(x / 8);
          bytes[index] |= (1 << (x % 8));
        }
      }
    }

    let binary = "";

    for (let i = 0; i < bytes.length; i++) {
      binary += String.fromCharCode(bytes[i]);
    }

    const response = await fetch("/frame", {
      method: "POST",
      headers: {"Content-Type": "text/plain"},
      body: btoa(binary),
      cache: "no-store"
    });

    const result = await response.text();

    if (!response.ok) {
      throw new Error(result || ("HTTP " + response.status));
    }

    setStatus("Drawing sent to OLED.");
  } catch (err) {
    setStatus("Could not send drawing: " + err.message);
  }
});

// ---------------- VIDEO SELECTION ----------------
fileInput.addEventListener("change", function() {
  const file = fileInput.files && fileInput.files[0];

  playing = false;
  session++;

  if (selectedURL) {
    URL.revokeObjectURL(selectedURL);
    selectedURL = null;
  }

  video.pause();
  video.removeAttribute("src");
  video.load();

  if (!file) {
    setStatus("No video selected.");
    return;
  }

  if (!file.type.startsWith("video/")) {
    setStatus("Please select a valid video file.");
    return;
  }

  selectedURL = URL.createObjectURL(file);
  video.src = selectedURL;
  video.muted = true;
  video.loop = true;
  video.playsInline = true;

  setStatus("Selected: " + file.name + "\nLoading video...");
  video.load();
});

video.addEventListener("loadedmetadata", function() {
  setStatus(
    "Video loaded successfully.\nResolution: " +
    video.videoWidth + " x " + video.videoHeight +
    "\nPress Play."
  );
});

video.addEventListener("canplay", function() {
  if (video.videoWidth > 0) {
    setStatus(
      "Video is ready to play.\nResolution: " +
      video.videoWidth + " x " + video.videoHeight +
      "\nPress Play to start."
    );
  }
});

video.addEventListener("error", function() {
  const err = video.error;
  let message = "The browser could not load this video.";

  if (err) {
    if (err.code === 2) {
      message = "A video loading error occurred.";
    } else if (err.code === 3) {
      message = "Video decoding failed. Try MP4 with H.264.";
    } else if (err.code === 4) {
      message = "Unsupported video format. Try MP4 with H.264.";
    }
  }

  setStatus(message);
});

// ---------------- CONVERT VIDEO TO FRAME ----------------
function drawVideoFrame() {
  const vw = video.videoWidth;
  const vh = video.videoHeight;

  if (!vw || !vh) {
    throw new Error("Video has no decoded frames yet.");
  }

  ctx.fillStyle = "black";
  ctx.fillRect(0, 0, 128, 64);

  ctx.save();
  ctx.translate(64, 32);
  ctx.rotate(Math.PI / 2);

  const scale = Math.max(128 / vh, 64 / vw);

  ctx.drawImage(
    video,
    -vw * scale / 2,
    -vh * scale / 2,
    vw * scale,
    vh * scale
  );

  ctx.restore();

  const image = ctx.getImageData(0, 0, 128, 64);
  const pixels = image.data;
  const bytes = new Uint8Array(1024);

  for (let y = 0; y < 64; y++) {
    for (let x = 0; x < 128; x++) {
      const p = (y * 128 + x) * 4;

      const gray =
        (pixels[p] * 299 +
         pixels[p + 1] * 587 +
         pixels[p + 2] * 114) / 1000;

      if (gray >= 128) {
        const index = y * 16 + Math.floor(x / 8);
        bytes[index] |= (1 << (x % 8));
      }
    }
  }

  let binary = "";

  for (let i = 0; i < bytes.length; i++) {
    binary += String.fromCharCode(bytes[i]);
  }

  return btoa(binary);
}

// ---------------- SEND FRAME ----------------
async function sendFrame(frame, currentSession) {
  const response = await fetch("/frame", {
    method: "POST",
    headers: {"Content-Type": "text/plain"},
    body: frame,
    cache: "no-store"
  });

  if (!response.ok) {
    throw new Error("ESP32 returned HTTP " + response.status);
  }

  if (currentSession !== session) return;

  await response.text();
}

// ---------------- VIDEO STREAM ----------------
async function streamFrames(currentSession) {
  if (sending) return;

  sending = true;

  try {
    while (playing && currentSession === session) {
      if (video.paused || video.ended) break;

      if (video.readyState < HTMLMediaElement.HAVE_CURRENT_DATA) {
        await sleep(100);
        continue;
      }

      try {
        const frame = drawVideoFrame();
        await sendFrame(frame, currentSession);

        setStatus(
          "Playing video and sending frames to OLED.\nVideo time: " +
          video.currentTime.toFixed(1) + " seconds"
        );
      } catch (err) {
        setStatus(
          "Video is playing, but OLED transfer failed: " +
          err.message
        );

        await sleep(500);
      }

      await sleep(180);
    }
  } finally {
    sending = false;
  }

  if (playing && currentSession === session) {
    playing = false;

    if (video.ended) {
      setStatus("Video ended. Press Play to start again.");
    } else if (video.paused) {
      setStatus("Video playback paused.");
    }
  }
}

// ---------------- PLAY BUTTON ----------------
playBtn.addEventListener("click", async function() {
  if (!video.src) {
    setStatus("First select a video file.");
    return;
  }

  if (video.error) {
    setStatus("Video cannot be decoded. Try MP4/H.264.");
    return;
  }

  if (video.readyState < HTMLMediaElement.HAVE_CURRENT_DATA) {
    setStatus("Video is still loading. Please wait.");
    video.load();
    return;
  }

  try {
    video.muted = true;
    video.loop = true;
    video.playsInline = true;

    await video.play();

    playing = true;
    const currentSession = ++session;

    setStatus("Video started. Connecting to OLED...");
    streamFrames(currentSession);
  } catch (err) {
    playing = false;
    setStatus("Playback failed: " + err.message);
  }
});

// ---------------- STOP VIDEO ----------------
stopBtn.addEventListener("click", function() {
  playing = false;
  session++;
  video.pause();
  setStatus("Video stopped.");
});

// ---------------- CLEAR OLED ----------------
async function clearOLED() {
  const response = await fetch("/clear", {
    method: "POST",
    cache: "no-store"
  });

  if (!response.ok) {
    throw new Error("HTTP " + response.status);
  }
}

clearBtn.addEventListener("click", async function() {
  try {
    await clearOLED();
    setStatus("OLED cleared.");
  } catch (err) {
    setStatus("Could not clear OLED: " + err.message);
  }
});

// ---------------- DISPLAY TEXT ----------------
displayTextBtn.addEventListener("click", async function() {
  const message = oledText.value.trim();

  if (!message) {
    setStatus("Type some text first.");
    return;
  }

  try {
    const response = await fetch("/text", {
      method: "POST",
      headers: {"Content-Type": "text/plain; charset=utf-8"},
      body: message,
      cache: "no-store"
    });

    const result = await response.text();

    if (!response.ok) {
      throw new Error(result || ("HTTP " + response.status));
    }

    setStatus("Text sent to OLED.");
  } catch (err) {
    setStatus("Could not send text: " + err.message);
  }
});

clearTextBtn.addEventListener("click", async function() {
  try {
    await clearOLED();
    setStatus("OLED cleared.");
  } catch (err) {
    setStatus("Could not clear OLED: " + err.message);
  }
});
</script>
</body>
</html>
)rawliteral";

// ---------------- BASE64 DECODER ----------------
int base64Value(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

bool decodeBase64(const String& input, uint8_t* output,
                  size_t outputSize, size_t& decodedSize) {
  uint32_t accumulator = 0;
  int bits = 0;
  decodedSize = 0;

  for (size_t i = 0; i < input.length(); i++) {
    char c = input[i];

    if (c == '=') break;

    int value = base64Value(c);
    if (value < 0) continue;

    accumulator = (accumulator << 6) | value;
    bits += 6;

    if (bits >= 8) {
      bits -= 8;

      if (decodedSize >= outputSize) return false;

      output[decodedSize++] = (accumulator >> bits) & 0xFF;
    }
  }

  return true;
}

// ---------------- VIDEO/DRAWING FRAME HANDLER ----------------
void handleFrame() {
  String body = server.arg("plain");

  if (body.length() == 0) {
    server.send(400, "text/plain", "Empty frame");
    return;
  }

  size_t decodedSize = 0;

  bool ok = decodeBase64(
    body,
    frameBuffer,
    sizeof(frameBuffer),
    decodedSize
  );

  if (!ok || decodedSize != sizeof(frameBuffer)) {
    server.send(400, "text/plain", "Invalid frame data");
    return;
  }

  oled.clearBuffer();
  oled.drawXBMP(0, 0, 128, 64, frameBuffer);
  oled.sendBuffer();

  server.send(200, "text/plain", "OK");
}

// ---------------- TEXT DISPLAY HANDLER ----------------
void handleText() {
  String message = server.arg("plain");
  message.trim();

  if (message.length() == 0) {
    server.send(400, "text/plain", "Text is empty");
    return;
  }

  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);

  const int maxLines = 6;
  int lineCount = 0;
  String line = "";
  String word = "";

  auto drawLine = [&](const String& textLine) {
    if (lineCount >= maxLines) return;

    oled.drawUTF8(
      0,
      10 + lineCount * 10,
      textLine.c_str()
    );

    lineCount++;
  };

  auto addWord = [&](const String& nextWord) {
    if (nextWord.length() == 0 || lineCount >= maxLines) return;

    String candidate =
      line.length() ? line + " " + nextWord : nextWord;

    if (oled.getUTF8Width(candidate.c_str()) <= 128) {
      line = candidate;
      return;
    }

    if (line.length() > 0) {
      drawLine(line);
      line = "";
    }

    for (size_t i = 0;
         i < nextWord.length() && lineCount < maxLines;
         i++) {

      String part = line + nextWord[i];

      if (oled.getUTF8Width(part.c_str()) > 128 &&
          line.length() > 0) {
        drawLine(line);
        line = String(nextWord[i]);
      } else {
        line = part;
      }
    }
  };

  for (size_t i = 0;
       i < message.length() && lineCount < maxLines;
       i++) {

    char c = message[i];

    if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
      addWord(word);
      word = "";

      if (c == '\n' && line.length() > 0) {
        drawLine(line);
        line = "";
      }
    } else {
      word += c;
    }
  }

  addWord(word);

  if (line.length() > 0 && lineCount < maxLines) {
    drawLine(line);
  }

  oled.sendBuffer();
  server.send(200, "text/plain", "Text displayed");
}

// ---------------- CLEAR OLED HANDLER ----------------
void handleClear() {
  oled.clearBuffer();
  oled.sendBuffer();

  server.send(200, "text/plain", "OLED cleared");
}

// ---------------- SETUP ----------------
void setup() {
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(100000);

  oled.begin();
  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 14, "ESP32-C3 OLED");
  oled.drawStr(0, 30, "Starting WiFi...");
  oled.sendBuffer();

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);

  bool apStarted = WiFi.softAP(
    AP_SSID,
    AP_PASSWORD,
    6,
    false,
    4
  );

  oled.clearBuffer();

  if (apStarted) {
    oled.drawStr(0, 12, "WiFi Ready!");
    oled.drawStr(0, 28, "C3-OLED");
    oled.drawStr(0, 44, "192.168.4.1");
    oled.drawStr(0, 60, "Select a video");
  } else {
    oled.drawStr(0, 20, "WiFi failed!");
    oled.drawStr(0, 40, "Check power");
  }

  oled.sendBuffer();

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", webpage);
  });

  server.on("/frame", HTTP_POST, handleFrame);
  server.on("/text", HTTP_POST, handleText);
  server.on("/clear", HTTP_POST, handleClear);

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });

  server.begin();
}

// ---------------- MAIN LOOP ----------------
void loop() {
  server.handleClient();
  delay(1);
}
