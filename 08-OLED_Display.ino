// ---------------------------------------------------------
// 08-OLED_Display.ino
// Pantalla OLED 128x32 con librería personalizada
// Rotación de valores Modbus cacheados
// ---------------------------------------------------------
#include <Wire.h>
#include <U8g2lib.h>
#include <WPriv_OLED128X32emasesa.h>  // Tu librería con funciones de display

#include "internalClient.h"
// ---------------------------------------------------------
// DISPLAY CONFIG
// ---------------------------------------------------------
U8G2 *u8g2;  // Puntero al objeto U8G2 (inicializado en initOLEAD)
// ---------------------------------------------------------
// TIMING DISPLAY
// ---------------------------------------------------------
const unsigned long DISPLAY_INTERVAL = 5000;   // 6 segundos es el punto dulce en este caso 
// ---------------------------------------------------------
// EXTERNOS (estado interno del gateway)
// ---------------------------------------------------------
//extern const byte slaveIDs[];
//extern const byte NUM_SLAVES;
//extern float slaveData[];
//extern unsigned long slaveLastUpdate[];
//extern bool slaveDisabled[];
//extern bool slaveIsNew[];
//extern const unsigned long STALE_TIMEOUT;
// ---------------------------------------------------------
// NOMBRES DE VARIABLES (UI)
// ---------------------------------------------------------
struct slave_Disp_data {
  const char* name; 
  const char* units;
  const int n_dec;
  //const float max_value; quiza en un futuro...
};
const slave_Disp_data slaveConfig[] = {
  {"Cl2", "ppm", 3},          // ID 1
  {"COND", "uS", 1},         // ID 2
  {"REDOX", "mV", 1},        // ID 3
  {"TURB", "NTU", 3},        // ID 4
  {"PH", "pH", 2}           // ID 5
  //{"Temperature", "C", 1}   // ID 10 (Cambiado "degrees" por "°C" para que quede mejor en pantalla)
};

// ---------------------------------------------------------
// ESTADO DISPLAY
// ---------------------------------------------------------
byte currentIdx = 0;
unsigned long lastChange = 0;

enum DisplayState {
  OLED_IDLE,
  START_RENDER,
  RENDERING_PAGES
};
DisplayState displayState = OLED_IDLE;
//unsigned long lastChange = 0;
//const unsigned long DISPLAY_INTERVAL = 5000; // 5 segundos entre sensores
//byte currentIdx = 0;
// Variables globales para guardar el texto actual y no recalcular en cada ráfaga
char cachedLine1[20];
char cachedLine2[40];
// Icono de EMASESA (debe estar definido en tu librería o en otro lugar)
extern const unsigned char emasesa_icon[];

