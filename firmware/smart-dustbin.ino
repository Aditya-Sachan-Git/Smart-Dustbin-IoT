#include <Servo.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

// Pin definitions
#define TRIG_PIN 5  // GPIO5
#define ECHO_PIN 4  // GPIO4
#define SOIL_SENSOR A0  // ADC pin
#define SERVO_PIN 2  // GPIO2

// IR Sensor pins for fill level monitoring
#define IR_DRY_PIN 12  // GPIO12 - D6 (Dry bin sensor)
#define IR_WET_PIN 14  // GPIO14 - D5 (Wet bin sensor)

// Smoke Sensor pin
#define SMOKE_SENSOR_PIN 13  // GPIO13 - D7 (Smoke/Gas sensor)

// Buzzer pin - CHANGED to D3 (GPIO0) which works better
#define BUZZER_PIN 0  // GPIO0 - D3 (Better working pin)

// Constants
const int DISTANCE_THRESHOLD = 7; // cm - object detection distance
const int MOISTURE_THRESHOLD = 940; // Adjust this value based on your calibration

// For 180-degree servo (0-180 range)
const int SERVO_ORIGIN = 80;      // Center position
const int SERVO_DRY_ANGLE = 170;   // Dry waste: Clockwise to 180°
const int SERVO_WET_ANGLE = 0;     // Wet waste: Anticlockwise to 0°

// Bin dimensions for fill level calculation
const float BIN_HEIGHT = 15.0;      // Total bin height in cm
const float SENSOR_HEIGHT = 2.5;     // IR sensor height from top in cm
const float MAX_FILL_HEIGHT = BIN_HEIGHT - SENSOR_HEIGHT; // 12.5cm usable height

// Smoke sensor constants
const unsigned long SMOKE_CHECK_INTERVAL = 500; // Check smoke every 500ms
bool smokeDetected = false;
unsigned long lastSmokeCheck = 0;

// Buzzer constants
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;
const unsigned long BUZZER_DURATION = 3000; // Buzzer sounds for 3 seconds when smoke detected

// Define IR sensor structure
struct IRSensor {
  int pin;
  int emptyPercent;
  String name;
};

// Create array of IR sensors
IRSensor irSensors[] = {
  {IR_DRY_PIN, 100, "Dry"},
  {IR_WET_PIN, 100, "Wet"}
};

const int NUM_SENSORS = 2;

// Objects
Servo myServo;

// Variables
long duration;
int distance;
int moistureValue;
String wasteType = "N/A";
unsigned long lastDetectionTime = 0;
const unsigned long DETECTION_COOLDOWN = 3000; // 3 seconds cooldown between detections

unsigned long lastFillCheck = 0;
const unsigned long FILL_CHECK_INTERVAL = 1000; // Check fill levels every 1 second

//WiFi Credentials
const char* ssid = "Redmi Note 12 5G";
const char* password = "redminote125G";

