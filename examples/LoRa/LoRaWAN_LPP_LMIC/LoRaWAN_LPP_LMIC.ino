// LMIC + LPP Example
// In Arduino IDE, click on Tools -> Manage Libraries
// and add: LMIC and CayenneLPP
// as LMIC is heavy, you need to remove support for Pings and Beacons:
// in the lmic file "config.h", UNCOMMENT the two macros:
// #define DISABLE_PING
// #define DISABLE_BEACONS

#include <lmic.h>
#include <hal/hal.h>
#include <CayenneLPP.h>
#include <Base64.h>

// Enter here the Device address in Hex format. Example: 2601143F
static const u4_t DEVADDR = 0x2601143F ; // <-- Change this address for every node!
// Enter here the Network session Key in Hex format. Example: 196AE5DFE9EBC9AB61F1EFCEA8E6D337
static const PROGMEM u1_t NWKSKEY[16] = { 0x19, 0x6A, 0xE5, 0xDF, 0xE9, 0xEB, 0xC9, 0xAB, 0x61, 0xF1, 0xEF, 0xCE, 0xA8, 0xE6, 0xD3, 0x37};
// Enter here the Application session Key in Hex format. Example: 93B9F0269BFB05610740EFEB8303D1E2
static const u1_t PROGMEM APPSKEY[16] = { 0x93, 0xB9, 0xF0, 0x26, 0x9B, 0xFB, 0x05, 0x61, 0x07, 0x40, 0xEF, 0xEB, 0x83, 0x03, 0xD1, 0xE2 };


// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }


static uint8_t mydata[50];
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 0;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {2, 3, LMIC_UNUSED_PIN},
};

CayenneLPP lpp(10);

// Define the single channel and data rate (SF) to use
int channel = 0;
int dr = DR_SF9; //12

// Disables all channels, except for the one defined above, and sets the
// data rate (SF). This only affects uplinks; for downlinks the default
// channels or the configuration from the OTAA Join Accept are used.
//
// Not LoRaWAN compliant; FOR TESTING ONLY!
//
void forceTxSingleChannelDr() {
    for(int i=0; i<9; i++) { // For EU; for US use i<71
        if(i != channel) {
            LMIC_disableChannel(i);
        }
    }
    // Set data rate (SF) and transmit power for uplink
    LMIC_setDrTxpow(dr, 7); //12
}

void do_send(osjob_t* j){
    //Create payload
    lpp.reset();
    lpp.addTemperature(1, 29.0);
    memcpy((char*)mydata, lpp.getBuffer(), lpp.getSize());

    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        //LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);
        LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    if (ev == EV_TXCOMPLETE) {
       Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
       if (LMIC.txrxFlags & TXRX_ACK)
         Serial.println(F("Received ack"));
        if (LMIC.dataLen) {
          char data[LMIC.dataLen];
          memcpy(data, LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
          char decoded[100];
          base64_decode(decoded, data, sizeof(data));
          Serial.println(decoded);

          if(strcmp(decoded, "true") == 0) {
            Serial.println(F("ON"));
          } else {
            Serial.println(F("OFF"));
          }
          
        }
       // Schedule next transmission
       os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
    }
}


void setup() {
    Serial.begin(115200);
    Serial.println(F("Starting"));

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);

    // Set static session parameters. Instead of dynamically establishing a session
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);

    // Set only one channel and force it
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    forceTxSingleChannelDr();

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // Start job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
