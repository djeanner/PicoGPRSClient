bool verbose = true;       // Set to false to reduce serial prints
bool testHttpBin = false;  // True to test testHttpBin
bool passthroughEnabled = false; // true, to manually send commands with usb serial port - not sure works

const char* hostTestHttpBin = "httpbin.org";
const char* host = "httpbin.org";

#define AIR780 Serial1
#define ENABLE_BLINK       // Comment this line to disable LED blinking
#define ENABLE_USB_SERIAL  // Comment this line to disable terminal connection to usb serial port

#ifdef ENABLE_BLINK
const int ledPin = 25;  // Onboard LED pin on Raspberry Pi Pico
#endif

#ifdef ENABLE_USB_SERIAL
#define SerUSB Serial  // rename to facilitate debug Serial is the USB ser port
#endif

void logToSerial(const char* msg) {
#ifdef ENABLE_USB_SERIAL
  if (verbose) {
    SerUSB.println(msg);
  }
#endif
}

void shortBlinks(int n, bool fastProblem = false) {
#ifdef ENABLE_BLINK
  const int numberCylcles = fastProblem ? 5 : 2;
  for (int j = 0; j < numberCylcles; j++) {
    for (int i = 0; i < n; i++) {
      const int durationOn = fastProblem ? 50 : 200;
      const int durationOff = fastProblem ? 80 : 300;
      digitalWrite(ledPin, HIGH);
      delay(durationOn);  // LED on for 100ms
      digitalWrite(ledPin, LOW);
      delay(durationOff);  // LED off for 100ms
    }
    delay(1000);
  }
#endif
}


String sendCommand(const char* cmd, unsigned long timeout = 1000) {
  String response = "";
  logToSerial((String(">> ") + cmd).c_str());
  AIR780.println(cmd);
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (AIR780.available()) {
      char c = AIR780.read();
      response += c;
#ifdef ENABLE_USB_SERIAL
      if (verbose) { SerUSB.write(c); }
#endif
    }
  }
#ifdef ENABLE_USB_SERIAL
  if (verbose) { logToSerial(""); }
#endif

  return response;
}

void sendHTTPRequest(const char* host, const char* path, const char* payload) {
  flushSerialInput();  // Clear any buffered input
  sendCommand("AT+CIPSEND", 2000);
  delay(500);

  String request =
    String("POST ") + path + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Content-Type: application/x-www-form-urlencoded\r\n" + "Content-Length: " + 
    String(strlen(payload)) + "\r\n" + "Connection: close\r\n\r\n" + payload;

  AIR780.print(request);
  AIR780.write(0x1A);
  AIR780.flush();
#ifdef ENABLE_USB_SERIAL

  logToSerial("[Sent request:]");
  logToSerial(request.c_str());
  logToSerial("[HTTP request sent]");
#endif

  String fullMessage;
  unsigned long lastActivity = millis();
  bool connectionClosed = false;

  while (millis() - lastActivity < 15000) {
    while (AIR780.available()) {
      char c = AIR780.read();
      fullMessage += c;
      lastActivity = millis();
      if (fullMessage.endsWith("CLOSED\r\n")) {
        connectionClosed = true;
        break;
      }
    }
    if (connectionClosed) break;
  }
#ifdef ENABLE_USB_SERIAL
  SerUSB.println("[Server Response Start]");
  SerUSB.print(fullMessage);
  SerUSB.println("[Server Response End]");
#endif
  if (testHttpBin) {
    if (fullMessage.indexOf("\"device\": \"AIR780\"") >= 0) {
      logToSerial("testHttpBin true: TEST OK Found device field in json in response");
      shortBlinks(5, false);  // false for OK
    } else {
      logToSerial("testHttpBin true: TEST FAILED Did not found device field in json in response");
      shortBlinks(5, true);  // true for ERROR
    }
  }

  if (!fullMessage.endsWith("CLOSED\r\n")) {
    disconnectTCP();
  }
}


