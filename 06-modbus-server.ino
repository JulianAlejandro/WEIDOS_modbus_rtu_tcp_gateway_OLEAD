#include <Arduino.h>


// FUNCTION CODES
static const uint8_t READ_COILS_FC = 1;
static const uint8_t READ_DISCRETE_INPUTS_FC = 2;
static const uint8_t READ_HOLDING_REGISTERS_FC = 3;
static const uint8_t READ_INPUT_REGISTERS_FC = 4;
static const uint8_t WRITE_SINGLE_COIL_FC = 5;
static const uint8_t WRITE_SINGLE_REGISTER_FC = 6;
static const uint8_t WRITE_MULTIPLE_COILS_FC = 15;
static const uint8_t WRITE_MULTIPLE_REGISTERS_FC = 16;

//  EXEPTION CODES
static const uint8_t ILLEGAL_FUNCTION = 0x01;
static const uint8_t ILLEGAL_DATA_ADDRESS = 0x02;

#define MAPPING_2
#ifndef MAPPING_2

static const uint16_t VOLTAGE_RESOLUTION = 1023;

static const uint8_t NUM_DI = 8;
static const uint8_t NUM_AI = 4;
static const uint8_t NUM_AO = 1;
static const uint8_t NUM_DO = 4;
uint32_t analogInputs[NUM_AI] = {ADI_0, ADI_1, ADI_2, ADI_3};
uint32_t digitalInputs[NUM_DI] = {ADI_0, ADI_1, ADI_2, ADI_3, DI_4, DI_5, DI_6, DI_7};
uint32_t analogOutput[NUM_AO] = {AO_0};
uint32_t digitalOutputs[NUM_DO] = {DO_0, DO_1, DO_2, DO_3};





// SERVER MAPPING
//Coils (Digital Outputs)
static const uint8_t FIRST_COIL_ADDRESS = 0;
static const uint8_t NUM_COILS = NUM_DO;
static const uint8_t LAST_COIL_ADDRESS = FIRST_COIL_ADDRESS + NUM_COILS;
bool coils[NUM_COILS];

//Discrete inputs (ADI/DI):
static const uint8_t FIRST_DISCRETE_INPUT_ADDRESS = 0;
static const uint8_t NUM_DISCRETE_INPUTS = NUM_DI;
static const uint8_t LAST_DISCRETE_INPUT_ADDRESS = FIRST_DISCRETE_INPUT_ADDRESS + NUM_DISCRETE_INPUTS;


//Input registers (Analog Inputs):
static const uint8_t FIRST_INPUT_REGISTER_ADDRESS = 0;
static const uint8_t NUM_INPUT_REGISTERS = NUM_AI;
static const uint8_t LAST_INPUT_REGISTER_ADDRESS = FIRST_INPUT_REGISTER_ADDRESS + NUM_INPUT_REGISTERS;

//Holding registers (Analog Output):
static const uint8_t FIRST_HOLDING_REGISTER_ADDRESS = 0;
static const uint8_t NUM_HOLDING_REGISTER = NUM_AO;
static const uint8_t LAST_HOLDING_REGISTER_ADDRESS = FIRST_HOLDING_REGISTER_ADDRESS + NUM_HOLDING_REGISTER;
uint16_t holdingRegisters[NUM_HOLDING_REGISTER];





void initCoils(){
  for(int i=0; i<NUM_COILS; i++) coils[i] = 0;
}

void initHoldingRegisters(){
  for(int i=0; i<NUM_HOLDING_REGISTER; i++) holdingRegisters[i] = 0;
}


