#include <Arduino.h>
#include <XSpaceBioV10.h>
#include <XSControl.h>
#include <NimBLEDevice.h>
#include <SPIFFS.h>

// Global variables to store raw and filtered ECG values
double raw_ecg = 0;
double filtered_ecg = 0;
double raw_ecg1 = 0;
double filtered_ecg1 = 0;
double filter3 = 0;
double filter3_3 = 0;

// Create an instance of the XSpaceBioV10Board to interact with the board
XSpaceBioV10Board Board;

// Create an instance of the XSFilter for signal filtering
XSFilter Filter;
XSFilter Filter1;

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

#define SERVICE_UUID        "0000180f-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "00002a19-0000-1000-8000-00805f9b34fb"

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
  }
};

void FilterTask(void *pv) {
  while (1) {
    raw_ecg = Board.AD8232_GetVoltage(AD8232_XS1);
    raw_ecg1 = Board.AD8232_GetVoltage(AD8232_XS2);

    filtered_ecg = Filter.SecondOrderLPF(raw_ecg, 40, 0.001);
    filtered_ecg1 = Filter1.SecondOrderLPF(raw_ecg1, 40, 0.001);
    filter3 = raw_ecg - raw_ecg1;
    filter3_3 = filtered_ecg - filtered_ecg1;

    if (deviceConnected) {
      char ecgData[100];
      snprintf(ecgData, sizeof(ecgData), "%.2f,%.2f,%.2f", filtered_ecg, filtered_ecg1, filter3_3);

      pCharacteristic->setValue(ecgData);
      pCharacteristic->notify();
    }

    // Guardar los datos en el archivo
    File file = SPIFFS.open("/ecg_data.txt", FILE_APPEND);
    if (file) {
      file.printf("%.2f,%.2f,%.2f\n", filtered_ecg, filtered_ecg1, filter3_3);
      file.close();
    }

    vTaskDelay(1);
  }
  vTaskDelete(NULL);
}

void setup() {
  Serial.begin(115200);
  Board.init();
  Board.AD8232_Wake(AD8232_XS1);
  Board.AD8232_Wake(AD8232_XS2);

  // Inicializar SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Error al montar el sistema de archivos");
    return;
  }

  xTaskCreate(FilterTask, "FilterTask", 3000, NULL, 1, NULL);

  NimBLEDevice::init("AuriCheck");
  NimBLEServer *pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID,
                    NIMBLE_PROPERTY::READ | 
                    NIMBLE_PROPERTY::NOTIFY
                  );
  pCharacteristic->setValue("0");

  pService->start();
  NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
  Serial.println("Waiting for a client connection...");
}

void loop() {
  // Empty loop
}