// === DESCRIPTIVE ANALYTICS ===
int totalCount = 0;
int wetCount = 0;
int dryCount = 0;
int smokeCount = -1;

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  int retry = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retry++;

    if (retry > 30) {
      Serial.println("\n❌ Failed to connect!");
      return;
    }
  }

  Serial.println("\n✅ Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Open this in browser:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());

  // Date and Time configuration
  configTime(5.5 * 3600, 0, "pool.ntp.org", "time.nist.gov"); // IST (UTC+5:30)

  Serial.println("Syncing time...");
  time_t now = time(nullptr);
  while (now < 100000) {
  delay(500);
  Serial.print(".");
  now = time(nullptr);
  }

  Serial.println("\n✅ Time synced!");

  Serial.println("\n\n=========================================");
  Serial.println("SMART DUSTBIN - DRY/WET SEGREGATION");
  Serial.println("WITH FILL LEVEL & SMOKE MONITORING");
  Serial.println("=========================================\n");
  
  // Initialize pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Initialize IR sensor pins using loop
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(irSensors[i].pin, INPUT_PULLUP);
  }
  
  // Initialize smoke sensor pin
  // pinMode(SMOKE_SENSOR_PIN, INPUT);
  pinMode(SMOKE_SENSOR_PIN, INPUT_PULLUP);
  
  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off initially
  
  // Initialize servo - set to origin (90 degrees)
  myServo.attach(SERVO_PIN);
  myServo.write(SERVO_ORIGIN);
  delay(500);
  
  Serial.println("System initialized!");
  Serial.print("Servo origin position: ");
  Serial.print(SERVO_ORIGIN);
  Serial.println("° (Center)");
  Serial.print("Dry waste: Clockwise to ");
  Serial.print(SERVO_DRY_ANGLE);
  Serial.println("°");
  Serial.print("Wet waste: Anticlockwise to ");
  Serial.print(SERVO_WET_ANGLE);
  Serial.println("°");
  
  Serial.println("\n=== BIN CONFIGURATION ===");
  Serial.print("Total bin height: ");
  Serial.print(BIN_HEIGHT);
  Serial.println(" cm");
  Serial.print("IR sensor position: ");
  Serial.print(SENSOR_HEIGHT);
  Serial.println(" cm from top");
  Serial.print("Usable fill height: ");
  Serial.print(MAX_FILL_HEIGHT);
  Serial.println(" cm");
  Serial.println("IR sensors will detect when waste reaches " + String(SENSOR_HEIGHT) + "cm from top");
  Serial.println("Empty percentage: 100% = completely empty, 0% = full up to sensor\n");
  
  // Display IR sensor configuration
  Serial.println("=== IR SENSOR CONFIGURATION ===");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(irSensors[i].name);
    Serial.print(" bin sensor on pin D");
    if (irSensors[i].pin == 12) Serial.print("6");
    else if (irSensors[i].pin == 14) Serial.print("5");
    Serial.println();
  }
  
  // Display smoke sensor configuration
  Serial.println("\n=== SMOKE/GAS SENSOR CONFIGURATION ===");
  Serial.println("Smoke/Gas sensor on pin D7 (GPIO13)");
  Serial.println("Sensor will detect smoke, gas, or harmful odors");
  Serial.println("HIGH = Normal air quality (No smoke)");
  Serial.println("LOW = Smoke/Gas detected! (Air quality warning)");
  Serial.println("NOTE: Smoke detection will NOT block waste disposal\n");
  
  // Display buzzer configuration
  Serial.println("=== BUZZER CONFIGURATION ===");
  Serial.println("Buzzer on pin D3 (GPIO0)");
  Serial.println("Buzzer will sound for 3 seconds when smoke/gas is detected");
  Serial.println("Buzzer test...");
  
  // Multiple buzzer tests to ensure it works
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
  Serial.println("✓ Buzzer test complete\n");
  
  // Test smoke sensor
  Serial.println("Testing smoke/gas sensor...");
  delay(1000);
  int smokeTest = digitalRead(SMOKE_SENSOR_PIN);
  if (smokeTest == LOW) {
    Serial.println("⚠️  WARNING: Smoke/Gas detected during startup!");
    Serial.println("   Check if there's burning smell or gas leak nearby.");
  } else {
    Serial.println("✓ Smoke/Gas sensor OK - Air quality normal");
  }
  Serial.println();
  
  Serial.println("Waiting for objects...\n");
  
  // Print header for readings
  Serial.println("Time\tDistance\tMoisture\tWaste Type\tServo Pos\tDry Empty%\tWet Empty%\tAir Quality\tStatus");
  Serial.println("--------------------------------------------------------------------------------");

  server.on("/", handleRoot);

  server.on("/data", []() {
    String json = "{";

    json += "\"waste\":\"" + wasteType + "\",";
    json += "\"dry\":" + String(digitalRead(IR_DRY_PIN) == HIGH ? 0 : 1) + ",";
    json += "\"wet\":" + String(digitalRead(IR_WET_PIN) == HIGH ? 0 : 1) + ",";
    json += "\"smoke\":" + String(smokeDetected ? 0 : 1);

    json += "}";

    server.send(200, "application/json", json);

  });

  server.on("/analytics", []() {
    float wetPercent = totalCount > 0 ? (wetCount * 100.0 / totalCount) : 0;
    float dryPercent = totalCount > 0 ? (dryCount * 100.0 / totalCount) : 0;

    String json = "{";
    json += "\"total\":" + String(totalCount) + ",";
    json += "\"wet\":" + String(wetPercent) + ",";
    json += "\"dry\":" + String(dryPercent) + ",";
    json += "\"smoke\":" + String(smokeCount);
    json += "}";

    server.send(200, "application/json", json);
  });

  server.begin();

  Serial.println("Web server started!");
}

