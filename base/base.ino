// INCLUDE LIBRARIES AND FILES
#include <SoftwareSerial.h>
#include "protocol_settings.h"
#include "protocol_functions.h"

// INITIALIZE VARS
// setup transceiver modules
SoftwareSerial transmitter0_serial(11, 10);
int transmitter0_set_pin = 12;
SoftwareSerial transmitter1_serial(51, 50);
int transmitter1_set_pin = 52;

// PROTOCOL INIT
const uint16_t PERSONAL_ADDRESS = 0; // must be unique
uint8_t ALERT_CHANNEL = 0;

uint16_t saved_pylon_addresses[] = {1, 2}; // addresses of pylons that will be parsed
bool logged_in_pylons[(sizeof(saved_pylon_addresses)/sizeof(saved_pylon_addresses[0]))] 
  = {false}; // saved state of all pylons addresss = saved_pylon_addresses[index]
  
/// parsing variables (cr => current request)
int16_t cr_address_index = 0; // save where the parsing currently is
uint8_t cr_rr_code = 0; // request code of current request
long cr_timeout = 0; // timeout of current request
long cr_start_time = 0; // for performance tests

// true: searching for unconnected devices
// false: always testing connections to pylons
bool pairing = true; 


void setup() {
  // configure set pins of transmitters
  pinMode(transmitter0_set_pin, OUTPUT);
  digitalWrite(transmitter0_set_pin, HIGH);
  pinMode(transmitter1_set_pin, OUTPUT);
  digitalWrite(transmitter1_set_pin, HIGH);

  // begin serial communication
  Serial.begin(9600);
  transmitter0_serial.begin(9600);
  // transmitter1_serial.begin(9600);
  Serial.println("////////// EIACH; BASE //////////");

  // use physical randomness
  randomSeed(analogRead(0));
  // channel should be at least 10 units from com_channel
  // maximum channel is 126
  ALERT_CHANNEL = random(11, 127);
  Serial.print("Random alert channel: ");
  Serial.println(ALERT_CHANNEL);

  // switch to default com channel
  switchChannel(&transmitter0_serial, &transmitter0_set_pin, COM_CHANNEL);
  // TODO: uncomment when transmitter1 is connected
  // switchChannel(&transmitter1_serial, &transmitter1_set_pin, ALERT_CHANNEL);
}

void loop() {
  long current_millis = millis();
  // only listen when a package was sent
  if(cr_rr_code > 0 && cr_timeout > 0) {
    if (transmitter0_serial.available() && cr_timeout <= current_millis) {
      // TODO: handle received data
    }
    else if(cr_timeout < current_millis) {
      // TODO: handle timeout
      Serial.println("Package timed out!");
      cr_rr_code = 0;
      cr_timeout = 0;
      cr_start_time = 0;
    }
    else {
      // waiting
    }
  }
  // when not waiting for a package continue parsing
  else {
    // checking for login of every pylon
    if(pairing && logged_in_pylons[cr_address_index] == false) {
      Serial.print("Sending pairing request to pylon-id ");
      Serial.println(saved_pylon_addresses[cr_address_index]);
      // pairing
      char package[MAX_PACKAGE_LENGTH] = {0};
      package[0] = VERSION;
      package[1] = 0; // TODO: create checksum
      package[2] = RR_CODE_LOGIN;
      package[3] = 9; // get length
      // source address
      package[4] = (PERSONAL_ADDRESS >> 8) & 0xff;
      package[5] = PERSONAL_ADDRESS & 0xff;
      // destination address
      package[6] = (saved_pylon_addresses[cr_address_index] >> 8) & 0xff;
      package[7] = saved_pylon_addresses[cr_address_index] & 0xff;
      // alert channel
      package[8] = ALERT_CHANNEL;
      delay(25); // TODO: investigate why delay is required
      // printPackage(package);

      for(int i = 0; i < MAX_PACKAGE_LENGTH; i++) {
        transmitter0_serial.write(package[i]);
      }

      // set cr_rr_code and cr_timeout to wait for response
      cr_rr_code = package[2];
      cr_start_time = millis();
      cr_timeout = cr_start_time + TIMEOUT;
    }
    // test availability of every pylon
    else if(!pairing && logged_in_pylons[cr_address_index] == true) {
      Serial.print("Sending ping to pylon-id ");
      Serial.println(saved_pylon_addresses[cr_address_index]);
      // TODO: pining
    }

    // proceed to next cr_address_index
    cr_address_index++;
    // begin at 0 if end is reached and end pairing
    if(cr_address_index >= sizeof(logged_in_pylons)) {
      cr_address_index = 0;
      pairing = false;
    }
  }
}
