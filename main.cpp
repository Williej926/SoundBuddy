#include <Arduino.h>
#include <driver/i2s.h>
#include <Adafruit_CAP1188.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "nvs.h"
#include "nvs_flash.h"
#include <HttpClient.h>
#include <WiFi.h>

#define I2S_WS 21
#define I2S_SCK 22
#define I2S_SD 17

//Software SPI setup
#define CAP1188_MISO  33
#define CAP1188_CS 26
#define CAP1188_MOSI 32
#define CAP1188_RESET 27
#define CAP1188_CLK 25
// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0
// For wifi connection
char ssid[50];
char pass[50];

//Display
TFT_eSPI tft = TFT_eSPI();


bool sensorOn = true;
int touchRefresh = -1;
int session = -1;

//cap 1188
Adafruit_CAP1188 cap = Adafruit_CAP1188(CAP1188_CLK, CAP1188_MISO, CAP1188_MOSI, CAP1188_CS, CAP1188_RESET);

void nvs_access()
{
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  // Open
  Serial.printf("\n");
  Serial.printf("Opening Non-Volatile Storage (NVS) handle... ");
  // Taken from lab 4
  nvs_handle_t my_handle;
  err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err != ESP_OK)
  {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
  }
  else
  {
    Serial.printf("Done\n");
    Serial.printf("Retrieving SSID/PASSWD\n");
    size_t ssid_len;
    size_t pass_len;
    err = nvs_get_str(my_handle, "ssid", ssid, &ssid_len);
    err |= nvs_get_str(my_handle, "pass", pass, &pass_len);
    switch (err)
    {
    case ESP_OK:

      Serial.printf("Done\n");
      Serial.printf("SSID = %s\n", ssid);
      Serial.printf("PASSWD = %s\n", pass);
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      Serial.printf("The value is not initialized yet!\n");
      break;
    default:
      Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
  }
  // Close
  nvs_close(my_handle);
}

// Define input buffer length
#define bufferLen 64
int16_t sBuffer[bufferLen];
 

// Setup for the mic taken from https://dronebotworkshop.com/esp32-i2s/
void i2s_install() {
  // Set up I2S Processor configuration
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 44100,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };
 
  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}
 
void i2s_setpin() {
  // Set I2S pin configuration
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
 
  i2s_set_pin(I2S_PORT, &pin_config);
}
 
void setup() {
  Wire.begin();
  // Set up Serial Monitor
  delay(1000);

  Serial.begin(9600);
  Serial.println(" ");
  tft.init();

  delay(1000);
  tft.begin();  
  tft.fillScreen(TFT_BLACK);

  tft.drawFloat(0,1,50,50,4);

 
  // Set up I2S
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);

  // Set up cap1188
  if(!cap.begin()){
    Serial.println("CAP1188 not found");
    while (1);
  }
 
  delay(500);
  nvs_access();
  // We start by connecting to a WiFi network
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
}
 
void loop() {
  WiFiClient c;
  HttpClient http(c);
  if(session == -1){
    session = rand() % 90000 + 10000;
    Serial.println("Session: " + String(session));
  }
  
  //Serial.println("NEW ITER");
    uint8_t touched = cap.touched();
    if(touchRefresh == -1){
      uint8_t numTouched = 0;
      
      for (uint8_t i=0; i<8; i++) {
        if (touched & (1 << i)) {
            numTouched+=1;
        }
      }
      if(numTouched >=4){
        if(sensorOn){
          Serial.println("Turning off...");
        }
        else{
          Serial.println("Turning on...");
          session = rand() % 90000 + 10000;
        }
        sensorOn = !sensorOn;
        touchRefresh = 0;
        delay(5000);
      }
    }
    else if(touchRefresh == 5){
      touchRefresh = -1;
    }
    else{
      touchRefresh+=1;
    }


  if(sensorOn){

    // Data collection taken from https://dronebotworkshop.com/esp32-i2s/
    int rangelimit = 3000;
    // Get I2S data and place in data buffer
    size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen, &bytesIn, portMAX_DELAY);

    if (result == ESP_OK)
    {
      // Read I2S data buffer
      int16_t samples_read = bytesIn / 8;
      if (samples_read > 0) {
        float mean = 0;
        for (int16_t i = 0; i < samples_read; ++i) {
          mean += (sBuffer[i]);
        }
  
        // Average the data reading
        mean /= samples_read;
        if(mean == mean){
        // Print to serial plotter
          Serial.println("MEAN: " + String(std::abs(mean)));
          double dB = 25 * std::log10(std::abs(mean));
          tft.drawFloat(std::abs(dB),1,50,50,4);
          Serial.println("dB: " + String(std::abs(dB)));
          int err = 0;
          String s = "/soundbuddy?session=" + String(session)+"&value="+String(std::abs(dB));
          err = http.get("54.90.190.11", 5000, s.c_str(), NULL);



        }


      }
    }
  }
  delay(500);
}