#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h> // To store Wi-Fi credentials

const char* ssid = "ESP32_AP";
const char* password = "12345678";

WebServer server(80);
Preferences preferences;

const char* mainPage = 
  "<!DOCTYPE html>"
  "<html>"
  "<head>"
  "<title>ESP32 Home</title>"
  "<style>"
  "body {font-family: Arial, sans-serif; background-color: #1e1e1e; color: #f4f4f4; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0;}"
  "h1 {color: #ff5722;}"
  "a {color: #ff5722; text-decoration: none; padding: 10px 20px; background-color: #333; border-radius: 5px; transition: background-color 0.3s ease;}"
  "a:hover {background-color: #ff5722; color: #1e1e1e;}"
  "</style>"
  "</head>"
  "<body>"
  "<h1>Welcome to ESP32</h1>"
  "<p>This is the main page.</p>"
  "<p><a href='/update'>Go to Firmware Update</a></p>"
  "</body>"
  "</html>";

const char* updatePage = 
  "<!DOCTYPE html>"
  "<html>"
  "<head>"
  "<title>ESP32 Firmware Update</title>"
  "<style>"
  "body {font-family: Arial, sans-serif; background-color: #1e1e1e; color: #f4f4f4; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0;}"
  "h1 {color: #ff5722;}"
  "#upload_form {display: flex; flex-direction: column; align-items: center;}"
  "input[type='file'] {padding: 10px;}"
  "input[type='submit'] {padding: 10px 20px; background-color: #ff5722; color: #1e1e1e; border: none; border-radius: 5px; cursor: pointer; transition: background-color 0.3s ease;}"
  "input[type='submit']:hover {background-color: #333; color: #ff5722;}"
  "progress {width: 100%; height: 20px; border-radius: 5px; overflow: hidden; color: #ff5722;}"
  "progress::-webkit-progress-bar {background-color: #333;}"
  "progress::-webkit-progress-value {background-color: #ff5722;}"
  "progress::-moz-progress-bar {background-color: #ff5722;}"
  "#status {margin-top: 20px; font-weight: bold;}"
  "</style>"
  "</head>"
  "<body>"
  "<h1>ESP32 Firmware Update</h1>"
  "<form id='upload_form'>"
  "<input type='file' name='update' id='fileInput' accept='.bin' required><br><br>"
  "<input type='submit' value='Update Firmware'><br><br>"
  "<progress id='progressBar' value='0' max='100'></progress>"
  "<p id='status'></p>"
  "</form>"
  "<script>"
  "document.getElementById('upload_form').onsubmit = function(event) {"
  "  event.preventDefault();"
  "  var fileInput = document.getElementById('fileInput');"
  "  var file = fileInput.files[0];"
  "  if (!file) return;"
  "  var formData = new FormData();"
  "  formData.append('update', file);"
  "  var xhr = new XMLHttpRequest();"
  "  xhr.open('POST', '/update', true);"
  "  xhr.upload.onprogress = function(event) {"
  "    if (event.lengthComputable) {"
  "      var percentComplete = Math.round((event.loaded / event.total) * 100);"
  "      document.getElementById('progressBar').value = percentComplete;"
  "    }"
  "  };"
  "  xhr.onload = function() {"
  "    if (xhr.status == 200) {"
  "      document.getElementById('status').innerHTML = 'Done!';"
  "      setTimeout(function() {"
  "        document.getElementById('status').innerHTML += ' Rebooting...';"
  "      }, 2000);"
  "    } else {"
  "      document.getElementById('status').innerHTML = 'Upload Failed. Try Again.';"
  "    }"
  "  };"
  "  document.getElementById('status').innerHTML = 'Uploading...';"
  "  xhr.send(formData);"
  "};"
  "</script>"
  "</body>"
  "</html>";

const char* wifiConfigPage = 
  "<!DOCTYPE html>"
  "<html>"
  "<head>"
  "<title>ESP32 Wi-Fi Configuration</title>"
  "<style>"
  "body {font-family: Arial, sans-serif; background-color: #1e1e1e; color: #f4f4f4; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0;}"
  "h1 {color: #ff5722;}"
  "#wifi_form {display: flex; flex-direction: column; align-items: center;}"
  "input[type='text'], input[type='password'] {padding: 10px; margin-bottom: 10px; width: 300px; border-radius: 5px; border: 1px solid #333; background-color: #333; color: #f4f4f4;}"
  "input[type='submit'] {padding: 10px 20px; background-color: #ff5722; color: #1e1e1e; border: none; border-radius: 5px; cursor: pointer; transition: background-color 0.3s ease;}"
  "input[type='submit']:hover {background-color: #333; color: #ff5722;}"
  "#status {margin-top: 20px; font-weight: bold;}"
  "</style>"
  "</head>"
  "<body>"
  "<h1>ESP32 Wi-Fi Configuration</h1>"
  "<form id='wifi_form' action='/wifi' method='POST'>"
  "<input type='text' name='ssid' placeholder='Enter Wi-Fi SSID' required><br>"
  "<input type='password' name='password' placeholder='Enter Wi-Fi Password'><br>"
  "<input type='submit' value='Save & Connect'>"
  "</form>"
  "<p id='status'></p>"
  "</body>"
  "</html>";


void handleUpdate() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if(!Update.begin(UPDATE_SIZE_UNKNOWN)){ //start with max available size
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    /* flashing firmware to ESP*/
    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if(Update.end(true)){ //true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  // Start ESP32 in Access Point mode to allow configuration
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());

  // Load saved Wi-Fi credentials if available
  preferences.begin("wifi-config", false);
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");

  if (!savedSSID.isEmpty()) {
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
      Serial.print("Connected to ");
      Serial.println(savedSSID);
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Failed to connect.");
    }
  }
  preferences.end();

  // Serve the main page at http://192.168.4.1/
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", mainPage);
  });

  // Serve the update page at http://192.168.4.1/update
  server.on("/update", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", updatePage);
  });

  // Serve the Wi-Fi configuration page at http://192.168.4.1/wifi
  server.on("/wifi", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", wifiConfigPage);
  });

  // Handle Wi-Fi configuration submission via POST at http://192.168.4.1/wifi
  server.on("/wifi", HTTP_POST, []() {
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    // Save the credentials in preferences
    preferences.begin("wifi-config", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    // Attempt to connect to the new Wi-Fi network
    WiFi.disconnect();
    WiFi.begin(ssid.c_str(), password.c_str());

    server.sendHeader("Connection", "close");
    server.send(200, "text/html", "<html><body><h1>Wi-Fi Configuration</h1><p>Connecting to network...</p></body></html>");

    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
      Serial.println("Connected to Wi-Fi successfully.");
      delay(2000);
      ESP.restart();  // Restart ESP to apply new IP from the connected network
    } else {
      Serial.println("Failed to connect to Wi-Fi.");
    }
  });

  // Handle the OTA Update route
  server.on("/update", HTTP_POST, [](){
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    delay(1000);
    ESP.restart();
  }, handleUpdate);

  server.begin();
}

void loop() {
  server.handleClient();
  
  //Blink so you know it works
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(500);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(500);
}