void loop() {
  distance = measureDistance();
  
  // Check fill levels periodically
  if (millis() - lastFillCheck > FILL_CHECK_INTERVAL) {
    updateFillLevels();
    lastFillCheck = millis();
  }
  
  // Check smoke sensor periodically
  if (millis() - lastSmokeCheck > SMOKE_CHECK_INTERVAL) {
    checkSmokeSensor();
    lastSmokeCheck = millis();
  }
  
  // Handle buzzer timing (turn off after duration)
  if (buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION)) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
    Serial.println("🔊 Buzzer turned OFF");
  }
  
  if (distance > 0 && distance < DISTANCE_THRESHOLD) {
    if (millis() - lastDetectionTime > DETECTION_COOLDOWN) {
      // Smoke detection does NOT block object detection
      handleObjectDetection();
      lastDetectionTime = millis();
    }
  }
  
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 1000) {
    printReadings();
    lastPrintTime = millis();
  }
  
  server.handleClient();

  // delay(10);
}

void updateFillLevels() {
  // Update all IR sensors using loop
  for (int i = 0; i < NUM_SENSORS; i++) {
    int sensorState = digitalRead(irSensors[i].pin);
    
    // Calculate empty percentage (same logic for all sensors)
    if (sensorState == HIGH) {
      irSensors[i].emptyPercent = 0;  // Waste detected - bin is full (0% empty)
    } else {
      irSensors[i].emptyPercent = 100;  // No waste detected - bin is empty (100% empty)
    }
  }
}

void checkSmokeSensor() {
  int smokeState = digitalRead(SMOKE_SENSOR_PIN);
  
  // Debug Smoke Detection
  // Serial.print("Smoke state: ");
  // Serial.println(smokeState);

  // Smoke sensor typically outputs LOW when smoke/gas is detected
  if (smokeState == LOW) {
    if (!smokeDetected) {
      smokeCount++;
      // Smoke just detected
      smokeDetected = true;
      
      // Activate buzzer with multiple beeps for better alert
      Serial.println("\n💨💨💨  SMOKE/GAS DETECTED!  💨💨💨");
      Serial.println("⚠️  WARNING: Unusual smoke or gas detected in or near the dustbin!");
      Serial.println("⚠️  Possible causes: Burning waste, chemical reaction, or gas leak");
      Serial.println("⚠️  Please ventilate the area and check for any burning materials.");
      
      // Activate buzzer with pattern (3 short beeps then continuous)
      for (int i = 0; i < 3; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
      }
      
      // Then continuous buzzer
      digitalWrite(BUZZER_PIN, HIGH);
      buzzerActive = true;
      buzzerStartTime = millis();
      
      Serial.println("🔊  ALARM ACTIVATED! (Buzzer sounding for 3 seconds)");
      Serial.println("✅  NOTE: Waste disposal system remains ACTIVE\n");
    }
  } else {
    if (smokeDetected) {
      // Smoke cleared
      smokeDetected = false;
      
      Serial.println("\n✓✓✓  AIR QUALITY NORMALIZED!  ✓✓✓");
      Serial.println("✓ Smoke/Gas hazard cleared");
      Serial.println("✓ System continues normal operation\n");
    }
  }
}

String getDateTime() {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);

  char buffer[25];

  // Format: DD-MM-YYYY HH:MM:SS
  sprintf(buffer, "%02d-%02d-%04d %02d:%02d:%02d",
          timeinfo->tm_mday,
          timeinfo->tm_mon + 1,
          timeinfo->tm_year + 1900,
          timeinfo->tm_hour,
          timeinfo->tm_min,
          timeinfo->tm_sec);

  return String(buffer);
}

