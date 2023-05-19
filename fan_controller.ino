#include <dhtnew.h>
#include <EEPROM.h>

DHTNEW mySensor(10); // 
uint32_t count = 0;
uint32_t start, stop;
uint32_t errors[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
bool fansGo, fansState;
bool volatile manOverride;
bool isErr = false;
int currTemp;
int currHum;


bool sensorChk()
{
  count++;
  
  // show counters for OK and errors.
  // if (count % 50 == 0)
  // {
  //   Serial.println();
  //   Serial.println("OK \tCRC \tTOA \tTOB \tTOC \tTOD \tSNR \tBS \tWFR \tUNK");
  //   for (uint8_t i = 0; i < 10; i++)
  //   {
  //     Serial.print(errors[i]);
  //     Serial.print('\t');
  //   }
  //   Serial.println();
  //   Serial.println();
  // }

  // if (count % 10 == 0)
  // {
  //   Serial.println();
  //   Serial.println("TIM\tCNT\tSTAT\tHUMI\tTEMP\tTIME\tTYPE");
  // }
  // Serial.print(millis());
  // Serial.print("\t");
  // Serial.print(count);
  // Serial.print("\t");

  start = micros();
  int chk = mySensor.read();
  stop = micros();

  switch (chk)
  {
    case DHTLIB_OK:
      Serial.print("OK,\t");
      errors[0]++;
      isErr = false;
      break;
    case DHTLIB_ERROR_CHECKSUM:
      Serial.print("CRC,\t");
      errors[1]++;
      isErr = true;
      break;
    case DHTLIB_ERROR_TIMEOUT_A:
      Serial.print("TOA,\t");
      errors[2]++;
      isErr = true;
      break;
    case DHTLIB_ERROR_TIMEOUT_B:
      Serial.print("TOB,\t");
      errors[3]++;
      isErr = true;
      break;
    case DHTLIB_ERROR_TIMEOUT_C:
      Serial.print("TOC,\t");
      errors[4]++;
      isErr = true;
      break;
    case DHTLIB_ERROR_TIMEOUT_D:
      Serial.print("TOD,\t");
      errors[5]++;
      isErr = true;
      break;
    case DHTLIB_ERROR_SENSOR_NOT_READY:
      Serial.print("SNR,\t");
      errors[6]++;
      isErr = false;
      break;
    case DHTLIB_ERROR_BIT_SHIFT:
      Serial.print("BS,\t");
      errors[7]++;
      isErr = true;
      break;
    case DHTLIB_WAITING_FOR_READ:
      Serial.print("WFR,\t");
      errors[8]++;
      isErr = false;
      break;
    default:
      Serial.print("U");
      Serial.print(chk);
      Serial.print(",\t");
      errors[9]++;
      isErr = false;

      break;
  }

  return isErr;
}

bool measTime() {
  bool meas;

  stop = millis();
  meas = stop - start;
  if (stop >= 5000)
  {
    start = millis();
    return true;
  }
  else
  {
    return false;
  }
}

bool testEnv(int currTemp, int currHum) {

  if (currTemp >= 73)
  {
    return false;
  }
  else if (currTemp > 65 && currHum >= 35)
  {
   return false;
  }
  else
  {
    return true;
  }
  
  
}

void displayData() {
  Serial.print("\nHUMI\t");
  Serial.print("TEMP\t");
  Serial.print("\tTIME");
  Serial.println("");
  Serial.print(mySensor.getHumidity(), 0);
  Serial.print("\t");
  float temp = mySensor.getTemperature();
  Serial.print((1.8*(temp)+32));
  Serial.print("::");
  Serial.print(temp);
  Serial.print("\t");
  Serial.print(stop - start);
  Serial.println("");
  delay(2000);

}


void manual_override()
{
  manOverride  = !manOverride;
  // If button pushed and relay not on
}

void manual_mode()
{
  Serial.println("MANUAL MODE");
  bool stateCh;

    while (manOverride)
  {
    if(digitalRead(5) == 0)
    {
      stateCh = !digitalRead(4);
      digitalWrite(4, stateCh);
      fansState = stateCh;
      Serial.println(stateCh ? "ON" : "OFF");
    }
    delay(200);
  }
}

bool errMode()
{
  fansGo = false;
  stop = millis();
  int chkCode;
  bool errPresent = true;

  while (errPresent)
  {
    chkCode = mySensor.read();
    if (chkCode == DHTLIB_OK)
    {
      errPresent = false;
      return true;
    }
    Serial.print("Err #:");
    Serial.println(chkCode);
    if ((start - micros()) >= 300000)
    {
      
      Serial.println("Timed out with status: " + mySensor.read());
      Serial.println("\Stopping, please reset...");
      return false;
      // ATDEVADR2 0x54 // Lowest page address (default initialiser value) for Address 2
      // uint32_t EEPROMStorage<short> _v2<, >
      // writeUnsignedInt
      
    }
  }

  return true;

}

void setup() {
  
  while(!Serial);        // MKR1010 needs this
  Serial.begin(115200);
  Serial.println(__FILE__);
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHTNEW_LIB_VERSION);
  Serial.println();

  // DHT 22 - 2302
  mySensor.setType(22); 

  // MKR1010 needs this
  mySensor.setDisableIRQ(false);

  displayData();
  //printHeader("EEPROM Contents (" + String(EEPROM.length()) + " bytes)");
  delay(3000);
  // ----------------------------------
  
  //Relay ctl
  pinMode(4, OUTPUT); //Relay Signal
  digitalWrite(4, LOW);  
  pinMode(2, INPUT_PULLUP); // man Button Read / HW interrupt
  attachInterrupt(digitalPinToInterrupt(2), manual_override, FALLING);
  pinMode(7, OUTPUT); // man Button VCC
  pinMode(6, OUTPUT); // man Button VCC
  digitalWrite(5, INPUT_PULLUP);  




  manOverride = false;
  start = micros();

}


void loop() {

  if (manOverride)
  {
    digitalWrite(6, HIGH);
    manual_mode();
  }else
  {
    if (digitalRead(6) == HIGH)
    {
      Serial.println("AUTO MODE");
      digitalWrite(6, LOW);
    }
  }

  if (measTime() == true && !manOverride)
  {
    isErr = sensorChk();
    // If no error codes
    if (!isErr) {
      // Test temp and humidity
      int currTemp = mySensor.getHumidity();
      // Convert C to F
      currTemp = ((1.8*(currTemp))+32);
      int currHum = mySensor.getTemperature();
      if (!testEnv(currTemp, currHum))
      {
        fansGo = true;
      }
    }
    else
    {
      // if (!errMode())
      // {
      // while(true)
      // {
      //   delay(2000);
      // }

      // }
      fansGo = false;
      Serial.println("");
    }
  }

  currTemp = mySensor.getHumidity();
  // Convert C to F
  currTemp = ((1.8*(currTemp))+32);
  currHum = mySensor.getTemperature();
  displayData();
  delay(2000);

  if (fansGo != fansState)
  {
    digitalWrite(4,fansGo);
  }

  fansState = fansGo;
  
}

  // digitalWrite(3,HIGH);
  // delay(5000);
  // digitalWrite(3, LOW);
  // delay(5000);
