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
char ssid[] = SECRET_SSID;  // your network SSID (name)
char pass[] = SECRET_PASS;  // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;           // your network key index number (needed only for WEP)

int led = LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

ArduinoLEDMatrix matrix;
int waitingFrame = 20;
int doneFrame = 25;
const uint32_t waiting[][4] = {
  { 0x0,
    0x0,
    0x8078000,
    66 },
  { 0x0,
    0x80,
    0x8038000,
    66 },
  { 0x0,
    0x80080,
    0x8018000,
    66 },
  { 0x0,
    0x80080080,
    0x8008000,
    66 },
  { 0x800,
    0x80080080,
    0x8000000,
    66 },
  { 0x1800,
    0x80080080,
    0x0,
    66 },
  { 0x3800,
    0x80080000,
    0x0,
    66 },
  { 0x7800,
    0x80000000,
    0x0,
    66 },
  { 0xf800,
    0x0,
    0x0,
    66 },
  { 0x1f000,
    0x0,
    0x0,
    66 },
  { 0x1e010,
    0x0,
    0x0,
    66 },
  { 0x1c010,
    0x1000000,
    0x0,
    66 },
  { 0x18010,
    0x1001000,
    0x0,
    66 },
  { 0x10010,
    0x1001001,
    0x0,
    66 },
  { 0x10,
    0x1001001,
    0x100000,
    66 },
  { 0x0,
    0x1001001,
    0x180000,
    66 },
  { 0x0,
    0x1001,
    0x1c0000,
    66 },
  { 0x0,
    0x1,
    0x1e0000,
    66 },
  { 0x0,
    0x0,
    0x1f0000,
    66 },
  { 0x0,
    0x0,
    0xf8000,
    66 }
};
const uint32_t done[][4] = {
  { 0x0,
    0x0,
    0xf8000,
    66 },
  { 0x0,
    0x0,
    0x80f8000,
    66 },
  { 0x0,
    0x80,
    0x80f8000,
    66 },
  { 0x0,
    0x80080,
    0x80f8000,
    66 },
  { 0x0,
    0x80080080,
    0x80f8000,
    66 },
  { 0x800,
    0x80080080,
    0x80f8000,
    66 },
  { 0x1800,
    0x80080080,
    0x80f8000,
    66 },
  { 0x3800,
    0x80080080,
    0x80f8000,
    66 },
  { 0x7800,
    0x80080080,
    0x80f8000,
    66 },
  { 0xf800,
    0x80080080,
    0x80f8000,
    66 },
  { 0x1f800,
    0x80080080,
    0x80f8000,
    66 },
  { 0x1f810,
    0x80080080,
    0x80f8000,
    66 },
  { 0x1f810,
    0x81080080,
    0x80f8000,
    66 },
  { 0x1f810,
    0x81081080,
    0x80f8000,
    66 },
  { 0x1f810,
    0x81081081,
    0x80f8000,
    66 },
  { 0x1f810,
    0x81081081,
    0x81f8000,
    250 },
  { 0x0,
    0x0,
    0x0,
    250 },
  { 0x1f810,
    0x81081081,
    0x81f8000,
    250 },
  { 0x0,
    0x0,
    0x0,
    250 },
  { 0x1f810,
    0x81081081,
    0x81f8000,
    250 },
  { 0x0,
    0x0,
    0x0,
    250 },
  { 0x1f810,
    0x81081081,
    0x81f8000,
    250 },
  { 0xf,
    0x900900,
    0xf0000000,
    250 },
  { 0x0,
    0x600600,
    0x0,
    250 },
  { 0x0,
    0x0,
    0x0,
    250 },
};

byte buf[40][8];         //フォントデータ格納用。一つめの要素数が文字数（多めでいい）
byte matrix_buff[2000];  //表示用90度回転ピクセルデータ。文字数×8ぶん必要？（多めでいい）

//スクロール時間
#define SCROLL_TIME 100  //ミリ秒
unsigned long tm = 0;

char *str;
int idx = 0, max_idx;
bool wifiConect = false;

void printTxt() {
  //スクロール時間が経過したらスライドさせる。末端に到達したら始めに戻る
  if (tm + SCROLL_TIME <= millis()) {
    if (idx < max_idx)
      idx++;
    else {
      idx = 0;
    }

    //経過時間を再計測
    tm = millis();
  }

  //12x8LED View
  for (int ic1 = 0; ic1 < 8; ic1++) {
    for (int ic2 = 0; ic2 < 12; ic2++) {
      turnLed(ic1 * 12 + ic2, matrix_buff[idx + ic2] >> (7 - ic1) & 0x01);
      delayMicroseconds(50);  //これを入れないとLEDがちらつく
    }
  }
}

void setTxt(String txt) {
  txt = " " + txt;
  free(str);
  char *pt = (char *)malloc(1000);
  str = pt;
  txt.toCharArray(str, 100);

  for (int i = 0; i < 2000; i++) {
    matrix_buff[i] = (byte)0;
  }

  char *ptr = str;
  max_idx = 10;
  byte line = 0;

  int n = 0;
  while (*ptr) {
    ptr = getFontData(&buf[n++][0], ptr);  // フォントデータの取得
  }
  max_idx = 0;
  for (byte j = 0; j < n; j++) {      //文字
    for (byte k = 0; k < 8; k++) {    //横
      for (byte i = 0; i < 8; i++) {  //縦
        line = bitWrite(line, 7 - i, bitRead(buf[j][i], 7 - k));
      }
      matrix_buff[max_idx] = line;
      max_idx++;
    }
  }
}

