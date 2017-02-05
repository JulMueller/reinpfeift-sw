#include <Arduino.h>

#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <SD.h>

void startRuehrwerk();
void stopRuehrwerk();
void updateDisplay();
void checkSwitches();
int getSwitchState();

char kocherAutoFlag = 0; //Variable für Zustand von RW und Kocher Zustand
char rwAutoFlag = 0;
char runFlag = 0;
char appStop = 0;
char switchChangeKocher = 0;
char serialConnected = 0;

String stepName[12] = {  "Aufheizen...",
                         "Einmeischen",
                         "Aufheizen 1. Rast",
                         "1. Rast",
                         "Aufheizen 2. Rast",
                         "2. Rast",
                         "Aufheizen 3. Rast",
                         "3. Rast",
                         "Aufheizen 4. Rast",
                         "4. Rast",
                         "Aufheizen Abmeischen",
                         "Abmeischen"
                      };

OneWire oneWire(PIN_ONE_WIRE_BUS); //setup a one wire instance in general
DallasTemperature sensors(&oneWire); //pass OneWire reference to Dallas Temperature

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

const int chipSelect = 53; //for SD Card

int SerialValue = 0;
int TempIst = 0;
long TimeStart = 0;
int i;

//
int EinmeischTempSoll = 0;
int Rast1TempSoll = 0;
int Rast1MinutenSoll = 0;
int Rast2TempSoll = 0;
int Rast2MinutenSoll = 0;
int Rast3TempSoll = 0;
int Rast3MinutenSoll = 0;
int Rast4TempSoll = 0;
int Rast4MinutenSoll = 0;
int AbmeischTempSoll = 0;
//
int Aufheiz0MinutenIst = 0;
int Aufheiz1MinutenIst = 0;
int Aufheiz2MinutenIst = 0;
int Aufheiz3MinutenIst = 0;
int Aufheiz4MinutenIst = 0;
int AufheizAbmeischMinutenIst = 0;
int AbmeischMinutenIst = 0;
int Rast1MinutenIst = 0;
int Rast2MinutenIst = 0;
int Rast3MinutenIst = 0;
int Rast4MinutenIst = 0;
//
volatile int StartStop = 0;//Flag Start / Stop
int EinmeischFlag = 0;
int SollTempAkt = 0;
int StatusIndex = 0;
int ZeitImStatus = 0;
//
int BufferTemp = 2;

unsigned long previousMillis = 0;
unsigned int seconds = 0;
unsigned int minutes = 0;
//
void setup() {
  lcd.begin(20, 4);
  lcd.print("Reinpfeift Brauomat");
  lcd.setCursor(0, 1);
  lcd.print("V "); lcd.print(VERSION);
  lcd.setCursor(0, 2);
  lcd.print("Starting up...");
  

  pinMode(PIN_KOCHER_RELAIS, OUTPUT);   //Relais für Kocher
  digitalWrite(PIN_KOCHER_RELAIS, HIGH);
  pinMode(PIN_RW_RELAIS, OUTPUT);
  digitalWrite(PIN_RW_RELAIS, HIGH);    //Relais für Rührwerk
  pinMode(PIN_KOCHER_MAN, INPUT);       //Schalter Kocher Manuell
  digitalWrite(PIN_KOCHER_MAN, HIGH);
  pinMode(PIN_KOCHER_AUTO, INPUT);      //Schalter Kocher Automatisch
  digitalWrite(PIN_KOCHER_AUTO, HIGH);
  pinMode(PIN_RW_MAN, INPUT);           //Schalter Rührwerk Manuell
  digitalWrite(PIN_RW_MAN, HIGH);
  pinMode(PIN_RW_AUTO, INPUT);          //Schalter Rührwerk Auto
  digitalWrite(PIN_RW_AUTO, HIGH);

  //Serial.begin(9600); //Serial for PC Communication
  Serial1.begin(9600); //Serial for Android Communication

  #ifdef LOGGING
  if (!SD.begin(chipSelect)) {
    lcd.clear();
    lcd.print("Fehler:        ");
    lcd.setCursor(0, 1);
    lcd.print("Keine SD Karte");
    delay(2000);
  }
  #endif

  sensors.begin(); //start up the sensor library
  delay(3);
  lcd.clear();

  lcd.print("Startup finished.");
  delay(3);
}