void setup() {
#ifdef ENABLE_BLINK
  pinMode(ledPin, OUTPUT);
#endif
#ifdef ENABLE_USB_SERIAL
  SerUSB.begin(9600);
  while (!SerUSB)
    ;
#endif
  AIR780.setTX(0);       // GPIO0
  AIR780.setRX(1);       // GPIO1
  AIR780.begin(115200);  // Default for AIR780

  logToSerial("Initializing AIR780 module...");
  delay(2000);

  logToSerial("Resetting modem...");
  sendCommand("AT+CIPCLOSE", 1000);  // Close TCP if open
  sendCommand("AT+CIPSHUT", 2000);   // Shut down IP stack
  sendCommand("AT+CRESET", 5000);    // Full soft reset
  delay(3000);                       // Give time to boot up

  // Optionally wait for "READY"
  for (int i = 0; i < 10; i++) {
    String resp = sendCommand("AT", 1000);
    if (resp.indexOf("OK") >= 0) {
      logToSerial("Modem is ready.");
      break;
    }
    delay(500);
  }
#ifdef ENABLE_BLINK
  shortBlinks(3);
#endif

  //runDiagnostics();

  if (!waitForReady()) return;

  configureAPN("internet");  // Replace with your real APN
  attachGPRS();

  //const char* host = "chemedata.github.io/ontologies/chemedata/playground/chemedata.owl";//"www.neverssl.com";
  
  const int port = 80;
  const int maxAttempts = 3;

  bool connected = false;
  for (int attempt = 1; attempt <= maxAttempts; attempt++) {
    logToSerial("[Attempt ");
    logToSerial(String(attempt).c_str());
    logToSerial("] Connecting...");
    if (testHttpBin) {
      if (connectTCP(hostTestHttpBin, port)) {
        connected = true;
        break;
      }
    } else {
      if (connectTCP(host, port)) {
        connected = true;
        break;
      }
    }


    delay(2000);  // wait before retry
  }

  if (connected) {


    if (testHttpBin) {
      sendHTTPRequest("httpbin.org", "/post", "device=AIR780&value=123");
    } else {
      sendHTTPRequest("httpbin.org", "/post", "device=AIR780&value=123");
    }

  } else {
    logToSerial("All connection attempts failed. Giving up.");
    shortBlinks(5, true);
  }
}
void loop() {
  passthroughSerial();  // Let user interact manually
}

void flushSerialInput() {
#ifdef ENABLE_USB_SERIAL

  while (SerUSB.available()) {
    SerUSB.read();  // Discard all incoming serial data
  }
#endif
}



bool connectTCP(const char* host, int port) {
  String cmd = String("AT+CIPSTART=\"TCP\",\"") + String(host) + "\"," + port;
  AIR780.println(cmd);
  logToSerial(String(">> " + cmd).c_str());

  String response = "";
  unsigned long start = millis();
  while (millis() - start < 10000) {
    while (AIR780.available()) {
      char c = AIR780.read();
      response += c;
#ifdef ENABLE_USB_SERIAL
      SerUSB.write(c);
#endif
    }
    if (response.indexOf("CONNECT OK") != -1) {
      logToSerial("TCP connection established.");
      return true;
    }
    if (response.indexOf("ERROR") != -1 || response.indexOf("CONNECT FAIL") != -1) {
      break;
    }
  }
  logToSerial("❌ Failed to connect.");
  return false;
}


void sendHTTPRequestOLDnotWorking(const char* host) {
  passthroughEnabled = false;  // 🛑 disable echo
  flushSerialInput();          // ✅ Wipe out any leftover serial input
  //sendCommand("AT+CIPSEND", 1000);
  //delay(500);

  while (AIR780.available()) AIR780.read();


  String request = String("AT+CIPSTART=\"TCP\",\"") + String(host) + String("\",80");  // Connect to ESP32 IP
  AIR780.println(request);
  delay(2000);  // Wait for connection

  String requestLength = "AT+CIPSEND=" + String(request.length());
  AIR780.println(requestLength);  // Send HTTP request length
  delay(1000);

  logToSerial("[Sent request:]");
  //SerUSB.println(httpRequest);
  logToSerial(request.c_str());
  logToSerial("[HTTP request sent]");

  //AIR780.println("GET / HTTP/1.1\r\nHost: 192.168.8.139\r\nConnection: keep-alive\r\n\r\n"); // HTTP GET request
  //String httpRequest = String("GET / HTTP/1.1\r\nHost: \"") + host + String("\"\r\nConnection: keep-alive\r\n\r\n");//close
  String httpRequest = String("GET / HTTP/1.1\r\nHost: \"") + String(host) + String("\"\r\nConnection: close\r\n\r\n");  //close
  AIR780.print(httpRequest);
  delay(1000);  // Wait for the request to be sent

  logToSerial("[HTTP request sent]");

  String fullMessage = "";
  unsigned long timeoutStart = millis();
  while (millis() - timeoutStart < 15000) {
    if (AIR780.available()) {
      char c = AIR780.read();
      fullMessage += c;
      timeoutStart = millis();  // reset timeout on activity
    }
  }

  logToSerial("[Server Response Start]");
  logToSerial(fullMessage.c_str());
  logToSerial("[Server Response End]");
  passthroughEnabled = true;  // ✅ enable again if needed
}


