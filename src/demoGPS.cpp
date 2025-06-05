// demo code from python code for G2 SIM868 GSM/GPRS/GNSS Module // because G2 is obsolete use only sound GPRS

// For SIM868
#define UART_PORT_SIM868 Serial1
#define UART_PORT_SIM868TX 0
#define UART_PORT_SIM868RX 1
#define PWR_EN_SIM868 14

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

void setup() {

  // INIT SIM868
  pinMode(PWR_EN_SIM868, OUTPUT);
  digitalWrite(PWR_EN_SIM868, LOW);

  //SerialUSBSIM868.begin(9600);        // USB serial
  // while (!SerialUSBSIM868)
  //  ;
  //SerialUSBSIM868.println("Open UART_PORT_SIM868 ...");

  UART_PORT_SIM868.setTX(UART_PORT_SIM868TX);  // GPIO0
  UART_PORT_SIM868.setRX(UART_PORT_SIM868RX);  // GPIO1
  UART_PORT_SIM868.begin(115200);              // SIM868 default

  checkStartSIM868();

  String returnedString = getGPSInfoSIM868();
}

void loop() {
  // You could place additional operations here
}
