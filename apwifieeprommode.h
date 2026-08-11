#ifndef APWIFIEEPROMMODE_H
#define APWIFIEEPROMMODE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EEPROM.h>

WebServer server(80);

String leerStringDeEEPROM(int direccion)
{
    String cadena = "";
    char caracter = EEPROM.read(direccion);
    int i = 0;

    while (caracter != '\0' && i < 100)
    {
        cadena += caracter;
        i++;
        caracter = EEPROM.read(direccion + i);
    }

    return cadena;
}

void escribirStringEnEEPROM(int direccion, String cadena)
{
    int longitud = cadena.length();

    for (int i = 0; i < longitud; i++)
    {
        EEPROM.write(direccion + i, cadena[i]);
    }

    EEPROM.write(direccion + longitud, '\0');
    EEPROM.commit();
}

//----------------------------------------------------
// PAGINA DE CONFIGURACION WIFI
//----------------------------------------------------

void handleRootWifi()
{
    String html = "<html><body>";
    html += "<h2>Configurar WiFi</h2>";
    html += "<form method='POST' action='/wifi'>";
    html += "SSID:<br>";
    html += "<input type='text' name='ssid'><br><br>";
    html += "Password:<br>";
    html += "<input type='password' name='password'><br><br>";
    html += "<input type='submit' value='Conectar'>";
    html += "</form>";
    html += "</body></html>";

    server.send(200, "text/html", html);
}

int posW = 50;

void handleWifi()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    Serial.print("Conectando a ");
    Serial.println(ssid);

    WiFi.disconnect(true);
    delay(500);

    WiFi.begin(ssid.c_str(), password.c_str());

    int cnt = 0;

    while (WiFi.status() != WL_CONNECTED && cnt < 10)
    {
        delay(1000);
        Serial.print(".");
        cnt++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        String varsave = leerStringDeEEPROM(300);

        if (varsave == "a")
        {
            posW = 0;
            escribirStringEnEEPROM(300, "b");
        }
        else
        {
            posW = 50;
            escribirStringEnEEPROM(300, "a");
        }

        escribirStringEnEEPROM(0 + posW, ssid);
        escribirStringEnEEPROM(100 + posW, password);

        Serial.println();
        Serial.println("Conectado correctamente");

        server.send(200, "text/plain", "Conexion exitosa");
    }
    else
    {
        Serial.println();
        Serial.println("No fue posible conectar");

        server.send(200, "text/plain", "Error de conexion");
    }
}

//----------------------------------------------------
// RECUPERAR RED GUARDADA
//----------------------------------------------------

bool lastRed()
{
    for (int psW = 0; psW <= 50; psW += 50)
    {
        String usu = leerStringDeEEPROM(0 + psW);
        String cla = leerStringDeEEPROM(100 + psW);

        if (usu.length() < 1)
            continue;

        Serial.println(usu);

        WiFi.disconnect(true);
        delay(500);

        WiFi.begin(usu.c_str(), cla.c_str());

        int cnt = 0;

        while (WiFi.status() != WL_CONNECTED && cnt < 8)
        {
            delay(1000);
            Serial.print(".");
            cnt++;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            Serial.println();
            Serial.println("Conectado a Red Wifi");
            Serial.println(WiFi.localIP());

            return true;
        }
    }

    return false;
}

//----------------------------------------------------
// MODO AP
//----------------------------------------------------

void initAP(const char *apSsid, const char *apPassword)
{
    WiFi.mode(WIFI_AP);

    WiFi.softAP(apSsid, apPassword);

    Serial.println();
    Serial.println("Modo configuracion WiFi");

    server.on("/", handleRootWifi);
    server.on("/wifi", handleWifi);

    server.begin();

    Serial.println("Servidor AP iniciado");
}

void loopAP()
{
    server.handleClient();
}

//----------------------------------------------------
// CONEXION AUTOMATICA
//----------------------------------------------------

void intentoconexion(const char *apname, const char *appassword)
{
    EEPROM.begin(512);

    Serial.println("Ingreso a intentoconexion");

    if (!lastRed())
    {
        Serial.println("No hay WiFi guardado");

        Serial.println("Conectarse a:");
        Serial.println(apname);

        Serial.println("Abrir:");
        Serial.println("192.168.4.1");

        initAP(apname, appassword);
    }

    while (WiFi.status() != WL_CONNECTED)
    {
        loopAP();
    }
}

#endif