void disconnectTCP() {
  sendCommand("AT+CIPCLOSE", 1000);
  delay(500);
}

void passthroughSerial() {
  if (!passthroughEnabled) return;
#ifdef ENABLE_USB_SERIAL

  if (SerUSB.available()) {
    String command = SerUSB.readStringUntil('\n');
    command.trim();
    if (command.length() > 0) {
      AIR780.println(command);
    }
  }
#endif

  while (AIR780.available()) {
#ifdef ENABLE_USB_SERIAL
    char c = AIR780.read();
    SerUSB.write(c);
#else
    AIR780.read();
#endif
  }
}



void sendAT(const char* cmd, unsigned long timeout = 1000) {
  AIR780.println(cmd);
  if (verbose) {
    logToSerial(">> ");
    logToSerial(cmd);
  }

  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (AIR780.available()) {
#ifdef ENABLE_USB_SERIAL
      char c = AIR780.read();
      SerUSB.write(c);  // always show device reply
#else
      AIR780.read();
#endif
    }
  }
  logToSerial("");
}

void runDiagnostics() {

  sendAT("AT");
  delay(500);

  sendAT("ATI");  // Module ID
  delay(500);

  sendAT("AT+CSQ");  // Signal strength
  delay(500);

  sendAT("AT+CREG?");  // Network registration
  delay(500);
  sendAT("AT+COPS?");  // Operator name
  delay(500);

  // MCC+MNC 22802:
  //  This is Salt (Switzerland) 🇨🇭
  //  Mode 2 = manual
  //  Access tech 7 = LTE
  // So: you're registered on Salt LTE.

  //sendAT("AT+CPIN=1133");  // dj1 sim  ! NO need of setting sim code
  //delay(500);

  sendAT("AT+CGATT?");  // GPRS attached?
  delay(500);

  sendAT("AT+CGDCONT=1,\"IP\",\"internet\"");  // Replace with correct APN!
  delay(1000);

  sendAT("AT+CGACT=1,1");  // Activate PDP
  delay(1000);

  sendAT("AT+CGPADDR=1");  // Get IP address
  delay(500);
}

// Toggle verbosity via serial input
void serialEvent() {
#ifdef ENABLE_USB_SERIAL
  if (SerUSB.available()) {
    String input = SerUSB.readStringUntil('\n');
    input.trim();
    if (input.equalsIgnoreCase("verbose on")) {
      verbose = true;
      SerUSB.println("[Verbose mode ON]");
    } else if (input.equalsIgnoreCase("verbose off")) {
      verbose = false;
      SerUSB.println("[Verbose mode OFF]");
    }
  }
#endif
}

bool waitForReady() {
  for (int i = 0; i < 10; i++) {
    String creg = sendCommand("AT+CREG?");
    String cgatt = sendCommand("AT+CGATT?");

    if (creg.indexOf("+CREG: 0,1") >= 0 && cgatt.indexOf("+CGATT: 1") >= 0) {
      logToSerial("Module registered and GPRS attached.");
      return true;
    }

    delay(1000);
  }

  logToSerial("Module not ready.");
  return false;
}


void configureAPN(const char* apn) {
  String cmd = String("AT+CGDCONT=1,\"IP\",\"") + apn + "\"";
  sendCommand(cmd.c_str());
  delay(1000);
}

void attachGPRS() {
  sendCommand("AT+CGACT=1,1");
  delay(1000);
  sendCommand("AT+CGPADDR=1");
  delay(1000);
}
