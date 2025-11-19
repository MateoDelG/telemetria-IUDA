#include <stdlib.h> 
#include "ConfigPortal.h"
#include "Globals.h"


static const byte DNS_PORT = 53;

String ConfigPortal::macSuffix() {
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  char buf[7];
  sprintf(buf, "%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(buf);
}

String ConfigPortal::makeApName() {
  return "SensorNivel-" + macSuffix();
}

void ConfigPortal::startAP() {
  _apName = makeApName();

  Serial.println("[ConfigPortal] Iniciando AP: " + _apName);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(_apName.c_str(), "12345678");   // contraseña por ahora fija
  delay(200);

  IPAddress apIP = WiFi.softAPIP();
  Serial.print("[ConfigPortal] IP AP: ");
  Serial.println(apIP);

  // DNS tipo "captive": cualquier dominio -> IP del AP
  _dns.start(DNS_PORT, "*", apIP);

  setupRoutes();

  _active = true;
}

void ConfigPortal::setupRoutes() {

  _server.on("/", HTTP_GET, [this]() {

    // --- Cargar datos actuales ---
    float minLevel   = Globals::getMinLevel();
    float maxLevel   = Globals::getMaxLevel();
    int   sample     = Globals::getSamplePeriod();

    float kalMea     = Globals::getKalMea();
    float kalEst     = Globals::getKalEst();
    float kalQ       = Globals::getKalQ();

    float distNow    = Globals::getDistanceFiltered();

    uint8_t i2cAddr  = Globals::getI2CAddress();


    // --- Página HTML pensada para móvil ---
    String html = R"HTML(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Sensor de Nivel – Configuración</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body {
  font-family: Arial, sans-serif; padding:20px;
  max-width:480px; margin:auto; background:#f9f9f9;
}
label { display:block; margin-top:14px; font-weight:bold; }
input { padding:10px; width:100%; box-sizing:border-box; font-size:1.1em; }
button {
  padding:12px; margin-top:12px; width:100%;
  font-size:1.1em; border:none; border-radius:6px;
  background:#007AFF; color:white;
}
.small-btn {
  padding:8px; background:#444; margin-top:6px; color:white;
  font-size:0.9em; width:100%;
}
.card {
  background:white; padding:16px; border-radius:10px;
  box-shadow:0 2px 6px rgba(0,0,0,0.15);
  margin-bottom:20px;
}
</style>
</head>
<body>

<h2>Configuración del Sensor</h2>
)HTML";

    // --- Inserción dinámica de valores actuales ---
    html += "<div class='card'><h3>Niveles</h3>";

    html += "<label>Nivel mínimo (cm):</label>";
    html += "<input id='min' type='number' step='0.1' value='" + String(minLevel,1) + "'>";
    html += "<button class='small-btn' onclick='setMinFromSensor()'>Usar lectura actual</button>";

    html += "<label>Nivel máximo (cm):</label>";
    html += "<input id='max' type='number' step='0.1' value='" + String(maxLevel,1) + "'>";
    html += "<button class='small-btn' onclick='setMaxFromSensor()'>Usar lectura actual</button>";

    html += "<label style='margin-top:18px;'>Lectura actual (cm):</label>";
    html += "<input id='currentDist' type='text' value='" + String(distNow,1) + " cm' readonly>";

    html += "</div>";


    // Intervalo de muestreo
    html += "<div class='card'><h3>Muestreo</h3>";
    html += "<label>Periodo (ms):</label>";
    html += "<input id='sample' type='number' value='" + String(sample) + "'>";
    html += "</div>";

    // Comunicación I2C (HEX)
    html += "<div class='card'><h3>Comunicación I2C</h3>";

    // formatear dirección actual en HEX (2 dígitos)
    char i2cHex[5];
    sprintf(i2cHex, "%02X", i2cAddr);

    html += "<label>Dirección I2C (HEX, 0x01–0x7F):</label>";
    html += String("<input id='i2c' type='text' pattern='[0-9A-Fa-fxX]+' ")
        + "placeholder='0x30' value='0x" + String(i2cHex) + "'>";
    html += "<small>Ingresa en hexadecimal, por ejemplo 0x30, 30, 0x3A, etc.</small>";
    html += "</div>";



    // Parámetros Kalman
    html += "<div class='card'><h3>Filtro Kalman</h3>";

    html += "<label>Ruido de medición (Mea):</label>";
    html += "<input id='kalMea' type='number' step='0.01' value='" + String(kalMea, 3) + "'>";
    html += "<small>Mayor valor = el sensor es más ruidoso.</small>";

    html += "<label>Error de estimación (Est):</label>";
    html += "<input id='kalEst' type='number' step='0.01' value='" + String(kalEst, 3) + "'>";
    html += "<small>Mayor valor = el filtro responde más lento.</small>";

    html += "<label>Ruido del proceso (Q):</label>";
    html += "<input id='kalQ' type='number' step='0.001' value='" + String(kalQ, 4) + "'>";
    html += "<small>Mayor valor = el filtro sigue cambios rápidos.</small>";

    html += "</div>";

    // Botón guardar
    html += "<button onclick='saveConfig()'>Guardar configuración</button>";

    // JS
    html += R"HTML(
<script>
function saveConfig() {
  let url = "/save?"
    + "min="    + document.getElementById('min').value
    + "&max="   + document.getElementById('max').value
    + "&sample="+ document.getElementById('sample').value
    + "&kalMea="+ document.getElementById('kalMea').value
    + "&kalEst="+ document.getElementById('kalEst').value
    + "&kalQ="  + document.getElementById('kalQ').value
    + "&i2c="   + document.getElementById('i2c').value;

  fetch(url).then(r => alert("Guardado"));
}

function setMinFromSensor() {
  fetch("/setMin").then(() => location.reload());
}

function setMaxFromSensor() {
  fetch("/setMax").then(() => location.reload());
}
</script>

<div class='card'>
  <h3>Gráfica en tiempo real</h3>
  <canvas id="chart" width="400" height="200" style="width:100%;"></canvas>
  <p id="liveValue" style="font-size:1.2em; margin-top:8px;">-- cm</p>
</div>

<script>
let data = [];
let maxPoints = 120;    // ~1 minuto si actualiza cada 500 ms

function fetchStatus() {
  fetch("/status")
    .then(r => r.json())
    .then(j => {
        const value = j.filt;
        if (!isNaN(value)) {
            data.push(value);
            if (data.length > maxPoints) data.shift();
            drawChart();
            document.getElementById("liveValue").innerText = value.toFixed(1) + " cm";

            // Actualizar campo de lectura actual en la tarjeta de niveles
            const cd = document.getElementById("currentDist");
            if (cd) {
              cd.value = value.toFixed(1) + " cm";
            }
        }
    });
}


// Variables persistentes para autoscale suave
let smoothMin = null;
let smoothMax = null;

function drawChart() {
    const canvas = document.getElementById("chart");
    const ctx = canvas.getContext("2d");
    const w = canvas.width;
    const h = canvas.height;

    ctx.clearRect(0, 0, w, h);
    if (data.length < 2) return;

    // --------------------------------------------------------
    // Rango real de datos
    // --------------------------------------------------------
    let realMin = Math.min(...data);
    let realMax = Math.max(...data);

    let span = realMax - realMin;

    // --------------------------------------------------------
    // Añadir margen visual del 10%
    // --------------------------------------------------------
    let margin = span * 0.1;
    realMin -= margin;
    realMax += margin;

    // --------------------------------------------------------
    // Nunca mostrar valores negativos
    // --------------------------------------------------------
    if (realMin < 0) realMin = 0;

    span = realMax - realMin;

    // --------------------------------------------------------
    // Zoom mínimo (70 cm)
    // Si el rango es pequeño, centramos la señal
    // --------------------------------------------------------
    const MIN_SPAN = 70;

    if (span < MIN_SPAN) {
        let mid = (realMin + realMax) / 2;

        realMin = mid - MIN_SPAN / 2;
        realMax = mid + MIN_SPAN / 2;

        // asegurar mínimo 0
        if (realMin < 0) {
            realMin = 0;
            realMax = MIN_SPAN;
        }

        span = MIN_SPAN;
    }

    // --------------------------------------------------------
    // Autoscale suave (damping)
    // --------------------------------------------------------
    if (smoothMin === null) {
        smoothMin = realMin;
        smoothMax = realMax;
    } else {
        const step = 1.0; // cuánto se ajusta por actualización

        // smoothMin
        if (realMin < smoothMin - step) smoothMin -= step;
        else if (realMin > smoothMin + step) smoothMin += step;
        else smoothMin = realMin;

        // smoothMax
        if (realMax < smoothMax - step) smoothMax -= step;
        else if (realMax > smoothMax + step) smoothMax += step;
        else smoothMax = realMax;
    }

    if (smoothMin < 0) smoothMin = 0;

    let range = smoothMax - smoothMin;
    if (range < MIN_SPAN) {
        smoothMax = smoothMin + MIN_SPAN;
        range = MIN_SPAN;
    }

    // --------------------------------------------------------
    // Cuadrícula + eje Y
    // --------------------------------------------------------
    ctx.strokeStyle = "#cccccc";
    ctx.lineWidth = 1;
    ctx.font = "12px Arial";
    ctx.fillStyle = "#555";

    let gridLines = 4;
    for (let i = 0; i <= gridLines; i++) {
        let gy = (i / gridLines) * h;
        let value = smoothMax - (i / gridLines) * range;

        if (value < 0) value = 0;

        ctx.beginPath();
        ctx.moveTo(0, gy);
        ctx.lineTo(w, gy);
        ctx.stroke();

        ctx.fillText(value.toFixed(1) + " cm", 5, gy - 3);
    }

    // --------------------------------------------------------
    // Línea de la gráfica
    // --------------------------------------------------------
    ctx.beginPath();
    ctx.lineWidth = 2;
    ctx.strokeStyle = "#007AFF";

    for (let i = 0; i < data.length; i++) {
        let x = (i / (maxPoints - 1)) * w;
        let y = h - ((data[i] - smoothMin) / range) * h;

        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
    }

    ctx.stroke();

    // --------------------------------------------------------
    // Punto actual
    // --------------------------------------------------------
    let lastX = ((data.length - 1) / (maxPoints - 1)) * w;
    let lastY = h - ((data[data.length - 1] - smoothMin) / range) * h;

    ctx.fillStyle = "#ff0000";
    ctx.beginPath();
    ctx.arc(lastX, lastY, 4, 0, 2 * Math.PI);
    ctx.fill();
}





// Actualización periódica
setInterval(fetchStatus, 30);
</script>

</body>
</html>
)HTML";

    _server.send(200, "text/html", html);
  });

  // --------------------------- GUARDAR VALORES ---------------------------
  _server.on("/save", HTTP_GET, [this]() {

    if (_server.hasArg("min"))
      Globals::setMinLevel(_server.arg("min").toFloat());

    if (_server.hasArg("max"))
      Globals::setMaxLevel(_server.arg("max").toFloat());

    if (_server.hasArg("sample"))
      Globals::setSamplePeriod(_server.arg("sample").toInt());

    if (_server.hasArg("kalMea"))
      Globals::setKalMea(_server.arg("kalMea").toFloat());

    if (_server.hasArg("kalEst"))
      Globals::setKalEst(_server.arg("kalEst").toFloat());

    if (_server.hasArg("kalQ"))
      Globals::setKalQ(_server.arg("kalQ").toFloat());

    if (_server.hasArg("i2c")) {
        String s = _server.arg("i2c");
        s.trim();
        s.toUpperCase();

        // Aceptar formas: "0x30", "30", "0X3A"
        if (s.startsWith("0X")) {
          s = s.substring(2);
        }

        // Limitar a 2–3 caracteres por seguridad
        s = s.substring(0, 3);

        char buf[5];
        s.toCharArray(buf, sizeof(buf));

        char* endPtr = nullptr;
        long val = strtoul(buf, &endPtr, 16);   // base 16

        if (val < 1)   val = 1;
        if (val > 127) val = 127;

        Globals::setI2CAddress((uint8_t)val);
        Serial.printf("[ConfigPortal] Nueva dirección I2C (HEX) = 0x%02lX (%ld)\n", val, val);
    }


    _server.send(200, "text/plain", "OK");
  });

  // --------------------------- NIVELES DESDE LECTURA ---------------------------
_server.on("/setMin", HTTP_GET, [this]() {
    Globals::setMinLevel(Globals::getDistanceFiltered());
    _server.send(200, "text/plain", "OK");
});

_server.on("/setMax", HTTP_GET, [this]() {
    Globals::setMaxLevel(Globals::getDistanceFiltered());
    _server.send(200, "text/plain", "OK");
});

_server.on("/status", HTTP_GET, [this]() {
    String json = "{";
    json += "\"raw\":" + String(Globals::getDistanceRaw()) + ",";
    json += "\"filt\":" + String(Globals::getDistanceFiltered()) + ",";
    json += "\"hasClient\":" + String(Globals::getAPClients());
    json += "}";
    _server.send(200, "application/json", json);
});


  _server.begin();
}

void ConfigPortal::begin() {
  startAP();
}

void ConfigPortal::loop() {
  if (!_active) return;

  _dns.processNextRequest();
  _server.handleClient();
}
