/* *******************************************************************
   Ethernet and serial interface functions

   startSerial()
   - starts HW serial interface which we use for RS485 line

   charTime(), charTimeOut(), frameDelay()
   - calculate Modbus RTU character timeout and inter-frame delay

   startEthernet()
   - initiates ethernet interface
   - if enabled, gets IP from DHCP
   - starts all servers (Modbus TCP, UDP, web server)

   resetFunc()
   - well... resets Arduino

   maintainDhcp()
   - maintain DHCP lease

   maintainUptime()
   - maintains up time in case of millis() overflow

   maintainCounters(), rollover()
   - synchronizes roll-over of data counters to zero

   resetStats()
   - resets Modbus stats

   generateMac()
   - generate random MAC using pseudo random generator (faster and than build-in random())

   manageSockets()
   - closes sockets which are waiting to be closed or which refuse to close
   - forwards sockets with data available (webserver or Modbus TCP) for further processing
   - disconnects (closes) sockets which are too old / idle for too long
   - opens new sockets if needed (and if available)

   CreateTrulyRandomSeed()
   - seed pseudorandom generator using  watch dog timer interrupt (works only on AVR)
   - see https://sites.google.com/site/astudyofentropy/project-definition/timer-jitter-entropy-sources/entropy-library/arduino-random-seed


   + preprocessor code for identifying microcontroller board

   ***************************************************************** */

#include <WiFiNINA.h>

void startSerial() {
  mySerial.begin((data.config.baud * 100UL), data.config.serialConfig);
#ifdef RS485_CONTROL_PIN
  pinMode(RS485_CONTROL_PIN, OUTPUT);
  digitalWrite(RS485_CONTROL_PIN, RS485_RECEIVE);  // Init Transceiver
#endif                                             /* RS485_CONTROL_PIN */
}

// number of bits per character (11 in default Modbus RTU settings)
byte bitsPerChar() {
  byte bits =
    1 +                                                         // start bit
    (((data.config.serialConfig & 0x06) >> 1) + 5) +            // data bits
    (((data.config.serialConfig & 0x08) >> 3) + 1);             // stop bits
  if (((data.config.serialConfig & 0x30) >> 4) > 1) bits += 1;  // parity bit (if present)
  return bits;
}

// character timeout in micros
uint32_t charTimeOut() {
  if (data.config.baud <= 192) {
    return (15000UL * bitsPerChar()) / data.config.baud;  // inter-character time-out should be 1,5T
  } else {
    return 750;
  }
}

// minimum frame delay in micros
uint32_t frameDelay() {
  if (data.config.baud <= 192) {
    return (35000UL * bitsPerChar()) / data.config.baud;  // inter-frame delay should be 3,5T
  } else {
    return 1750;  // 1750 μs
  }
}

void startEthernet() {
  if (ETH_RESET_PIN != 0) {
    pinMode(ETH_RESET_PIN, OUTPUT);
    digitalWrite(ETH_RESET_PIN, LOW);
    delay(25);
    digitalWrite(ETH_RESET_PIN, HIGH);
    delay(ETH_RESET_DELAY);
  }
#ifdef ENABLE_DHCP
  if (data.config.enableDhcp) {
    dhcpSuccess = Ethernet.begin(data.mac);
  }
  if (!data.config.enableDhcp || dhcpSuccess == false) {
    Ethernet.begin(data.mac, data.config.ip, data.config.dns, data.config.gateway, data.config.subnet);
  }
#else  /* ENABLE_DHCP */
  Ethernet.begin(data.mac, data.config.ip, {}, data.config.gateway, data.config.subnet);  // No DNS
#endif /* ENABLE_DHCP */
  W5100.setRetransmissionTime(TCP_RETRANSMISSION_TIMEOUT);
  W5100.setRetransmissionCount(TCP_RETRANSMISSION_COUNT);
  modbusServer = EthernetServer(data.config.tcpPort);
  webServer = EthernetServer(data.config.webPort);
  Udp.begin(data.config.udpPort);
  modbusServer.begin();
  ///// AMR Begin
  ///// Add condition to start webserver only if DI is in HIGH
    if (digitalRead(DI_7)==1)
    {
    webServer.begin();
    }
  ///// AMR End
#if MAX_SOCK_NUM > 4
  if (W5100.getChip() == 51) maxSockNum = 4;  // W5100 chip never supports more than 4 sockets
#endif
}

