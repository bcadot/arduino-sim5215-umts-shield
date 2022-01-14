#ifndef SIM5215_UMTS_SHIELD_H
#define SIM5215_UMTS_SHIELD_H

#include <Arduino.h>
#include <SoftwareSerial.h>

class Shield {

  private:
    int onModulePin;
    SoftwareSerial shieldSerial;

  public:
    //Create instance and setup Serial and power pin
    Shield(int onPin, int shieldRX, int shieldTX);

    //Init shield
    int init(char pin_number[], char apn[], char user_name[], char password[]);

    //Power on the shield if shut down
    void power_on();

    //Sends an AT command to the shield
    int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout);

    //Sends a POST request to url:port via AT commands to the shield
    int8_t sendPOSTrequest(char url[], int port, char request[], unsigned int timeout);
	
	  int8_t sendTextMessage(char phone_number[], char text[]);
};

#endif
