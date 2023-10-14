// Import required libraries
#include "owm_credentials.h"
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <TimeLib.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Wire.h"
#include "Max44009.h"

// Define constants and variables
#define ONE_WIRE_BUS 2  // OneWire bus pin
#define lightSensorSDA 21  // SDA pin for Max44009 light sensor
#define lightSensorSCL 22  // SCL pin for Max44009 light sensor
#define TEMP_READ_INTERVAL 60000  // 60-second interval for temperature reading
#define NTP_SERVER "europe.pool.ntp.org"  // NTP server
#define TIME_OFFSET 7200  // Time offset in seconds
#define UPDATE_INTERVAL 60000  // 60-second NTP update interval
// #define RESYNC_WAIT_IN_SECONDS 432000  // Time interval for NTP sync (5 days)
#define RESYNC_WAIT_IN_SECONDS 3600  // Time interval for NTP sync (5 days)

// Define global variables for lux thresholds and contrast levels
float luxThresholdLow = 100.0;
float luxThresholdMedium = 300.0;
float luxThresholdHigh = 500.0;

// Define global variables for contrast levels
uint8_t contrastLow = 0;
uint8_t contrastMedium = 64;
uint8_t contrastHigh = 128;
uint8_t contrastMax = 255;

unsigned long lastWiFiAttempt = 0;
unsigned long wifiRetryInterval = 21600000;  // 6 hours in milliseconds
int consecutiveFails = 0;  // Consecutive number of WiFi connection failures
bool isWiFiConnected = false;

// Initialize battery stats
#include <Battery18650Stats.h>
#define ADC_PIN 35  // ADC pin for battery reading
#define CONVERSION_FACTOR 1.78  // Conversion factor for battery voltage
#define READS 5 // Quantity of reads to get an average of pin readings
Battery18650Stats battery(ADC_PIN, CONVERSION_FACTOR, READS);

// Initialize global variables
unsigned long lastTime = 0;  // Last time of temperature reading
uint8_t batteryPercent = 0;  // Battery percentage
time_t lastSyncTime;  // Last NTP sync time
float temperature = 0;  // Temperature value

unsigned long lastBatteryCheck = 0;
const unsigned long batteryCheckInterval = 60000;  // 1 minute in milliseconds

// Initialize hardware components
WiFiUDP ntpUDP;  // UDP instance for NTP
NTPClient timeClient(ntpUDP, NTP_SERVER, TIME_OFFSET, UPDATE_INTERVAL);  // NTP client
OneWire oneWire(ONE_WIRE_BUS);  // OneWire instance
DallasTemperature tempSensor(&oneWire);  // Temperature sensor
Max44009 max44009(0x4A, lightSensorSDA, lightSensorSCL);  // Light sensor
U8G2_SSD1322_NHD_256X64_1_4W_SW_SPI u8g2(U8G2_R0, 27, 25, 0, 32, 4);  // Display

// Degree symbol for temperature display
const char DEGREE_SYMBOL[] = { 0xB0, '\0' };

// Enum for different display types
enum DisplayType {
  DISPLAY_TIME,
  DISPLAY_TEMPERATURE,
  DISPLAY_LAST_SYNC,
  DISPLAY_BATTERY
};

// Function prototypes
float readTemperature();  // Read temperature from sensor
void setupWiFi();  // Set up WiFi connection
void setupTime();  // Set up time using NTP
void updateNTPTime();  // Update time from NTP
uint8_t BatteryPercentage();  // Calculate battery percentage
void updateDisplay(char* timeString, float tempValue, time_t lastSyncValue, uint8_t batteryValue);  // Update all displays
void adjustBrightness(float lux);  // Adjust display brightness based on ambient light

// Setup function
void setup() {
  // Initialize serial communication, display, WiFi, and time
  Serial.begin(115200);
  u8g2.begin();
  u8g2.enableUTF8Print();  // Enable UTF-8 for degree symbol
  setupWiFi();
  setupTime();
  // Initialize the temperature sensor
  tempSensor.begin();
  // Immediately read the temperature
  temperature = readTemperature();
  // Initialize the battery stats
  batteryPercent = BatteryPercentage();  // Immediately read the battery percentage
  Wire.begin();  // Initialize I2C
  Wire.setClock(100000);  // Set I2C clock speed
}