void (*resetFunc)(void) = 0;  //declare reset function at address 0

void checkEthernet() {
  static byte attempts = 0;
  IPAddress tempIP = Ethernet.localIP();
  if (tempIP[0] == 0) {
    attempts++;
    if (attempts >= 3) {
      resetFunc();
    }
  } else {
    attempts = 0;
  }
  checkEthTimer.sleep(CHECK_ETH_INTERVAL);
}

#ifdef ENABLE_DHCP
void maintainDhcp() {
  if (data.config.enableDhcp && dhcpSuccess == true) {  // only call maintain if initial DHCP request by startEthernet was successfull
    Ethernet.maintain();
  }
}
#endif /* ENABLE_DHCP */

#ifdef ENABLE_EXTENDED_WEBUI
void maintainUptime() {
  uint32_t milliseconds = millis();
  if (last_milliseconds > milliseconds) {
    //in case of millis() overflow, store existing passed seconds
    remaining_seconds = seconds;
  }
  //store last millis(), so that we can detect on the next call
  //if there is a millis() overflow ( millis() returns 0 )
  last_milliseconds = milliseconds;
  //In case of overflow, the "remaining_seconds" variable contains seconds counted before the overflow.
  //We add the "remaining_seconds", so that we can continue measuring the time passed from the last boot of the device.
  seconds = (milliseconds / 1000) + remaining_seconds;
}
#endif /* ENABLE_EXTENDED_WEBUI */

bool rollover() {
  // synchronize roll-over of run time, data counters and modbus stats to zero, at 0xFFFFFF00
  const uint32_t ROLLOVER = 0xFFFFFF00;
  for (byte i = 0; i < ERROR_LAST; i++) {
    if (data.errorCnt[i] > ROLLOVER) {
      return true;
    }
  }
#ifdef ENABLE_EXTENDED_WEBUI
  if (seconds > ROLLOVER) {
    return true;
  }
  for (byte i = 0; i < DATA_LAST; i++) {
    if (data.rtuCnt[i] > ROLLOVER || data.ethCnt[i] > ROLLOVER) {
      return true;
    }
  }
#endif /* ENABLE_EXTENDED_WEBUI */
  return false;
}

// resets counters to 0: data.errorCnt, data.rtuCnt, data.ethCnt
void resetStats() {
  memset(data.errorCnt, 0, sizeof(data.errorCnt));
#ifdef ENABLE_EXTENDED_WEBUI
  memset(data.rtuCnt, 0, sizeof(data.rtuCnt));
  memset(data.ethCnt, 0, sizeof(data.ethCnt));
  remaining_seconds = -(millis() / 1000);
#endif /* ENABLE_EXTENDED_WEBUI */
}


void generateMac() {
  
  WiFi.status();
  byte mac[6];
  WiFi.macAddress(mac);

  for(int i=0; i<6; i++){
    data.mac[i] = mac[i];
  }
  data.mac[5] = data.mac[5] + 1;

  return;
}

void updateEeprom() {
  eepromTimer.sleep(EEPROM_INTERVAL * 60UL * 60UL * 1000UL);  // EEPROM_INTERVAL is in hours, sleep is in milliseconds!
  data.eepromWrites++;                                        // we assume that at least some bytes are written to EEPROM during EEPROM.update or EEPROM.put
  E2PROM.put(DATA_START, data);
}

#if MAX_SOCK_NUM == 8
uint32_t lastSocketUse[MAX_SOCK_NUM] = { 0, 0, 0, 0, 0, 0, 0, 0 };
byte socketInQueue[MAX_SOCK_NUM] = { 0, 0, 0, 0, 0, 0, 0, 0 };
#elif MAX_SOCK_NUM == 4
uint32_t lastSocketUse[MAX_SOCK_NUM] = { 0, 0, 0, 0 };
byte socketInQueue[MAX_SOCK_NUM] = { 0, 0, 0, 0 };
#endif

