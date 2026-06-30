// ---------------------------------------------------------
// 07-internalClient.ino
// Cliente interno Modbus RTU (polling periódico)
// ---------------------------------------------------------
#include "internalClient.h"
// ---- Configuración ----

const byte slaveIDs[NUM_SLAVES] = {1, 2, 3, 4, 5};
//const byte slaveIDs[] = {1, 2, 3, 4, 5}; 
//const byte NUM_SLAVES = sizeof(slaveIDs) / sizeof(slaveIDs[0]);

// Configuración de polling interno
//struct PollConfig {
//  byte func;        // 0x03 = holding registers, 0x04 = input registers
//  uint16_t address; // Dirección inicial del primer registro
//};
//Variables de control de ploling
const unsigned long INTERNAL_POLL_THRESHOLD = 30000; // 30 segundos sin actualizar -> hacer polling
const unsigned long INTERNAL_CHECK_INTERVAL = 5000;  // Revisar cada 5 segundos

bool internalPending[NUM_SLAVES] = {false};          // true si hay una petición interna en curso
unsigned long lastInternalCheck = 0;
// Ajustar las direcciones según tu hardware real
const PollConfig pollConfig[NUM_SLAVES] = {
  {0x03, 0x0000}, // ID 1: Cloro   (input register 0)
  {0x03, 0x0000}, // ID 2: pH       (input register 2)
  {0x03, 0x0000}, // ID 3: Densidad (input register 4)
  {0x03, 0x0000}, // ID 4: Turbidez (input register 6)
  {0x03, 0x0000}, // ID 5: Caudal   (input register 8)
  //{0x03, 0x0000}, // ID 10: Temperatura (input register 10)
};

float slaveData[NUM_SLAVES];          // último valor leído
unsigned long slaveLastUpdate[NUM_SLAVES];
bool slaveIsNew[NUM_SLAVES] = {false}; 

//--------------Estados --------------//
const unsigned long STALE_TIMEOUT     = 90000; // 90 segundos    
const unsigned long OFFLINE_TIMEOUT = 120000; // 120 segundos = 4 minutos

// Gestión de reintentos / offline
//byte slaveRetries[NUM_SLAVES] = {0};
bool slaveDisabled[NUM_SLAVES] = {false};
unsigned long slaveDisabledUntil[NUM_SLAVES] = {0};

const unsigned long OFFLINE_RETRY_TIME = 300000; // 5 minutos
unsigned long slaveOfflineSince[NUM_SLAVES] = {0};

const uint16_t MAX_TRY_PENDINGS = 5;  // intentamos 3 veces enviar desde internal, si no vuelve.....mal
uint16_t countPendings[NUM_SLAVES] = {0};


byte count_slave_polling = 0;

uint16_t tid_to_search = 0;

// ---------------------------------------------------------
// Actualiza valor cuando llega respuesta (llamada externa)
// ---------------------------------------------------------
void updateSlaveData(byte slaveID, float value) { // esta funcion la llaman 

    int i = slaveID - 1; 
    slaveData[i] = value; 

    //ESTADO
    slaveLastUpdate[i] = millis();  
    slaveDisabled[i] = false; // 
    internalPending[i] = false;
    slaveIsNew[i] = true;

    countPendings[i] = 0; 
 
}

// ---------------------------------------------------------
// Averigua si Slave queda sin responder por x tiempo y le 
// da de baja marcando Offline (llamada externa desde main loop)
// ---------------------------------------------------------
void checkOfflineStatus()
{
    unsigned long now = millis();

    for (byte i = 0; i < NUM_SLAVES; i++)
    {
        // -------- ONLINE --------
        if (!slaveDisabled[i]){
            if (slaveLastUpdate[i] != 0 && now - slaveLastUpdate[i] > OFFLINE_TIMEOUT){
                slaveDisabled[i] = true;
                slaveOfflineSince[i] = now;

                Serial.print("[INT] Slave ");
                Serial.print(slaveIDs[i]);
                Serial.println(" OFFLINE");
            }
        }else { // -------- OFFLINE --------
            if (now - slaveOfflineSince[i] > OFFLINE_RETRY_TIME){
                Serial.print("[INT] Reintentando slave ");
                Serial.println(slaveIDs[i]);

                slaveDisabled[i] = false;

                // forzar polling inmediato
                slaveLastUpdate[i] = 0;

                // permitir nueva petición
                internalPending[i] = false;

                // limpiar marca
                slaveOfflineSince[i] = 0;
            }
        }
    }
}

