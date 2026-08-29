#include <Arduino.h>
#include <WiFi.h>
#include "apwifieeprommode.h"

//==================================================
// PINES HC-SR04
//==================================================
#define TRIG1 23 // Superior
#define ECHO1 22

#define TRIG2 21 // Frontal
#define ECHO2 19

#define TRIG3 18 // Inferior
#define ECHO3 5

//==================================================
// MOTORES HÁPTICOS
//==================================================
#define MOTOR_IZQ 25 
#define MOTOR_DER 26 

//==================================================
// VARIABLES DE ESTADO Y SETPOINTS (DISTANCIAS EN CM)
//==================================================
float superior = -1;
float medio = -1;
float inferior = -1;

int umbralSup = 80;
int umbralMed = 100;
int umbralInf = 70;

String estadoSup = "DESACTIVADO";
String estadoMed = "DESACTIVADO";
String estadoInf = "DESACTIVADO";

bool ledSup = false;
bool ledMed = false;
bool ledInf = false;

unsigned long ultimoMuestreo = 0;

//==================================================
// MEDICIÓN Y MEDIANA
//==================================================
float medirDistancia(int trig, int echo) {
    digitalWrite(trig, LOW);
    delayMicroseconds(3);
    digitalWrite(trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig, LOW);

    long tiempo = pulseIn(echo, HIGH, 20000);
    if (tiempo == 0) return -1;

    float distancia = tiempo * 0.0343 / 2;
    if (distancia < 2 || distancia > 250) return -1;

    return distancia;
}

float mediana3(int trig, int echo) {
    float v[3];
    for (int i = 0; i < 3; i++) {
        v[i] = medirDistancia(trig, echo);
        delay(5); 
    }
    for (int i = 0; i < 2; i++) {
        for (int j = i + 1; j < 3; j++) {
            if (v[j] < v[i]) {
                float aux = v[i];
                v[i] = v[j];
                v[j] = aux;
            }
        }
    }
    return v[1];
}

//==================================================
// EVALUAR ALERTAS
//==================================================
void evaluarAlertas() {
    ledSup = false; ledMed = false; ledInf = false;
    estadoSup = "DESACTIVADO"; estadoMed = "DESACTIVADO"; estadoInf = "DESACTIVADO";
    
    bool encenderIzquierdo = false;
    bool encenderDerecho = false;

    if (superior > 0 && superior < umbralSup) {
        ledSup = true;
        estadoSup = "PELIGRO ALTO";
        encenderIzquierdo = true;
    }

    if (medio > 0 && medio < umbralMed) {
        ledMed = true;
        estadoMed = "OBSTACULO FRENTE";
        encenderIzquierdo = true;
        encenderDerecho = true;
    }

    if (inferior > 0 && inferior < umbralInf) {
        ledInf = true;
        estadoInf = "OBSTACULO SUELO";
        encenderDerecho = true;
    }

    digitalWrite(MOTOR_IZQ, encenderIzquierdo ? HIGH : LOW);
    digitalWrite(MOTOR_DER, encenderDerecho ? HIGH : LOW);
}

//==================================================
// ENDPOINT SETPOINTS VIA WEB
//==================================================
void handleSet() {
    if (server.hasArg("sup")) umbralSup = server.arg("sup").toInt();
    if (server.hasArg("med")) umbralMed = server.arg("med").toInt();
    if (server.hasArg("inf")) umbralInf = server.arg("inf").toInt();
    server.send(200, "text/plain", "OK");
}

