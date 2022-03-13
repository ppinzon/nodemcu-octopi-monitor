#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

// LCD Libraries
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

const char* ssid PROGMEM = "wifi-ssid";
const char* password PROGMEM = "wifi password";

//Your Domain name with URL path or IP address with path
const char* serverNamePrinter PROGMEM = "http://192.168.100.151/api/printer";
const char* serverNameJob PROGMEM = "http://192.168.100.151/api/job";

const char* apiToken PROGMEM = "Your octopi api token";

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);
  lcd.begin();
  lcd.home();
  Serial.println("wifi connecting");
  lcd.print("wifi connecting");
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected!");
}


void loop() {
  Serial.println("LoopStart");
  delay(5000);
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    StaticJsonDocument<128> filter;
    filter["temperature"]["bed"]["actual"] = true;
    filter["temperature"]["tool0"]["actual"] = true;

    http.begin(serverNamePrinter);
    http.addHeader("Authorization", apiToken);
    int httpResponseCode = http.GET();
    Serial.print("Printer code: ");
    Serial.println(httpResponseCode);
    StaticJsonDocument<128> doc;
    
    if (httpResponseCode == 200){
      Stream& responsePrinter = http.getStream();


      DeserializationError error = deserializeJson(doc, responsePrinter, DeserializationOption::Filter(filter));
      http.end();

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }

      int temperature_bed_actual = doc["temperature"]["bed"]["actual"];
      int temperature_tool_actual = doc["temperature"]["tool0"]["actual"];

      Serial.print("bed temp:");
      Serial.println(temperature_bed_actual);
      Serial.print("tool temp:");
      Serial.println(temperature_tool_actual);

      doc.clear();
      filter.clear();
      lcd.clear();
      lcd.print("BED:");
      lcd.print(temperature_bed_actual);
      lcd.setCursor(0, 1);
      lcd.print("HE:");
      lcd.print(temperature_tool_actual);
    } else if (httpResponseCode == -1){
      lcd.clear();
      lcd.print("Octoprint Offline");
      lcd.setCursor(0, 1);
      lcd.print("or unreacheable");
    } else if (httpResponseCode == 409){
      lcd.clear();
      lcd.print("Printer Offline");
      return;
    } else {
      lcd.clear();
      lcd.print("HTTP CODE:");
      lcd.print(httpResponseCode);
    }
    


    http.begin(serverNameJob);
    http.addHeader("Authorization", apiToken);
    int httpResponseCode2 = http.GET();
    Serial.print("Job code: ");
    Serial.println(httpResponseCode2);

    Stream& responseJob = http.getStream();

    filter["progress"]["completion"] = true;
    filter["progress"]["printTimeLeft"] = true;
    filter["state"] = true;

    DeserializationError error2 = deserializeJson(doc, responseJob, DeserializationOption::Filter(filter));
    http.end();

    if (error2) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error2.f_str());
      return;
    }

    int job_progress = doc["progress"]["completion"];
    float job_left_seconds = doc["progress"]["printTimeLeft"];

    Serial.print("job progres: %");
    Serial.println(job_progress);



    lcd.setCursor(8, 0);
    lcd.print("DON:");
    lcd.print(job_progress);
    lcd.print("%");
    lcd.setCursor(8, 1);
    lcd.print("ETA:");
    if (job_left_seconds >=  3600){
      float job_left_hours = job_left_seconds / 3600;
      Serial.print("hours left: ");
      Serial.println(job_left_hours);
      lcd.print(job_left_hours);
      lcd.print("h");
      } else {
      int job_left_minutes = job_left_seconds / 60;
      Serial.print("minutes left: ");
      Serial.println(job_left_minutes);
      lcd.print(job_left_minutes);
      lcd.print("m");
      }
    
  }
  else {
    Serial.println("WiFi Disconnected");
  }
}