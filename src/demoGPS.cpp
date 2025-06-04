// initial demo code from python code for G2 SIM868 GSM/GPRS/GNSS Module // because G2 is obsolete will try to use only sound GPRS
#define UART_PORT Serial1
#define PWR_EN 14
#define LED_PIN 25
#define APN "CMNET"

void ledBlink() {
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
  digitalWrite(LED_PIN, LOW);
}

void powerOnOff() {
  digitalWrite(PWR_EN, HIGH);
  delay(2000);
  digitalWrite(PWR_EN, LOW);
}

String waitRespInfo(unsigned long timeout = 2000) {
  unsigned long start = millis();
  String response = "";
  while (millis() - start < timeout) {
    while (UART_PORT.available()) {
      response += (char)UART_PORT.read();
    }
  }
  Serial.println(response);
  return response;
}

bool sendAT(const String& cmd, const String& expected, unsigned long timeout = 2000) {
  UART_PORT.println(cmd);
  String response = "";
  unsigned long start = millis();
  while (millis() - start < timeout) {
    while (UART_PORT.available()) {
      response += (char)UART_PORT.read();
    }
    if (response.indexOf(expected) != -1) {
      Serial.println(response);
      return true;
    }
  }
  Serial.println(cmd + " back:\t" + response);
  return false;
}

void checkStart() {
  while (true) {
    UART_PORT.println("ATE1");
    delay(2000);
    UART_PORT.println("AT");
    String resp = waitRespInfo();
    if (resp.indexOf("OK") != -1) {
      Serial.println("SIM868 is ready");
      break;
    } else {
      powerOnOff();
      Serial.println("SIM868 is starting up, please wait...");
      delay(8000);
    }
  }
}

void checkNetwork() {
  for (int i = 0; i < 3; i++) {
    if (sendAT("AT+CGREG?", "0,1")) {
      Serial.println("SIM868 is online");
      break;
    } else {
      Serial.println("SIM868 is offline, waiting...");
      delay(5000);
    }
  }

  sendAT("AT+CPIN?", "OK");
  sendAT("AT+CSQ", "OK");
  sendAT("AT+COPS?", "OK");
  sendAT("AT+CGATT?", "OK");
  sendAT("AT+CGDCONT?", "OK");
  sendAT("AT+CSTT?", "OK");
  sendAT("AT+CSTT=\"" + String(APN) + "\"", "OK");
  sendAT("AT+CIICR", "OK");
  sendAT("AT+CIFSR", "OK");
}

void getGPSInfo() {
  Serial.println("Start GPS session...");
  sendAT("AT+CGNSPWR=1", "OK");
  delay(2000);
  int count = 0;
  for (int i = 0; i < 10; i++) {
    UART_PORT.println("AT+CGNSINF");
    String response = waitRespInfo();
    if (response.indexOf(",,,") != -1) {
      Serial.println("GPS is not ready");
      if (i >= 9) {
        Serial.println("GPS failed, check antenna");
        sendAT("AT+CGNSPWR=0", "OK");
      } else {
        delay(2000);
        continue;
      }
    } else {
      if (count++ <= 3) {
        Serial.println("GPS info:");
        Serial.println(response);
      } else {
        sendAT("AT+CGNSPWR=0", "OK");
        break;
      }
    }
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(PWR_EN, OUTPUT);
  digitalWrite(PWR_EN, LOW);

  Serial.begin(115200);        // USB serial
  UART_PORT.setTX(0);          // GPIO0
  UART_PORT.setRX(1);          // GPIO1
  UART_PORT.begin(115200);     // SIM868 default

  Serial.println("Starting SIM868 Test...");

  checkStart();
  checkNetwork();
  getGPSInfo();
}

void loop() {
  // You could place additional operations here
}
