#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <time.h>
#include <PubSubClient.h>
#include <DHT.h>

// Definiciones

// Ancho de la pantalla (en pixeles)
#define SCREEN_WIDTH 128
// Alto de la pantalla (en pixeles)
#define SCREEN_HEIGHT 64
// Pin del sensor de temperatura y humedad
#define DHTPIN 4
// Tipo de sensor de temperatura y humedad
#define DHTTYPE DHT11
// Intervalo en segundo de las mediciones
#define MEASURE_INTERVAL 2
// Duración aproximada en la pantalla de las alertas que se reciban
#define ALERT_DURATION 60

// Declaraciones

// Sensor DHT
DHT dht(DHTPIN, DHTTYPE);
// Pantalla OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// Cliente WiFi
WiFiClient net;
// Cliente MQTT
PubSubClient client(net);

// Variables a editar TODO

// WiFi (ESP32 solo soporta redes 2.4 GHz, no 5 GHz)
// Nombre exacto de la red WiFi (sensible a mayúsculas y espacios)
const char ssid[] = "Lucy"; // TODO: nombre exacto de tu red
// Contraseña de la red WiFi
const char pass[] = "#1Sol.Rocky"; // TODO: contraseña

// Conexión a Mosquitto
#define USER "pe.perez" // TODO Reemplace UsuarioMQTT por un usuario (no administrador) que haya creado en la configuración del bróker de MQTT.
const char MQTT_HOST[] = "18.208.171.240"; // TODO Reemplace ip.maquina.mqtt por la IP del bróker MQTT que usted desplegó. Ej: 192.168.0.1
const int MQTT_PORT = 8082;
const char MQTT_USER[] = USER;
// Contraseña de MQTT
const char MQTT_PASS[] = "abc123"; // TODO Reemplace ContrasenaMQTT por la contraseña correpondiente al usuario especificado.

// Tópico al que se recibirán los datos
// El tópico de publicación debe tener estructura: <país>/<estado>/<ciudad>/<usuario>/out
const char MQTT_TOPIC_PUB[] = "colombia/cundinamarca/bogota/" USER "/out"; // TODO Reemplace el valor por el tópico de publicación que le corresponde.
// El tópico de suscripción debe tener estructura: <país>/<estado>/<ciudad>/<usuario>/in
const char MQTT_TOPIC_SUB[] = "colombia/cundinamarca/bogota/" USER "/in"; // TODO Reemplace el valor por el tópico de suscripción que le corresponde.

// Declaración de variables globales

// Timestamp de la fecha actual.
time_t now;
// Tiempo de la última medición
long long int measureTime = millis();
// Tiempo en que inició la última alerta
long long int alertTime = millis();
// Mensaje para mostrar en la pantalla
String alert = "";
// Valor de la medición de temperatura
float temp;
// Valor de la medición de la humedad
float humi;

/**
 * Conecta el dispositivo con el bróker MQTT usando
 * las credenciales establecidas.
 * Si ocurre un error lo imprime en la consola.
 */
void mqtt_connect()
{
  while (!client.connected()) {

    Serial.print("MQTT connecting ... ");

    if (client.connect(MQTT_USER, MQTT_USER, MQTT_PASS)) {

      Serial.println("connected.");
      client.subscribe(MQTT_TOPIC_SUB);

    } else {

      int state = client.state();
      Serial.println("fallo.");
      Serial.print("Codigo MQTT = ");
      Serial.println(state);
      if (state == 5) {
        Serial.println("(5 = UNAUTHORIZED: usuario o contrasena incorrectos)");
        Serial.print("Intentando con USER='");
        Serial.print(MQTT_USER);
        Serial.println("' - Revisa USER y MQTT_PASS en el codigo y en el broker.");
      } else if (state == 4) {
        Serial.println("(4 = BAD_CREDENTIALS)");
      } else if (state == 2) {
        Serial.println("(2 = BAD_CLIENT_ID)");
      } else if (state == -2) {
        Serial.println("(-2 = CONNECT_FAILED - revisa MQTT_HOST y MQTT_PORT, y que el broker acepte conexiones)");
      }
      alert = "MQTT error: " + String(state);

      delay(5000);
    }
  }
}

/**
 * Publica la temperatura y humedad dadas al tópico configurado usando el cliente MQTT.
 */
void sendSensorData(float temperatura, float humedad) {
  String data = "{";
  data += "\"temperatura\": "+ String(temperatura, 1) +", ";
  data += "\"humedad\": "+ String(humedad, 1);
  data += "}";
  char payload[data.length()+1];
  data.toCharArray(payload,data.length()+1);

  client.publish(MQTT_TOPIC_PUB, payload);
}

/**
 * Lee la temperatura del sensor DHT, la imprime en consola y la devuelve.
 */
float readTemperatura() {
  float t = dht.readTemperature();
  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.println(" *C ");
  return t;
}

/**
 * Lee la humedad del sensor DHT, la imprime en consola y la devuelve.
 */
float readHumedad() {
  float h = dht.readHumidity();
  Serial.print("Humedad: ");
  Serial.print(h);
  Serial.println(" %\t");
  return h;
}

/**
 * Verifica si las variables ingresadas son números válidos.
 */
bool checkMeasures(float t, float h) {
  if (isnan(t) || isnan(h)) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    return false;
  }
  return true;
}

/**
 * Vincula la pantalla al dispositivo.
 * SDA=GPIO22, SCL=GPIO21 (cableado actual)
 */
void startDisplay() {
  Wire.begin(22, 21); // SDA=GPIO22, SCL=GPIO21
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.setTextColor(SSD1306_WHITE);
}

/**
 * Imprime en la pantalla un mensaje de "No hay señal".
 */