// Main loop function
void loop() {
  // Read temperature at TEMP_READ_INTERVAL and update display
  unsigned long currentMillis = millis();
  if (currentMillis - lastTime >= TEMP_READ_INTERVAL) {
    lastTime = currentMillis;
    temperature = readTemperature();
  }

  // Update NTP time every 5 days
  if (currentMillis / 1000 >= RESYNC_WAIT_IN_SECONDS) {
    updateNTPTime();
  }

  // Get current time and convert to string
  int hrs = hour();
  int mins = minute();
  char time_str[6];
  sprintf(time_str, "%02d:%02d", hrs, mins);

  // Read ambient light and adjust brightness
  float lux = max44009.getLux();
  int err = max44009.getError();
  if (err != 0) {
    Serial.print("Lux reading Error:\t");
    Serial.println(err);
  } else {
    // Serial.print("Lux reading:\t");
    // Serial.println(lux);
    adjustBrightness(lux);
  }

  // Logic to update battery percentage every 1 minute
  if (currentMillis - lastBatteryCheck >= batteryCheckInterval) {
    batteryPercent = BatteryPercentage();
    lastBatteryCheck = currentMillis;
  }

  // Check if it's time to attempt WiFi reconnection
  if (millis() - lastWiFiAttempt >= wifiRetryInterval) {
    setupWiFi();
  }

  // Update the display, including the fail counter if necessary
  updateAllDisplays(time_str, temperature, lastSyncTime, batteryPercent);

}

void updateAllDisplays(char* time_str, float tempValue, time_t lastSyncValue, uint8_t batteryValue) {
  char value_str[10]; // Buffer to hold the converted integer value
  char last_sync_str[20]; // Buffer to hold the last sync time string
  // Constants for positioning
  const int degreeSymbolX = 235;
  const int cSymbolX = 240;
  const int yPosition = 20;

  struct tm * timeinfo;
  timeinfo = localtime(&lastSyncValue);
  strftime(last_sync_str, sizeof(last_sync_str), "%m-%d %H:%M", timeinfo);

  u8g2.firstPage();
  do {
    // Generate time strings for hour and minute
    char hour_str[3];
    char minute_str[3];
    sprintf(hour_str, "%02d", hour());
    sprintf(minute_str, "%02d", minute());

    // Fixed X positions for hour, colon, and minute
    int hourX = 0;
    int minuteX = 95; // Adjust based on the width of your hour digits and colon
    int colonX = 75; // Position for the colon (approximately between hour and minute)

    // Y position (same for all)
    int yPosition_hour_min = 63;
    int yPosition_colon = 58;

    // Display Time
    u8g2.setFont(u8g2_font_logisoso62_tn);
    
    // Draw hour and minute at fixed positions
    u8g2.drawStr(hourX, yPosition_hour_min, hour_str);
    u8g2.drawStr(minuteX, yPosition_hour_min, minute_str);

    // Draw the blinking colon
    int secs = second();
    if (secs % 2 == 0) {
      u8g2.drawStr(colonX, yPosition_colon, ":");
    }

    // Display Temperature
    u8g2.setFont(u8g2_font_helvB12_tf); // Font that supports degree symbol
    dtostrf(tempValue, 4, 1, value_str);

    // Calculate the width of the temperature string
    int strWidth = u8g2.getStrWidth(value_str);

    // Calculate the X position for the temperature value string
    int xPosition = degreeSymbolX - strWidth;

    // Draw the temperature string
    u8g2.drawStr(xPosition, yPosition, value_str);

    // Draw the degree symbol and "C" always at the same position
    u8g2.drawUTF8(degreeSymbolX, yPosition, DEGREE_SYMBOL);
    u8g2.drawStr(cSymbolX, yPosition, "C");

    // Display Battery
    u8g2.setFont(u8g2_font_ncenB08_tr); 
    sprintf(value_str, "%d%%", batteryValue);
    u8g2.drawStr(230, 50, value_str); 

    // Display Last Sync (with smaller font and moved to last)
    u8g2.setFont(u8g2_font_ncenB08_tr); // Smaller font for Last Sync
    u8g2.drawStr(194, 60, last_sync_str); // Moved to the bottom-right corner

    // Display the number of consecutive fails if any
    if (consecutiveFails > 0) {
      char fail_str[10];
      sprintf(fail_str, "CF#: %d", consecutiveFails);
      u8g2.setFont(u8g2_font_ncenB08_tr);  // Smaller font for fail counter
      u8g2.drawStr(5, 10, fail_str);  // Draw in top-left corner
    }
  } while (u8g2.nextPage());
}