// ---------------------------------------------------------
// Encola peticion interna
// ---------------------------------------------------------

void enqueueInternalRequest(byte idx) {
  if (idx >= NUM_SLAVES) return;
  if (internalPending[idx]) return;      // Ya hay una petición en curso
  if (slaveDisabled[idx]) return;        // Esclavo marcado como offline, no intentar

  byte slaveID = slaveIDs[idx];
  byte func = pollConfig[idx].func;
  uint16_t addr = pollConfig[idx].address;
  uint16_t quantity = 2; // Leer 2 registros (float)

  // PDU: [func, addrHigh, addrLow, quantHigh, quantLow]
  byte pdu[] = {func, highByte(addr), lowByte(addr), highByte(quantity), lowByte(quantity)};
  byte msgLen = 1 + sizeof(pdu); // unit ID + PDU

  // Verificar espacio en cola (opcional, pero recomendado)
  if (queueHeaders.available() < 1 || queueData.available() < msgLen) {
    // No hay espacio, lo intentamos más tarde
    return;
  }
  // Marcar como pendiente
  internalPending[idx] = true;

  // Insertar en cola
  queueHeaders.push(header_t{
    { 0x00, 0x00 },           // transaction ID (no usado)
    msgLen,                   // longitud total en queueData
    { 0, 0, 0, 0 },           // remoteIP (0 para interno)
    0,                        // remotePort (0)
    INTERNAL_REQUEST,         // tipo de request
    0                         // atts (sin usar)
  });

  queueData.push(slaveID);    // unit ID
  for (byte i = 0; i < sizeof(pdu); i++) {
    queueData.push(pdu[i]);
  }
}

/*
// ---------------------------------------------------------
// Encola peticion interna (CON PRIORIDAD AL FRENTE)
// ---------------------------------------------------------
void enqueueInternalRequest(byte idx) {

  if (idx >= NUM_SLAVES) return;
  //if (internalPending[idx]) return; 

  if (internalPending[idx]){ // esta pendiente de la ultima vez......problemas de conexion??? que hacemos? 
    
    if(countPendings[idx] < MAX_TRY_PENDINGS){ // no podemos eternamente estar metiendo datos en la cola...
      countPendings[idx]++; 
    }else{
      return;  // si se intentaron x veces y no se consiguio...ya esperar al offline
    }
    
  }      // Ya hay una petición en curso...
  
  if (slaveDisabled[idx]) return;        // Esclavo marcado como offline, no intentar

  byte slaveID = slaveIDs[idx];
  byte func = pollConfig[idx].func;
  uint16_t addr = pollConfig[idx].address;
  uint16_t quantity = 2; // Leer 2 registros (float)

  // PDU: [func, addrHigh, addrLow, quantHigh, quantLow]
  byte pdu[] = {func, highByte(addr), lowByte(addr), highByte(quantity), lowByte(quantity)};
  byte msgLen = 1 + sizeof(pdu); // unit ID + PDU

  // Verificar espacio en cola
  if (queueHeaders.available() < 1 || queueData.available() < msgLen) {
    // No hay espacio, lo intentamos más tarde
    return;
  }

  // Marcar como pendiente
  internalPending[idx] = true;

  // ========================================================
  // INYECCIÓN PRIORITARIA USANDO UNSHIFT()
  // ========================================================
  
  // 1. Los datos Modbus de queueData se deben insertar al revés en la cabeza 
  // para que al hacerles 'shift()' en el bucle principal salgan en orden correcto:
  // Orden normal de salida deseado: [slaveID, func, addrH, addrL, quantH, quantL]
  
  for (int i = sizeof(pdu) - 1; i >= 0; i--) {
    queueData.unshift(pdu[i]); // Metemos la PDU al revés desde el final
  }
  queueData.unshift(slaveID);   // Al final metemos el Unit ID en el frente del todo

  // 2. Insertamos la cabecera al principio de queueHeaders
  queueHeaders.unshift(header_t{
    { 0x00, 0x00 },           // transaction ID (no usado)
    msgLen,                   // longitud total en queueData
    { 0, 0, 0, 0 },           // remoteIP (0 para interno)
    0,                        // remotePort (0)
    INTERNAL_REQUEST,         // tipo de request (¡Pasa al frente de la cola!)
    0                         // atts (sin usar)
  });
}
*/

