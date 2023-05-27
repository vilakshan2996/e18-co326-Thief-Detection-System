 /*
  AsyncElegantOTA Demo Example - This example will work for both ESP8266 & ESP32 microcontrollers.
  -----
  Author: Ayush Sharma ( https://github.com/ayushsharma82 )
  
  Important Notice: Star the repository on Github if you like the library! :)
  Repository Link: https://github.com/ayushsharma82/AsyncElegantOTA
*/


#include <WiFi.h>
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <PubSubClient.h>
#include "esp_camera.h"
#include <WebSerial.h>



const char* ssid = "Vilakshan";
const char* password = "Alimatha29";
const char* mqttServer = "192.168.1.116";
const int mqttPort = 1883;
const char* mqttTopic = "work/cam0";
const unsigned long captureInterval = 10000; // Capture image every 1 minute
const size_t bufferSize = 4096; // Adjust buffer size as per your requirement



WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);


AsyncWebServer server(80);




#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


void cameraInit(){
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA; // 640x480
  config.jpeg_quality = 10;
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
    return;
  }
}


void recvMsg(uint8_t *data, size_t len){
  WebSerial.println("Received Data...");
  String d = "";
  for(int i=0; i < len; i++){
    d += char(data[i]);
  }
  WebSerial.println(d);
  // if (d == "ON"){
  //   digitalWrite(LED, HIGH);
  // }
  // if (d=="OFF"){
  //   digitalWrite(LED, LOW);
  // }
}


void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  // if ((char)payload[0] == '1') {
  //   digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
  //   // but actually the LED is on; this is because
  //   // it is active low on the ESP-01)
  // } else {
  //   digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  // }

}

void connectToMQTT() {
  mqttClient.setBufferSize(50*1024);
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void publishImageToMQTT(const char* topic, const char* payload) {
  mqttClient.publish(topic, payload);
}

void captureAndPublishImage() {
  Serial.println(".............Capture and Publish Image Function called..................");
  camera_fb_t* fb = esp_camera_fb_get();
  //fb->len <= bufferSize
  Serial.print(fb!=NULL);
  Serial.print(fb->format);
  if (fb != NULL && fb->format == PIXFORMAT_JPEG  ) {
    WebSerial.print("\t Publish Image: ");
    bool result = mqttClient.publish(mqttTopic,fb->buf, fb->len,false);
    WebSerial.println(result);
    // Serial.print(fb->height); Serial.write(','); // write the height
  // Serial.print(fb->width); Serial.write(','); // write the width
  // for (int i = 0; i < fb->len; i++){ // dump the pixels' value
  // Serial.print(fb->buf[i]);
  // if (i != fb->len -1) Serial.write(',');
  // else Serial.println();
  // }



    // if (!result) {
    //   ESP.restart();
    // }
  }
  esp_camera_fb_return(fb);
}

void setup(void) {
  Serial.begin(115200);

  // Disable brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  setupWiFi();
  cameraInit();
  
  connectToMQTT();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! This is a sample response.");
  });

  AsyncElegantOTA.begin(&server);    // Start AsyncElegantOTA
  // WebSerial is accessible at "<IP Address>/webserial" in browser
  WebSerial.begin(&server);
  WebSerial.msgCallback(recvMsg);


  server.begin();

  Serial.println("HTTP server started");
}

void loop(void) {
  if (!mqttClient.connected()) {
    connectToMQTT();
  }
  mqttClient.loop();

  static unsigned long lastCaptureTime = 0;

  // Capture and publish image every 1 minute
  if (millis() - lastCaptureTime >= captureInterval) {
    captureAndPublishImage();
    lastCaptureTime = millis();
  }
}


