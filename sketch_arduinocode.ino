#include <Keypad.h>

#include <SoftwareSerial.h>



// ESP8266 통신용: RX = A0, TX = A1

SoftwareSerial espSerial(A0, A1);



// 핀 설정

const byte DOOR_LOCK_PIN = 2;

const byte STATUS_LED_PIN = 13;

const byte DOOR_LIMIT_SWITCH_PIN = 10;

const byte BOOK_LIMIT_SWITCH_PIN = 11;



const int cabinetSlot = 1;



// 4x3 키패드 설정

const byte ROWS = 4;

const byte COLS = 3;



char keys[ROWS][COLS] = {

  {'1', '2', '3'},

  {'4', '5', '6'},

  {'7', '8', '9'},

  {'*', '0', '#'}

};



byte rowPins[ROWS] = {9, 8, 7, 6};

byte colPins[COLS] = {5, 4, 3};



Keypad keypad = Keypad(

  makeKeymap(keys),

  rowPins,

  colPins,

  ROWS,

  COLS

);



// 비밀번호

String doorPassword = "1234";

String enteredPassword = "";



// 상태 변수

bool lastDoorClosed = false;

bool lastBookPresent = false;

bool isDoorUnlocked = false;



// 함수 선언

bool isDoorClosed();

bool isBookPresent();

void sendStateToESP();

void printDoorState(bool state);

void printBookState(bool state);

void unlockDoor();

void lockDoor();

void handleKeypadInput(char key);



void setup() {

  Serial.begin(9600);

  espSerial.begin(9600);



  pinMode(DOOR_LOCK_PIN, OUTPUT);

  pinMode(STATUS_LED_PIN, OUTPUT);



  pinMode(DOOR_LIMIT_SWITCH_PIN, INPUT_PULLUP);

  pinMode(BOOK_LIMIT_SWITCH_PIN, INPUT_PULLUP);



  digitalWrite(DOOR_LOCK_PIN, LOW);

  digitalWrite(STATUS_LED_PIN, LOW);



  lastDoorClosed = isDoorClosed();

  lastBookPresent = isBookPresent();



  Serial.println("==============================");

  Serial.println("도어락 시스템 시작");

  Serial.println("키패드: 4x3");



  Serial.print("보관함 칸 번호: ");

  Serial.println(cabinetSlot);



  Serial.println("==============================");



  printDoorState(lastDoorClosed);

  printBookState(lastBookPresent);



  Serial.println("비밀번호: 1234");

  Serial.println("문이 닫힌 상태에서 비밀번호를 입력하세요.");



  sendStateToESP();

}



void loop() {

  // A. 문 열림/닫힘 확인

  bool currentDoorClosed = isDoorClosed();



  if (currentDoorClosed != lastDoorClosed) {

    lastDoorClosed = currentDoorClosed;



    Serial.println();

    Serial.println("[문 감지] 상태 변경!");



    printDoorState(currentDoorClosed);



    enteredPassword = "";



    if (currentDoorClosed) {

      lockDoor();

      Serial.println("[도어락] 문 닫힘 - 잠금 실행");

    } else {

      digitalWrite(STATUS_LED_PIN, LOW);

      Serial.println("[도어락] 문 열림 감지");

      sendStateToESP();

    }

  }



  // B. 책 유무 확인

  bool currentBookPresent = isBookPresent();



  if (currentBookPresent != lastBookPresent) {

    lastBookPresent = currentBookPresent;



    Serial.println();

    Serial.println("[책 감지] 상태 변경!");



    printBookState(currentBookPresent);

    sendStateToESP();

  }



  // C. 키패드 입력 확인 및 ESP8266 전송

  char key = keypad.getKey();



  if (key != NO_KEY) {

    Serial.println();

    Serial.print("[키패드 입력 성공] ");

    Serial.println(key);



    // ESP8266으로 키패드 입력 전송

    espSerial.print("KEY:");

    espSerial.println(key);



    Serial.print("[ESP 전송 키패드] KEY:");

    Serial.println(key);



    if (currentDoorClosed && !isDoorUnlocked) {

      handleKeypadInput(key);

    } else if (!currentDoorClosed) {

      Serial.println("[안내] 문이 열린 상태입니다.");

    } else {

      Serial.println("[안내] 이미 잠금 해제된 상태입니다.");

    }

  }



  // D. ESP8266 원격 잠금 해제 명령 수신

  if (espSerial.available() > 0) {

    String cmd = espSerial.readStringUntil('\n');

    cmd.trim();



    if (cmd == "UNLOCK") {

      Serial.println();

      Serial.println("[ESP8266] 원격 잠금 해제 명령 수신");

      unlockDoor();

    }

  }

}



// 비밀번호 입력 처리

void handleKeypadInput(char key) {

  if (key == '*') {

    enteredPassword = "";

    Serial.println("[키패드] 입력 초기화");

  }

  else if (key == '#') {

    if (enteredPassword == doorPassword) {

      Serial.println("[비밀번호] 일치");

      unlockDoor();

    } else {

      Serial.println("[비밀번호] 불일치");

      Serial.println("[안내] 다시 입력하세요.");

    }



    enteredPassword = "";

  }

  else if (key >= '0' && key <= '9') {

    enteredPassword += key;



    Serial.print("[비밀번호 입력] ");



    for (unsigned int i = 0; i < enteredPassword.length(); i++) {

      Serial.print("*");

    }



    Serial.println();

  }

}



// 잠금 해제

void unlockDoor() {

  isDoorUnlocked = true;



  digitalWrite(DOOR_LOCK_PIN, HIGH);

  digitalWrite(STATUS_LED_PIN, LOW);



  Serial.println("[도어락] 잠금 해제!");



  sendStateToESP();

}



// 잠금 실행

void lockDoor() {

  digitalWrite(DOOR_LOCK_PIN, LOW);

  digitalWrite(STATUS_LED_PIN, HIGH);



  isDoorUnlocked = false;



  Serial.println("[도어락] 잠금 상태");



  sendStateToESP();

}



// 문이 닫혀 있으면 true

bool isDoorClosed() {

  return digitalRead(DOOR_LIMIT_SWITCH_PIN) == LOW;

}



// 책이 감지되면 true

bool isBookPresent() {

  return digitalRead(BOOK_LIMIT_SWITCH_PIN) == LOW;

}



// 문 상태 출력

void printDoorState(bool state) {

  Serial.print("[문 상태] ");

  Serial.println(state ? "닫힘 (true)" : "열림 (false)");

}



// 책 상태 출력

void printBookState(bool state) {

  Serial.print("[책 상태] ");

  Serial.println(state ? "책 있음 (true)" : "책 없음 (false)");

}



// 상태 JSON을 ESP8266으로 전송

void sendStateToESP() {

  delay(50);

  String jsonPayload = "{";



  jsonPayload += "\"cabinetSlot\":";

  jsonPayload += String(cabinetSlot);



  jsonPayload += ",\"doorStatus\":";

  jsonPayload += (lastDoorClosed ? "true" : "false");



  jsonPayload += ",\"limitSwitchStatus\":";

  jsonPayload += (lastBookPresent ? "true" : "false");



  jsonPayload += "}";



  espSerial.println(jsonPayload);

  delay(50);



  Serial.print("[ESP 전송 JSON] ");

  Serial.println(jsonPayload);

}