void handleObjectDetection() {
  Serial.println("\n*** OBJECT DETECTED! ***");
  
  // Wait for 3 seconds before reading moisture sensor
  Serial.println("→ Waiting 3 seconds before moisture detection...");
  delay(3000);
  
  // Read moisture sensor after delay
  Serial.println("→ Reading moisture sensor...");
  moistureValue = analogRead(SOIL_SENSOR);
  
  // Determine waste type
  if (moistureValue > MOISTURE_THRESHOLD) {
    wasteType = "DRY";
  } else {
    wasteType = "WET";
  }
  
  Serial.print("→ Waste detected: ");
  Serial.println(wasteType);
  
  totalCount++;
  if (wasteType == "WET") {
    wetCount++;
  } else if (wasteType == "DRY") {
    dryCount++;
  }

  // Move servo based on waste type
  int targetAngle;

  if (wasteType == "DRY") {
    // DRY waste: Move CLOCKWISE (increase angle)
    targetAngle = SERVO_DRY_ANGLE;
    Serial.print("→ Moving servo CLOCKWISE to ");
    Serial.print(targetAngle);
    Serial.println("° for DRY waste...");
  } else {
    // WET waste: Move ANTICLOCKWISE (decrease angle)
    targetAngle = SERVO_WET_ANGLE;
    Serial.print("→ Moving servo ANTICLOCKWISE to ");
    Serial.print(targetAngle);
    Serial.println("° for WET waste...");
  }
  
  targetAngle = constrain(targetAngle, 0, 180);

  // Move servo gradually to observe direction
  moveServoGradually(targetAngle);
  
  // Print results
  Serial.println("\n========== ANALYSIS RESULTS ==========");
  Serial.print("Moisture Sensor Value: ");
  Serial.println(moistureValue);
  Serial.print("Waste Classification: ");
  Serial.print(wasteType);
  
  if (wasteType == "WET") {
    Serial.println(" 💧 (Wet Waste)");
    Serial.print("Servo Movement: ANTICLOCKWISE to ");
    Serial.print(targetAngle);
    Serial.println("°");
  } else {
    Serial.println(" 🏜️ (Dry Waste)");
    Serial.print("Servo Movement: CLOCKWISE to ");
    Serial.print(targetAngle);
    Serial.println("°");
  }
  
  // Show current fill levels using loop
  Serial.println("\n--- CURRENT FILL STATUS ---");
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(irSensors[i].name);
    Serial.print(" bin: ");
    Serial.print(irSensors[i].emptyPercent);
    Serial.println("% filled");
  }
  Serial.println("======================================\n");
  
  // Return to origin
  delay(2000);
  Serial.println("→ Returning servo to origin position...");
  moveServoGradually(SERVO_ORIGIN);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Skipping Firebase log.");
    return;
  }
  // Firebase
  delay(200);

  WiFiClientSecure client;
  client.setInsecure();
  
  HTTPClient http;

  String url = "https://smartdustbin-c2ade-default-rtdb.asia-southeast1.firebasedatabase.app/logs/" + String(time(nullptr)) + ".json";
  
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");

  // Time
  // unsigned long t = millis()/1000;
  // String timeStr = String(t);
  // unsigned long t = millis()/1000;
  // int minutes = t / 60;
  // int seconds = t % 60;
  //String timeStr = String(minutes) + ":" + (seconds < 10 ? "0" : "") + String(seconds);
  
  String timeStr = getDateTime();

  String wetStatus = digitalRead(IR_WET_PIN) ? "EMPTY" : "FILLED";
  String dryStatus = digitalRead(IR_DRY_PIN) ? "EMPTY" : "FILLED";
  String airStatus = smokeDetected ? "CLEAN" : "SMOKE";

  String json = "{";
  json += "\"type\":\"" + wasteType + "\",";
  json += "\"wet\":\"" + wetStatus + "\",";
  json += "\"dry\":\"" + dryStatus + "\",";
  json += "\"air\":\"" + airStatus + "\",";
  json += "\"time\":\"" + timeStr + "\"";
  json += "}";
  
  // Send
  int httpResponseCode = http.POST(json);

  Serial.print("Firebase Response: ");
  if (httpResponseCode > 0) {
    Serial.println("Data sent successfully");

  } else {
    Serial.print("Error sending data: ");
    Serial.println(httpResponseCode);
  }
  http.end();

  Serial.println("System ready for next object...\n");
}

// Function to move servo gradually so you can see the direction
void moveServoGradually(int targetAngle) {
  int currentAngle = myServo.read();
  
  if (targetAngle > currentAngle) {
    // Moving clockwise
    for (int angle = currentAngle; angle <= targetAngle; angle += 5) {
      myServo.write(angle);
      delay(50);
    }
  } else {
    // Moving anticlockwise
    for (int angle = currentAngle; angle >= targetAngle; angle -= 5) {
      myServo.write(angle);
      delay(50);
    }
  }
  
  // Ensure final position
  myServo.write(targetAngle);
  delay(500);
}

int measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  
  if (duration == 0) return 999;
  
  distance = duration * 0.034 / 2;
  return distance;
}

