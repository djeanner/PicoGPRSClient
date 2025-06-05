#include "secrets.h"


// MAIN MACROS
#define GPS_MODULE_DATA
#define AIR780_MODULE

#ifdef GPS_MODULE_DATA            // For SIM868 Using powerpins 36-40, 1,2,6,7,19,22 one groud (18,23)
#define UART_PORT_SIM868 Serial1  // may need to change to Serial1 if change port (see below)
#define UART_PORT_SIM868TX 0      // GPIO0 pin1
#define UART_PORT_SIM868RX 1      // GPIO1 pin2
#define PWR_EN_SIM868 14          // GPI14 pint 19
#endif

#ifdef AIR780_MODULE  // For AIR780
// for pi pico with a gravity uart (AIR7880EU chip) 4G CAT I mocules V1.0 connected with
// gravity board / pi pico
// GND black/ pin 37
// VCC red  / pin 40
// Rx blue  / pin 1 GPIO0 Serial1 or pin 6 GPIO4 Serial2 see setTX
// Tx green / pin 2 GPIO1 Serial1 or pin 7 GPIO5 Serial2 see setRX
#define AIR780 Serial2  // Serial1 or Serial2
#define AIR780SER_TX 4  // 0 GPIO0 pin 1 for Serial1 or 4 GPIO4 pin 6 for Serial2
#define AIR780SER_RX 5  // 1 GPIO0 pin 2 for Serial1 or 5 GPIO4 pin 7 for Serial2
#endif

int setNumber = 100;  // sent to server
bool verbose = true;  // Set to false to reduce serial prints
unsigned long counter = 0;
const String deviceName = "AIR780";

#define ENABLE_BLINK  // Comment this line to disable LED blinking
// #define ENABLE_USB_SERIAL  // Comment this line to disable terminal connection to usb serial port
#define SAVE_SET_EEPROM  // disable to avoid using flash memory

#ifdef SAVE_SET_EEPROM
#include <EEPROM.h>
#endif

#ifdef ENABLE_BLINK
const int ledPinBlink = 25;  // Onboard LED pin on Raspberry Pi Pico
#endif

#ifdef ENABLE_USB_SERIAL
#define SerUSB Serial  // Renamed to facilitate and debug (Serial is the USB ser port)
#endif

#ifdef GPS_MODULE_DATA  // For SIM868

class GpsInfo {
public:
  int runStatus = 0;
  int fixStatus = 0;
  String utc;
  double lat = 0.0;
  double lon = 0.0;
  double alt = 0.0;
  double speed = 0.0;
  double course = 0.0;
  int fixMode = 0;
  double hdop = 0.0;
  double pdop = 0.0;
  double vdop = 0.0;
  int satsInView = 0;
  int satsUsed = 0;
  int glonassUsed = 0;
  int cn0max = 0;
  int numberValidGPSdata = 0;
  void fromCGNSINF(const String& infoLine) {
    String fields[22];
    int start = 0, end = 0, index = 0;
    while ((end = infoLine.indexOf(',', start)) != -1 && index < 21) {
      fields[index++] = infoLine.substring(start, end);
      start = end + 1;
    }
    fields[index] = infoLine.substring(start);  // last field

    runStatus = fields[0].toInt();
    fixStatus = fields[1].toInt();
    utc = fields[2];
    lat = fields[3].toDouble();
    lon = fields[4].toDouble();
    alt = fields[5].toDouble();
    speed = fields[6].toDouble();
    course = fields[7].toDouble();
    fixMode = fields[8].toInt();
    hdop = fields[10].toDouble();
    pdop = fields[11].toDouble();
    vdop = fields[12].toDouble();
    satsInView = fields[14].toInt();
    satsUsed = fields[15].toInt();
    glonassUsed = fields[16].toInt();
    cn0max = fields[19].toInt();
  }

