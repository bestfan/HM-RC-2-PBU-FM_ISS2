//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2023-05-12 bestfan (cc)

//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=644p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER

// Arduino IDE Settings
// use support for 644PA from MightyCore: https://github.com/MCUdude/MightyCore
// settings:
// Board:        ATMega644
// Pinout:       Standard
// Clock:        8MHz external
// Variant:      644P / 644PA
// BOD:          2.7V
// Compiler LTO: Enabled

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <SPI.h>  // after including SPI Library - we can use LibSPI class
#include <AskSinPP.h>
#include <LowPower.h>

#include <MultiChannelDevice.h>
#include <Remote.h>


#define RELAY_PIN          12 //PD4
#define BTN1_PIN           14 //PD6
#define BTN2_PIN           8  //PD0
#define LED_PIN            0  //PB0
#define CONFIG_BUTTON_PIN  15 //PD7
#define CS_PIN             4  //PB4
#define GDO0_PIN           10 //PD2



// number of available peers per channel
#define PEERS_PER_CHANNEL 10

// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
    {0x02,0xE0,0x01},       // Device ID
    "JPRC2FM001",           // Device Serial
    {0x00,0xE0},            // Device Model
    0x14,                   // Firmware Version
    as::DeviceType::Remote, // Device Type
    {0x00,0x00}             // Info Bytes
};

/**
 * Configure the used hardware
 */
typedef LibSPI<CS_PIN> SPIType;
typedef Radio<SPIType,GDO0_PIN> RadioType;
//typedef DualStatusLed<LED_PIN2, LED_PIN> LedType;
typedef StatusLed<LED_PIN> LedType;
typedef AskSin<LedType,BatterySensor,RadioType> HalType;
class Hal : public HalType {
  // extra clock to count button press events
  AlarmClock btncounter;
public:
  void init (const HMID& id) {
    HalType::init(id);
    // get new battery value after 50 key press
    battery.init(50,btncounter);
    battery.low(22);
    battery.critical(19);
  }

  void sendPeer () {
    --btncounter;
  }

  bool runready () {
    return HalType::runready() || btncounter.runready();
  }
};

typedef RemoteChannel<Hal,PEERS_PER_CHANNEL,List0> ChannelType;
typedef MultiChannelDevice<Hal,ChannelType,2> RemoteType;

Hal hal;
RemoteType sdev(devinfo,0x20);
ConfigButton<RemoteType> cfgBtn(sdev);

void setup () {
  DINIT(57600,ASKSIN_PLUS_PLUS_IDENTIFIER);
  sdev.init(hal);
  remoteISR(sdev,1,BTN1_PIN);
  remoteISR(sdev,2,BTN2_PIN);
  buttonISR(cfgBtn,CONFIG_BUTTON_PIN);
  sdev.initDone();
}

void loop() {
  bool worked = hal.runready();
  bool poll = sdev.pollRadio();
  if( worked == false && poll == false ) {
    hal.activity.savePower<Idle<>>(hal);
  }
}