// ---------------------------------------------------------
// Verifica Polling interno
// ---------------------------------------------------------
// forzamos envio cada 30 segundos (si no ha habido peticion por parte de SCADA)
/*
void checkInternalPolling() {
  unsigned long now = millis();
  if (now - lastInternalCheck < INTERNAL_CHECK_INTERVAL) return;
  lastInternalCheck = now;

  // Evaluamos secuencialmente usando el índice actual de rotación del cliente
  byte i = count_slave_polling;  
  if (!slaveDisabled[i]) {
    // ...y ha pasado más de INTERNAL_POLL_THRESHOLD desde la última actualización
    if (slaveLastUpdate[i] == 0 || (now - slaveLastUpdate[i] > INTERNAL_POLL_THRESHOLD)) {
      enqueueInternalRequest(i);
    }
  }

  count_slave_polling++; 
  if(count_slave_polling >= NUM_SLAVES){
    count_slave_polling = 0; 
  }
}
*/
void checkInternalPolling() {
  unsigned long now = millis();
  if (now - lastInternalCheck < INTERNAL_CHECK_INTERVAL) return;
  lastInternalCheck = now;

  for (byte i = 0; i < NUM_SLAVES; i++) {
    // Si el esclavo no está deshabilitado y no hay petición pendiente...
    if (!slaveDisabled[i] && !internalPending[i]) {
      // ...y ha pasado más de INTERNAL_POLL_THRESHOLD desde la última actualización
      if (slaveLastUpdate[i] == 0 || (now - slaveLastUpdate[i] > INTERNAL_POLL_THRESHOLD)) {
        enqueueInternalRequest(i);
      }
    }
  }
}

// esta funcion captura las tramas que viajan de vuelta al modbus TCP
// estas tramas pueden llegar generadas por el scada
// estas tramas pueden llegar generadas por el internal. 
bool internalClientFunction(const byte MBAP[] ,const byte PDU[],  const uint16_t pduLength){

  //bool idValido = false; 
  //for (int i = 0; i < NUM_SLAVES; i++) {
  //  if (PDU[0] == slaveIDs[i]) {
  //    idValido = true; 
  //    break; 
  //  }
  //}
 //
  //if (!idValido) return;

  // 1. Reconstruimos el TID de la respuesta de forma segura
  uint16_t tid_respuesta = (MBAP[0] << 8) | MBAP[1];

  // de momento solo tendremos en cuenta los tid_to_search
  if(tid_respuesta != 0){ // si el tid es 0 , es uno del internal client
    if (tid_to_search != tid_respuesta) { 
      //Serial.println("a"); 
      return true; // Peticio de SCADA que no esta dentro de las ID deseadas, ni en las direcciones y size. 
    }
  }
  tid_to_search = 0; //una vez valorado el tid_to_search lo reinicializamos por si acaso. 

  //Serial.println("a"); 
  if (digitalRead(DI_7) != LOW) return true; // todo pensar
  // --- DEBUG RAW ---
   // Serial.print("[MODBUS RESP] PDU: ");
   // for (int i = 0; i < pduLength; i++) {
   //   Serial.print(PDU[i], HEX);
   //   Serial.print(" ");
   // }
   // Serial.println();
  byte func = PDU[1];

  if ((func == 0x03 || func == 0x04) && pduLength >= 7 && PDU[2] >= 4 && (PDU[2] % 4 == 0)) {
    byte byteCount = PDU[2];

    for (int i = 0; i < byteCount; i += 4) {

      int dataOffset = 3 + i;

      int32_t intValue; // nuveo cambio para hacer la conversion

      uint8_t intBytes[4] = {
        PDU[dataOffset + 3], // D
        PDU[dataOffset + 2], // C
        PDU[dataOffset + 1], // B
        PDU[dataOffset + 0]  // A
      };

      memcpy(&intValue, intBytes, 4);

      //  Serial.print("Entero 32 bits (DCBA): ");
      //  Serial.println(intValue);

      float value = (float)intValue / 1000;
        
      //float value = (float)intValue;

      // --- NUEVO: guardar para display ---
      updateSlaveData(PDU[0], value);

    }
  }
  if(tid_respuesta == 0){ // en caso de que la peticion haya sido hecha por el internal client, se indica false para evitar que se envie repuesta al modbus TCP. 
    return false; 
  }
  return true; 
}



