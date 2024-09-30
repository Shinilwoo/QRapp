#include <usbhid.h>
#include <usbhub.h>
#include <hiduniversal.h>
#include <hidboot.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <TimeLib.h>
//unsigned long startTime;
  
class MyParser : public HIDReportParser {
  public:
    MyParser();
    void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
  
    String scannedQRCode;
    String targetQRCode;  // 목표 QR 코드
    String targetQRCode2;
    unsigned long validUntil; // 유효 시간
  private:
    void updateCurrentTime();  
  protected:
    uint8_t KeyToAscii(bool upper, uint8_t mod, uint8_t key);
    virtual void OnKeyScanned(bool upper, uint8_t mod, uint8_t key);
    virtual void OnScanFinished();
};

MyParser::MyParser() {
  validUntil = 0;
  //startTime = millis(); 
}
void MyParser::updateCurrentTime() {
  setTime(now() + 1); // 1초마다 현재 시간 업데이트
}

void MyParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf) {
  // If error or empty, return
  if (buf[2] == 1 || buf[2] == 0) return;

  for (uint8_t i = 7; i >= 2; i--) {
    // If empty, skip
    if (buf[i] == 0) continue;

    // If not, continue normally
    else {
      // If bit position not in 2, it's uppercase words
      OnKeyScanned(i > 2, buf[1], buf[i]);
    }

    // If enter signal emitted, scan finished
    if (buf[i] == UHS_HID_BOOT_KEY_ENTER) {
      OnScanFinished();
    }
  }
}


uint8_t MyParser::KeyToAscii(bool upper, uint8_t mod, uint8_t key) {
  // Letters
  if (VALUE_WITHIN(key, 0x04, 0x1d)) {
    if (upper) return (key - 4 + 'A');
    else return (key - 4 + 'a');
  }

  // Numbers
  else if (VALUE_WITHIN(key, 0x1e, 0x27)) {
    return ((key == UHS_HID_BOOT_KEY_ZERO) ? '0' : key - 0x1e + '1');
  }

  return 0;
}

void MyParser::OnKeyScanned(bool upper, uint8_t mod, uint8_t key) {
  uint8_t ascii = KeyToAscii(upper, mod, key);
  Serial.print((char)ascii);
  scannedQRCode += (char)ascii;
}

// 7-세그먼트 디스플레이를 위한 핀 연결
const int segmentPins[] = {2,3,4,5,6,7,8};

// 7-세그먼트 디스플레이에 표시할 숫자 패턴
const byte digitPatterns[] = {
  B00111111,  // 0
  B00000110,  // 1
  B01011011,  // 2
  B01001111,  // 3
  B01100110,  // 4
  B01101101,  // 5
  B01111101,  // 6
  B00000111,  // 7
  B01111111,  // 8
  B01101111   // 9
};
// 숫자를 7-세그먼트 디스플레이에 표시하는 함수
void displayNumber(int number,unsigned long duration) {
  if (number >= 0 && number <= 9) {
    for (int i = 0; i < 7; i++) {
      digitalWrite(segmentPins[i], bitRead(digitPatterns[number], i));
    }
    delay(duration);
  }
  clearDisplay();
}

// 7-세그먼트 디스플레이 비우는 함수
void clearDisplay() {
  for (int i = 0; i < 7; i++) {
    digitalWrite(segmentPins[i], LOW);
  }
}
// QR 코드에서 추출한 유효 시간 문자열을 파싱하여 시간으로 변환하는 함수
unsigned long parseQRCodeTimestamp(String timestampString) {
  // 시간 문자열을 분해하여 시간 요소 추출
  int year = timestampString.substring(0, 4).toInt();
  int month = timestampString.substring(4, 6).toInt();
  int day = timestampString.substring(6, 8).toInt();
  int hour = timestampString.substring(8, 10).toInt();
  int minute = timestampString.substring(10, 12).toInt();
  int second = timestampString.substring(12, 14).toInt();
  
  // 시간 요소를 기반으로 시간값 계산
  tmElements_t tm;
  tm.Year = CalendarYrToTm(year);
  tm.Month = month;
  tm.Day = day;
  tm.Hour = hour;
  tm.Minute = minute;
  tm.Second = second;
  time_t timeValue = makeTime(tm);
  
  // 현재 시간과의 차이를 계산하여 밀리초 단위로 반환
  return (timeValue - now()) * 1000;
}


