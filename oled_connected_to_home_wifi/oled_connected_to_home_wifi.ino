
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <U8g2lib.h>

// ------------ OLED PINS ------------
#define OLED_SDA 6
#define OLED_SCL 7

// ------------ HOME WIFI ------------
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ------------ OLED -----------------
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(
  U8G2_R0,
  U8X8_PIN_NONE
);

WebServer server(80);
uint8_t frameBuffer[1024];
bool wifiConnected = false;

// ------------ WEBPAGE ---------------
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 OLED Controller</title>

<style>
*{box-sizing:border-box}
body{margin:0;padding:18px;background:#10131a;color:#f1f5f9;font-family:Arial;text-align:center}
.panel{max-width:480px;margin:auto;padding:16px;background:#1c2230;border-radius:14px}
h2{color:#60a5fa}
.note{color:#cbd5e1;font-size:13px;line-height:1.5}
video{width:100%;max-height:260px;background:#000;border-radius:8px;margin:12px auto}
input[type=file]{width:100%;margin:12px 0;padding:10px;background:#293244;color:white}
.buttons{display:flex;gap:10px;margin-top:12px}
button{flex:1;padding:13px 8px;border:0;border-radius:8px;font-size:15px;font-weight:bold;cursor:pointer;touch-action:manipulation}
#playBtn{background:#22c55e}
#stopBtn{background:#ef4444;color:white}
#clearBtn{background:#475569;color:white;width:100%;margin-top:10px}
.section{margin-top:20px;padding-top:14px;border-top:1px solid #374151;text-align:left}
textarea{width:100%;padding:12px;border:0;border-radius:8px;background:#293244;color:white;font:16px Arial;resize:vertical}
#status{margin-top:14px;padding:12px;background:#111827;border-radius:8px;text-align:left;font-size:14px;line-height:1.5;overflow-wrap:anywhere}
#drawingCanvas{display:block;width:100%;max-width:384px;height:auto;aspect-ratio:2/1;margin:12px auto;background:#000;border:2px solid #64748b;border-radius:8px;touch-action:none}
</style>
</head>

<body>
<div class="panel">

<h2>ESP32 OLED Controller</h2>
<p class="note">Video, text and drawing controller</p>

<h2>Video Player</h2>
<input id="fileInput" type="file" accept="video/*">
<video id="video" playsinline webkit-playsinline muted loop preload="metadata"></video>

<div class="buttons">
<button id="playBtn">▶ Play</button>
<button id="stopBtn">■ Stop</button>
</div>

<button id="clearBtn">Clear OLED</button>
<div id="status">Select a video or send text/drawing.</div>

<div class="section">
<h2>Display Text</h2>
<textarea id="oledText" rows="3" maxlength="300" placeholder="Type text here..."></textarea>
<div class="buttons">
<button id="displayTextBtn" style="background:#60a5fa">Display Text</button>
<button id="clearTextBtn" style="background:#475569;color:white">Clear Text</button>
</div>
</div>

<div class="section">
<h2>Draw on OLED</h2>
<p class="note">Draw with your finger or mouse.</p>
<canvas id="drawingCanvas" width="128" height="64"></canvas>
<div class="buttons">
<button id="displayDrawingBtn" style="background:#60a5fa">Display Drawing</button>
<button id="clearDrawingBtn" style="background:#475569;color:white">Clear Drawing</button>
</div>
</div>

</div>

<script>
"use strict";

const $ = id => document.getElementById(id);
const video = $("video");
const fileInput = $("fileInput");
const statusBox = $("status");
const drawCanvas = $("drawingCanvas");
const drawCtx = drawCanvas.getContext("2d");

let videoURL = null;
let playing = false;
let session = 0;

function status(s) {
  statusBox.textContent = s;
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

// ------------ DRAWING CANVAS ------------
drawCtx.fillStyle = "#000";
drawCtx.fillRect(0, 0, 128, 64);
drawCtx.strokeStyle = "#fff";
drawCtx.lineWidth = 2;
drawCtx.lineCap = "round";
drawCtx.lineJoin = "round";

let drawing = false;

function drawPos(e) {
  const r = drawCanvas.getBoundingClientRect();

  return {
    x: (e.clientX - r.left) * 128 / r.width,
    y: (e.clientY - r.top) * 64 / r.height
  };
}

drawCanvas.addEventListener("pointerdown", e => {
  e.preventDefault();
  drawing = true;
  drawCanvas.setPointerCapture(e.pointerId);

  const p = drawPos(e);
  drawCtx.beginPath();
  drawCtx.moveTo(p.x, p.y);
  drawCtx.lineTo(p.x + 0.1, p.y + 0.1);
  drawCtx.stroke();
});

drawCanvas.addEventListener("pointermove", e => {
  if (!drawing) return;

  e.preventDefault();
  const p = drawPos(e);
  drawCtx.lineTo(p.x, p.y);
  drawCtx.stroke();
});

function stopDrawing() {
  drawing = false;
  drawCtx.closePath();
}

drawCanvas.addEventListener("pointerup", stopDrawing);
drawCanvas.addEventListener("pointercancel", stopDrawing);
drawCanvas.addEventListener("lostpointercapture", stopDrawing);

$("clearDrawingBtn").onclick = () => {
  drawCtx.fillStyle = "#000";
  drawCtx.fillRect(0, 0, 128, 64);
};

// Convert 128x64 pixels into a 1024-byte monochrome XBM frame.
function canvasToBase64(ctx) {
  const image = ctx.getImageData(0, 0, 128, 64).data;
  const bytes = new Uint8Array(1024);

  for (let y = 0; y < 64; y++) {
    for (let x = 0; x < 128; x++) {
      const p = (y * 128 + x) * 4;
      const gray =
        (image[p] * 299 +
         image[p + 1] * 587 +
         image[p + 2] * 114) / 1000;

      if (gray >= 128) {
        bytes[y * 16 + Math.floor(x / 8)] |= 1 << (x % 8);
      }
    }
  }

  let binary = "";
  for (let i = 0; i < bytes.length; i++) {
    binary += String.fromCharCode(bytes[i]);
  }

  return btoa(binary);
}

async function sendFrame(frame) {
  const response = await fetch("/frame", {
    method: "POST",
    headers: { "Content-Type": "text/plain" },
    body: frame,
    cache: "no-store"
  });

  if (!response.ok) {
    throw new Error("Frame transfer failed: HTTP " + response.status);
  }

  return response.text();
}

// ------------ DISPLAY DRAWING ------------
$("displayDrawingBtn").onclick = async () => {
  try {
    await sendFrame(canvasToBase64(drawCtx));
    status("Drawing displayed on OLED.");
  } catch (e) {
    status("Drawing failed: " + e.message);
  }
};

// ------------ DISPLAY TEXT ------------
$("displayTextBtn").onclick = async () => {
  const message = $("oledText").value.trim();

  if (!message) {
    status("Enter some text first.");
    return;
  }

  try {
    const response = await fetch("/text", {
      method: "POST",
      headers: { "Content-Type": "text/plain;charset=utf-8" },
      body: message
    });

    const result = await response.text();

    if (!response.ok) throw new Error(result);
    status("Text displayed on OLED.");
  } catch (e) {
    status("Text transfer failed: " + e.message);
  }
};

async function clearOLED() {
  const response = await fetch("/clear", { method: "POST" });
  if (!response.ok) throw new Error("Clear failed");
}

$("clearBtn").onclick = async () => {
  try {
    await clearOLED();
    status("OLED cleared.");
  } catch (e) {
    status(e.message);
  }
};

$("clearTextBtn").onclick = $("clearBtn").onclick;

// ------------ VIDEO SELECTION ------------
fileInput.addEventListener("change", () => {
  const file = fileInput.files[0];

  stopVideo();

  if (videoURL) URL.revokeObjectURL(videoURL);
  video.removeAttribute("src");
  video.load();

  if (!file) return;

  videoURL = URL.createObjectURL(file);
  video.src = videoURL;
  video.muted = true;
  video.loop = true;
  video.load();

  status("Loading " + file.name);
});

video.addEventListener("loadedmetadata", () => {
  status("Video ready: " + video.videoWidth + " × " + video.videoHeight);
});

video.addEventListener("error", () => {
  status("Video error. Try an MP4 encoded with H.264.");
});

// ------------ VIDEO FRAME CONVERSION ------------
const frameCanvas = document.createElement("canvas");
frameCanvas.width = 128;
frameCanvas.height = 64;

const frameCtx = frameCanvas.getContext("2d", {
  willReadFrequently: true,
  alpha: false
});

function videoFrame() {
  const w = video.videoWidth;
  const h = video.videoHeight;

  if (!w || !h) throw new Error("Video not ready");

  frameCtx.fillStyle = "#000";
  frameCtx.fillRect(0, 0, 128, 64);

  frameCtx.save();
  frameCtx.translate(64, 32);
  frameCtx.rotate(Math.PI / 2);

  const scale = Math.max(128 / h, 64 / w);

  frameCtx.drawImage(
    video,
    -w * scale / 2,
    -h * scale / 2,
    w * scale,
    h * scale
  );

  frameCtx.restore();

  return canvasToBase64(frameCtx);
}

async function streamFrames(mySession) {
  try {
    while (playing && mySession === session && !video.paused) {
      if (video.readyState >= 2) {
        try {
          await sendFrame(videoFrame());
          status("Video playing and sending frames to OLED.");
        } catch (e) {
          status("Video/OLED transfer failed: " + e.message);
          await sleep(500);
        }
      }

      await sleep(180);
    }
  } finally {
    // The session check prevents an older stream from updating status.
  }
}

$("playBtn").onclick = async () => {
  if (!video.src) {
    status("Select a video first.");
    return;
  }

  try {
    await video.play();
    playing = true;
    const mySession = ++session;
    streamFrames(mySession);
  } catch (e) {
    status("Playback failed: " + e.message);
  }
};

function stopVideo() {
  playing = false;
  session++;
  video.pause();
}

$("stopBtn").onclick = () => {
  stopVideo();
  status("Video stopped.");
};
</script>
</body>
</html>
)rawliteral";

// ------------ BASE64 DECODER ------------
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

// ------------ FRAME HANDLER ------------
void handleFrame() {
  String body = server.arg("plain");
  size_t decodedSize = 0;

  bool ok = decodeBase64(
    body, frameBuffer, sizeof(frameBuffer), decodedSize
  );

  if (!ok || decodedSize != sizeof(frameBuffer)) {
    server.send(400, "text/plain", "Invalid frame");
    return;
  }

  oled.clearBuffer();
  oled.drawXBMP(0, 0, 128, 64, frameBuffer);
  oled.sendBuffer();

  server.send(200, "text/plain", "OK");
}

// ------------ TEXT HANDLER ------------
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

  auto drawLine = [&](const String& s) {
    if (lineCount >= maxLines) return;
    oled.drawUTF8(0, 10 + lineCount * 10, s.c_str());
    lineCount++;
  };

  auto addWord = [&](const String& w) {
    if (!w.length() || lineCount >= maxLines) return;

    String candidate = line.length() ? line + " " + w : w;

    if (oled.getUTF8Width(candidate.c_str()) <= 128) {
      line = candidate;
      return;
    }

    if (line.length()) {
      drawLine(line);
      line = "";
    }

    for (size_t i = 0; i < w.length() && lineCount < maxLines; i++) {
      String part = line + w[i];

      if (oled.getUTF8Width(part.c_str()) > 128 && line.length()) {
        drawLine(line);
        line = String(w[i]);
      } else {
        line = part;
      }
    }
  };

  for (size_t i = 0; i < message.length() && lineCount < maxLines; i++) {
    char c = message[i];

    if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
      addWord(word);
      word = "";

      if (c == '\n' && line.length()) {
        drawLine(line);
        line = "";
      }
    } else {
      word += c;
    }
  }

  addWord(word);
  if (line.length() && lineCount < maxLines) drawLine(line);

  oled.sendBuffer();
  server.send(200, "text/plain", "Text displayed");
}

// ------------ CLEAR HANDLER ------------
void handleClear() {
  oled.clearBuffer();
  oled.sendBuffer();
  server.send(200, "text/plain", "OLED cleared");
}

// ------------ WIFI CONNECTION ------------
// Uses the same basic connection sequence as the working diagnostic sketch.
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  Serial.println();
  Serial.println("ESP32-C3 Wi-Fi Controller");
  Serial.print("Connecting to: ");
  Serial.println(WIFI_SSID);

  oled.clearBuffer();
  oled.setFont(u8g2_font_6x10_tf);
  oled.drawStr(0, 14, "Connecting WiFi...");
  oled.sendBuffer();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Same 20-second initial wait as the successful diagnostic sketch.
  for (int i = 0; i < 40; i++) {
    if (WiFi.status() == WL_CONNECTED) break;

    Serial.print("WiFi status: ");
    Serial.println(WiFi.status());
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;

    String ip = WiFi.localIP().toString();

    Serial.println();
    Serial.println("SUCCESS: Wi-Fi connected!");
    Serial.print("IP address: ");
    Serial.println(ip);
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    oled.clearBuffer();
    oled.drawStr(0, 12, "WiFi Connected!");
    oled.drawStr(0, 29, "Open browser:");
    oled.drawStr(0, 46, ip.c_str());
    oled.drawStr(0, 62, "Video/Text/Draw");
    oled.sendBuffer();

    return true;
  }

  wifiConnected = false;

  Serial.println();
  Serial.println("Wi-Fi connection failed.");
  Serial.print("Final Wi-Fi status: ");
  Serial.println(WiFi.status());

  oled.clearBuffer();
  oled.drawStr(0, 14, "WiFi FAILED!");
  oled.drawStr(0, 32, "Check 2.4GHz WiFi");
  oled.drawStr(0, 48, "SSID/password");
  oled.sendBuffer();

  return false;
}

// ------------ SETUP ------------
void setup() {
  Serial.begin(115200);
  delay(1500);

  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(100000);

  oled.begin();
  oled.setFont(u8g2_font_6x10_tf);
  oled.clearBuffer();
  oled.drawStr(0, 14, "Starting...");
  oled.sendBuffer();

  // Only one Wi-Fi connection attempt during startup.
  connectWiFi();

  // Register webpage and controller endpoints.
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
  Serial.println("Web server started.");
}

// ------------ LOOP ------------
void loop() {
  // Do not repeatedly restart Wi-Fi while it is connecting.
  server.handleClient();
  delay(1);
}