uint8_t calculateBatteryPercentage(float voltage) {
  float maxVoltage = 4.2;  // Fully charged voltage
  float minVoltage = 3.0;  // Minimum safe voltage

  if(voltage >= maxVoltage) return 100;
  if(voltage <= minVoltage) return 0;

  return (uint8_t)((voltage - minVoltage) / (maxVoltage - minVoltage) * 100);
}

uint8_t BatteryPercentage() {
  uint8_t percentage = 0;
  float voltage = battery.getBatteryVolts();
  if (voltage > 1) { // Only display if there is a valid reading
    percentage = calculateBatteryPercentage(voltage);
    Serial.println("Voltage = " + String(voltage) + "V, Battery = " + String(percentage) + "%");
  }
  return percentage;
}

// Function to read temperature from Dallas sensor
float readTemperature() {
  tempSensor.requestTemperatures();  // Request temperature from sensor
  float celsius = tempSensor.getTempCByIndex(0);  // Get temperature in Celsius
  // Display temperature in serial monitor for debugging
  Serial.print("Temperature = " + String(celsius));
  Serial.print(" \xC2\xB0");
  Serial.println("C");
  return celsius;  // Return the read temperature
}

// Function to set up WiFi connection
void setupWiFi() {
  int maxAttempts = 20;  // Max attempts to connect to WiFi
  int currentAttempt = 0;  // Current attempt counter

  WiFi.begin(ssid, password);  // Begin WiFi connection
  
  // Attempt to connect to WiFi
  while (WiFi.status() != WL_CONNECTED && currentAttempt < maxAttempts) {
    delay(500);
    Serial.print(".");
    currentAttempt++;
  }
  
  // If failed to connect, increment the fail counter
  if (currentAttempt == maxAttempts) {
    Serial.println("Failed to connect to WiFi. Will retry in 6 hours.");
    consecutiveFails++;
    isWiFiConnected = false;
  } else {
    Serial.println("Connected to WiFi.");
    consecutiveFails = 0;  // Reset the fail counter
    isWiFiConnected = true;
  }

  // Update the last attempt time
  lastWiFiAttempt = millis();
}

// Function to set up time using NTP
void setupTime() {
  timeClient.begin();  // Begin NTP client
  // Wait for time to be updated
  while (!timeClient.update()) {
    Serial.print(".");
    delay(1000);
  }
  WiFi.disconnect();  // Disconnect from WiFi to save power
  setTime(timeClient.getEpochTime());  // Set the internal time
  lastSyncTime = now();  // Update the last synced time with the actual NTP time
}

// Function to adjust display brightness based on ambient light
void adjustBrightness(float lux) {
  uint8_t contrast;  // Contrast level for display

  // Map lux to contrast
  if (lux < luxThresholdLow) {
    contrast = contrastLow;  // Dimmest
  } else if (lux < luxThresholdMedium) {
    contrast = map(lux, luxThresholdLow, luxThresholdMedium, contrastLow, contrastMedium);  // Dim
  } else if (lux < luxThresholdHigh) {
    contrast = map(lux, luxThresholdMedium, luxThresholdHigh, contrastMedium, contrastHigh);  // Medium
  } else {
    contrast = contrastMax;  // Brightest
  }
  
  u8g2.setContrast(contrast);  // Set the contrast
}
 
// Function to update NTP time
void updateNTPTime() {
  unsigned long startMillis = millis();  // Record start time of update
  WiFi.begin(ssid, password);  // Begin WiFi connection

  int connectionAttempts = 0;
  // Attempt to connect to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    if(connectionAttempts > 10) {
      Serial.println("Failed to connect to WiFi. Please check your credentials and signal strength.");
      return;
    }
    delay(500);
    Serial.print(".");
    connectionAttempts++;
  }

  int ntpAttempts = 0;
  // Attempt to update NTP time
  while(!timeClient.update()) {
    if(ntpAttempts > 10) {
      Serial.println("Failed to update NTP time. Check your network connection and NTP server status.");
      return;
    }
    Serial.print(".");
    delay(1000);
    ntpAttempts++;
  }

  setTime(timeClient.getEpochTime());  // Update internal clock with NTP time
  lastSyncTime = now();  // Update the last NTP sync time
  WiFi.disconnect();  // Disconnect from WiFi to save power

  unsigned long endMillis = millis();  // Record end time of update
  lastTime += endMillis - startMillis;  // Adjust lastTime to account for update time
}
