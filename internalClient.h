#ifndef INTERNAL_CLIENT_H
#define INTERNAL_CLIENT_H

#include <Arduino.h>

// 1. CONSTANTES DE CONFIGURACIÓN DE TAMAÑO Y COLA
#define NUM_SLAVES 5

// 2. DEFINICIÓN DE ESTRUCTURAS
struct PollConfig {
    byte func;        // 0x03 = holding registers, 0x04 = input registers
    uint16_t address; // Dirección inicial del primer registro
};

// 3. ANUNCIOS (extern) PARA EL OTRO INO (08-OLED_Display.ino)
extern const byte slaveIDs[NUM_SLAVES];
extern const PollConfig pollConfig[NUM_SLAVES];

extern float slaveData[NUM_SLAVES];
extern unsigned long slaveLastUpdate[NUM_SLAVES];
extern bool slaveDisabled[NUM_SLAVES];
extern bool slaveIsNew[NUM_SLAVES];

extern const unsigned long STALE_TIMEOUT;

extern uint16_t tid_to_search;

// 4. DECLARACIÓN DE FUNCIONES COMPARTIDAS (Opcional, pero muy recomendado)
void updateSlaveData(byte slaveID, float value);
void checkOfflineStatus();
void enqueueInternalRequest(byte idx);
void checkInternalPolling();
void internalClientFunction(const byte PDU[], const uint16_t pduLength);

#endif