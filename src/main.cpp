#include <Adafruit_Sensor.h>
#include <ArduinoOTA.h>
#include <DHT.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>

const String FIRMWARE_VERSION = "4.0";
const char *ssid = "nauac's Iphone";
const char *password = "12345678910";
const int ledPin = LED_BUILTIN;

#define DHTPIN D2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

ESP8266WebServer server(80);

float temperatura = NAN;
float umidade = NAN;

unsigned long previousMillisDHT = 0;
const unsigned long DHT_INTERVAL = 2000;

const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Painel NodeMCU - Roteiro 7</title>
  <style>
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background-color: #f0f2f5;
      margin: 0;
      padding: 20px;
      color: #333;
    }
    .container {
      max-width: 480px;
      margin: 0 auto;
      background: #ffffff;
      border-radius: 12px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.1);
      padding: 25px;
      text-align: center;
    }
    h1 {
      font-size: 22px;
      margin-top: 0;
      color: #1a73e8;
    }
    .badge {
      display: inline-block;
      background: #e8f0fe;
      color: #1a73e8;
      padding: 4px 12px;
      border-radius: 16px;
      font-weight: 600;
      font-size: 13px;
      margin-bottom: 20px;
    }
    .alert-box {
      background-color: #f8d7da;
      color: #721c24;
      border: 1px solid #f5c6cb;
      border-radius: 8px;
      padding: 12px;
      margin-bottom: 20px;
      font-weight: bold;
      animation: pulse 1.5s infinite;
    }
    @keyframes pulse {
      0% { opacity: 1; }
      50% { opacity: 0.8; }
      100% { opacity: 1; }
    }
    .card-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
      margin-bottom: 20px;
    }
    .card {
      background: #f8f9fa;
      border: 1px solid #e9ecef;
      border-radius: 8px;
      padding: 15px;
    }
    .card-title {
      font-size: 13px;
      color: #6c757d;
      margin-bottom: 6px;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    .card-value {
      font-size: 24px;
      font-weight: 700;
      color: #212529;
      transition: color 0.3s;
    }
    .error-text {
      color: #dc3545;
      font-size: 15px;
      font-weight: 600;
    }
    .led-section {
      border-top: 1px solid #dee2e6;
      padding-top: 18px;
      margin-top: 10px;
    }
    .led-status {
      font-size: 16px;
      margin-bottom: 15px;
      font-weight: 500;
    }
    .status-indicator {
      display: inline-block;
      width: 10px;
      height: 10px;
      border-radius: 50%;
      margin-right: 6px;
      transition: background-color 0.3s;
    }
    .status-on {
      background-color: #28a745;
      box-shadow: 0 0 8px #28a745;
    }
    .status-off {
      background-color: #dc3545;
    }
    .btn-group {
      display: flex;
      gap: 10px;
      justify-content: center;
    }
    .btn {
      flex: 1;
      padding: 12px;
      font-size: 16px;
      font-weight: bold;
      border: none;
      border-radius: 6px;
      cursor: pointer;
      text-decoration: none;
      display: inline-block;
      transition: background 0.2s, transform 0.1s;
    }
    .btn:active {
      transform: scale(0.98);
    }
    .btn-on {
      background-color: #28a745;
      color: white;
    }
    .btn-on:hover {
      background-color: #218838;
    }
    .btn-off {
      background-color: #dc3545;
      color: white;
    }
    .btn-off:hover {
      background-color: #c82333;
    }
    .footer {
      margin-top: 25px;
      font-size: 12px;
      color: #888;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Monitoramento e Controle</h1>
    <div class="badge">Firmware v%VERSAO%</div>

    <div id="alert-container">%ALERTA%</div>

    <div class="card-grid">
      <div class="card">
        <div class="card-title">Temperatura</div>
        <div class="card-value" id="val-temp">%TEMP%</div>
      </div>
      <div class="card">
        <div class="card-title">Umidade</div>
        <div class="card-value" id="val-umid">%UMID%</div>
      </div>
    </div>

    <div class="led-section">
      <div class="led-status">
        <span class="status-indicator %LED_CLASS%" id="led-ind"></span>LED Interno: <strong id="led-status">%LED_STATUS%</strong>
      </div>
      <div class="btn-group">
        <button type="button" onclick="controlarLed('/on')" class="btn btn-on">Ligar LED</button>
        <button type="button" onclick="controlarLed('/off')" class="btn btn-off">Desligar LED</button>
      </div>
    </div>

    <div class="footer">
      IP: %IP% | Atualização em tempo real via AJAX<br>
      <a href="/dados" style="color:#1a73e8; text-decoration:none;">API JSON (/dados)</a>
    </div>
  </div>

  <script>
    async function atualizarDados() {
      try {
        const response = await fetch('/dados');
        if (!response.ok) return;
        const data = await response.json();

        const tempEl = document.getElementById('val-temp');
        if (data.temperatura !== null && data.temperatura !== undefined) {
          tempEl.innerHTML = data.temperatura.toFixed(1) + ' &deg;C';
        } else {
          tempEl.innerHTML = '<span class="error-text">Falha na leitura</span>';
        }

        const umidEl = document.getElementById('val-umid');
        if (data.umidade !== null && data.umidade !== undefined) {
          umidEl.innerHTML = data.umidade.toFixed(1) + ' %';
        } else {
          umidEl.innerHTML = '<span class="error-text">Falha na leitura</span>';
        }

        const indEl = document.getElementById('led-ind');
        const statusEl = document.getElementById('led-status');
        if (data.led_ligado) {
          indEl.className = 'status-indicator status-on';
          statusEl.textContent = 'LIGADO';
        } else {
          indEl.className = 'status-indicator status-off';
          statusEl.textContent = 'DESLIGADO';
        }

        const alertEl = document.getElementById('alert-container');
        if (data.versao >= '4.1' && data.temperatura !== null && data.temperatura > 30.0) {
          alertEl.innerHTML = '<div class="alert-box">⚠️ ALERTA: Temperatura elevada (' + data.temperatura.toFixed(1) + ' &deg;C &gt; 30.0 &deg;C)!</div>';
        } else {
          alertEl.innerHTML = '';
        }
      } catch (err) {
        console.error('Erro na requisição AJAX:', err);
      }
    }

    async function controlarLed(endpoint) {
      try {
        await fetch(endpoint);
        atualizarDados();
      } catch (err) {
        console.error(err);
      }
    }

    setInterval(atualizarDados, 2000);
  </script>
</body>
</html>
)rawliteral();

void handleRoot() {
  String html = FPSTR(PAGE_HTML);

  html.replace("%VERSAO%", FIRMWARE_VERSION);
  html.replace("%IP%", WiFi.localIP().toString());

  bool ledLigado = (digitalRead(ledPin) == LOW);
  html.replace("%LED_STATUS%", ledLigado ? "LIGADO" : "DESLIGADO");
  html.replace("%LED_CLASS%", ledLigado ? "status-on" : "status-off");

  if (FIRMWARE_VERSION >= "4.1" && !isnan(temperatura) && temperatura > 30.0) {
    String alertBox = "<div class='alert-box'>⚠️ ALERTA: Temperatura elevada (" +
                      String(temperatura, 1) +
                      " &deg;C &gt; 30.0 &deg;C)!</div>";
    html.replace("%ALERTA%", alertBox);
  } else {
    html.replace("%ALERTA%", "");
  }

  if (isnan(temperatura)) {
    html.replace("%TEMP%", "<span class='error-text'>Falha na leitura</span>");
  } else {
    html.replace("%TEMP%", String(temperatura, 1) + " &deg;C");
  }

  if (isnan(umidade)) {
    html.replace("%UMID%", "<span class='error-text'>Falha na leitura</span>");
  } else {
    html.replace("%UMID%", String(umidade, 1) + " %");
  }

  server.send(200, "text/html; charset=utf-8", html);
}

void handleOn() {
  digitalWrite(ledPin, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  digitalWrite(ledPin, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDados() {
  char json[160];
  char tempBuf[12];
  char umidBuf[12];

  if (isnan(temperatura)) {
    strcpy(tempBuf, "null");
  } else {
    dtostrf(temperatura, 1, 1, tempBuf);
  }

  if (isnan(umidade)) {
    strcpy(umidBuf, "null");
  } else {
    dtostrf(umidade, 1, 1, umidBuf);
  }

  snprintf_P(json, sizeof(json),
             PSTR("{"versao":"%s","temperatura":%s,"umidade":%s,"led_"
                  "ligado":%s}"),
             FIRMWARE_VERSION.c_str(), tempBuf, umidBuf,
             (digitalRead(ledPin) == LOW) ? "true" : "false");

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json; charset=utf-8", json);
}

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  Serial.begin(9600);
  Serial.println();
  Serial.println("==========================================");
  Serial.println("Iniciando NodeMCU ESP8266 - Firmware v" + FIRMWARE_VERSION);
  Serial.println("==========================================");

  dht.begin();
  Serial.println("[DHT11] Sensor inicializado no pino D2");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("
Tentando com formato iOS (apóstrofo inteligente)...");
    WiFi.disconnect();
    delay(200);
    WiFi.begin("nauacâs Iphone", password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
  }

  Serial.println();
  Serial.print("Conectado com sucesso! Endereço IP: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname("ESP8266-DHT11");
  ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    String type =
        (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("[OTA] Iniciando atualização de " + type);
  });
  ArduinoOTA.onEnd([]() { Serial.println("
[OTA] Concluído com sucesso!"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progresso: %u%%", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Erro [%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Serial.println("Falha na autenticação");
    else if (error == OTA_BEGIN_ERROR)
      Serial.println("Falha ao iniciar");
    else if (error == OTA_CONNECT_ERROR)
      Serial.println("Falha na conexão");
    else if (error == OTA_RECEIVE_ERROR)
      Serial.println("Falha na recepção");
    else if (error == OTA_END_ERROR)
      Serial.println("Falha no encerramento");
  });

  ArduinoOTA.begin();
  Serial.println("[OTA] Serviço ArduinoOTA ativo e pronto!");

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/dados", handleDados);

  server.begin();
  Serial.println("[HTTP] Servidor Web iniciado na porta 80!");
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisDHT >= DHT_INTERVAL) {
    previousMillisDHT = currentMillis;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println("[DHT11] Aviso: Falha na leitura do sensor!");
      temperatura = NAN;
      umidade = NAN;
    } else {
      temperatura = t;
      umidade = h;
      Serial.printf("[DHT11] Temp: %.1f °C | Umid: %.1f %%
", temperatura,
                    umidade);
    }
  }
}