void loop() {
  unsigned long currentMillis = millis();
  #ifdef LOGGING
    if ((currentMillis - previousMillis) >= 1000) {
       seconds++;
       if(seconds >= 60) {
        minutes++;
        seconds = 0;
       }
       if(seconds%10 == 0) {
        log();
       }
       previousMillis = currentMillis;
    }
  #endif
  // get temperatures
  sensors.requestTemperatures(); // Send the command to get temperatures
  TempIst = (int) sensors.getTempCByIndex(0);

  //check status of switches
  checkSwitches();

  if (Serial1.available() > 0) {
    if (serialConnected == 0) {
      lcd.clear();
      lcd.setCursor(0, 2);
      lcd.print("Verbindung");
      lcd.setCursor(0, 3);
      lcd.print("hergestellt");
      serialConnected = 1;
    }
    delay(3);

    SerialValue = Serial1.read();
    if (SerialValue == BT_CMD_TIMER_UPDATE) { //Sende Brauparameter zu Android
      delay(3);
      Serial1.println(TempIst);         // sende Werte
      Serial1.println(EinmeischTempSoll);
      Serial1.println(Rast1TempSoll);
      Serial1.println(Rast1MinutenSoll);
      Serial1.println(Rast2TempSoll);
      Serial1.println(Rast2MinutenSoll);
      Serial1.println(Rast3TempSoll);
      Serial1.println(Rast3MinutenSoll);
      Serial1.println(Rast4TempSoll);
      Serial1.println(Rast4MinutenSoll);
      Serial1.println(AbmeischTempSoll);
      Serial1.println(StatusIndex);
      Serial1.println(ZeitImStatus);
      Serial1.println(kocherAutoFlag);
    }
    if (SerialValue == BT_CMD_START && kocherAutoFlag == 1) { //Start Braugang
      StartStop = 1;
      appStop = 0;
      if (StatusIndex == 0) { //fresh start only
        lcd.clear();
        lcd.setCursor(0, 2);
        lcd.print("Brauprogramm");
        lcd.setCursor(0, 3);
        lcd.print("START");
        TimeStart = millis();
        StatusIndex = 1;
      }
    }
    else if(SerialValue == BT_CMD_START && kocherAutoFlag == 0) {
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Kocher Schalter auf");
      lcd.setCursor(0,2);
      lcd.print("\"automatisch\"");
      lcd.setCursor(0, 3);
      lcd.print("und erneut versuchen!");
      delay(2000);
    }
    if (SerialValue == BT_CMD_STOP) { //Stopp Braugang
      StartStop = 0;
      appStop = 1;
	    rwAutoFlag = 0;
    }
    if (SerialValue == BT_CMD_SEND_RECIPE) { //Android sendet Werte
      delay(3);
      EinmeischTempSoll = Serial1.read(); // read Werte
      delay(3);
      Rast1TempSoll = Serial1.read();
      delay(3);
      Rast1MinutenSoll = Serial1.read();
      delay(3);
      Rast2TempSoll = Serial1.read();
      delay(3);
      Rast2MinutenSoll = Serial1.read();
      delay(3);
      Rast3TempSoll = Serial1.read();
      delay(3);
      Rast3MinutenSoll = Serial1.read();
      delay(3);
      Rast4TempSoll = Serial1.read();
      delay(3);
      Rast4MinutenSoll = Serial1.read();
      delay(3);
      AbmeischTempSoll = Serial1.read();
      lcd.clear();
      lcd.setCursor(0, 1);
      lcd.print("Brauprogramm");
      lcd.setCursor(0, 2);
      lcd.print("erfolgreich");
      lcd.setCursor(0, 3);
      lcd.print("empfangen");
      delay(1500);

    }
    if (SerialValue == BT_CMD_MASHED ) { // EimeischButten gedrueckt
      EinmeischFlag = 1;
      lcd.clear();
      lcd.setCursor(0, 2);
      lcd.print("Einmaischen");
      lcd.setCursor(0, 3);
      lcd.print("eingeleitet");
      delay(1500);
    }

    if (SerialValue == BT_CMD_PLUS_5) { //Zeit +5 min via Android
      switch(StatusIndex) {
        case 4: //1. Rast
          Rast1MinutenSoll += 5;
          break;
        case 6:  //2. Rast
          Rast2MinutenSoll += 5;
          break;
        case 8:  //3. Rast
          Rast3MinutenSoll += 5;
          break;
        case 10: //4. Rast
          Rast4MinutenSoll += 5;
          break;
        default:
           lcd.setCursor(0, 0);
          lcd.print("Nicht möglich");
          lcd.setCursor(1,0);
          lcd.print("in diesem Status");
          break;
      }
    }
    if (SerialValue == BT_CMD_PREVIOUS_STEP) { //change to previous step
      if(StatusIndex > 1) {
        StatusIndex--;
        lcd.clear();
      }
      else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Nicht moeglich");
        delay(1000);
      }
    }
    if (SerialValue == BT_CMD_NEXT_STEP) { //change to next step
      if(StatusIndex < 12) {
        StatusIndex++;
        lcd.clear();
      }
      else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Nicht moeglich");
        delay(1000);
      }
    }
  }

  else if (serialConnected == 0) // Keine Verbindung zu Android
  {
    lcd.clear();
    lcd.setCursor(0, 3);
    lcd.print("Warte auf Android");
    for (int i = 0; i < 3; i++)
    {
      delay(250);
      lcd.setCursor(17 + i, 3);
      lcd.print(".");
      delay(250);
    }
  }
  //
  if (StartStop == 1) { //falls Braugang laufen darf
    if (StatusIndex == 1) { //Aufheizen für Einmeischen
      SollTempAkt = EinmeischTempSoll;
      Aufheiz0MinutenIst = int((millis() - TimeStart) / 60000);//Zeit im Status "Aufheizen Einmeischen"
      ZeitImStatus = Aufheiz0MinutenIst;//für Übermittlung zu Android
      if (TempIst >= EinmeischTempSoll-BufferTemp) {
        StatusIndex = 2;
        lcd.clear();
      }
    }
    if (StatusIndex == 2) { //Einmeischen
      SollTempAkt = EinmeischTempSoll;
      Aufheiz0MinutenIst = int((millis() - TimeStart) / 60000);//Zeit im Status "Aufheizen Einmeischen"
      ZeitImStatus = Aufheiz0MinutenIst;//für Übermittlung zu Android
      if (EinmeischFlag == 1) {
        StatusIndex = 3;
        lcd.clear();
      }
    }
    if (StatusIndex == 3) { //Aufheizen 1. Rast
      SollTempAkt = Rast1TempSoll;
      Aufheiz1MinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst;//Zeit im Status "Aufheizen 1. Rast"
      ZeitImStatus = Aufheiz1MinutenIst;//für Übermittlung zu Android
      if (TempIst >= Rast1TempSoll-BufferTemp) { //Rast 1Temp. ist erreicht
        StatusIndex = 4;//Rast 1 beginnt
        lcd.clear();
      }
    }
    if (StatusIndex == 4) { //1. Rast
      Rast1MinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst - Aufheiz1MinutenIst;//Zeit im Status "1. Rast"
      ZeitImStatus =  Rast1MinutenIst;
      if (Rast1MinutenIst >= Rast1MinutenSoll) { //wenn Rast 1 Zeit abgelaufen...
        StatusIndex = 5;//Aufheizen für 2. Rast beginnt
        lcd.clear();
      }
    }
    if (StatusIndex == 5) { //Aufheizen 2. Rast
      SollTempAkt = Rast2TempSoll;
      Aufheiz2MinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst - Aufheiz1MinutenIst - Rast1MinutenIst;//Zeit im Status "Aufheizen 2. Rast"
      ZeitImStatus = Aufheiz2MinutenIst;//für Übermittlung zu Android
      if (TempIst >= Rast2TempSoll-BufferTemp) { //Rast 2 Temp. ist erreicht
        StatusIndex = 6;//Rast 2 beginnt
        lcd.clear();
      }
    }
    if (StatusIndex == 6) { //2. Rast
      Rast2MinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst - Aufheiz1MinutenIst - Rast1MinutenIst - Aufheiz2MinutenIst;//Zeit im Status "2. Rast"
      ZeitImStatus =  Rast2MinutenIst;
      if (Rast2MinutenIst >= Rast2MinutenSoll) { //wenn Rast 2 Zeit abgelaufen...
        StatusIndex = 7;//Aufheizen für 3. Rast beginnt
        lcd.clear();
      }
    }
    if (StatusIndex == 7) { //Aufheizen 3. Rast
      SollTempAkt = Rast3TempSoll;
      Aufheiz3MinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst - Aufheiz1MinutenIst - Rast1MinutenIst - Aufheiz2MinutenIst - Rast2MinutenIst;//Zeit im Status "Aufheizen 3. Rast"
      ZeitImStatus = Aufheiz3MinutenIst;//für Übermittlung zu Android
      if (TempIst >= Rast3TempSoll) { //Rast 3 Temp. ist erreicht
        StatusIndex = 8;//Rast 3 beginnt
        lcd.clear();
      }
    }
    if (StatusIndex == 8) { //3. Rast
      Rast3MinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst - Aufheiz1MinutenIst - Rast1MinutenIst - Aufheiz2MinutenIst - Rast2MinutenIst - Aufheiz3MinutenIst;//Zeit im Status "3. Rast"
      ZeitImStatus =  Rast3MinutenIst;
      if (Rast3MinutenIst >= Rast3MinutenSoll) { //wenn Rast 3 Zeit abgelaufen...
        StatusIndex = 9;//Aufheizen für 4. Rast beginnt
        lcd.clear();
      }
    }
    if (StatusIndex == 9) { //Aufheizen 4. Rast
      SollTempAkt = Rast4TempSoll;
      Aufheiz4MinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst - Aufheiz1MinutenIst - Rast1MinutenIst - Aufheiz2MinutenIst - Rast2MinutenIst - Aufheiz3MinutenIst - Rast3MinutenIst;//Zeit im Status "Aufheizen 4. Rast"
      ZeitImStatus = Aufheiz4MinutenIst;//für Übermittlung zu Android
      if (TempIst >= Rast4TempSoll) { //Rast 4 Temp. ist erreicht
        StatusIndex = 10;//Rast 4 beginnt
        lcd.clear();
      }
    }
    if (StatusIndex == 10) { //4. Rast
      Rast4MinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst - Aufheiz1MinutenIst - Rast1MinutenIst - Aufheiz2MinutenIst - Rast2MinutenIst - Aufheiz3MinutenIst - Rast3MinutenIst - Aufheiz4MinutenIst;//Zeit im Status "4. Rast"
      ZeitImStatus =  Rast4MinutenIst;
      if (Rast4MinutenIst >= Rast4MinutenSoll) { //wenn Rast 4 Zeit abgelaufen...
        StatusIndex = 11;//Aufheizen für Abmeischen beginnt
        lcd.clear();
      }
    }
    if (StatusIndex == 11) { //Aufheizen Abmeischen
      SollTempAkt = AbmeischTempSoll;
      AufheizAbmeischMinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst - Aufheiz1MinutenIst - Rast1MinutenIst - Aufheiz2MinutenIst - Rast2MinutenIst - Aufheiz3MinutenIst - Rast3MinutenIst - Aufheiz4MinutenIst - Rast4MinutenIst;//Zeit im Status "Aufheizen Abmeischen"
      ZeitImStatus = AufheizAbmeischMinutenIst;//für Übermittlung zu Android
      if (TempIst >= AbmeischTempSoll) { //Abmeisch Temp. ist erreicht
        StatusIndex = 12;//Abmeischen beginnt = Ende
        lcd.clear();
      }
    }
    if (StatusIndex == 12) { //Abmeischen
      AbmeischMinutenIst = int((millis() - TimeStart) / 60000) - Aufheiz0MinutenIst - Aufheiz1MinutenIst - Rast1MinutenIst - Aufheiz2MinutenIst - Rast2MinutenIst - Aufheiz3MinutenIst - Rast3MinutenIst - Aufheiz4MinutenIst - Rast4MinutenIst - AufheizAbmeischMinutenIst;//Zeit im Status "Abmeischen"
      ZeitImStatus =  AbmeischMinutenIst;
    }

    //**********************
    // Begin heating control
    //**********************
    if ((TempIst < (SollTempAkt)) && (digitalRead(PIN_KOCHER_RELAIS) == HIGH)) { //turn on heater if temp too low
      digitalWrite(PIN_KOCHER_RELAIS, LOW);
      lcd.begin(20, 4);
      delay(1000);
    }
    if ((TempIst >= SollTempAkt) && (digitalRead(PIN_KOCHER_RELAIS) == LOW)) { //turn off heater if temp too high
      digitalWrite(PIN_KOCHER_RELAIS, HIGH);
      lcd.begin(20, 4);
      delay(1000);
    }
    if (rwAutoFlag == 1 && digitalRead(PIN_RW_RELAIS) == HIGH && StartStop == 1) { //turn on RW
        digitalWrite(PIN_RW_RELAIS, LOW);
    }
    if ((rwAutoFlag == 0 && digitalRead(PIN_RW_RELAIS) == LOW && digitalRead(PIN_RW_MAN) == HIGH) || StartStop == 0) {
        digitalWrite(PIN_RW_RELAIS, HIGH);
    }
    //end heating control
    //**********************

    //lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(stepName[StatusIndex - 1]);
    lcd.setCursor(0, 1);
    lcd.print("Temp:  "); lcd.print(TempIst); lcd.print(" ("); lcd.print(SollTempAkt); lcd.print(") C");
    lcd.setCursor(0, 2);
    switch(StatusIndex) {
      case 4:
        lcd.print("t:     "); lcd.print(ZeitImStatus);  lcd.print(" ("); lcd.print(Rast1MinutenSoll); lcd.print(")  min");
        break;
      case 6:
        lcd.print("t:     "); lcd.print(ZeitImStatus);  lcd.print(" ("); lcd.print(Rast2MinutenSoll); lcd.print(")  min");
        break;
      case 8:
        lcd.print("t:     "); lcd.print(ZeitImStatus);  lcd.print(" ("); lcd.print(Rast3MinutenSoll); lcd.print(")  min");
        break;
      case 10:
        lcd.print("t:     "); lcd.print(ZeitImStatus);  lcd.print(" ("); lcd.print(Rast4MinutenSoll); lcd.print(")  min");
        break;
      default:
        lcd.print("t:     "); lcd.print(ZeitImStatus); lcd.print("  min");
        break;
    }
    lcd.setCursor(0, 3);
    lcd.print("t tot: "); lcd.print(int((millis() - TimeStart) / 60000)); lcd.print("  min");

  }//ende if StartStop = 1
  if (StartStop == 0 && (digitalRead(PIN_KOCHER_MAN) == HIGH) && (digitalRead(PIN_RW_MAN) == HIGH) && serialConnected == 1) { //falls Braugang gestoppt werden soll
    rwAutoFlag = 0;
    digitalWrite(PIN_KOCHER_RELAIS, HIGH);
    digitalWrite(PIN_RW_RELAIS, HIGH);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Braugang PAUSE");
    lcd.setCursor(0, 2);
    lcd.print("Temp:  "); lcd.print(TempIst); lcd.print(" C");
    lcd.setCursor(0, 3);
    lcd.print("t tot: "); lcd.print(int((millis() - TimeStart) / 60000)); lcd.print(" min");
    delay(100);
  }
}

