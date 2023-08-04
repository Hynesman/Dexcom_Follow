#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Dexcom_follow.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char *ssid = "your ssid";
const char *password = "your password";

void setup()
{
  Serial.begin(115200);

  Wire.begin();

  // Initialize OLED display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }

  display.display();
  delay(500);
  display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi!");
  Follower follower(true, "birkwitt", "Donaldduck57");
  follower.getNewSessionID();
  follower.GlucoseLevelsNow();
  display.println(follower.GlucoseNow.mmol_l);
  display.println(follower.GlucoseNow.trend_Symbol);
  display.display();

  

}

void loop()
{
  
  delay(5000); 
}