unsigned long lastSliceTime = 0;
// ---------------------------------------------------------
// INIT OLEAD
// ---------------------------------------------------------
void initOLEAD() {
  // Mantenemos el constructor de alta velocidad para la pasarela
  u8g2 = new U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
  u8g2->setBusClock(400000); 
  u8g2->begin();

  Wire.setTimeout(2); 

  // ──> CORRECCIÓN CRÍTICA DEL LOGO:
  // Envolvemos el dibujo en un bucle 'do-while' de páginas.
  // Esto obliga a la pantalla a procesar las 4 franjas de 8px del logo.
  u8g2->firstPage();
  do {
    // Llamamos a tu función de librería, pero ahora se ejecutará cooperativamente 
    // por cada franja de la pantalla, completando el logotipo de EMASESA.
    showLogo2LinesLeft(*u8g2, emasesa_icon, "EMASESA", "Calidad del agua");
  } while (u8g2->nextPage());

  // Dejamos el delay para que los operarios en planta puedan ver el logo al arrancar
  delay(1500); 
}
/*
void initOLEAD() {
  // Crear el objeto U8G2 según tu hardware
  u8g2 = new U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
  u8g2->setBusClock(400000); // Esto le dice a U8g2 que configure internamente el bus a 400kHz
  u8g2->begin();
  // ¡AQUÍ LO PONES! 
  // Una vez que u8g2->begin() ha encendido el hardware I2C, le aplicamos el escudo anticaídas:
  // En lugar de setWireTimeout, en algunas versiones de SAMD21 se usa:
  Wire.setTimeout(3); // El tiempo límite en milisegundos o microsegundos según la variante
  // Pantalla de inicio con logo
  showLogo2LinesLeft(*u8g2, emasesa_icon, "EMASESA", "Calidad del agua");
  delay(1500);
}
*/
// ---------------------------------------------------------
// Dibuja pantalla actual (llamada por updateOLEAD)
// ---------------------------------------------------------
/*
void drawOLEAD(byte idx) {
  char line1[20];
  char line2[40];  
  unsigned long now = millis();
  
  // Línea 1: Nombre del esclavo
  strcpy(line1, slaveConfig[idx].name);
  
  // Línea 2: Evaluación del estado real del dato
  if (slaveDisabled[idx]) {
    strcpy(line2, "OFFLINE");
  } else if (slaveLastUpdate[idx] == 0) {
    strcpy(line2, "unknown");
  } else {
    // Convertimos el valor flotante a string según sus decimales configurados
    String valStr = String(slaveData[idx], slaveConfig[idx].n_dec);  
    
    if ((now - slaveLastUpdate[idx]) > STALE_TIMEOUT) {
      // ESTADO 1: El tiempo ha expirado sin actualizaciones -> STALE [Dato] [Unidad]
      snprintf(line2, sizeof(line2), "Stale %s %s", valStr.c_str(), slaveConfig[idx].units);
    } 
    else if (slaveIsNew[idx]) {
      // ESTADO 2: El dato acaba de entrar en este ciclo -> NEW [Dato] [Unidad]
      snprintf(line2, sizeof(line2), "n %s %s", valStr.c_str(), slaveConfig[idx].units);
      
      // CRÍTICO: Una vez mostrado una vez en pantalla, deja de ser "nuevo"
      slaveIsNew[idx] = false; 
    } 
    else {
      // ESTADO 3: El dato es válido pero es el mismo de antes -> [Dato] [Unidad] (Sin prefijos)
      snprintf(line2, sizeof(line2), "%s %s", valStr.c_str(), slaveConfig[idx].units);
    }
  }
  
  // Mostrar en la pantalla física
  show2LinesFull(*u8g2, line1, line2);
}
*/
/*
void julian_Draw(byte idx, float data){
  char line1[20];
  char line2[40];  
  strcpy(line1, slaveConfig[idx].name);
  snprintf(line2, sizeof(line2), "new: %f %s", data, slaveConfig[idx].units);
  show2LinesFull(*u8g2, line1, line2);
}
*/
/*
void drawOLEAD(byte idx) {
  char line1[20];
  char line2[30];  // Un poco más grande para valor + estado
  unsigned long now = millis();
  
  // Línea 1: nombre del esclavo
  strcpy(line1, slaveNames[idx]);
  
  // Línea 2: estado + valor
  if (slaveDisabled[idx]) {
    strcpy(line2, "OFFLINE");
  } else if (slaveLastUpdate[idx] == 0) {
    strcpy(line2, "UNKNOWN");
  } else {
    String valStr = String(slaveData[idx], 2);  // 2 decimales
    if ((now - slaveLastUpdate[idx]) > STALE_TIMEOUT) {
      sprintf(line2, "STALE %s", valStr.c_str());
    } else {
      sprintf(line2, "OK    %s", valStr.c_str());
    }
  }
  
  // Mostrar usando la función de dos líneas de tu librería
  show2LinesFull(*u8g2, line1, line2);
}
*/
// ---------------------------------------------------------
// Dibuja pantalla actual (llamada por updateOLEAD)
// ---------------------------------------------------------
// ---------------------------------------------------------
// Dibuja pantalla actual (Modificado para arquitectura de páginas)
// ---------------------------------------------------------
void drawWaitingOLEAD() {
  char line1[20];
  char line2[20];
  strcpy(line1, "Webserver Active");
  strcpy(line2, "");

  // ──> CORRECCIÓN CRÍTICA: Bucle de páginas para renderizado completo
  u8g2->firstPage();
  do {
    // Se llama repetidamente para que pinte el texto en cada franja de la pantalla
    show2LinesFull(*u8g2, line1, line2);
  } while (u8g2->nextPage());
}
// ---------------------------------------------------------
// Llamar desde loop() periódicamente
// ---------------------------------------------------------
void prepareTextData(byte idx) {
  unsigned long now = millis();
  
  // Línea 1: Nombre del esclavo
  strcpy(cachedLine1, slaveConfig[idx].name);
  
  // Línea 2: Evaluación del estado real del dato
  if (slaveDisabled[idx]) {
    strcpy(cachedLine2, "OFFLINE");
  } else if (slaveLastUpdate[idx] == 0) {
    strcpy(cachedLine2, "unknown");
  } else {
    String valStr = String(slaveData[idx], slaveConfig[idx].n_dec);  
    
    if ((now - slaveLastUpdate[idx]) > STALE_TIMEOUT) {
      snprintf(cachedLine2, sizeof(cachedLine2), "Stale %s %s", valStr.c_str(), slaveConfig[idx].units);
    } 
    else if (slaveIsNew[idx]) {
      snprintf(cachedLine2, sizeof(cachedLine2), "->%s %s", valStr.c_str(), slaveConfig[idx].units);
      slaveIsNew[idx] = false; // Consumir bandera de nuevo
    } 
    else {
      snprintf(cachedLine2, sizeof(cachedLine2), "%s %s", valStr.c_str(), slaveConfig[idx].units);
    }
  }
}
/*
void updateOLEAD() { 
  unsigned long now = millis();

  switch (displayState) {
    
    case OLED_IDLE:
      if (now - lastChange >= DISPLAY_INTERVAL) {
        lastChange = now;
        prepareTextData(currentIdx);
        displayState = START_RENDER;
      }
      break;

    case START_RENDER:
      u8g2->firstPage();
      lastSliceTime = now; 
      displayState = RENDERING_PAGES;
      break;

    case RENDERING_PAGES:
      // ──> CONTROL CRÍTICO DE TRÁFICO: 
      // Si no han pasado al menos 6 milisegundos desde la última página del OLED, 
      // salimos inmediatamente. Esto le da ventanas de tiempo reales al loop() 
      // para atender los buffers serie de Modbus RTU y los sockets de Modbus TCP.
      if (now - lastSliceTime < 6) {
        return; 
      }
      lastSliceTime = now; // Actualizar el temporizador de la rebanada

      // Uso de fuentes '_tf' o '_tr' optimizadas en peso para pasarelas de datos:
      // Línea 1: Sensor (12px)
      u8g2->setFont(u8g2_font_helvB12_tr); 
      u8g2->drawStr(0, 13, cachedLine1);

      // Línea 2: Telemetría (12px)
      u8g2->setFont(u8g2_font_helvB12_tr);
      u8g2->drawStr(0, 30, cachedLine2);

      // nextPage() devuelve 0 cuando el dibujo de las 4 páginas ha terminado
      if (u8g2->nextPage() == 0) {
        displayState = OLED_IDLE;
        
        currentIdx++;
        if (currentIdx >= NUM_SLAVES) {
          currentIdx = 0;
        }
      }
      break;
  }
}
*/