  String toJson() const {
    return String("{") + "\"runStatus\":" + String(runStatus) + "," + "\"fixStatus\":" + String(fixStatus) + "," + "\"utc\":\"" + utc + "\"," + "\"lat\":" + String(lat, 6) + "," + "\"lon\":" + String(lon, 6) + "," + "\"alt\":" + String(alt, 3) + "," + "\"speed\":" + String(speed, 2) + "," + "\"course\":" + String(course, 1) + "," + "\"fixMode\":" + String(fixMode) + "," + "\"hdop\":" + String(hdop, 1) + "," + "\"pdop\":" + String(pdop, 1) + "," + "\"vdop\":" + String(vdop, 1) + "," + "\"satsInView\":" + String(satsInView) + "," + "\"satsUsed\":" + String(satsUsed) + "," + "\"glonassUsed\":" + String(glonassUsed) + "," + "\"cn0max\":" + String(cn0max) + "}";
  }

  String toAmpersandString() const {
    return "runStatus=" + String(runStatus) + "&" + "fixStatus=" + String(fixStatus) + "&" + "utc=" + utc + "&" + "lat=" + String(lat, 6) + "&" + "lon=" + String(lon, 6) + "&" + "alt=" + String(alt, 3) + "&" + "speed=" + String(speed, 2) + "&" + "course=" + String(course, 1) + "&" + "fixMode=" + String(fixMode) + "&" + "hdop=" + String(hdop, 1) + "&" + "pdop=" + String(pdop, 1) + "&" + "vdop=" + String(vdop, 1) + "&" + "satsInView=" + String(satsInView) + "&" + "satsUsed=" + String(satsUsed) + "&" + "glonassUsed=" + String(glonassUsed) + "&" + "cn0max=" + String(cn0max);
  }

  bool isValidFix() const {
    return fixStatus == 1;
  }
};

void powerOnOffSIM868() {
  digitalWrite(PWR_EN_SIM868, HIGH);
  delay(2000);
  digitalWrite(PWR_EN_SIM868, LOW);
}

String waitRespInfoSIM868(unsigned long timeout = 2000) {
  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (UART_PORT_SIM868.available()) {
      response += (char)UART_PORT_SIM868.read();
    }
  }
  //SerialUSBSIM868.println(response);
  return response;
}

bool sendATSIM868(const String& cmd, const String& expected, unsigned long timeout = 2000) {
  UART_PORT_SIM868.println(cmd);
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (UART_PORT_SIM868.available()) {
      response += (char)UART_PORT_SIM868.read();
    }
    if (response.indexOf(expected) != -1) {
      //SerialUSBSIM868.println(response);
      return true;
    }
  }
  //SerialUSBSIM868.println(cmd + " back:\t" + response);
  return false;
}

void checkStartSIM868() {
  while (true) {
    UART_PORT_SIM868.println("ATE1");
    delay(2000);
    UART_PORT_SIM868.println("AT");
    String resp = waitRespInfoSIM868();
    if (resp.indexOf("OK") != -1) {
      //SerialUSBSIM868.println("SIM868 is ready");
      break;
    } else {
      powerOnOffSIM868();
      //SerialUSBSIM868.println("SIM868 is starting up, please wait...");
      delay(8000);
    }
  }
}

String getGPSInfoSIM868() {
  //SerialUSBSIM868.println("Start GPS session...");

  if (!sendATSIM868("AT+CGNSPWR=1", "OK")) {
    //SerialUSBSIM868.println("Failed to power on GPS module.");
    return "";
  }

  delay(2000);
  const int maxRetries = 10;
  int numberValidGPSdata = 0;
  GpsInfo bestFix;
  float bestHdop = 99.9;

  for (int i = 0; i < maxRetries; ++i) {
    UART_PORT_SIM868.println("AT+CGNSINF");
    String response = waitRespInfoSIM868();
    int idx = response.indexOf("+CGNSINF:");
    if (idx != -1) {
      String dataLine = response.substring(idx + 9);
      dataLine.trim();

      GpsInfo currentFix;
      currentFix.fromCGNSINF(dataLine);

      if (currentFix.isValidFix() && currentFix.hdop < bestHdop) {
        bestFix = currentFix;
        bestHdop = currentFix.hdop;
        numberValidGPSdata++;
        bestFix.numberValidGPSdata = numberValidGPSdata;
        //SerialUSBSIM868.println("GPS quality improved >>>");
      } else {
        //SerialUSBSIM868.println("GPS quality NOT improved >>>");
      }
    }

    delay(2000);
  }

  sendATSIM868("AT+CGNSPWR=0", "OK");

  if (bestFix.isValidFix()) {
    return bestFix.toAmpersandString();  // or toJson() depending on use case
  } else {
    //SerialUSBSIM868.println("❌ No valid GPS fix found.");
    return "";
  }
}

