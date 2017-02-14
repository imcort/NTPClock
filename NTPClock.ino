#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#define PIN            D1

#define NUMPIXELS      60

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.asia.apple.com";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE ]; //buffer to hold incoming and outgoing packets

WiFiUDP udp;

long nextCheck = 0;

char Dot_small[8] = { 5, 10,  20, 25,  35, 40,  50, 55};
char Dot_big[4] = {0, 15, 30, 45};

unsigned long setLocalTime;

uint32_t color[6] = {0x0000ff, 0x00ff00, 0xff0000, 0xffffff, 0x010100, 0x4a1300}; //Hour Minute Second Big Small Pen

ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

unsigned long sendNTPpacket(IPAddress& address) {
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

unsigned long getNetTime() {
  udp.begin(localPort);
  Serial.print("Starting UDP Local port: ");
  Serial.println(udp.localPort());

  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);

  int cb = udp.parsePacket();
  if (!cb) {
    return -1;
  }
  else {
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    const unsigned long seventyYears = 2208988800UL;
    unsigned long epoch = secsSince1900 - seventyYears;
    return epoch;
  }
  udp.stop();
}

bool calibrateLocalTime() {

  unsigned long NetTime = getNetTime();
  if (NetTime != -1) {
    setLocalTime = NetTime - (millis() / 1000) + 28800; //28800 is UTC to GMT+8 time
    Serial.println(NetTime);
    return true;
  }
  else
    return false;
}

void printTime() {
  unsigned long epoch = setLocalTime + (millis() / 1000);
  Serial.print("The time is ");       // UTC is the time at Greenwich Meridian (GMT)
  int RealHour = ((epoch  % 43200L) / 3600);
  int RealMinute = (epoch  % 3600) / 60;
  int RealSecond = epoch % 60;
  Serial.print(RealHour); // print the hour (86400 equals secs per day)
  Serial.print(':');
  Serial.print(RealMinute); // print the minute (3600 equals secs per minute)
  Serial.print(':');
  Serial.println(RealSecond); // print the second
}

void showdot() {
  if (color[4])
    for (int i = 0; i < 8; i++)
      pixels.setPixelColor(Dot_small[i], color[4]);
  if (color[5])
    if (millis() % 1000 < 500) {
      int dotnum = map(millis() % 1000, 0, 499, 20, 36);
      for (int i = dotnum; i < dotnum + 5; i++)
        pixels.setPixelColor(i, color[5]);
    }
    else {
      int dotnum = map(millis() % 1000, 500, 999, 36, 20);
      for (int i = dotnum; i < dotnum + 5; i++)
        pixels.setPixelColor(i, color[5]);
    }

  if (color[3])
    for (int i = 0; i < 4; i++)
      pixels.setPixelColor(Dot_big[i], color[3]);
}

void showHour1() {
  unsigned long epoch = setLocalTime + (millis() / 1000);
  int RealHour = ((epoch  % 43200L) / 3600) * 5;
  for (int i = 0; i < RealHour; i++) {
    pixels.setPixelColor(i, color[0]);
  }
}

void showMinute1() {
  unsigned long epoch = setLocalTime + (millis() / 1000);
  int RealMinute = (epoch  % 3600) / 60;
  for (int i = 0; i < RealMinute; i++) {
    pixels.setPixelColor(i, color[1]);
  }
}

void showSecond1() {
  unsigned long epoch = setLocalTime + (millis() / 1000);
  int RealSecond = epoch % 60;
  for (int i = 0; i < RealSecond + 1; i++) {
    pixels.setPixelColor(i, color[2]);
  }
}

void showHour2() {
  unsigned long epoch = setLocalTime + (millis() / 1000);
  int RealHour = ((epoch  % 43200L) / 3600) * 5;
  pixels.setPixelColor(RealHour, color[0]);
}

void showMinute2() {
  unsigned long epoch = setLocalTime + (millis() / 1000);
  int RealMinute = (epoch  % 3600) / 60;
  pixels.setPixelColor(RealMinute, color[1]);
}

void showSecond2() {
  unsigned long epoch = setLocalTime + (millis() / 1000);
  int RealSecond = epoch % 60;
  pixels.setPixelColor(RealSecond - 1, map(millis() % 1000, 999, 0, 0, 255), 0, 0);
  pixels.setPixelColor(RealSecond, 255, 0, 0);
  pixels.setPixelColor(RealSecond + 1, map(millis() % 1000, 0, 999, 0, 255), 0, 0);
}

void show1() {
  unsigned long epoch = setLocalTime + (millis() / 1000);
  int Hour = ((epoch  % 43200L) / 3600) * 5;
  int Minute = (epoch  % 3600) / 60;
  int Second = epoch % 60;

  showHour1();
  showMinute1();

  if (Hour > Minute) {
    showHour1();
    showMinute1();
  }
  else {
    showMinute1();
    showHour1();
  }
  //showdot();
  showSecond2();
}

void show2() {

  //showdot(pixels.Color(255, 255, 255), pixels.Color(2, 2, 0), 0x4a1300 );
  showdot();
  showSecond2();
  showHour2();
  showMinute2();

}

void setup()
{
  Serial.begin(115200);

  pixels.begin();
  //pixels.setBrightness(160);


  WiFiManager wifiManager;
  //reset saved settings

  wifiManager.autoConnect("AutoConnectAP");

  Serial.println("Wi-Fi Connected");
  Serial.println("Starting AP...");

  WiFi.mode(WIFI_AP_STA);

  //   We start by making an Access Point
  Serial.println(WiFi.softAP("ESPsoftAP_01", "12345678") ? "Ready" : "Failed!");
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Time Setting...");
  while (!calibrateLocalTime()) {};
  Serial.println("Time Set Success");
}

void loop()
{

  pixels.clear();

  show2();

  //pixels.show();
  if (millis() >= nextCheck) {
    Serial.println("Time Setting...");
    if (calibrateLocalTime())
      Serial.println("Time Set Success");
    printTime();

    nextCheck = millis() + 5000;
  }
  server.handleClient();
  Serial.println("Client Success");
}//hehet