void updateOLEAD() { 
  unsigned long now = millis();
  switch (displayState) {
    
    case OLED_IDLE:
      // Esperar a que expire el tiempo del carrusel o a que entre telemetría fresca
      if (now - lastChange >= DISPLAY_INTERVAL) {
        lastChange = now;
        
        // Procesar las cadenas de texto en memoria (operación ultra rápida de CPU)
        prepareTextData(currentIdx);
        
        // Disparar la secuencia de envío físico
        displayState = START_RENDER;
      }
      break;
    case START_RENDER:
      // Iniciar el entorno de rebanadas de página de la librería U8g2
      u8g2->firstPage();
      displayState = RENDERING_PAGES;
      break;
    case RENDERING_PAGES:
      // 1. Configurar fuente y posición para la Línea 1 (Nombre del Sensor)
      u8g2->setFont(u8g2_font_9x15_tr); // Fuente Helvetica de buen tamaño y negrita
      u8g2->drawStr(0, 15, cachedLine1);    // Subimos a Y=12 para dar espacio abajo

      // 2. Configurar fuente y posición para la Línea 2 (Datos / Estado)
      //u8g2->setFont(u8g2_font_profont12_mf); // Fuente muy clara para números y datos
      u8g2->setFont(u8g2_font_9x15_tr);
      u8g2->drawStr(0, 31, cachedLine2);    // Posición Y=28 para la línea inferior

      // nextPage() envía esta sección por I2C (~1.5ms) y devuelve 0 al terminar el frame
      if (u8g2->nextPage() == 0) {
        displayState = OLED_IDLE;
        
        currentIdx++;
        if (currentIdx >= NUM_SLAVES) {
          currentIdx = 0;
        }
      }
      break;
  }
}