static const char table[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
char decode_hex_to_char(const char *c) {
  // XXX: [0-9a-zA-Z]以外の全ての文字は、0として扱われる (e.g. "%@X" => 0)
  return (table[static_cast<unsigned char>(c[1])] << 4) + table[static_cast<unsigned char>(c[2])];
}

String url_decode(String str) {
  String dist;
  for (int i = 0; str[i] != '\0'; i++) {
    switch (str[i]) {
      case '%':
        dist += decode_hex_to_char(&str[i]);
        i += 2;
        break;  // XXX: 末尾に'%'が来る不正な文字列が渡された場合の挙動は未定義(ex. "abc%")
      case '+': dist += ' '; break;
      default: dist += str[i]; break;
    }
  }
  return dist;
}

String ncr_decode(String str) {
  String result;
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == '&' && str[i + 1] == '#') {
      int num = 0, j = i + 2;
      while (str[j] >= '0' && str[j] <= '9') {
        num = num * 10 + (int)(str[j] - '0');
        j++;
      }
      if (j - i - 2 == 3) {
        if (num == 60) {
          result += '<';
          i += 5;
          continue;
        } else if (num == 62) {
          result += '>';
          i += 5;
          continue;
        } else if (num == 38) {
          result += '&';
          i += 5;
          continue;
        } else if (num == 34) {
          result += '"';
          i += 5;
          continue;
        }
      } else if (j - i - 2 == 5) {
        String hiragana[] = { "ぁ", "あ", "ぃ", "い", "ぅ", "う", "ぇ", "え", "ぉ", "お", "か", "が", "き", "ぎ", "く", "ぐ", "け", "げ", "こ", "ご", "さ", "ざ", "し", "じ", "す", "ず", "せ", "ぜ", "そ", "ぞ", "た", "だ", "ち", "ぢ", "っ", "つ", "ず", "て", "で", "と", "ど", "な", "に", "ぬ", "ね", "の", "は", "ば", "ぱ", "ひ", "び", "ぴ", "ふ", "ぶ", "ぷ", "へ", "べ", "ぺ", "ほ", "ぼ", "ぽ", "ま", "み", "む", "め", "も", "ゃ", "や", "ゅ", "ゆ", "ょ", "よ", "ら", "り", "る", "れ", "ろ", "ゎ", "わ", "ゐ", "ゑ", "を", "ん" };
        String katakana[] = { "ァ", "ア", "ィ", "イ", "ゥ", "ウ", "ェ", "エ", "ォ", "オ", "カ", "ガ", "キ", "ギ", "ク", "グ", "ケ", "ゲ", "コ", "ゴ", "サ", "ザ", "シ", "ジ", "ス", "ズ", "セ", "ゼ", "ソ", "ゾ", "タ", "ダ", "チ", "ヂ", "ッ", "ツ", "ヅ", "テ", "テ", "ト", "ド", "ナ", "ニ", "ヌ", "ネ", "ノ", "ハ", "バ", "パ", "ヒ", "ビ", "ピ", "フ", "ブ", "プ", "ヘ", "ベ", "ペ", "ホ", "ボ", "ポ", "マ", "ミ", "ム", "メ", "モ", "ャ", "ヤ", "ュ", "ユ", "ョ", "ヨ", "ラ", "ル", "ッ", "レ", "ロ", "ヮ", "ワ", "ヰ", "ヱ", "ヲ", "ン", "ヴ", "ヵ", "ヶ" };
        if (num == 12540) {
          result += "ー";
          i += 7;
          continue;
        } else if (num == 12316) {
          result += "〜";
          i += 7;
          continue;
        } else if (num == 12540) {
          result += "、";
          i += 7;
          continue;
        } else if (num >= 12354 && num <= 12435 && hiragana[num - 12353] != "#") {
          result += hiragana[num - 12353];
          i += 7;
          continue;
        } else if (num >= 12448 && num <= 12534) {
          result += katakana[num - 12449];
          i += 7;
          continue;
        }
      }
    }
    result += str[i];
  }
  return result;
}

void setup() {
  Serial.begin(9600);    // initialize serial communication
  pinMode(led, OUTPUT);  // set the LED pin mode
  matrix.begin();

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true)
      ;
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    for (int j = 0; j < waitingFrame; j++) {
      matrix.loadFrame(waiting[j]);
      delay(waiting[j][3]);
    }
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);  // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
  }
  server.begin();     // start the web server on port 80
  printWifiStatus();  // you're connected now, so print out the status

  for (int j = 0; j < doneFrame; j++) {
    matrix.loadFrame(done[j]);  //アニメーションデータのうちi番目を表示
    delay(done[j][3]);          //アニメーションデータのi番目から待機時間を読み取り待機
  }
  setTxt("オンラインです");
}


void loop() {
  for (int i = 0; i < 50; i++) printTxt();
  WiFiClient client = server.available();  // listen for incoming clients
  for (int i = 0; i < 50; i++) printTxt();
  if (client) {                    // if you get a client,
    Serial.println("new client");  // print a message out the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected()) {   // loop while the client's connected
      printTxt();
      if (client.available()) {  // if there's bytes to read from the client,
        char c = client.read();  // read a byte, then
        Serial.write(c);         // print it out to the serial monitor
        if (c == '\n') {         // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<CENTER><form method=get>Enter text to be displayed<input type=text size=10 name='txt'> <input type=submit value=submit></form></CENTER></body></html><br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {  // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
        String readStr = client.readStringUntil('\r');
        if (readStr.indexOf("txt=") >= 4) {
          readStr = readStr.substring(readStr.indexOf("txt=") + 4);
          if (readStr.indexOf(" ") != -1) {
            readStr = readStr.substring(0, readStr.indexOf(" "));
          }
          readStr = url_decode(readStr);
          readStr = ncr_decode(readStr);
          idx = 0;
          setTxt(readStr);
          Serial.println("文字列が更新されました");
          Serial.println(readStr);
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