#endif


void logToSerial(const char* msg, const bool endOfLine = true) {
#ifdef ENABLE_USB_SERIAL
  if (verbose) {
    if (endOfLine) {
      SerUSB.println(msg);
    } else {
      SerUSB.print(msg);
    }
  }
#endif
}

void shortBlinks(int n, bool fastProblem = false) {
#ifdef ENABLE_BLINK
  const int numberCylcles = fastProblem ? 5 : 2;
  for (int j = 0; j < numberCylcles; j++) {
    for (int i = 0; i < n; i++) {
      const int durationOn = fastProblem ? 50 : 200;    // LED on in ms
      const int durationOff = fastProblem ? 300 : 300;  // LED off in ms
      digitalWrite(ledPinBlink, HIGH);
      delay(durationOn);
      digitalWrite(ledPinBlink, LOW);
      delay(durationOff);
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

  String request = String("POST ") + path + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Content-Type: application/x-www-form-urlencoded\r\n" + "Content-Length: " + String(strlen(payload)) + "\r\n" + "Connection: close\r\n\r\n" + payload;

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

  if (!fullMessage.endsWith("CLOSED\r\n")) {
    disconnectTCP();
  }
}

void shutdownModem() {
  logToSerial("Shutting down modem...");

  sendCommand("AT+CIPCLOSE", 1000);  // Close TCP
  sendCommand("AT+CIPSHUT", 2000);   // Shut IP stack
  sendCommand("AT+CGATT=0", 1000);   // Detach from GPRS
  sendCommand("AT+CFUN=0", 1000);    // Minimum functionality mode
  shortBlinks(2);                    // Feedback
  logToSerial("Modem in low-power mode.");
}

void wakeModem() {
  logToSerial("Waking modem...");

  sendCommand("AT+CFUN=1", 3000);  // Restore full functionality
  delay(2000);                     // Give time to come up

  if (!waitForReady()) {
    logToSerial("Modem failed to wake up.");
    shortBlinks(4, true);
  } else {
    configureAPN("internet");  // Your APN here
    attachGPRS();
    logToSerial("Modem is ready after wake.");
  }
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
  logToSerial("Returned data...");

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
  flushSerialInput();  // ✅ Wipe out any leftover serial input

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

  //String httpRequest = String("GET / HTTP/1.1\r\nHost: \"") + String(host) + String("\"\r\nConnection: keep-alive\r\n\r\n");//close
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
}

void disconnectTCP() {
  sendCommand("AT+CIPCLOSE", 1000);
  delay(500);
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
    if (creg.indexOf("+CREG: 0,5") >= 0 && cgatt.indexOf("+CGATT: 1") >= 0) {
      logToSerial("Module registered and GPRS attached with roaming.");
      return true;
    }
    if (creg.indexOf("+CREG: 0,0") >= 0) {
      logToSerial("Not registered, not currently searching for a new operator");
    }
    if (creg.indexOf("+CREG: 0,2") >= 0) {
      logToSerial("Not registered, but searching for a new operator");
    }
    if (creg.indexOf("+CREG: 0,3") >= 0) {
      logToSerial("Registration denied");
    }
    if (creg.indexOf("+CREG: 0,4") >= 0) {
      logToSerial("Unknown");
    }
    /*
stat value	Meaning
0	Not registered, not currently searching for a new operator
1	Registered, home network
2	Not registered, but searching for a new operator
3	Registration denied
4	Unknown
5	Registered, roaming
*/
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

void sendDataOnce(const char* data) {
  if (!waitForReady()) {
    shortBlinks(4, true);  // AIR780 problem
    return;
  }
  configureAPN("internet");
  attachGPRS();

  const int maxAttempts = 3;
  bool connected = false;
  for (int attempt = 1; attempt <= maxAttempts; attempt++) {
    logToSerial("[Attempt ", false);
    logToSerial(String(attempt).c_str(), false);
    logToSerial("/", false);
    logToSerial(String(maxAttempts).c_str(), false);
    logToSerial("] Connecting...");
    if (connectTCP(host, port)) {
      connected = true;
      break;
    }
    delay(2000);
  }

  if (connected) {
    sendHTTPRequest(host, "/submit", data);
  } else {
    logToSerial("All connection attempts failed. Giving up.");
    shortBlinks(5, true);
  }
}

void setup() {

#ifdef GPS_MODULE_DATA  // For SIM868 INIT SIM868
  pinMode(PWR_EN_SIM868, OUTPUT);
  digitalWrite(PWR_EN_SIM868, LOW);

  UART_PORT_SIM868.setTX(UART_PORT_SIM868TX);  // GPIO0
  UART_PORT_SIM868.setRX(UART_PORT_SIM868RX);  // GPIO1
  UART_PORT_SIM868.begin(115200);              // SIM868 default
  sendATSIM868("AT+SLED=0", "OK");             // Disable status LED set to 1 to set back

  // checkStartSIM868();
  powerOnOffSIM868();  // Pulses PWRKEY

#endif

//////////////////////////////////////// save data in EEPROM // start
#ifdef SAVE_SET_EEPROM
  EEPROM.begin(512);  // Initialize 512 bytes
  const int addr = 0;
  const int magicAddr = addr + sizeof(int);
  const int MAGIC_VALUE = 0xABC0;

  int magic = 0;
  EEPROM.get(magicAddr, magic);

  if (magic == MAGIC_VALUE) {
    // Value is valid, increment it
    EEPROM.get(addr, setNumber);
    setNumber++;
    EEPROM.put(addr, setNumber);
  } else {
    // First time use, initialize value and set magic number
    setNumber = 0;
    EEPROM.put(addr, 0);
    EEPROM.put(magicAddr, MAGIC_VALUE);
  }
  EEPROM.commit();  // Only one commit needed
#endif
  //////////////////////////////////////// save data in EEPROM // end



#ifdef ENABLE_BLINK
  pinMode(ledPinBlink, OUTPUT);
#endif

#ifdef ENABLE_USB_SERIAL
  SerUSB.begin(9600);
  while (!SerUSB)
    ;
#endif
  AIR780.setTX(AIR780SER_TX);  // 0: GPIO0 or 4:GPIO4
  AIR780.setRX(AIR780SER_RX);  // 1: GPIO1 or 5:GPIO5
  AIR780.begin(115200);        // Default for AIR780

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
}
void loop() {
  counter++;

  String mainData = "";
  String separator = "&";
  mainData += "device=" + deviceName;
  mainData += separator;
  mainData += "setNumber=" + String(setNumber);
  mainData += separator;
  mainData += "value=" + String(counter);

#ifdef GPS_MODULE_DATA  // For SIM868
  String gpsString = getGPSInfoSIM868();
  mainData += separator;
  mainData += "GPS=" + String(gpsString);
#endif

  wakeModem();                     // Power up network stack
  sendDataOnce(mainData.c_str());  // Send to server up to 200–500 mA for AIT780EU
  shutdownModem();                 // Shut everything down ~5–10 mA for AIT780EU

  logToSerial("Sleeping for 1 min...");
  delay(60000);  // Wait 1 min (60 sec × 1000 ms)
}
