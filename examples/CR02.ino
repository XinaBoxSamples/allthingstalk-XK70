#include <Wire.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

char incomingKey;
char keyNo[3] = {'0', '0', '0'};
byte  appeui[8];
byte  deveui[8];
byte  appkey[16];
char incomingByte[3];
bool debug = true;
bool keysSet = false;
bool nextKey = true;
uint8_t byteLen;

void os_getArtEui (u1_t* buf) {
  for (int i = 0; i < sizeof(appeui); i++)
    buf[i] = appeui[sizeof(appeui) - 1 - i];
}

void os_getDevEui (u1_t* buf) {
  for (int i = 0; i < sizeof(deveui); i++)
    buf[i] = deveui[sizeof(deveui) - 1 - i];
}

void os_getDevKey (u1_t* buf) {
  for (int i = 0; i < sizeof(appkey); i++)
    buf[i] = appkey[i];
}

static uint8_t payload[32];
static osjob_t sendjob;

const unsigned TX_INTERVAL = 60;

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = LMIC_UNUSED_PIN,
  .dio = {LMIC_UNUSED_PIN, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN}, //2,4,na
};

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:

      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      digitalWrite(16, LOW);
      digitalWrite(15, HIGH);
      Serial.println(F("EV_JOINED"));

      // Disable link check validation (automatically enabled
      // during join, but not supported by TTN at this time).
      LMIC_setLinkCheckMode(0);
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));

      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      digitalWrite(14, HIGH);
      delay(500);
      digitalWrite(14, LOW);
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, payload, byteLen, 0);
    Serial.println(F("Packet queued"));
  }

  // Next TX is scheduled after TX_COMPLETE event.
}
void setup() {
  Serial.end();
  Wire.begin(8);                // join i2c bus with address #8
  Wire.onReceive(receiveHandle); // register event
  Wire.onRequest(requestHandle);
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(16, OUTPUT);


  // LMIC init
  while (!keysSet) {
    delay(1000);
  }

  os_init();
  //  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  //
  //  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);

  //delay(5000);
}

void loop() {
  while (keysSet) {
    os_runloop_once();
  }

}

// function that executes whenever data is received from master
// this function is registered as an event, see setup()
void receiveHandle(int howMany) {
  if (debug) {
    while (Wire.available() == 1 ) {
      char command = Wire.read();
      switch (command) {
        case '0':
          Serial.end();
          break;
        case '1':
          Serial.begin(115200);
          break;
      }
      debug = false;
      break;
    }
  }

  while ((Wire.available() >= 1) && keysSet == false) {
    if (nextKey) {
      incomingKey = Wire.read();
      nextKey = false;
      break;
    }
    switch (incomingKey) {
      case 'a':
        keyNo[0] = incomingKey;
        Serial.print("APPEUI: ");
        for (uint16_t i = 0; i < 8; i++) {
          incomingByte[0] = Wire.read();
          incomingByte[1] = Wire.read();
          incomingByte[2] = '\0';
          appeui[i] = (byte) strtol(incomingByte, 0, 16);
          sprintf(incomingByte, "%02X ", appeui[i]);
          Serial.print(incomingByte);
        }
        nextKey = true;
        Serial.println();
        break;
      case 'b':
        keyNo[1] = incomingKey;
        Serial.print("DEVEUI: ");
        for (uint16_t i = 0; i < 8; i++) {
          incomingByte[0] = Wire.read();
          incomingByte[1] = Wire.read();
          incomingByte[2] = '\0';
          deveui[i] = (byte) strtol(incomingByte, 0, 16);
          sprintf(incomingByte, "%02X ", deveui[i]);
          Serial.print(incomingByte);
        }
        nextKey = true;
        Serial.println();
        break;
      case 'c':
        keyNo[2] = incomingKey;
        Serial.print("APPKEY: ");
        for (uint16_t i = 0; i < 16; i++) {
          incomingByte[0] = Wire.read();
          incomingByte[1] = Wire.read();
          incomingByte[2] = '\0';
          appkey[i] = (byte) strtol(incomingByte, 0, 16);
          sprintf(incomingByte, "%02X ", appkey[i]);
          Serial.print(incomingByte);
        }
        nextKey = true;
        keysSet = true;
        Serial.println();
        break;
    }
  }

  while (Wire.available() > 1 && keysSet == true) {
    Serial.println("while entered");
    char s = Wire.read();
    if (s == '0') {
      byteLen = Wire.available();
      Serial.print("DATA SIZE: "); Serial.println(byteLen);
      Serial.print("DATA: ");
      for (int i = 0; i < byteLen; i++) {
        payload[i] = Wire.read();
        Serial.print(payload[i], HEX);
      }
      Serial.println();
    }
  }
}

void requestHandle() {
//  switch (keysSet) {
//    case true:
//      Wire.write("1");
//      break;
//    default:
//      Wire.write("0");
//      break;
//  }
}