void displayNoSignal() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.println("No hay senal");
  display.display();
}

/**
 * Agrega a la pantalla el header con mensaje "IOT Sensors" y la hora actual.
 */
void displayHeader() {
  display.setTextSize(1);
  long long int milli = now + millis() / 1000;
  struct tm* tinfo;
  tinfo = localtime(&milli);
  String hour = String(asctime(tinfo)).substring(11, 19);
  String title = "IOT Sensors  " + hour;
  display.println(title);
}

/**
 * Agrega los valores medidos de temperatura y humedad a la pantalla.
 */
void displayMeasures() {
  display.println("");
  display.print("T: ");
  display.print(temp);
  display.print("    ");
  display.print("H: ");
  display.print(humi);
  display.println("");
}

/**
 * Agrega el mensaje indicado a la pantalla.
 */
void displayMessage(String message) {
  display.setTextSize(1);
  display.println("\nMsg:");
  display.setTextSize(2);
  if (message.equals("OK")) {
    display.println("    " + message);
  } else {
    display.println("");
    display.println("");
    display.println(message);
  }
}

/**
 * Muestra en la pantalla el mensaje de "Connecting to:".
 */
void displayConnecting(String ssid) {
  display.clearDisplay();
  display.setTextSize(1);
  display.println("Connecting to:\n");
  display.println(ssid);
  display.display();
}

/**
 * Verifica si ha llegado alguna alerta al dispositivo.
 */
String checkAlert() {
  String message = "OK";
  if (alert.length() != 0) {
    message = alert;
    if ((millis() - alertTime) >= ALERT_DURATION * 1000 ) {
      alert = "";
      alertTime = millis();
    }
  }
  return message;
}

/**
 * Función que se ejecuta cuando llega un mensaje a la suscripción MQTT.
 */
void receivedCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received [");
  Serial.print(topic);
  Serial.print("]: ");
  String data = "";
  for (int i = 0; i < length; i++) {
    data += String((char)payload[i]);
  }
  Serial.print(data);
  if (data.indexOf("ALERT") >= 0) {
    alert = data;
  }
}

/**
 * Verifica si el dispositivo está conectado al WiFi.
 */
void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Checking wifi");
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
      WiFi.begin(ssid, pass);
      Serial.print(".");
      displayNoSignal();
      delay(10);
    }
    Serial.println("connected");
  }
  else
  {
    if (!client.connected())
    {
      mqtt_connect();
    }
    else
    {
      client.loop();
    }
  }
}

/**
 * Imprime en consola las redes WiFi disponibles.
 */
void listWiFiNetworks() {
  int numberOfNetworks = WiFi.scanNetworks();
  Serial.println("\nNumber of networks: ");
  Serial.print(numberOfNetworks);
  for(int i = 0; i < numberOfNetworks; i++){
    Serial.println(WiFi.SSID(i));
  }
}

/**
 * Inicia el servicio de WiFi e intenta conectarse a la red especificada.
 */
void startWiFi() {
  WiFi.setHostname(USER);
  WiFi.mode(WIFI_STA);

  // Verificar que ssid y pass no estén vacíos
  Serial.print("\n\nSSID configurado: '");
  Serial.print(ssid);
  Serial.println("'");
  Serial.print("Longitud SSID: ");
  Serial.println(strlen(ssid));
  if (strlen(ssid) == 0) {
    Serial.println("ERROR: ssid esta vacio. Revise const char ssid[] en el codigo.");
    return;
  }

  WiFi.begin(ssid, pass);
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);

  unsigned long startAttempt = millis();
  const unsigned long timeoutMs = 20000; // 20 segundos de espera

  while (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - startAttempt >= timeoutMs) {
      Serial.println("\nTimeout: no se pudo conectar. Codigo estado WiFi: ");
      Serial.println(WiFi.status());
      Serial.println("(1=NO_SSID_AVAIL, 4=CONNECT_FAILED, 6=DISCONNECTED)");
      return;
    }
    if (WiFi.status() == WL_NO_SSID_AVAIL) {
      Serial.println("\nNo se encuentra la red WiFi (revisa nombre exacto). Estado=1");
      WiFi.begin(ssid, pass);
    } else if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("\nFallo conexion (clave incorrecta o red 5GHz?). Estado=4. ESP32 solo 2.4GHz.");
      WiFi.begin(ssid, pass);
    }
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nconnected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

/**
 * Consulta y guarda el tiempo actual con servidores SNTP.
 */
void setTime() {
  Serial.print("Setting time using SNTP");
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  now = time(nullptr);

  while (now < 1510592825) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println("done!");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

/**
 * Configura el servidor MQTT y la función callback.
 */
void configureMQTT() {
  client.setServer(MQTT_HOST, MQTT_PORT);
  client.setCallback(receivedCallback);

  mqtt_connect();
}

/**
 * Mide temperatura y humedad y las envía si es momento.
 */
void measure() {
  if ((millis() - measureTime) >= MEASURE_INTERVAL * 1000 ) {
    Serial.println("\nMidiendo variables...");
    measureTime = millis();

    temp = readTemperatura();
    humi = readHumedad();

    if (checkMeasures(temp, humi)) {
      sendSensorData(temp, humi);
    }
  }
}

/////////////////////////////////////
//         FUNCIONES ARDUINO       //
/////////////////////////////////////

void setup() {
  Serial.begin(115200);

  listWiFiNetworks();

  startDisplay();

  displayConnecting(ssid);

  startWiFi();

  dht.begin();

  setTime();

  configureMQTT();
}

void loop() {

  checkWiFi();

  String message = checkAlert();

  measure();

  display.clearDisplay();
  display.setCursor(0,0);

  displayHeader();
  displayMeasures();
  displayMessage(message);

  display.display();
}
