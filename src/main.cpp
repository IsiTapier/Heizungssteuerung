#include <Arduino.h>
#include "webserver.h"

#define LED_BLUE D5
#define LED_RED D6

#define MOTOR_L D1
#define MOTOR_R D2

#define TASTER_RED D8
#define TASTER_BLACK D7
#define TASTER_BLUE A0

bool tasterRedState = 0;
bool tasterBlackState = 0;
bool tasterBlueState = 0;

unsigned long lastTimeCheck = 0;
unsigned long lastTasterCheck = 0;
unsigned long lastSleepCheck = 0;

#define TASTER_CHECK_PERIOD 20
#define TIME_CHECK_PERIOD 10*1000
#define LAST_ACTIVITY_PERIOD 5*60*1000
#define SLEEP_CHECK_PERIOD 1*60*1000
#define PREWAKEUP_TIME 5

unsigned long openTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(2000);
  delay(100);
  Serial.println();
 
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(MOTOR_L, OUTPUT);
  pinMode(MOTOR_R, OUTPUT);
  
  pinMode(TASTER_RED, INPUT);
  pinMode(TASTER_BLACK, INPUT);
  // pinMode(TASTER_BLUE, INPUT);

  // digitalWrite(TASTER_RED, 0);
  // digitalWrite(TASTER_BLACK, 0);
  // digitalWrite(TASTER_BLUE, 0);

  digitalWrite(MOTOR_L, 0);
  digitalWrite(MOTOR_R, 0);
  webserverSetup();
}

void turnMotor(int amount) {
  digitalWrite(MOTOR_L, amount>0);
  digitalWrite(MOTOR_R, amount<0);
  delay(abs(amount));
  digitalWrite(amount>0?MOTOR_L:MOTOR_R, 0);
}

void openValve(bool force = settings.force) {
  if(settings.valveState && !force) return;
  if(!force) settings.valveState = 1;
  if(!force) notifyClients(getStateValues());
  turnMotor(settings.impulsDuration/(force?10:1));
  if(force) return;
  Serial.println("Heizung an");
  openTime = millis();
  storeEEPROM();
}

void closeValve(bool force = settings.force) {
  if(!settings.valveState && !force) return;
  if(!force) settings.valveState = 0;
  if(!force) notifyClients(getStateValues());
  turnMotor(-settings.impulsDuration/(force?10:1));
  if(force) return;
  Serial.println("Heizung aus");
  storeEEPROM();
}

int timeStringToMinutes(String timestring) {
  int hours = timestring.substring(0, 2).toInt();
  int minutes = timestring.substring(3).toInt();
  return hours*60 + minutes;
}

void loop() {
  webserverLoop();

  if(millis()-lastTasterCheck < TASTER_CHECK_PERIOD && !(pressRed || pressBlack || pressBlack)) return;
  bool tasterRedValue = digitalRead(TASTER_RED) || pressRed;
  bool tasterBlackValue = digitalRead(TASTER_BLACK) || pressBlack;
  bool tasterBlueValue = analogRead(TASTER_BLUE)>1000 || pressBlue;
  pressRed = false; pressBlack = false; pressBlue = false;
  if(tasterRedValue && (!tasterRedState || settings.force)) openValve();
  if(tasterBlueValue && (!tasterBlueState || settings.force)) closeValve();
  if(tasterBlackValue && !tasterBlackState) { settings.force = !settings.force; notifyClients(getStateValues()); storeEEPROM(); closeValve(false); }
  tasterRedState = tasterRedValue;
  tasterBlackState = tasterBlackValue;
  tasterBlueState = tasterBlueValue;
  if(tasterRedValue||tasterBlackValue||tasterBlueValue) lastActivity = millis();
  digitalWrite(LED_RED, (millis()%5000<40&&settings.valveState) || (settings.force&&millis()%1000<400));
  digitalWrite(LED_BLUE, (millis()%5000<20&&!settings.valveState) || (settings.force&&millis()%1000<200));
  lastTasterCheck = millis();

  if(millis()-lastTimeCheck < TIME_CHECK_PERIOD) return;
  // auto close
  if(settings.valveState && millis()-openTime > settings.warmphaseDuration*60*1000) closeValve(false);
  
  // auto open
  bool weekend = timeClient.getDay() == 0 || timeClient.getDay() == 6;
  int currentMin = timeClient.getHours()*60 + timeClient.getMinutes();
  for(String time : (weekend ? settings.weekendHeattimes : settings.workdayHeattimes)) {
    int min = timeStringToMinutes(time);
    if(currentMin == min) openValve(false);
  }
  lastTimeCheck = millis();

  if(millis()-lastSave >= AUTO_SAVE_PERIOD) storeEEPROM();

  // deepsleep
  if(millis()-lastSleepCheck < SLEEP_CHECK_PERIOD) return;
  lastSleepCheck = millis();
  if(settings.valveState) return;
  if(settings.force) return;
  if(!ws.getClients().isEmpty()) return;
  // if(millis()-lastActivity < LAST_ACTIVITY_PERIOD) return;
  int startMin = timeStringToMinutes(weekend ? settings.weekendStart : settings.workdayStart);
  int endMin   = timeStringToMinutes(weekend ? settings.weekendEnd   : settings.workdayEnd);
  if((currentMin >= startMin || startMin > endMin) && currentMin < endMin) {
    int sleepTime = 71;
    sleepTime = min(sleepTime, (currentMin<endMin ? endMin-currentMin : endMin+24*60-currentMin));

    for(String time : (weekend ? settings.weekendHeattimes : settings.workdayHeattimes)) {
      int heatmin = timeStringToMinutes(time);
      sleepTime = min(sleepTime, (currentMin<heatmin ? heatmin-currentMin : (timeClient.getDay()==5 ? sleepTime : heatmin+24*60-currentMin))-PREWAKEUP_TIME);
    }
    if(currentMin<endMin && timeClient.getDay()==5)
      for(String time : (weekend ? settings.weekendHeattimes : settings.workdayHeattimes)) {
        int heatmin = timeStringToMinutes(time);
        sleepTime = min(sleepTime, (currentMin<heatmin ? heatmin-currentMin : (timeClient.getDay()==5 ? sleepTime : heatmin+24*60-currentMin))-PREWAKEUP_TIME);
      }
    
    if(sleepTime > 0) {
      double sleepCountdown = (LAST_ACTIVITY_PERIOD-(long)(millis()-lastActivity))/6e4;
      Serial.println("Will sleep "+(sleepCountdown>0 ? "in "+String(sleepCountdown)+" Minutes" : "now")+" for "+String(sleepTime)+" Minutes.");
      if(millis()-lastActivity < LAST_ACTIVITY_PERIOD) return;

      storeEEPROM(true);
      ESP.deepSleep(sleepTime*60*1e6);
    }
  }

}
