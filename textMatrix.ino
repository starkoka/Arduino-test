/*
  WiFi Web Server LED Blink

  A simple web server that lets you blink an LED via the web.
  This sketch will print the IP address of your WiFi module (once connected)
  to the Serial Monitor. From there, you can open that address in a web browser
  to turn on and off the LED_BUILTIN.

  If the IP address of your board is yourAddress:
  http://yourAddress/H turns the LED on
  http://yourAddress/L turns it off

  This example is written for a network using WPA encryption. For
  WEP or WPA, change the WiFi.begin() call accordingly.

  Circuit:
  * Board with NINA module (Arduino MKR WiFi 1010, MKR VIDOR 4000 and Uno WiFi Rev.2)
  * LED attached to pin 9

  created 25 Nov 2012
  by Tom Igoe

  Find the full UNO R4 WiFi Network documentation here:
  https://docs.arduino.cc/tutorials/uno-r4-wifi/wifi-examples#simple-webserver
 */

#include "WiFiS3.h"
#include "Arduino_LED_Matrix.h"
#include "arduino_secrets.h" 
#include <misakiUTF16.h>

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

ArduinoLEDMatrix matrix;
const uint32_t animation[][4] = {
  {
    0x00000000,
    0x00000000,
    0x00000000,
    66
  }
};

byte buf[40][8];   //フォントデータ格納用。一つめの要素数が文字数（多めでいい）
byte matrix_buff[2000];  //表示用90度回転ピクセルデータ。文字数×8ぶん必要？（多めでいい）

//スクロール時間
#define SCROLL_TIME 100   //ミリ秒
unsigned long tm = 0;

char *str;
int idx = 0, max_idx;
bool txtPrint = false;

void printTxt(){
  //スクロール時間が経過したらスライドさせる。末端に到達したら始めに戻る
    if(tm + SCROLL_TIME <= millis()) {
      if (idx < max_idx)
        idx++;
      else{
        idx = 0;
      }

      //経過時間を再計測
      tm = millis();
    }

    //12x8LED View
    for(int ic1 = 0; ic1 < 8; ic1++){
      for(int ic2 = 0; ic2 < 12; ic2++){
        turnLed(ic1 * 12 + ic2 , matrix_buff[idx + ic2] >> (7 - ic1) & 0x01);
        delayMicroseconds(50);  //これを入れないとLEDがちらつく
      }
    }
}

void setTxt(String txt){
  txt = " "+txt;
  free(str);
  char *pt = (char *)malloc(100);
  str = pt;
  txt.toCharArray(str, 100);

  Serial.println((int)&pt);
  Serial.println(txt);
  
  for(int i=0;i<2000;i++){
       matrix_buff[i] = (byte)0;
  }
  
  char *ptr = str;
  max_idx = 10;
  byte line = 0;

  int n=0;
  while(*ptr) {
    ptr = getFontData(&buf[n++][0], ptr);  // フォントデータの取得
  }
  max_idx = 0;
  for (byte j=0; j < n; j++) { //文字
    for (byte k=0; k<8;k++) { //横
      for (byte i=0; i < 8; i++) { //縦
        line = bitWrite(line, 7-i, bitRead(buf[j][i],7-k));
      }
      matrix_buff[max_idx] = line;
      max_idx++;
    }
  }
  Serial.println(max_idx);
}


void setup() {
  Serial.begin(9600);      // initialize serial communication
  pinMode(led, OUTPUT);      // set the LED pin mode
  matrix.begin();

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status
  
  setTxt("こかすたーのたんじょうび");
}


void loop() {
  for(int i=0;i<100;i++)printTxt();
  WiFiClient client = server.available();   // listen for incoming clients
  for(int i=0;i<100;i++)printTxt();
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      printTxt();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<p style=\"font-size:7vw;\">Click <a href=\"/H\">here</a> to view the bird logo<br></p>");
            client.print("<p style=\"font-size:7vw;\">Click <a href=\"/L\">here</a> to hide the bird logo<br></p>");
            client.print("<CENTER><form method=get>Enter text to be displayed<input type=text size=3 name=txt> <input type=submit value=submit></form></CENTER></body></html><br>");
            
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /H turns the LED on
          idx = 0;
          setTxt("ロボ研");
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);                // GET /L turns the LED off
          idx = 0;
          setTxt("期待できない");
        }
      }
      
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
    printTxt();
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