/*
void updateOLEAD() { 
  unsigned long now = millis();
  if (now - lastChange < DISPLAY_INTERVAL) return;
  lastChange = now;
  drawOLEAD(currentIdx);
  currentIdx++;
  if (currentIdx >= NUM_SLAVES) {
    currentIdx = 0;
  }
}*/

//---------------------------------------------------------ANTERIOR ---------------------------------------------------
/*
// ---------------------------------------------------------
// 08-OLED_Display.ino
// Pantalla OLED 128x32 con librería personalizada
// Rotación de valores Modbus cacheados
// ---------------------------------------------------------

#include <Wire.h>
#include <U8g2lib.h>
#include <WPriv_OLED128X32emasesa.h>  // Tu librería con funciones de display

// ---------------------------------------------------------
// DISPLAY CONFIG
// ---------------------------------------------------------
U8G2 *u8g2;  // Puntero al objeto U8G2 (inicializado en initOLEAD)

// ---------------------------------------------------------
// TIMING DISPLAY
// ---------------------------------------------------------
const unsigned long DISPLAY_INTERVAL = 5000;   // 6 segundos es el punto dulce en este caso 

// ---------------------------------------------------------
// EXTERNOS (estado interno del gateway)
// ---------------------------------------------------------
extern const byte slaveIDs[];
extern const byte NUM_SLAVES;
extern float slaveData[];
extern unsigned long slaveLastUpdate[];
extern bool slaveDisabled[];
extern bool slaveIsNew[];
extern const unsigned long STALE_TIMEOUT;

// ---------------------------------------------------------
// NOMBRES DE VARIABLES (UI)
// ---------------------------------------------------------
struct slave_Disp_data {
  const char* name; 
  const char* units;
  const int n_dec;
  //const float max_value; quiza en un futuro...
};

const slave_Disp_data slaveConfig[] = {
  {"Cl2", "ppm", 3},          // ID 1
  {"COND", "uS", 1},         // ID 2
  {"REDOX", "mV", 1},        // ID 3
  {"TURB", "NTU", 3},        // ID 4
  {"PH", "pH", 2}           // ID 5
  //{"Temperature", "C", 1}   // ID 10 (Cambiado "degrees" por "°C" para que quede mejor en pantalla)
};


// ---------------------------------------------------------
// ESTADO DISPLAY
// ---------------------------------------------------------
byte currentIdx = 0;
unsigned long lastChange = 0;

// Icono de EMASESA (debe estar definido en tu librería o en otro lugar)
extern const unsigned char emasesa_icon[];

// ---------------------------------------------------------
// INIT OLEAD
// ---------------------------------------------------------
void initOLEAD() {
  // Crear el objeto U8G2 según tu hardware
  u8g2 = new U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
  u8g2->setBusClock(400000); // Esto le dice a U8g2 que configure internamente el bus a 400kHz
  u8g2->begin();

  // ¡AQUÍ LO PONES! 
  // Una vez que u8g2->begin() ha encendido el hardware I2C, le aplicamos el escudo anticaídas:
  // En lugar de setWireTimeout, en algunas versiones de SAMD21 se usa:
  Wire.setTimeout(3); // El tiempo límite en milisegundos o microsegundos según la variante

  // Pantalla de inicio con logo
  showLogo2LinesLeft(*u8g2, emasesa_icon, "EMASESA", "Calidad del agua");
  delay(1500);
}


// ---------------------------------------------------------
// Dibuja pantalla actual (llamada por updateOLEAD)
// ---------------------------------------------------------
void drawOLEAD(byte idx) {
  char line1[20];
  char line2[40];  
  unsigned long now = millis();
  
  // Línea 1: Nombre del esclavo
  strcpy(line1, slaveConfig[idx].name);
  
  // Línea 2: Evaluación del estado real del dato
  if (slaveDisabled[idx]) {
    strcpy(line2, "OFFLINE");
  } else if (slaveLastUpdate[idx] == 0) {
    strcpy(line2, "unknown");
  } else {
    // Convertimos el valor flotante a string según sus decimales configurados
    String valStr = String(slaveData[idx], slaveConfig[idx].n_dec);  
    
    if ((now - slaveLastUpdate[idx]) > STALE_TIMEOUT) {
      // ESTADO 1: El tiempo ha expirado sin actualizaciones -> STALE [Dato] [Unidad]
      snprintf(line2, sizeof(line2), "Stale %s %s", valStr.c_str(), slaveConfig[idx].units);
    } 
    else if (slaveIsNew[idx]) {
      // ESTADO 2: El dato acaba de entrar en este ciclo -> NEW [Dato] [Unidad]
      snprintf(line2, sizeof(line2), "new %s %s", valStr.c_str(), slaveConfig[idx].units);
      
      // CRÍTICO: Una vez mostrado una vez en pantalla, deja de ser "nuevo"
      slaveIsNew[idx] = false; 
    } 
    else {
      // ESTADO 3: El dato es válido pero es el mismo de antes -> [Dato] [Unidad] (Sin prefijos)
      snprintf(line2, sizeof(line2), "%s %s", valStr.c_str(), slaveConfig[idx].units);
    }
  }
  
  // Mostrar en la pantalla física
  show2LinesFull(*u8g2, line1, line2);
}


//void julian_Draw(byte idx, float data){
//
//  char line1[20];
//  char line2[40];  
//
//  strcpy(line1, slaveConfig[idx].name);
//  snprintf(line2, sizeof(line2), "new: %f %s", data, slaveConfig[idx].units);
//
//  show2LinesFull(*u8g2, line1, line2);
//}



//void drawOLEAD(byte idx) {
//  char line1[20];
//  char line2[30];  // Un poco más grande para valor + estado
//  unsigned long now = millis();
//  
//  // Línea 1: nombre del esclavo
//  strcpy(line1, slaveNames[idx]);
//  
//  // Línea 2: estado + valor
//  if (slaveDisabled[idx]) {
//    strcpy(line2, "OFFLINE");
//  } else if (slaveLastUpdate[idx] == 0) {
//    strcpy(line2, "UNKNOWN");
//  } else {
//    String valStr = String(slaveData[idx], 2);  // 2 decimales
//    if ((now - slaveLastUpdate[idx]) > STALE_TIMEOUT) {
//      sprintf(line2, "STALE %s", valStr.c_str());
//    } else {
//      sprintf(line2, "OK    %s", valStr.c_str());
//    }
//  }
//  
//  // Mostrar usando la función de dos líneas de tu librería
//  show2LinesFull(*u8g2, line1, line2);
//}


// ---------------------------------------------------------
// Dibuja pantalla actual (llamada por updateOLEAD)
// ---------------------------------------------------------
void drawWaitingOLEAD() {
    char line1[20];
    char line2[20];

    strcpy(line1, "Webserver Active");
    strcpy(line2, "");

    show2LinesFull(*u8g2, line1, line2);
}
// ---------------------------------------------------------
// Llamar desde loop() periódicamente
// ---------------------------------------------------------
void updateOLEAD() { 

  unsigned long now = millis();
  if (now - lastChange < DISPLAY_INTERVAL) return;
  lastChange = now;

  drawOLEAD(currentIdx);

  currentIdx++;
  if (currentIdx >= NUM_SLAVES) {
    currentIdx = 0;
  }
}

*/