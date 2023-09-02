#include <NimBLEDevice.h>
#define PinAnalogIn 25
#define PinDigitalIn 27

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;
char valNoti[20];
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

bool rqsNotify;
unsigned long prvMillis;
#define INTERVAL_READ 1000
int valNotify;

float voltage;
int bat_percentage;
int analogInBatt = 4;
int sensorValue;
float calibration = 0.3;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;

      rqsNotify = false;
      prvMillis = millis();
      Serial.println("Device connected");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      rqsNotify = false;
      Serial.println("Device disconnected");
    }
};

void prcRead(){
  if (deviceConnected){
    unsigned long curMillis = millis();
    if((curMillis - prvMillis)>= INTERVAL_READ){
      int valDIO = digitalRead(PinDigitalIn);
      int valAIO = analogRead(PinAnalogIn);
      valNotify = map(valAIO, 0, 4095, 0,255);
      sensorValue = analogRead(analogInBatt);
      voltage = (( (sensorValue * 3.3) / 4095 ) *2) + calibration;    // analog value *3.3V/4095
      bat_percentage= ((voltage - 2.8)/0.1)*7.142857; // example 2.8V=0%, 2.81V=7.142857%,.... example (3.9V)-2.8V=1.1V 0.1V=7.142857%, so 1.1V=78.57%
      sprintf(valNoti,"%d|%d|%d",valDIO,valNotify,bat_percentage);
      //Serial.println(valNotify);
      //Serial.println(valNoti);
      //Serial.println(sensorValue);
      //Serial.println(voltage);
      //Serial.println(bat_percentage);
      rqsNotify = true;
      prvMillis = curMillis;
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PinDigitalIn,INPUT_PULLUP);
  pinMode(14,OUTPUT);
  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID, 
                      NIMBLE_PROPERTY::READ   |
                      NIMBLE_PROPERTY::WRITE  |
                      NIMBLE_PROPERTY::NOTIFY |
                      NIMBLE_PROPERTY::INDICATE
                    );
  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  
  /** Note, this could be left out as that is the default value */
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter

  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  attachInterrupt(digitalPinToInterrupt(27),wakeup, CHANGE);
  //LowPower.powerDown(SLEEP_FOREVER,ADC_OFF,BOD_OFF);
  detachInterrupt(digitalPinToInterrupt(27));

  digitalWrite(14,HIGH);
  delay(500);
  digitalWrite(14,LOW);
    // notify changed value
    if (deviceConnected) {
      if(rqsNotify){
        rqsNotify = false;
        
        //value=valNotify;
        pCharacteristic->setValue(std::string(valNoti, 8));
        pCharacteristic->notify();
        //delay(3); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    prcRead();
}
void wakeup(){

}