//==================================================
// PÁGINA WEB - DISEÑO REDISEÑADO CON FILAS Y ROTACIÓN
//==================================================
void dashboard() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="utf-8">
<title>Blind Assistant - Panel IoT</title>
<style>
* { box-sizing:border-box; }
body { margin:0; font-family:Arial, sans-serif; background:#101820; color:white; padding: 25px; }

.main-layout {
    display: flex;
    justify-content: center;
    align-items: center;
    gap: 40px;
    max-width: 1300px;
    margin: auto;
}

/* Título Vertical Rotado 90° hacia la izquierda */
.sidebar-title-container {
    width: 60px;
    display: flex;
    justify-content: center;
    align-items: center;
    height: 500px;
}
.sidebar-title {
    writing-mode: vertical-rl;
    transform: rotate(180deg);
    font-size: 38px;
    font-weight: bold;
    color: #00d4ff;
    letter-spacing: 5px;
    text-shadow: 0 0 12px rgba(0,212,255,0.4);
    white-space: nowrap;
}

/* Contenedor Central de las Fila-Tarjetas */
.rows-container {
    display: flex;
    flex-direction: column;
    gap: 25px;
}

/* Fila Combinada (Sensor + Slider) */
.sensor-row {
    display: flex;
    align-items: center;
    gap: 20px;
}

/* Tarjeta del Sensor */
.card-sensor {
    width: 250px;
    background: #1d2a35;
    border-radius: 18px;
    padding: 18px;
    box-shadow: 0 0 12px #000;
    text-align: center;
}
.sensor-title { font-size: 18px; margin-bottom: 6px; }
.distancia { font-size: 26px; color: #00d4ff; margin: 8px; font-weight: bold; }
.led { width: 20px; height: 20px; border-radius: 50%; margin: 6px auto; }
.rojo { background: red; box-shadow: 0 0 10px red; }
.verde { background: #00ff00; box-shadow: 0 0 10px #00ff00; }
.estado { font-size: 13px; margin-top: 8px; font-weight: bold; }

/* Tarjeta del Slider Integrado */
.card-slider {
    width: 420px;
    background: #1d2a35;
    border-radius: 18px;
    padding: 20px 25px;
    box-shadow: 0 0 12px #000;
}
.slider-label { font-size: 15px; color: #a0aec0; margin-bottom: 10px; display: block; }
input[type=range] { width: 100%; accent-color: #00d4ff; cursor: pointer; }

/* Columna Derecha: Botones Predefinidos Espaciados */
.predefined-column {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 25px; /* Más separación entre botones */
    margin-left: 10px;
}
.predef-title {
    font-size: 18px;
    color: #00d4ff;
    font-weight: bold;
    margin-bottom: 5px;
}
.circle-btn {
    width: 85px;
    height: 85px;
    border-radius: 50%;
    background: #1d2a35;
    border: 3px solid #ffffff;
    color: white;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    box-shadow: 0 0 10px rgba(0,0,0,0.8);
    transition: 0.2s;
    padding: 0;
}
.circle-btn:hover {
    border-color: #00d4ff;
    background: #243442;
    transform: scale(1.06);
}
.circle-btn svg {
    width: 30px;
    height: 30px;
    fill: #ffffff;
    margin-bottom: 2px;
}
.circle-btn span {
    font-size: 10px;
    font-weight: bold;
}
</style>
</head>
<body>

<div class="main-layout">
    
    <!-- TÍTULO ROTADO 90° A LA IZQUIERDA -->
    <div class="sidebar-title-container">
        <div class="sidebar-title">BLIND ASSISTANT</div>
    </div>

    <!-- FILAS COMBINADAS (SENSOR + SLIDER) -->
    <div class="rows-container">
        
        <!-- Fila 1: Superior -->
        <div class="sensor-row">
            <div class="card-sensor">
                <div class="sensor-title">Superior (+45°)</div>
                <div id="supDist" class="distancia">---</div>
                <div id="supLed" class="led rojo"></div>
                <div id="supEstado" class="estado">DESACTIVADO</div>
            </div>
            <div class="card-slider">
                <span class="slider-label">Umbral Sensor Superior: <span id="valSup">80</span> cm</span>
                <input type="range" id="sliderSup" min="30" max="180" value="80" oninput="updateSlider('sup', this.value)">
            </div>
        </div>

        <!-- Fila 2: Frontal -->
        <div class="sensor-row">
            <div class="card-sensor">
                <div class="sensor-title">Frontal (0°)</div>
                <div id="medDist" class="distancia">---</div>
                <div id="medLed" class="led rojo"></div>
                <div id="medEstado" class="estado">DESACTIVADO</div>
            </div>
            <div class="card-slider">
                <span class="slider-label">Umbral Sensor Frontal: <span id="valMed">100</span> cm</span>
                <input type="range" id="sliderMed" min="30" max="220" value="100" oninput="updateSlider('med', this.value)">
            </div>
        </div>

        <!-- Fila 3: Inferior -->
        <div class="sensor-row">
            <div class="card-sensor">
                <div class="sensor-title">Inferior (-45°)</div>
                <div id="infDist" class="distancia">---</div>
                <div id="infLed" class="led rojo"></div>
                <div id="infEstado" class="estado">DESACTIVADO</div>
            </div>
            <div class="card-slider">
                <span class="slider-label">Umbral Sensor Inferior: <span id="valInf">70</span> cm</span>
                <input type="range" id="sliderInf" min="30" max="140" value="70" oninput="updateSlider('inf', this.value)">
            </div>
        </div>

    </div>

    <!-- BOTONES PREDEFINIDOS ESPACIADOS -->
    <div class="predefined-column">
        <div class="predef-title">Predefinidos</div>
        
        <button class="circle-btn" onclick="setMode(50, 70, 40)" title="En Casa">
            <svg viewBox="0 0 24 24"><path d="M10 20v-6h4v6h5v-8h3L12 3 2 12h3v8z"/></svg>
            <span>En Casa</span>
        </button>

        <button class="circle-btn" onclick="setMode(80, 100, 70)" title="Normal">
            <svg viewBox="0 0 24 24"><path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8zm-1-13h2v6h-2zm0 8h2v2h-2z"/></svg>
            <span>Normal</span>
        </button>

        <button class="circle-btn" onclick="setMode(120, 160, 90)" title="Exteriores">
            <svg viewBox="0 0 24 24"><path d="M17 12h2L12 3 5 12h2l-2.5 3h5.5v5h4v-5h5.5L17 12z"/></svg>
            <span>Exteriores</span>
        </button>
    </div>

</div>

<script>
function actualizar() {
    fetch("/datos")
    .then(response => response.json())
    .then(data => {
        document.getElementById("supDist").innerHTML = data.superior;
        document.getElementById("medDist").innerHTML = data.medio;
        document.getElementById("infDist").innerHTML = data.inferior;
        
        document.getElementById("supEstado").innerHTML = data.estadoSup;
        document.getElementById("medEstado").innerHTML = data.estadoMed;
        document.getElementById("infEstado").innerHTML = data.estadoInf;
        
        document.getElementById("supLed").className = "led " + (data.ledSup ? "verde" : "rojo");
        document.getElementById("medLed").className = "led " + (data.ledMed ? "verde" : "rojo");
        document.getElementById("infLed").className = "led " + (data.ledInf ? "verde" : "rojo");
    });
}

function updateSlider(param, val) {
    document.getElementById("val" + (param === 'sup' ? 'Sup' : param === 'med' ? 'Med' : 'Inf')).innerText = val;
    fetch(`/set?${param}=${val}`);
}

function setMode(sup, med, inf) {
    document.getElementById("sliderSup").value = sup;
    document.getElementById("valSup").innerText = sup;
    document.getElementById("sliderMed").value = med;
    document.getElementById("valMed").innerText = med;
    document.getElementById("sliderInf").value = inf;
    document.getElementById("valInf").innerText = inf;
    
    fetch(`/set?sup=${sup}&med=${med}&inf=${inf}`);
}

setInterval(actualizar, 200);
actualizar();
</script>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
}

void enviarDatos() {
    String json="{";
    json+="\"superior\":\"";
    if(superior<0) json+="Sin eco"; else json+=String(superior,1)+" cm";
    json+="\",\"medio\":\"";
    if(medio<0) json+="Sin eco"; else json+=String(medio,1)+" cm";
    json+="\",\"inferior\":\"";
    if(inferior<0) json+="Sin eco"; else json+=String(inferior,1)+" cm";
    json+="\",\"estadoSup\":\"" + estadoSup + "\",";
    json+="\"estadoMed\":\"" + estadoMed + "\",";
    json+="\"estadoInf\":\"" + estadoInf + "\",";
    json+="\"ledSup\":" + String(ledSup?"true":"false") + ",";
    json+="\"ledMed\":" + String(ledMed?"true":"false") + ",";
    json+="\"ledInf\":" + String(ledInf?"true":"false");
    json+="}";

    server.send(200,"application/json",json);
}

//==================================================
// SETUP
//==================================================
void setup() {
    Serial.begin(115200);

    pinMode(TRIG1, OUTPUT); pinMode(ECHO1, INPUT);
    pinMode(TRIG2, OUTPUT); pinMode(ECHO2, INPUT);
    pinMode(TRIG3, OUTPUT); pinMode(ECHO3, INPUT);

    pinMode(MOTOR_IZQ, OUTPUT);
    pinMode(MOTOR_DER, OUTPUT);
    digitalWrite(MOTOR_IZQ, LOW);
    digitalWrite(MOTOR_DER, LOW);

    Serial.println("\nIniciando Blind Assistant");

    intentoconexion("BLIND_ASSISTANT", "12345678");

    Serial.println("\nIP: " + WiFi.localIP().toString());

    server.on("/", dashboard);
    server.on("/datos", enviarDatos);
    server.on("/set", handleSet);
    
    server.begin();
    Serial.println("Servidor web iniciado");
}

//==================================================
// LOOP
//==================================================
void loop() {
    superior = mediana3(TRIG1, ECHO1);
    medio = mediana3(TRIG2, ECHO2);
    inferior = mediana3(TRIG3, ECHO3);

    evaluarAlertas();

    if (millis() - ultimoMuestreo >= 250) {
        ultimoMuestreo = millis();
        
        Serial.print("SUP: ");
        if(superior<0) Serial.print("---"); else Serial.print(superior);
        Serial.print(" | MED: ");
        if(medio<0) Serial.print("---"); else Serial.print(medio);
        Serial.print(" | INF: ");
        if(inferior<0) Serial.print("---"); else Serial.print(inferior);
        Serial.println();
    }

    server.handleClient();
}

    server.handleClient();
}