void MyParser::OnScanFinished() {
  Serial.println(" - Finished");
  targetQRCode = "3floor";
  targetQRCode2 ="4floor";
  // QR 코드에서 유효 시간 필드 추출
  int validUntilIndex = scannedQRCode.indexOf("limittime") + 9; // "limittime" 다음 인덱스
  int validUntilLength = 14; // 시간 문자열의 길이 (시분초만 사용)
  String validUntilString = scannedQRCode.substring(validUntilIndex, validUntilIndex + validUntilLength);

  // 유효 시간 설정 (현재 시간과 추출한 시간 비교)
  int validYear = validUntilString.substring(0, 4).toInt();
  int validMonth = validUntilString.substring(4, 6).toInt();
  int validDay = validUntilString.substring(6, 8).toInt();
  int validHour = validUntilString.substring(8, 10).toInt();
  int validMinute = validUntilString.substring(10, 12).toInt();
  int validSecond = validUntilString.substring(12, 14).toInt();

  // 현재 시간 가져오기
  time_t currentTime = now();
  tmElements_t currentTm;
  breakTime(currentTime, currentTm);
  int currentYear = currentTm.Year + 2023;
  int currentMonth = currentTm.Month+5;
  int currentDay = currentTm.Day+19;
  int currentHour = currentTm.Hour+12;
  int currentMinute = currentTm.Minute+20;
  int currentSecond = currentTm.Second;
  Serial.println(validUntilIndex);
  Serial.println(validUntilLength);
  Serial.println(validYear);
  Serial.println(validMonth);
  Serial.println(validDay);
  Serial.println(validMinute);
  Serial.println(validSecond);
  Serial.println(currentYear);
  Serial.println(currentMonth);
  Serial.println(currentDay);
  Serial.println(currentHour);
  Serial.println(currentMinute);
  Serial.println(currentSecond);
  
  // 유효 시간과 현재 시간 비교
  if (currentYear < validYear ||
      (currentYear == validYear && currentMonth < validMonth) ||
      (currentYear == validYear && currentMonth == validMonth && currentDay < validDay) ||
      (currentYear == validYear && currentMonth == validMonth && currentDay == validDay && currentHour < validHour) ||
      (currentYear == validYear && currentMonth == validMonth && currentDay == validDay && currentHour == validHour && currentMinute < validMinute) ||
      (currentYear == validYear && currentMonth == validMonth && currentDay == validDay && currentHour == validHour && currentMinute == validMinute && currentSecond < validSecond)) {
    
    // 유효 시간 내에 스캔된 경우
    String scannedQRCodeId = scannedQRCode.substring(0, targetQRCode.length());
    String scannedQRCodeId2= scannedQRCode.substring(0,targetQRCode2.length());
    Serial.print("Target QR Code: ");
    Serial.println(targetQRCode);
    Serial.println(targetQRCode2);
    Serial.print("Scanned QR Code ID: ");
    Serial.println(scannedQRCodeId);
    Serial.println(scannedQRCodeId2);

    if (scannedQRCodeId.equals(targetQRCode)) {
      // 스캔된 QR 코드와 목표 QR 코드가 일치하는 경우 작업 수행
      Serial.println("QR Code Matched!");

      displayNumber(3, 5000);
    }else if(scannedQRCodeId2.equals(targetQRCode2)){
      Serial.println("QR Code Matched!");

      displayNumber(4, 5000);
    } else {
      Serial.println("QR Code Not Matched!");
      clearDisplay();
    }
  } else {
    // 유효 시간 초과
    Serial.println("QR Code Expired!");
    clearDisplay();
  }

  // 작업 수행 후 스캔된 QR 코드 초기화
  scannedQRCode = "";
}




USB          Usb;
USBHub       Hub(&Usb);
HIDUniversal Hid(&Usb);
MyParser     Parser;

void setup() {
  Serial.begin( 115200 );
  Serial.println("Start");

  if (Usb.Init() == -1) {
    Serial.println("OSC did not start.");
  }

// 7-세그먼트 디스플레이의 핀을 출력 모드로 설정
  for (int i = 0; i < 8; i++) {
    pinMode(segmentPins[i], OUTPUT);
  }
  delay( 2000 );
  
  Hid.SetReportParser(0, &Parser);
  //Parser.startTime = millis();
  //setTime(0,23,1,20,6,2023);
}

void loop() {
  Usb.Task();
}