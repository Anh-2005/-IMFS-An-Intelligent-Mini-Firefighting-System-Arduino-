#include <Servo.h>

// ===== Pin Map =====
const int ENA = 5;
const int ENB = 6;
const int IN1 = 7;
const int IN2 = 8;
const int IN3 = 9;
const int IN4 = 10;

const int SERVO_PIN = 11;
const int TRIG_PIN = 2;
const int ECHO_PIN = 3;
const int PUMP_PIN = A1;

const int FLAME_L = 12;
const int FLAME_C = 4;
const int FLAME_R = 13;

#define WATER_SENSOR A4
#define LED_FIRE A2
#define LED_WATER A3
#define BUZZER A0

// ===== Tham số =====
int waterThreshold = 300;

int normalSpeed = 60;
int avoidSpeed  = 90;
// int fireSpeed   = 60; // Không dùng nữa vì xe sẽ đứng yên khi chữa cháy

int servoCenter = 90;
int servoLeft   = 150;
int servoRight  = 30;

int obstacleStopDist = 20;

bool useSerialDebug = true;

// ===== Biến =====
Servo nozzle;

// ===== Prototype =====
long readDistanceCM();
void moveForward();
void moveBackward();
void turnLeft();
void turnRight();
void stopCar();
void aimNozzleCenter();
void aimNozzleLeft();
void aimNozzleRight();
void pumpOn();
void pumpOff();

void setup() {
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(PUMP_PIN, OUTPUT);
  pumpOff();

  pinMode(FLAME_L, INPUT);
  pinMode(FLAME_C, INPUT);
  pinMode(FLAME_R, INPUT);

  pinMode(LED_FIRE, OUTPUT);
  pinMode(LED_WATER, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  nozzle.attach(SERVO_PIN);
  aimNozzleCenter();

  if (useSerialDebug) {
    Serial.begin(9600);
    delay(300);
    Serial.println(F("=== XE CHỮA CHÁY BẮT ĐẦU ==="));
  }

  stopCar();
}

void loop() {

  // ==== Đọc mực nước ====
  int waterLevel = analogRead(WATER_SENSOR);
  bool waterEmpty = (waterLevel < waterThreshold);
  digitalWrite(LED_WATER, waterEmpty ? HIGH : LOW);

  // ==== Đọc khoảng cách ====
  long dist = readDistanceCM();

  // ==== Đọc cảm biến lửa ====
  // Lưu ý: Cảm biến lửa thường trả về 0 (LOW) khi có lửa, 1 (HIGH) khi không có
  bool flameL = (digitalRead(FLAME_L) == LOW);
  bool flameC = (digitalRead(FLAME_C) == LOW);
  bool flameR = (digitalRead(FLAME_R) == LOW);
  bool fireDetected = flameL || flameC || flameR;


  // ===== Trường hợp hết nước -> dừng mọi thứ =====
  if (waterEmpty) {
    stopCar();
    pumpOff();
    aimNozzleCenter();
    noTone(BUZZER);
    digitalWrite(LED_FIRE, LOW);
    return;
  }


  // ==================================================================
  //  🔥 ƯU TIÊN CAO NHẤT: PHÁT HIỆN LỬA
  //  -> XE DỪNG HẲN, CHỈ QUAY SERVO VÀ BƠM
  // ==================================================================
  if (fireDetected) {

    // 1. Xe dừng ngay lập tức
    stopCar();

    // 2. Báo lửa
    digitalWrite(LED_FIRE, HIGH);
    tone(BUZZER, 1200);

    // 3. Quay servo theo vị trí lửa
    if (flameL)      aimNozzleLeft();
    else if (flameR) aimNozzleRight();
    else             aimNozzleCenter();

    // 4. Bật bơm ngay lập tức
    pumpOn();
    
    // Delay nhỏ để giữ trạng thái bơm ổn định, tránh servo quay quá nhanh
    delay(100); 

    // *** QUAN TRỌNG ***
    // Return ngay tại đây để lặp lại vòng loop().
    // Nếu vẫn còn lửa -> Lại vào block này -> Tiếp tục đứng yên và bơm.
    // Nếu hết lửa -> Block này sẽ bị bỏ qua ở lần loop tiếp theo -> Xuống dưới tắt bơm.
    return;
  }


  // ==================================================================
  //  🔥 KHÔNG CÒN LỬA -> TẮT BƠM + TRẢ SERVO VỀ GIỮA
  // ==================================================================
  pumpOff();
  aimNozzleCenter();
  digitalWrite(LED_FIRE, LOW);
  noTone(BUZZER);


  // ==================================================================
  //  🚧 ƯU TIÊN 2: Né vật cản (Chỉ chạy khi KHÔNG có lửa)
  // ==================================================================
  if (dist > 0 && dist <= obstacleStopDist) {
    stopCar();
    delay(100);

    moveBackward();
    delay(300);

    stopCar();
    delay(100);

    turnLeft(); // Hoặc turnRight tùy thiết kế xe
    delay(400);

    stopCar();
    delay(100);

    return;
  }

  // ==================================================================
  //  🚗 ƯU TIÊN 3: Chạy thẳng tuần tra
  // ==================================================================
  moveForward();
  delay(20);
}


// ===== Các hàm phụ trợ =====

long readDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 25000UL);
  if (duration == 0) return 0;
  return duration / 58;
}

void moveForward() {
  analogWrite(ENA, normalSpeed);
  analogWrite(ENB, normalSpeed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void moveBackward() {
  analogWrite(ENA, avoidSpeed);
  analogWrite(ENB, avoidSpeed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnLeft() {
  analogWrite(ENA, avoidSpeed);
  analogWrite(ENB, avoidSpeed); 
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void turnRight() {
  analogWrite(ENA, avoidSpeed);
  analogWrite(ENB, avoidSpeed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void stopCar() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void aimNozzleCenter() { nozzle.write(servoCenter); }
void aimNozzleLeft()   { nozzle.write(servoLeft); }
void aimNozzleRight()  { nozzle.write(servoRight); }

void pumpOn()  { digitalWrite(PUMP_PIN, HIGH); }
void pumpOff() { digitalWrite(PUMP_PIN, LOW); }