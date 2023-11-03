#include <Arduino.h>
#include <HardwareSerial.h>
#include <DaikinController.h>
#include <esp_task_wdt.h>


#define TAG "Main"

#define BTN_1 0
#define LED_ON 5
#define LED_ACT 6


#define WDT_TIMEOUT 30



DaikinController hvac;

bool led_sta = LOW;





void IRAM_ATTR InterruptBTN() {
  static volatile unsigned long BTNPresedTime = 0; 
  if(!digitalRead(BTN_1)){    //Pressed
    BTNPresedTime = millis();
    digitalWrite(LED_ACT,1);
    ESP.restart();
    return;
  }else{

    unsigned long pressedTime = millis() - BTNPresedTime;


    digitalWrite(LED_ACT,0);
    if (pressedTime > 5000){
      uint8_t *arr;
      arr[5] = 10;
      }


  }
  


}

void safeMode(){
  Serial.begin(115200);       //USB CDC
  delay(1000);

  Serial.println("Safemode entered");
  pinMode(LED_ON,OUTPUT);
  pinMode(LED_ACT,OUTPUT);

  while(1){
      digitalWrite(LED_ON,!digitalRead(LED_ON));
      delay(2000);
  }


}


void setup() {

  if (esp_reset_reason() == ESP_RST_TASK_WDT){
    esp_task_wdt_init(60, true); 
    safeMode();
  }

  esp_task_wdt_init(WDT_TIMEOUT, true); 
  esp_task_wdt_add(NULL); 

  Serial.begin(115200);       //USB CDC
  delay(1000);


  Serial.println("BOOT");
  pinMode(LED_ON,OUTPUT);
  pinMode(LED_ACT,OUTPUT);

  digitalWrite(LED_ON,HIGH);
  digitalWrite(LED_ACT,HIGH);

	pinMode(BTN_1, INPUT);
	attachInterrupt(BTN_1, InterruptBTN, CHANGE);



  hvac.connect(&Serial0);

  delay(2000);
  digitalWrite(LED_ACT,LOW);

}


void loop() {
  hvac.sync();
  hvac.readState();

  // HVACSettings newSettings;
  // newSettings.power = "OFF";
  // newSettings.fan = "AUTO";
  // newSettings.temperature = 28.5;
  // newSettings.verticalVane = "HOLD";
  // newSettings.horizontalVane = "HOLD";
  // hvac.setBasic({"OFF","COOL",25.5,"AUTO","HOLD","HOLD"});
  // hvac.setVerticalVaneSetting("SWING");
  // hvac.setPowerSetting(true);
  hvac.setFanSpeed("AUTO");
  hvac.setTemperature(27.5);
  hvac.update();
  // // hvac.set(1,(DaikinClimateMode) '1',25.0,(DaikinFanMode)'A');

  delay(20000);
  // hvac.readState();
  esp_task_wdt_reset();

}