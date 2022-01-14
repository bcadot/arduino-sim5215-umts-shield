#include "sim5215_umts_shield.h"

Shield::Shield(int onPin, int shieldRX, int shieldTX) : shieldSerial(shieldRX, shieldTX){
  shieldSerial.begin(115200);
	onModulePin = onPin;
	pinMode(onModulePin, OUTPUT);
}

int Shield::init(char pin_number[], char apn[], char user_name[], char password[]) {
	char aux_str[50];
	int answer = 0;
	//sets the PIN code
	sprintf(aux_str, "AT+CPIN=%s", pin_number);
	answer &= sendATcommand(aux_str, "OK", 2000);

	delay(3000);
	
	//check that connected to home network or partner network
	while( (sendATcommand("AT+CREG?", "+CREG: 0,1", 500) || sendATcommand("AT+CREG?", "+CREG: 0,5", 500)) == 0 );

	// sets APN, user name and password
	sprintf(aux_str, "AT+CGSOCKCONT=1,\"IP\",\"%s\"", apn);
	answer &= sendATcommand(aux_str, "OK", 2000);

	sprintf(aux_str, "AT+CSOCKAUTH=1,1,\"%s\",\"%s\"", user_name, password);
	answer &= sendATcommand(aux_str, "OK", 2000);
	
	return answer;
}

void Shield::power_on() {

  uint8_t answer=0;

  // checks if the module is started
  answer = sendATcommand("AT", "OK", 2000);
  if (answer == 0) {
    // power on pulse
    digitalWrite(onModulePin,HIGH);
    delay(3000);
    digitalWrite(onModulePin,LOW);
    // waits for an answer from the module
    while (answer == 0) {
      // Send AT every two seconds and wait for the answer
      answer = sendATcommand("AT", "OK", 2000);
    }
  }
}

int8_t Shield::sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout) {
	uint8_t x=0, answer = 0;
	char response[100];
	unsigned long previous;

	memset(response, '\0', 100);    // Initialize the string

	delay(100);

	while (shieldSerial.available() > 0) shieldSerial.read();   // Clean the input buffer
//  shieldSerial.println("-- Serial cleaning done --");

	int bytes = shieldSerial.println(ATcommand);    // Send the AT command

	x = 0;
	previous = millis();
  
	// this loop waits for the answer
	do {
		// if there are data in the UART input buffer, reads it and checks for the asnwer
		if (shieldSerial.available() != 0) {
			// check if the desired answer is in the response of the module
      response[x] = shieldSerial.read();
      x++;
			if (strstr(response, expected_answer) != NULL) {
				answer = 1;
			}
		}
		// Waits for the answer with time out
	} while ((answer == 0) && ((millis() - previous) < timeout));
	if (answer == 0 && strlen(response) != 0) {
		Serial.print("AT response=");
		Serial.println(response);
	}
	return answer;
}