// from https://github.com/SapientHetero/Ethernet/blob/master/src/socket.cpp
void manageSockets() {
  uint32_t maxAge = 0;         // the 'age' of the socket in a 'disconnectable' state that was last used the longest time ago
  byte oldest = MAX_SOCK_NUM;  // the socket number of the 'oldest' disconnectable socket
  byte modbusListening = MAX_SOCK_NUM;
  byte webListening = MAX_SOCK_NUM;
  byte dataAvailable = MAX_SOCK_NUM;
  byte socketsAvailable = 0;
  SPI.beginTransaction(SPI_ETHERNET_SETTINGS);  // begin SPI transaction
  // look at all the hardware sockets, record and take action based on current states
  for (byte s = 0; s < maxSockNum; s++) {            // for each hardware socket ...
    byte status = W5100.readSnSR(s);                 //  get socket status...
    uint32_t sockAge = millis() - lastSocketUse[s];  // age of the current socket
    if (socketInQueue[s] > 0) {
      lastSocketUse[s] = millis();
      continue;  // do not close Modbus TCP sockets currently processed (in queue)
    }

    switch (status) {
      case SnSR::CLOSED:
        {
          socketsAvailable++;
        }
        break;
      case SnSR::LISTEN:
      case SnSR::SYNRECV:
        {
          lastSocketUse[s] = millis();
          if (W5100.readSnPORT(s) == data.config.webPort) {
            webListening = s;
          } else {
            modbusListening = s;
          }
        }
        break;
      case SnSR::FIN_WAIT:
      case SnSR::CLOSING:
      case SnSR::TIME_WAIT:
      case SnSR::LAST_ACK:
        {
          socketsAvailable++;                  // socket will be available soon
          if (sockAge > TCP_DISCON_TIMEOUT) {  //     if it's been more than TCP_CLIENT_DISCON_TIMEOUT since disconnect command was sent...
            W5100.execCmdSn(s, Sock_CLOSE);    //	    send CLOSE command...
            lastSocketUse[s] = millis();       //       and record time at which it was sent so we don't do it repeatedly.
          }
        }
        break;
      case SnSR::ESTABLISHED:
      case SnSR::CLOSE_WAIT:
        {
          if (EthernetClient(s).available() > 0) {
            dataAvailable = s;
            lastSocketUse[s] = millis();
          } else {
            // remote host closed connection, our end still open
            if (status == SnSR::CLOSE_WAIT) {
              socketsAvailable++;               // socket will be available soon
              W5100.execCmdSn(s, Sock_DISCON);  //  send DISCON command...
              lastSocketUse[s] = millis();      //   record time at which it was sent...
                                                // status becomes LAST_ACK for short time
            } else if (((W5100.readSnPORT(s) == data.config.webPort && sockAge > WEB_IDLE_TIMEOUT)
                        || (W5100.readSnPORT(s) == data.config.tcpPort && sockAge > (data.config.tcpTimeout * 1000UL)))
                       && sockAge > maxAge) {
              oldest = s;        //     record the socket number...
              maxAge = sockAge;  //      and make its age the new max age.
            }
          }
        }
        break;
      default:
        break;
    }
  }

  if (dataAvailable != MAX_SOCK_NUM) {
    EthernetClient client = EthernetClient(dataAvailable);
    if (W5100.readSnPORT(dataAvailable) == data.config.webPort) {
      recvWeb(client);
    } else {
      recvTcp(client);
    }
  }

  if (modbusListening == MAX_SOCK_NUM) {
    modbusServer.begin();
  } else if (webListening == MAX_SOCK_NUM) {
     ///// Add condition to start webserver only if DI is in HIGH
    if (digitalRead(DI_7)==1)
    {
    webServer.begin();
    }
  }

  // If needed, disconnect socket that's been idle (ESTABLISHED without data recieved) the longest
  if (oldest != MAX_SOCK_NUM && socketsAvailable == 0 && (webListening == MAX_SOCK_NUM || modbusListening == MAX_SOCK_NUM)) {
    disconSocket(oldest);
  }

  SPI.endTransaction();  // Serves to o release the bus for other devices to access it. Since the ethernet chip is the only device
  // we do not need SPI.beginTransaction(SPI_ETHERNET_SETTINGS) or SPI.endTransaction() ??
}

void disconSocket(byte s) {
  if (W5100.readSnSR(s) == SnSR::ESTABLISHED) {
    W5100.execCmdSn(s, Sock_DISCON);  // Sock_DISCON does not close LISTEN sockets
    lastSocketUse[s] = millis();      //   record time at which it was sent...
  } else {
    W5100.execCmdSn(s, Sock_CLOSE);  //  send DISCON command...
  }
}

// Board definitions
#define BOARD F("Weidos-MKR1010-A1")