void readCoilStatus(uint8_t txNdx, byte* response, uint8_t& rxNdx){
  modbus_request_t request;
  request.modbusId = queueData[txNdx];
  request.functionCode = queueData[txNdx+1];

  uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
  uint16_t quantity = (queueData[txNdx+4] << 8) + queueData[txNdx+5];

  if(address<FIRST_COIL_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  if(address + quantity > LAST_COIL_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  response[rxNdx++] = request.modbusId;
  response[rxNdx++] = READ_COILS_FC;
  uint8_t numBytes = quantity/8;
  if(quantity%8 !=0 ) numBytes++;
  

  response[rxNdx++] = numBytes;
  response[rxNdx] = 0;
  int bitCounter = 0;

  for(int i=address; i<address+quantity; i++){
      uint16_t data = getCoilValue(i);
      response[rxNdx] = (response[rxNdx]>>1) + (getCoilValue(i)<<7);
      bitCounter++;

      if(bitCounter%8 == 0 && i != address+quantity-1){
        rxNdx++;
        response[rxNdx] = 0;
      } 
  }

  while(bitCounter%8 != 0){
    response[rxNdx] = (response[rxNdx]>>1);
    bitCounter++;
  }
  rxNdx++;

}

void readDiscreteInputs(uint8_t txNdx, byte* response, uint8_t& rxNdx){
  modbus_request_t request;
  request.modbusId = queueData[txNdx];
  request.functionCode = queueData[txNdx+1];
  //andre//Serial.println(FIRST_DISCRETE_INPUT_ADDRESS)

  uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
  uint16_t quantity = (queueData[txNdx+4] << 8) + queueData[txNdx+5];

  if(address<FIRST_DISCRETE_INPUT_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  if(address + quantity > LAST_DISCRETE_INPUT_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  response[rxNdx++] = request.modbusId;
  response[rxNdx++] = READ_DISCRETE_INPUTS_FC;
  uint8_t numBytes = quantity/8;
  if(quantity%8 !=0 ) numBytes++;
  

  response[rxNdx++] = numBytes;
  response[rxNdx] = 0;
  int bitCounter = 0;

  for(int i=address; i<address+quantity; i++){
      uint16_t data = getDiscreteInputValue(i);
      response[rxNdx] = (response[rxNdx]>>1) + (getDiscreteInputValue(i)<<7);
      bitCounter++;

      if(bitCounter%8 == 0 && i != address+quantity-1){
        rxNdx++;
        response[rxNdx] = 0;
      } 
  }

  while(bitCounter%8 != 0){
    response[rxNdx] = (response[rxNdx]>>1);
    bitCounter++;
  }
  rxNdx++;

}

void readHoldingRegisters(uint8_t txNdx, byte* response, uint8_t& rxNdx){
  modbus_request_t request;
  request.modbusId = queueData[txNdx];
  request.functionCode = queueData[txNdx+1];

  uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
  uint16_t quantity = (queueData[txNdx+4] << 8) + queueData[txNdx+5];

  if(address<FIRST_HOLDING_REGISTER_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  if(address + quantity > LAST_HOLDING_REGISTER_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  response[rxNdx++] = request.modbusId;
  response[rxNdx++] = READ_HOLDING_REGISTERS_FC;
  response[rxNdx++] = quantity*2;
  
  for(int i=0; i<quantity; i++){
      int index = address + i;
      uint16_t data = holdingRegisters[index];
      response[rxNdx++] = highByte(data);
      response[rxNdx++] = lowByte(data);
  }
}

void readInputRegisters(uint8_t txNdx, byte* response, uint8_t& rxNdx){
  modbus_request_t request;
  request.modbusId = queueData[txNdx];
  request.functionCode = queueData[txNdx+1];

  uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
  uint16_t quantity = (queueData[txNdx+4] << 8) + queueData[txNdx+5];

  if(address<FIRST_INPUT_REGISTER_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  if(address + quantity > LAST_INPUT_REGISTER_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  response[rxNdx++] = request.modbusId;
  response[rxNdx++] = READ_INPUT_REGISTERS_FC;
  response[rxNdx++] = quantity*2;
  
  for(int i=address; i<address+quantity; i++){
      uint16_t data = getInputRegisterValue(i);
      response[rxNdx++] = highByte(data);
      response[rxNdx++] = lowByte(data);
  }
}

void writeSingleCoil(uint8_t txNdx, byte* response, uint8_t& rxNdx){
  modbus_request_t request;
      request.modbusId = queueData[txNdx];
      request.functionCode = queueData[txNdx+1];

      uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
      uint16_t status = (queueData[txNdx+4] << 8) + queueData[txNdx+5];

      if(address<FIRST_COIL_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      if(address > LAST_COIL_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      if(status == 0xFF00) setCoil(address, true);
      else setCoil(address, false);
      
      
      //Build response
      response[rxNdx++] = request.modbusId;
      response[rxNdx++] = WRITE_SINGLE_COIL_FC;
      response[rxNdx++] = highByte(address);
      response[rxNdx++] = lowByte(address);
      response[rxNdx++] = highByte(status);
      response[rxNdx++] = lowByte(status);
}

void writeSingleHoldingRegister(uint8_t txNdx, byte* response, uint8_t& rxNdx){
      modbus_request_t request;
      request.modbusId = queueData[txNdx];
      request.functionCode = queueData[txNdx+1];

      uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
      uint16_t value = (queueData[txNdx+4] << 8) + queueData[txNdx+5];

      if(address<FIRST_HOLDING_REGISTER_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      if(address > LAST_HOLDING_REGISTER_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      setHoldingRegister(address, value);
 
      //Build response
      response[rxNdx++] = request.modbusId;
      response[rxNdx++] = WRITE_SINGLE_REGISTER_FC;
      response[rxNdx++] = highByte(address);
      response[rxNdx++] = lowByte(address);
      response[rxNdx++] = highByte(value);
      response[rxNdx++] = lowByte(value);
}

void writeMultipleHoldingRegisters(uint8_t txNdx, byte* response, uint8_t& rxNdx){
      modbus_request_t request;
      request.modbusId = queueData[txNdx];
      request.functionCode = queueData[txNdx+1];

      uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
      uint16_t quantity = (queueData[txNdx+4] << 8) + queueData[txNdx+5];
      uint8_t numData = queueData[txNdx+6];


      if(address<FIRST_HOLDING_REGISTER_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      if(address + quantity > LAST_HOLDING_REGISTER_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      for(int i=0; i<quantity; i++){
        uint16_t value = (queueData[txNdx+7+2*i] << 8) + queueData[txNdx+7+(2*i+1)];
        setHoldingRegister(address+i, value);
      }

    
      //Build response
      response[rxNdx++] = request.modbusId;
      response[rxNdx++] = WRITE_MULTIPLE_REGISTERS_FC;
      response[rxNdx++] = highByte(address);
      response[rxNdx++] = lowByte(address);
      response[rxNdx++] = highByte(quantity);
      response[rxNdx++] = lowByte(quantity);
}

void writeMultipleCoils(uint8_t txNdx, byte* response, uint8_t& rxNdx){
      modbus_request_t request;
      request.modbusId = queueData[txNdx];
      request.functionCode = queueData[txNdx+1];

      uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
      uint16_t numCoils = (queueData[txNdx+4] << 8) + queueData[txNdx+5];
      uint8_t numBytes = queueData[txNdx+6];

      if(address<FIRST_COIL_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      if(address + numCoils > LAST_COIL_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }


      uint8_t numWrittenCoils = 0;
      for(int i=0; i<numBytes; i++){
        uint8_t data = queueData[txNdx+7+i];

        for(int j=0; j<8; j++){
          numWrittenCoils++;
          uint8_t value = ((data>>j) & (1));
          setCoil(address+i*8+j, value);
          if(numWrittenCoils == numCoils) break;
        }
      }

      //Build response
      response[rxNdx++] = request.modbusId;
      response[rxNdx++] = WRITE_MULTIPLE_COILS_FC;
      response[rxNdx++] = highByte(address);
      response[rxNdx++] = lowByte(address);
      response[rxNdx++] = highByte(numCoils);
      response[rxNdx++] = lowByte(numCoils);
}


void processSlaveRequest(uint8_t txNdx){
    modbus_request_t request;
    request.modbusId = queueData[txNdx];
    request.functionCode = queueData[txNdx+1];


    static byte response[MODBUS_SIZE];
    static uint8_t rxNdx = 0;

    switch(request.functionCode){
      case READ_COILS_FC:
        readCoilStatus(txNdx, response, rxNdx);
        break;
      case READ_DISCRETE_INPUTS_FC:
        readDiscreteInputs(txNdx, response, rxNdx);
        break;
      case READ_HOLDING_REGISTERS_FC:
        readHoldingRegisters(txNdx, response, rxNdx);
        break;
      case READ_INPUT_REGISTERS_FC:
        readInputRegisters(txNdx, response, rxNdx);
        break;
      case WRITE_SINGLE_COIL_FC:
        writeSingleCoil(txNdx, response, rxNdx);
        break;
      case WRITE_SINGLE_REGISTER_FC:
        writeSingleHoldingRegister(txNdx, response, rxNdx);
        break;
      case WRITE_MULTIPLE_COILS_FC:
        writeMultipleCoils(txNdx, response, rxNdx);
        break;
      case WRITE_MULTIPLE_REGISTERS_FC:
        writeMultipleHoldingRegisters(txNdx, response, rxNdx);
        break;      
      default:
        sendErrorCode(request, ILLEGAL_FUNCTION, response, rxNdx);
        break;
    }


    //Check sum stuff
    crc = 0xFFFF;
    for (byte i = 0; i < rxNdx; i++) {
      calculateCRC(response[i]);
    }
    response[rxNdx++] = lowByte(crc);
    response[rxNdx++] = highByte(crc);


    // Process Serial data
    // Checks: 1) CRC; 2) address of incoming packet against first request in queue; 3) only expected responses are forwarded to TCP/UDP
    header_t myHeader = queueHeaders.first();
    if (checkCRC(response, rxNdx) == true && serialState == WAITING) {
      if (response[1] > 0x80 && (myHeader.requestType & SCAN_REQUEST) == false) {
        setSlaveStatus(response[0], SLAVE_ERROR_0X, true, false);
      } else {
        setSlaveStatus(response[0], SLAVE_OK, true, myHeader.requestType & SCAN_REQUEST);
      }
      byte MBAP[] = {
        myHeader.tid[0],
        myHeader.tid[1],
        0x00,
        0x00,
        highByte(rxNdx - 2),
        lowByte(rxNdx - 2)
      };
      
      sendResponse(MBAP, response, rxNdx);
      serialState = IDLE;
    } else {
      data.errorCnt[ERROR_RTU]++;
    }
#ifdef ENABLE_EXTENDED_WEBUI
    data.rtuCnt[DATA_RX] += rxNdx;
#endif /* ENABLE_EXTENDED_WEBUI */
    rxNdx = 0;

}




//Functions that returns Input Register's value. (Analog value in given pin)
uint16_t getInputRegisterValue(uint16_t address){
  int pinIndex = address;
  uint32_t pin = analogInputs[pinIndex];
  return analogRead(pin);
}

//Functions that returns Discrete Input's value. (ADI/DI value in given pin)
bool getDiscreteInputValue(uint16_t address){
  int pinIndex = address;
  if(address>7) return true;
  uint32_t pin = digitalInputs[pinIndex];
  pinMode(pin, INPUT);
  return digitalRead(pin);
}

//Functions that returns Coil's value. (DO value in given pin)
bool getCoilValue(uint16_t address){
  int index = address;
  return coils[index];
}

//Function to write into Holding registers. It stores the current value and sets it to Analog Output.
void setHoldingRegister(uint16_t address, uint16_t value){
  int index = address;
  holdingRegisters[index] = value;

  if(value>VOLTAGE_RESOLUTION) value = VOLTAGE_RESOLUTION;

  analogWrite(analogOutput[index], value);
  return;
}

//Function to write into Coils. It stores the current value and sets it to a Digital Output.
void setCoil(uint16_t address, bool value){
  int index = address;
  coils[index] = value;
  digitalWrite(digitalOutputs[index], value);
}






void sendErrorCode(modbus_request_t request, uint8_t exeptionCode, byte* response, uint8_t& rxNdx){
    response[rxNdx++] = request.modbusId;
    response[rxNdx++] = request.functionCode + 0x80;
    response[rxNdx++] = exeptionCode;
    return;
}


#endif

#ifdef MAPPING_2

static const uint16_t VOLTAGE_RESOLUTION = 1023;


static const uint8_t NUM_OUTPUTS = 5;
static const uint8_t NUM_INPUTS = 8;
uint32_t outputs[NUM_OUTPUTS] = {AO_0, DO_0, DO_1, DO_2, DO_3};
uint32_t inputs[NUM_INPUTS] = {ADI_0, ADI_1, ADI_2, ADI_3, DI_4, DI_5, DI_6, DI_7};

// SERVER MAPPING

//Input registers (Analog Inputs):
static const uint8_t FIRST_INPUT_REGISTER_ADDRESS = 0;
static const uint8_t NUM_INPUT_REGISTERS = NUM_INPUTS;
static const uint8_t LAST_INPUT_REGISTER_ADDRESS = FIRST_INPUT_REGISTER_ADDRESS + NUM_INPUT_REGISTERS;

//Holding registers (Analog Output):
static const uint8_t FIRST_HOLDING_REGISTER_ADDRESS = 0;
static const uint8_t NUM_HOLDING_REGISTER = NUM_OUTPUTS;
static const uint8_t LAST_HOLDING_REGISTER_ADDRESS = FIRST_HOLDING_REGISTER_ADDRESS + NUM_HOLDING_REGISTER;
uint16_t holdingRegisters[NUM_HOLDING_REGISTER];



void initCoils(){
  return;
}

void initHoldingRegisters(){
  for(int i=0; i<NUM_HOLDING_REGISTER; i++) holdingRegisters[i] = 0;
}




void readHoldingRegisters(uint8_t txNdx, byte* response, uint8_t& rxNdx){
  modbus_request_t request;
  request.modbusId = queueData[txNdx];
  request.functionCode = queueData[txNdx+1];

  uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
  uint16_t quantity = (queueData[txNdx+4] << 8) + queueData[txNdx+5];

  if(address<FIRST_HOLDING_REGISTER_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  if(address + quantity > LAST_HOLDING_REGISTER_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  response[rxNdx++] = request.modbusId;
  response[rxNdx++] = READ_HOLDING_REGISTERS_FC;
  response[rxNdx++] = quantity*2;
  
  for(int i=0; i<quantity; i++){
      int index = address + i;
      uint16_t data = holdingRegisters[index];
      response[rxNdx++] = highByte(data);
      response[rxNdx++] = lowByte(data);
  }
}

void readInputRegisters(uint8_t txNdx, byte* response, uint8_t& rxNdx){
  modbus_request_t request;
  request.modbusId = queueData[txNdx];
  request.functionCode = queueData[txNdx+1];

  uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
  uint16_t quantity = (queueData[txNdx+4] << 8) + queueData[txNdx+5];

  if(address<FIRST_INPUT_REGISTER_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  if(address + quantity > LAST_INPUT_REGISTER_ADDRESS){
      sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
      return;
  }

  response[rxNdx++] = request.modbusId;
  response[rxNdx++] = READ_INPUT_REGISTERS_FC;
  response[rxNdx++] = quantity*2;
  
  for(int i=address; i<address+quantity; i++){
      uint16_t data = getInputRegisterValue(i);
      response[rxNdx++] = highByte(data);
      response[rxNdx++] = lowByte(data);
  }
}

void writeSingleHoldingRegister(uint8_t txNdx, byte* response, uint8_t& rxNdx){
      modbus_request_t request;
      request.modbusId = queueData[txNdx];
      request.functionCode = queueData[txNdx+1];

      uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
      uint16_t value = (queueData[txNdx+4] << 8) + queueData[txNdx+5];

      if(address<FIRST_HOLDING_REGISTER_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      if(address > LAST_HOLDING_REGISTER_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      setHoldingRegister(address, value);
 
      //Build response
      response[rxNdx++] = request.modbusId;
      response[rxNdx++] = WRITE_SINGLE_REGISTER_FC;
      response[rxNdx++] = highByte(address);
      response[rxNdx++] = lowByte(address);
      response[rxNdx++] = highByte(value);
      response[rxNdx++] = lowByte(value);
}

void writeMultipleHoldingRegisters(uint8_t txNdx, byte* response, uint8_t& rxNdx){
      modbus_request_t request;
      request.modbusId = queueData[txNdx];
      request.functionCode = queueData[txNdx+1];

      uint16_t address = (queueData[txNdx+2] << 8) + queueData[txNdx+3];
      uint16_t quantity = (queueData[txNdx+4] << 8) + queueData[txNdx+5];
      uint8_t numData = queueData[txNdx+6];


      if(address<FIRST_HOLDING_REGISTER_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      if(address + quantity > LAST_HOLDING_REGISTER_ADDRESS){
          sendErrorCode(request, ILLEGAL_DATA_ADDRESS, response, rxNdx);
          return;
      }

      for(int i=0; i<quantity; i++){
        uint16_t value = (queueData[txNdx+7+2*i] << 8) + queueData[txNdx+7+(2*i+1)];
        setHoldingRegister(address+i, value);
      }

    
      //Build response
      response[rxNdx++] = request.modbusId;
      response[rxNdx++] = WRITE_MULTIPLE_REGISTERS_FC;
      response[rxNdx++] = highByte(address);
      response[rxNdx++] = lowByte(address);
      response[rxNdx++] = highByte(quantity);
      response[rxNdx++] = lowByte(quantity);
}


void processSlaveRequest(uint8_t txNdx){
    modbus_request_t request;
    request.modbusId = queueData[txNdx];
    request.functionCode = queueData[txNdx+1];


    static byte response[MODBUS_SIZE];
    static uint8_t rxNdx = 0;

    switch(request.functionCode){
      case READ_INPUT_REGISTERS_FC:
        readInputRegisters(txNdx, response, rxNdx);
        break;
      case READ_HOLDING_REGISTERS_FC:
        readHoldingRegisters(txNdx, response, rxNdx);
        break;
      case WRITE_SINGLE_REGISTER_FC:
        writeSingleHoldingRegister(txNdx, response, rxNdx);
        break;
      case WRITE_MULTIPLE_REGISTERS_FC:
        writeMultipleHoldingRegisters(txNdx, response, rxNdx);
        break;      
      default:
        sendErrorCode(request, ILLEGAL_FUNCTION, response, rxNdx);
        break;
    }


    //Check sum stuff
    crc = 0xFFFF;
    for (byte i = 0; i < rxNdx; i++) {
      calculateCRC(response[i]);
    }
    response[rxNdx++] = lowByte(crc);
    response[rxNdx++] = highByte(crc);


    // Process Serial data
    // Checks: 1) CRC; 2) address of incoming packet against first request in queue; 3) only expected responses are forwarded to TCP/UDP
    header_t myHeader = queueHeaders.first();
    if (checkCRC(response, rxNdx) == true && serialState == WAITING) {
      if (response[1] > 0x80 && (myHeader.requestType & SCAN_REQUEST) == false) {
        setSlaveStatus(response[0], SLAVE_ERROR_0X, true, false);
      } else {
        setSlaveStatus(response[0], SLAVE_OK, true, myHeader.requestType & SCAN_REQUEST);
      }
      byte MBAP[] = {
        myHeader.tid[0],
        myHeader.tid[1],
        0x00,
        0x00,
        highByte(rxNdx - 2),
        lowByte(rxNdx - 2)
      };
      
      sendResponse(MBAP, response, rxNdx);
      serialState = IDLE;
    } else {
      data.errorCnt[ERROR_RTU]++;
    }
#ifdef ENABLE_EXTENDED_WEBUI
    data.rtuCnt[DATA_RX] += rxNdx;
#endif /* ENABLE_EXTENDED_WEBUI */
    rxNdx = 0;

}



//Functions that returns Input Register's value. (Analog value in given pin)
uint16_t getInputRegisterValue(uint16_t address){
  int pinIndex = address;
  uint32_t pin = inputs[pinIndex];
  if(isAnalogPin(pin)) return analogRead(pin);
  pinMode(pin, INPUT);
  digitalRead(pin);
}



//Function to write into Holding registers. It stores the current value and sets it to Analog Output.
void setHoldingRegister(uint16_t address, uint16_t value){
  int index = address;
  uint32_t pin = outputs[index];
  if(isAnalogPin(pin)){
    if(value>VOLTAGE_RESOLUTION) value = VOLTAGE_RESOLUTION;
    holdingRegisters[index] = value;
    analogWrite(pin, value);
    return;
  }

  if(value>0) value = 1;
  else value = 0;

  holdingRegisters[index] = value;
  digitalWrite(pin, value);
  return;
}


bool isAnalogPin(uint32_t pin){
  if(pin == pin15) return true;
  if(pin == pin16) return true;
  if(pin == pin45) return true;
  return false;
}




void sendErrorCode(modbus_request_t request, uint8_t exeptionCode, byte* response, uint8_t& rxNdx){
    response[rxNdx++] = request.modbusId;
    response[rxNdx++] = request.functionCode + 0x80;
    response[rxNdx++] = exeptionCode;
    return;
}
#endif