int8_t Shield::sendPOSTrequest(char url[], int port, char request[], unsigned int timeout) {
  char aux_str[50];
  unsigned long previous;
  int data_size = 0;
  int http_x, http_status, x, answer;
  char data[400];
	previous = millis();
	sprintf(aux_str, "AT+CHTTPACT=\"%s\",%d", url, port);
	answer = sendATcommand(aux_str, "+CHTTPACT: REQUEST", timeout);
	if (answer == 1) {
		// Sends the request
		shieldSerial.println(request);
		// Sends <Ctrl+Z>
		shieldSerial.write(0x1A);
		http_status = 1;

		while ((http_status == 1) || (http_status == 2)) {
			answer = sendATcommand("", "+CHTTPACT: ", timeout);
			if (answer == 0) {
				if (http_status == 1) {
					http_status = 3;
				} else if (http_status == 2) {
					http_status = 5;
				}
				Serial.print(F("http_status= "));
				Serial.println(http_status, DEC);
			} else {
				// answer == 1
				while(shieldSerial.available()==0);
				aux_str[0] = shieldSerial.read();
			
				if ((aux_str[0] == 'D') && (http_status == 1)) {
					Serial.println("DATA,xxx line");
					// Data packet with header
					while(shieldSerial.available()<4);
					shieldSerial.read();  // A
					shieldSerial.read();  // T
					shieldSerial.read();  // A
					shieldSerial.read();  // ,

					// Reads the packet size
					x=0;
					do {
						while(shieldSerial.available()==0);
						aux_str[x] = shieldSerial.read();
						x++;
					} while((aux_str[x-1] != '\r') && (aux_str[x-1] != '\n'));

					aux_str[x-1] = '\0';
					data_size = atoi(aux_str);
	//          	Serial.print("Packet size=");
	//          	Serial.println(data_size, DEC);

					// Now, search the end of the HTTP header (\r\n\r\n)
					do {
					//wait for 4 characters
						while (shieldSerial.available() < 3);

						data_size--;
						if (shieldSerial.read() == '\r') {
							Serial.println("Found first");
							data_size--;
							if (shieldSerial.read() == '\n') {
								Serial.println("Found second");
								data_size--;
								if (shieldSerial.read() == '\r') {
									Serial.println("Found third");
									data_size--;
									if (shieldSerial.read() == '\n') {
										Serial.println("Found end of the header");
										// End of the header found
										http_status = 2;
									}
								}
							}  
						}
					} while ((http_status == 1) && (data_size > 0));

					if (http_status == 2) {
						// Reads the data
						http_x = 0;
						Serial.println("Starting reading data");
						do {
							if (shieldSerial.available() != 0) {
								data[http_x] = shieldSerial.read();
								http_x++;
								data_size--;
	//       			        Serial.print("Bytes read=");
	//                			Serial.println(http_x, DEC);
							} else {
								delay(1);
							}
						} while(data_size > 0);
						data[http_x] = '\0';
						Serial.println("Finished reading data");
					}
				} else if ((aux_str[0] == 'D') && (http_status == 2)) {
					// Data packet with header
					while(shieldSerial.available()<4);
					shieldSerial.read();  // A
					shieldSerial.read();  // T
					shieldSerial.read();  // A
					shieldSerial.read();  // ,

					// Reads the packet size
					x=0;
					do {
						while(shieldSerial.available()==0);
						aux_str[x] = shieldSerial.read();
						x++;
					} while((aux_str[x-1] != '\r') && (aux_str[x-1] != '\n'));

					aux_str[x-1] = '\0';
					data_size = atoi(aux_str);

					do {
						if(shieldSerial.available() != 0){
							data[http_x] = shieldSerial.read();
							http_x++;
						} else {
							delay(1);
						}
					} while(data_size > 0);
					data[http_x] = '\0';

				} else if (aux_str[0] == '0') {
					// end of the AT command
					http_status = 0;
			
				} else {
					// unknown response
					http_status = 4;
					Serial.print(char(aux_str[0]));
					Serial.print(char(shieldSerial.read()));
					Serial.print(char(shieldSerial.read()));
					Serial.print(char(shieldSerial.read()));
					Serial.print(char(shieldSerial.read()));
					Serial.print(char(shieldSerial.read()));
					Serial.print(char(shieldSerial.read()));
					Serial.print(char(shieldSerial.read()));
					Serial.print(char(shieldSerial.read()));
				}     
			}   
		}

		previous = millis() - previous;

//		Serial.println(previous, DEC);
		if (http_status == 0) {
		Serial.print(F("HTTP data: "));
		Serial.println(data);
		} else {
		Serial.print(F("http_status= "));
		Serial.println(http_status, DEC);
		}
	} else {
	Serial.println("!Error on CHTTPACT!");
	}
  return http_status;
}

int8_t Shield::sendTextMessage(char phone_number[], char text[]) {
	char aux_str[50];
	int aux, answer;
	
	sendATcommand("AT+CMGF=1", "OK", 1000);    // sets the SMS mode to text
	
	sprintf(aux_str,"AT+CMGS=\"%s\"", phone_number);
	answer = sendATcommand(aux_str, ">", 2000);    // send the SMS number
	if (answer == 1) {
		shieldSerial.println(text);
		shieldSerial.write(0x1A);
		answer = sendATcommand("", "OK", 20000);
		if (answer == 1) {
			Serial.print("Text message sent.");
		} else {
			Serial.print("Could not send text message.");
		}
	} else {
		Serial.print("Could not send text message");
		Serial.println(answer, DEC);
	}
	return answer;
}
