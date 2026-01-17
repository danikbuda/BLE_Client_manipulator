#include <Arduino.h>
#include <ESP32Servo.h>
#include <BLEGamepadClient.h>

// ================= НАСТРОЙКИ =================
#define MAX_ARM_SPEED 40      // скорость (градусы*10 за тик)
#define DEADZONE 0.20f

// Пределы (градусы * 10)
#define CLAW_MIN 0
#define CLAW_MAX 900//300
#define WRIST_MIN 300
#define WRIST_MAX 800//900 //450
#define ARM_MIN 0 // 150
#define ARM_MAX 900//600
#define YAW_MIN 300
#define YAW_MAX 750//900

// Пины серв
#define SERVO_CLAW 13
#define SERVO_WRIST 14
#define SERVO_ARM 12
#define SERVO_YAW 27

// Пины моторов
#define IN1_PIN 25
#define IN2_PIN 26
#define IN3_PIN 32
#define IN4_PIN 33
// ============================================

// ===== BLE Xbox =====
XboxController controller;
XboxControlsEvent prevEvent;

// ===== Сервы =====
Servo claw, wrist, arm, yaw;

// Позиции (градусы *10)
int clawP = 450;
int wristP = 450;
int armP = 450;
int yawP = 450;

// ===== МОТОРЫ =====
enum MotorCmd {
  MOTOR_STOP,
  MOTOR_UP,
  MOTOR_DOWN,
  MOTOR_LEFT,
  MOTOR_RIGHT
};

void setMotors(MotorCmd cmd) {
  static const uint8_t table[][4] = {
    {0,0,0,0}, // STOP
    {1,0,1,0}, // UP
    {0,1,0,1}, // DOWN
    {1,0,0,1}, // LEFT
    {0,1,1,0}  // RIGHT
  };

  digitalWrite(IN1_PIN, table[cmd][0]);
  digitalWrite(IN2_PIN, table[cmd][1]);
  digitalWrite(IN3_PIN, table[cmd][2]);
  digitalWrite(IN4_PIN, table[cmd][3]);
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(IN3_PIN, OUTPUT);
  pinMode(IN4_PIN, OUTPUT);

  claw.setPeriodHertz(50);
  wrist.setPeriodHertz(50);
  arm.setPeriodHertz(50);
  yaw.setPeriodHertz(50);

  claw.attach(SERVO_CLAW, 500, 2500);
  wrist.attach(SERVO_WRIST, 500, 2500);
  arm.attach(SERVO_ARM, 500, 2500);
  yaw.attach(SERVO_YAW, 500, 2500);

  Serial.println("BLE Xbox controller init...");
  controller.begin();
}

// ================= LOOP =================
void loop() {
  if (controller.isConnected()) {
   
  XboxControlsEvent e;
  controller.read(&e);

  // ===== DEADZONE =====
  if (abs(e.leftStickX) < DEADZONE)  e.leftStickX = 0;
  if (abs(e.leftStickY) < DEADZONE)  e.leftStickY = 0;
  if (abs(e.rightStickX) < DEADZONE) e.rightStickX = 0;
  if (abs(e.rightStickY) < DEADZONE) e.rightStickY = 0;
// ====== СЕРВОПРИВОДЫ ======
static uint32_t tmr;
  if (millis() - tmr >= 30) {
    tmr = millis();

     armP += e.rightStickX * MAX_ARM_SPEED;
    clawP -= e.rightStickY * MAX_ARM_SPEED;
    wristP   += e.leftStickY  * MAX_ARM_SPEED;
    yawP   += e.leftStickX  * MAX_ARM_SPEED;

    clawP  = constrain(clawP,  CLAW_MIN,  CLAW_MAX);
    wristP = constrain(wristP, WRIST_MIN, WRIST_MAX);
    armP   = constrain(armP,   ARM_MIN,   ARM_MAX);
    yawP   = constrain(yawP,   YAW_MIN,   YAW_MAX);

    claw.write(clawP / 5);
  
    wrist.write(wristP / 5 );
    
    arm.write(armP / 5);
 
    yaw.write(yawP / 5);
   
  }
 
 
if (memcmp(&e, &prevEvent, sizeof(XboxControlsEvent)) != 0) 
    {
      
     // ===== МОТОРЫ (D-PAD) =====
    setMotors(MOTOR_STOP);
  // === ЛЕВЫЙ СТИК С МЁРТВОЙ ЗОНОЙ 0.20 ===
    // if(e.leftStickX != prevEvent.leftStickX || e.leftStickY!= prevEvent.leftStickY)
     if(e.leftStickX !=0.0f )
     {
      Serial.printf("LX: %.2f \n " , e.leftStickX);
      Serial.printf("yawP: %d ", yawP);
   
    } 
     if( e.leftStickY !=0.0f)
     {
      Serial.printf("LY: %.2f \n",  e.leftStickY);
      Serial.printf("wristP: %d ", wristP);
     }// Стик L
     
      if(e.rightStickX != 0.0f)
      {
        Serial.printf("RX: %.2f \n", e.rightStickX );
       
        Serial.printf("armP: %d ", armP);
      }  // Стик R
      
      if(e.rightStickY!= 0.0f)
      {
        Serial.printf("RY: %.2f \n",  e.rightStickY );
        Serial.printf("clawP: %d ", clawP);
       
   
      }  // Стик R

     //if(e.leftTrigger != prevEvent.leftTrigger || e.rightTrigger!= prevEvent.rightTrigger)
       if(e.leftTrigger != 0.0f|| e.rightTrigger!= 0.0f)
      {Serial.printf("LT:%.0f  RT:%.0f\n", e.leftTrigger * 100, e.rightTrigger * 100);}  // Триггеры в % для удобства

      // Кнопки (только если нажаты)
      if (e.buttonA) Serial.print("A ");
      if (e.buttonB) Serial.print("B ");
      if (e.buttonX) Serial.print("X ");
      if (e.buttonY) Serial.print("Y ");
      if (e.leftBumper) Serial.print("LB ");
      if (e.rightBumper) Serial.print("RB ");
      if (e.dpadUp)    setMotors(MOTOR_UP);
      if (e.dpadDown) setMotors(MOTOR_DOWN);
      if (e.dpadLeft)  setMotors(MOTOR_LEFT);
      if (e.dpadRight) setMotors(MOTOR_RIGHT);
      if (e.menuButton) Serial.print("MENU ");  // Бывшая Start
      if (e.viewButton) Serial.print("VIEW ");  // Бывшая Back
      if (e.xboxButton) Serial.print("XBOX ");
      if (e.leftStickButton) Serial.print("leftStickButton");
      if (e.rightStickButton) Serial.print("rightStickButton");
      
      Serial.println();
      
   
      prevEvent = e;  // Сохраняем для следующей проверки
    }
  }
  
else {
Serial.println("Ожидание Xbox геймпада...");
    delay(2000);
}
  delay(50);
}