void checkSwitches()
{
  //--Man Kocher Mode ON
  if (digitalRead(PIN_KOCHER_MAN) == LOW) {
    digitalWrite(PIN_KOCHER_RELAIS, LOW); //turn on Kocher
    StartStop = 0;
    lcd.clear();
    lcd.setCursor(0, 2);
    lcd.print("Kocher MANUELL");
    lcd.setCursor(0, 3);
    lcd.print("Temp:  "); lcd.print(TempIst); lcd.print(" ("); lcd.print(SollTempAkt); lcd.print(") C");
    delay(100);
  }
  //--Man Kocher Mode OFF
  else if (digitalRead(PIN_KOCHER_MAN) == HIGH && (StartStop != 1)) {
    digitalWrite(PIN_KOCHER_RELAIS, HIGH); //turn off Kocher
  }
  //--Auto Kocher Mode ON
  if (digitalRead(PIN_KOCHER_AUTO) == LOW) { //if Switch on Auto Mode
    kocherAutoFlag = 1;
    if(StatusIndex >= 1 && appStop == 0) {
      StartStop = 1;
     }
  }
  //--Auto Kocher Mode OFF
  else if (digitalRead(PIN_KOCHER_AUTO) == HIGH) { //if Switch on OFF
    kocherAutoFlag = 0;
    StartStop = 0;
  }
  //--Man RW Mode ON
  if (digitalRead(PIN_RW_MAN) == LOW) {
    rwAutoFlag = 0;
    digitalWrite(PIN_RW_RELAIS, LOW);
  }
  //--Man RW Mode OFF
  else if (digitalRead(PIN_RW_MAN) == HIGH && digitalRead(PIN_RW_AUTO) == HIGH) {
    digitalWrite(PIN_RW_RELAIS, HIGH);
    rwAutoFlag = 0;
  }
  //--Auto RW Mode ON
  if (digitalRead(PIN_RW_AUTO) == LOW) {
    rwAutoFlag = 1;
  }
  //--Auto RW Mode OFF
  else if (digitalRead(PIN_RW_AUTO) == HIGH) {
    rwAutoFlag = 0;
  }
}

#ifdef LOGGING
void log() {
  String dataString = "";
  dataString = String(minutes)+ "." + String(seconds) + "," + String(TempIst) + "," + String(StatusIndex);
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if(dataFile) {
    dataFile.println(dataString);
    dataFile.close();
  }
  else {
    lcd.clear();
    lcd.print("SD Error datalog.txt");
  }
}
#endif