void printReadings() {
  unsigned long currentTime = millis() / 1000;
  
  int minutes = currentTime / 60;
  int seconds = currentTime % 60;
  
  Serial.print(minutes);
  Serial.print(":");
  if (seconds < 10) Serial.print("0");
  Serial.print(seconds);
  Serial.print("s\t");
  
  if (distance > 100) {
    Serial.print("---\t\t");
  } else {
    Serial.print(distance);
    Serial.print("cm\t\t");
  }
  
  moistureValue = analogRead(SOIL_SENSOR);
  Serial.print(moistureValue);
  Serial.print("\t\t");
  
  if (moistureValue > MOISTURE_THRESHOLD) {
    Serial.print("DRY\t\t");
  } else {
    Serial.print("WET\t\t");
  }
  
  // Show current servo position
  int servoPos = myServo.read();
  Serial.print(servoPos);
  Serial.print("°\t\t");
  
  // Show empty percentages using loop
  for (int i = 0; i < NUM_SENSORS; i++) {
    Serial.print(irSensors[i].emptyPercent);
    Serial.print("%\t\t");
  }
  
  // Show smoke sensor status with buzzer indicator
  int smokeState = digitalRead(SMOKE_SENSOR_PIN);
  if (smokeState == HIGH) {
    if (buzzerActive) {
      Serial.print("💨SMOKE🔊\t");
    } else {
      Serial.print("💨SMOKE\t");
    }
  } else {
    Serial.print("✓CLEAN\t\t");
  }
  
  if (distance < DISTANCE_THRESHOLD && distance > 0) {
    Serial.println("OBJECT NEAR");
  } else {
    Serial.println("Waiting...");
  }
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Smart Dustbin Dashboard</title>";

  // CSS
  html += "<style>";
  html += "body{font-family:Arial;background:#f4f6f8;margin:0;text-align:center;}";
  html += "h1{background:#2c3e50;color:white;padding:15px;margin:0;}";
  html += ".container{display:flex;justify-content:center;flex-wrap:wrap;margin-top:20px;align-items:flex-start;}";
  html += ".card{background:white;padding:20px;margin:15px;border-radius:12px;";
  html += "box-shadow:0 4px 10px rgba(0,0,0,0.1);width:240px;min-height:80px;}";
  html += ".air-card{min-height:80px;}";
  html += ".air-text{display:flex;align-items:center;justify-content:center;text-align:center;}";
  html += ".title{font-size:20px;font-weight:bold;}";
  html += ".value{font-size:28px;margin-top:10px;min-height:40px;}";
  html += ".green{color:green;} .red{color:red;} .blue{color:#2980b9;} .brown{color:brown;}";
  html += "table td, table th{padding:10px;border-bottom:1px solid #ddd;}";
  html += "</style>";

  // JavaScript (AJAX)
  html += "<script>";
  html += "function updateData(){";
  html += "fetch('/data')";
  html += ".then(res=>res.json())";
  html += ".then(data=>{";

  // Wet Waste
  html += "if(data.wet==1){";
  html += "document.getElementById('wet').innerHTML='FILLED';";
  html += "document.getElementById('wet').className='value red';";
  html += "}else{";
  html += "document.getElementById('wet').innerHTML='EMPTY';";
  html += "document.getElementById('wet').className='value green';";
  html += "}";

  // Dry Waste
  html += "if(data.dry==1){";
  html += "document.getElementById('dry').innerHTML='FILLED';";
  html += "document.getElementById('dry').className='value red';";
  html += "}else{";
  html += "document.getElementById('dry').innerHTML='EMPTY';";
  html += "document.getElementById('dry').className='value green';";
  html += "}";

  // Detected Waste
  html += "if(data.waste=='N/A'){";
  html += "document.getElementById('waste').innerHTML='N/A';";
  html += "document.getElementById('waste').className='value red';";
  html += "}else if(data.waste=='WET'){";
  html += "document.getElementById('waste').innerHTML='WET';";
  html += "document.getElementById('waste').className='value blue';";
  html += "}else if(data.waste=='DRY'){";
  html += "document.getElementById('waste').innerHTML='DRY';";
  html += "document.getElementById('waste').className='value brown';";
  html += "}";

  html += "if(data.smoke==1){";
  html += "document.getElementById('air').innerHTML='🚨<br>SMOKE DETECTED';";
  html += "document.getElementById('air').className='value red';";
  html += "}else{";
  html += "document.getElementById('air').innerHTML='CLEAN ✅';";
  html += "document.getElementById('air').className='value green';";
  html += "}";

  html += "}).catch(err=>console.log(err));}";

  html += "function updateAnalytics(){";
  html += "fetch('/analytics').then(res=>res.json()).then(data=>{";

  html += "let text = 'Total: ' + data.total + '<br>';";
  html += "text += 'Wet: ' + data.wet.toFixed(1) + '%<br>';";
  html += "text += 'Dry: ' + data.dry.toFixed(1) + '%<br>';";
  html += "text += 'Smoke Events: ' + data.smoke;";

  html += "document.getElementById('analytics').innerHTML = text;";
  html += "});}";

  html += "function updateLogs(){";
  html += "fetch('https://smartdustbin-c2ade-default-rtdb.asia-southeast1.firebasedatabase.app/logs.json')";
  html += ".then(res=>res.json())";
  html += ".then(data=>{";

  html += "if(!data){";
  html += "document.getElementById('logTable').innerHTML = '<tr><td colspan=\"5\">No logs yet</td></tr>'; return;}";

  html += "let rows='';";

  html += "let keys = Object.keys(data).reverse();";

  html += "for(let i=0;i<Math.min(keys.length,10);i++){";

  html += "let outer = data[keys[i]];";
  html += "let innerKeys = Object.keys(outer);";
  html += "let log = outer[innerKeys[0]];";
  
  // Safety check
  html += "if(!log) continue;";
  html += "let time = log.time;";

  // Row start
  html += "if(i==0){rows += '<tr style=\"background:#e8f5e9\">';}";
  html += "else{rows += '<tr>'; }";

  // Time
  html += "rows += '<td>'+time+'</td>';";

  // Type
  html += "if(log.type=='WET'){rows += '<td style=\"color:blue\">WET</td>';}";
  html += "else{rows += '<td style=\"color:brown\">DRY</td>'; }";

  // Wet
  html += "if(log.wet=='FILLED'){";
  html += "rows += '<td style=\"color:red\">FILLED</td>';}";
  html += "else{";
  html += "rows += '<td style=\"color:green\">EMPTY</td>'; }";

  // Dry
  html += "if(log.dry=='FILLED'){";
  html += "rows += '<td style=\"color:red\">FILLED</td>';}";
  html += "else{";
  html += "rows += '<td style=\"color:green\">EMPTY</td>'; }";

  // Air
  html += "if(log.air=='SMOKE'){rows += '<td style=\"color:red\">SMOKE</td>';}";
  html += "else{rows += '<td style=\"color:green\">CLEAN</td>'; }";

  // Close row
  html += "rows += '</tr>';";

  html += "}";

  // Inject table
  html += "let table = document.getElementById('logTable');";
  html += "if(table){ table.innerHTML = rows; }";
  html += "else{ console.log('Table not found'); }";

  html += "})";
  html += ".catch(err=>console.log(err));";
  html += "}";
  
  html += "setInterval(updateData,1000);";
  html += "setInterval(updateAnalytics,2000);";
  html += "setInterval(updateLogs,2000);";
  html += "</script>";

  html += "</head><body onload='setTimeout(updateData(),1000); updateAnalytics(); updateLogs()';>";

  html += "<h1>Smart Dustbin Dashboard</h1>";

  html += "<div class='container'>";

  // Wet Waste
  html += "<div class='card'>";
  html += "<div class='title'>Wet Waste</div>";
  html += "<div id='wet' class='value'>--</div>";
  html += "</div>";

  // Dry Waste
  html += "<div class='card'>";
  html += "<div class='title'>Dry Waste</div>";
  html += "<div id='dry' class='value'>--</div>";
  html += "</div>";

  // Waste Type
  html += "<div class='card'>";
  html += "<div class='title'>Detected Waste</div>";
  html += "<div id='waste' class='value blue'>--</div>";
  html += "</div>";

  // Air Quality
  html += "<div class='card air-card'>";
  html += "<div class='title'>Air Quality</div>";
  html += "<div id='air' class='value air-text'>--</div>";
  html += "</div>";

  //Analytics
  html += "<div class='card'>";
  html += "<div class='title'>Analytics</div>";
  html += "<div id='analytics' class='value'>--</div>";
  html += "</div>";

  html += "</div>";

  html += "<h2 style='margin-top:30px;'>Waste Log (Cloud)</h2>";

  html += "<table style='margin:auto;width:90%;border-collapse:collapse;background:white;box-shadow:0 4px 10px rgba(0,0,0,0.1);'>";

  html += "<tr style='background:#2c3e50;color:white;'>";
  html += "<th>Time</th><th>Type</th><th>Wet</th><th>Dry</th><th>Air</th>";
  html += "</tr>";

  html += "<tbody id='logTable'></tbody>";
  html += "</table>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}