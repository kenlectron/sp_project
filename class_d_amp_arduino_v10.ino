//------------------------------------------
//--PINS
//------------------------------------------
//--RELAYS
#define RELAY_SPK_CMD							
#define RELAY_PWR_CMD
#define RELAY_LIVE_CMD
//--FAN CONTROL
#define FAN_PWM
//--ADC INPUTS
#define ADC_RAILS_POS
#define ADC_RAILS_NEG
#define ADC_SPK_VOLTAGE
#define ADC_SPK_CURRENT
#define ADC_TEMP
//--AMPLIFIER CONTROL
#define CTRL_ERR_AMP_EN
#define CTRL_BST_CHRG
#define CTRL_PWM_EN
#define CTRL_MUTE
//--POWER
#define BTN_STDBY

//------------------------------------------
//--PARAMETERS
//------------------------------------------
#define MAIN_CLOCK									16000000						//--Hz
#define VOLTAGE_RAIL_POS_TH					798									//--RAW ADC (70 V)
#define VOLTAGE_RAIL_NEG_TH					798									//--RAW ADC (70 V)
#define STARTUP_DELAY								100									//--ms
#define OCP_PROTECT									614									//--RAW ADC (15 A)
#define OFFSET_PROTECT							10									//--RAW ADC (0.8V)
#define ADC_SAMPLING_RATE						7500								//--Hz
#define ADC_SAMPLES									32									//--Samples
#define ADC_VOLTAGE_REF							4096								//--mV

//------------------------------------------
//--STATUS REGISTER MASK
//------------------------------------------
#define AMP_STATUS_MASK_VPOS		0x80
#define AMP_STATUS_MASK_VNEG		0x40
#define AMP_STATUS_MASK_SPOK		0x20
#define AMP_STATUS_MASK_OFFS		0x10
#define AMP_STATUS_MASK_MUTE		0x08
#define AMP_STATUS_MASK_STBY		0x04
#define AMP_STATUS_MASK_PWEN		0x01


#define TIMER_COMP									MAIN_CLOCK/ADC_SAMPLING_RATE

uint16_t	ADC_VOLTAGE_SAMPLES[ADC_SAMPLES];
uint16_t	ADC_CURRENT_SAMPLES[ADC_SAMPLES];
uint8_t		AMP_STATUS_REGISTER = 0x00;

//AMP STATUS REGISTER STRUCTURE
//BIT:								7					6						5						4			     |			3						2						1						0
//DESCRIPTION:  		V(+)_OK   V(-)_OK    SPK_NOK 			 OFFSET_OK				MUTE				 STDBY       N.U.       PWR_EN
//V(+)_OK -> Positive voltage rail checked
//V(-)_OK -> Negative voltage rail checked
//SPK_NOK -> If the current is twice the voltage, R = 0.5Ohms, the bit will set
//OFFSET_OK -> Check output offset 
//N.U. -> Not used
//MUTE -> Bit is set if the amplifier is in "mute" state
//STDBY -> Bit is set if the amplifier is operating
//PWR_EN -> Power amplifier is fully turned off

//STAND-BY TURNS OFF THE AMPLIFIER AND SPEAKER'S OUTPUT, BUT KEEPS THE VOLTAGE RAILS

void setup() {
  //ADC REFERENCE
  analogReference(EXTERNAL);
}

void loop() {
  // put your main code here, to run repeatedly:

}
//----------------------------------------------------------------------
// TIMER
//----------------------------------------------------------------------
void timer_init(){
		TCCR2A = 0x00;						//Disable OC2 pin output (normal pin operation)
		TCCR2B = 0x05;						//Set clock precaler to 128
		OCR2A = TIMER_COMP;				//Set the desired value to be compared with
}

void timer_enable(){
	TIMSK2 = 0x00;
	TIFR2 = 0x00;
	while(!(TIFR2&0x02)); //Wait for 2nd bit from TIFR2 to be set
}
//----------------------------------------------------------------------
// ADC ACQUISITION
//----------------------------------------------------------------------
void adc_wfm_sampling() {
			for(int i = 0; i < ADC_SAMPLES; i++) {
				//ADC Sampling Rate is way higher than what we need, so we make a conversion at a certain amount of time
				//To have accurate measurements, they delay between current and voltage measurements must be as small as possible
				//therefore, the actual sampling rate of ADC is around 200KHz but a timer is used 
				timer_enable();
				ADC_VOLTAGE_SAMPLES[i] = analogRead(ADC_SPK_VOLTAGE);
				ADC_CURRENT_SAMPLES[i] = analogRead(ADC_SPK_CURRENT);		
			}
}

//----------------------------------------------------------------------
// COMMAND DECODING; FRAME FORMAT: CC DDDDD.D
//----------------------------------------------------------------------
void	 Serial_Command() {
			char			Serial_Received_Command[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
			char			Serial_Incoming_Byte = 0;
			uint8_t		Serial_Received_Bytes_Cnt = 0;
			bool			Serial_Received_Data = false;
			uint32_t	Serial_String_To_Int = 0;
			uint8_t		Char_to_Int_LUT = 0;
			bool			Check_Dec_Point = false;
			uint8_t		Cmd_ID = 0;
			//If there are bits waiting to be received
			if (Serial.available() > 0){
				//While the NEW LINE char is not received, continously read from Serial Port
				while(Serial_Incoming_Byte != '\n') {
					//Read a byte from serial
					if (Serial.available() > 0){	//Check for a new byte
					Serial_Incoming_Byte = Serial.read();
					//Write in the char array as a string if the received that 
					if (Serial_Incoming_Byte != '\n') {Serial_Received_Command[Serial_Received_Bytes_Cnt] = Serial_Incoming_Byte;} else { Serial_Received_Command[Serial_Received_Bytes_Cnt] = 0; }
					//Increment the received byte counter
					Serial_Received_Bytes_Cnt++;
					//In order to avoid command loops.
					Serial_Received_Data = true;
					}
				}
			}
			
			if (Serial_Received_Data == true){
			//Invalid commands detection: check if the first received char is '$' and the no. of bytes is larger than 5
			if (Serial_Received_Command[0] != '$' || Serial_Received_Bytes_Cnt < 5 || Serial_Received_Command[3] != ' ') {
				Serial.println("Invalid command. Command structure: $CC VVVV.V; CC- command id, V - value. $10 - HELP Menu");
			} else {
							for (int char_cnt = 1; char_cnt < Serial_Received_Bytes_Cnt - 1; char_cnt++) {
								switch(Serial_Received_Command[char_cnt]) {
									case '0': Char_to_Int_LUT = 0; break;
									case '1': Char_to_Int_LUT = 1; break;
									case '2': Char_to_Int_LUT = 2; break;
									case '3': Char_to_Int_LUT = 3; break;
									case '4': Char_to_Int_LUT = 4; break;
									case '5': Char_to_Int_LUT = 5; break;
									case '6': Char_to_Int_LUT = 6; break;
									case '7': Char_to_Int_LUT = 7; break;
									case '8': Char_to_Int_LUT = 8; break;
									case '9': Char_to_Int_LUT = 9; break;
									case '.': Check_Dec_Point = true; break;
								}
								if (char_cnt < 4 && Serial_Received_Command[char_cnt] != '$') {Cmd_ID = Cmd_ID*10 + Char_to_Int_LUT;}
								if (char_cnt > 3 && Serial_Received_Command[char_cnt] != '.') {Serial_String_To_Int = (Serial_String_To_Int*10) + Char_to_Int_LUT;}
							}
							switch(Cmd_ID) {
								case 10:
									Serial.println("$11 V -> V = 1/0 -> Enables/Disables amplifier operation");
									Serial.println("$12 V -> V = 1/0 -> Stand-by ");
									Serial.println("$13 V -> V = 1/0 -> Mute on/off ");
									Serial.println("$14 V -> V = 1/0 -> Enables/Disables power rails (completly turn off the power) ");
									Serial.println("$15 V -> V = 1/0 ->  ");
									Serial.println("$13 -> Returns value of positive voltage rail (ADC raw value)");
									Serial.println("$14 -> Returns value of negative voltage rail (ADC raw value)");
									Serial.println("$15 -> Returns string of acquired voltage samples (ADC raw value)");
									Serial.println("$16 -> Returns string of acquired current samples (ADC raw value)");
									break;
							}
			}			
			}
}

//----------------------------------------------------------------------
// AMPLIFIER CONTROL
//----------------------------------------------------------------------
void cmd_toggle_pwr(bool state){
		//POWER-DOWN SEQUENCE
		if (state == 0) {
			//Mute the amplifier (no input signal)
			cmd_amp_mute(1);
			delay(150);
			//Turn off the amplifier
			cmd_amp_enable(0);
			delay(100);
			//Disable power rails
			digitalWrite(RELAY_PWR_CMD, 0);
			delay(100);
			//Turn-Off the transformer
			digitalWrite(RELAY_LIVE_CMD, 0);
		}
		
		//POWER-UP SEQUENCE
		if (state == 1) {
			//Mute the amplifier (no input signal)
			cmd_amp_mute(1);
			//POWER-UP SEQUENCE: TRANSFORMER MAINS RELAY ON -> WAIT FOR VOLTAGE RAILS TO BUILD-UP & CHECK -> BYPASS SOFT-START RESISTORS -> TURN ON AMPLIFIER -> CHECK OFFSET -> ENABLE SPK OUTPUT
			digitalWrite(RELAY_LIVE_CMD, 1);
			while ((AMP_STATUS_REGISTER & (AMP_STATUS_MASK_VPOS | AMP_STATUS_MASK_VNEG)) != (AMP_STATUS_MASK_VPOS | AMP_STATUS_MASK_VNEG)) {
				Serial_Command();	//Accept serial commands
				if (analogRead(ADC_RAILS_POS) > VOLTAGE_RAIL_POS_TH) {
					AMP_STATUS_REGISTER |= (1 << AMP_STATUS_MASK_VPOS);				//Set the V(+) Check bit
				}
				if (analogRead(ADC_RAILS_NEG) > VOLTAGE_RAIL_NEG_TH) {
					AMP_STATUS_REGISTER |= (1 << AMP_STATUS_MASK_VNEG);				//Set the V(-) Check bit
				}
			}

			if ((AMP_STATUS_REGISTER & (AMP_STATUS_MASK_VPOS | AMP_STATUS_MASK_VNEG)) == (AMP_STATUS_MASK_VPOS | AMP_STATUS_MASK_VNEG)) {
				//Bypass soft-start resistors & turn-on the amplifier
				digitalWrite(RELAY_PWR_CMD, 1);
				delay(150);
				//Turn on the amplifier
				cmd_amp_enable(1);
				
			}
		}
}

//STAND-BY
void cmd_amp_enable(bool state) {
		if (state == 0) {
				cmd_amp_mute(1);
				delay(200);
				digitalWrite(RELAY_SPK_CMD, 0);
				delay(100);
				digitalWrite(CTRL_PWM_EN, 0);
				delay(20);
				digitalWrite(CTRL_ERR_AMP_EN, 0);
				AMP_STATUS_REGISTER &= ~(AMP_STATUS_MASK_STBY);
		}

		if (state == 1) {
				//Charge bootstrap capacitor
				digitalWrite(CTRL_BST_CHRG, 1);
				delay(500);
				//Enable PWM Comparator
				digitalWrite(CTRL_PWM_EN, 1);
				delay(10);
				//Enable ERROR Amplifier
				digitalWrite(CTRL_ERR_AMP_EN, 1);
				delay(200);
				digitalWrite(CTRL_BST_CHRG, 0);
				//Check offset, 129 samples to avoid any noises
				uint16_t spk_voltage_avg = 0;
				for (int i = 0; i < 129; i++) {
					spk_voltage_avg += analogRead(ADC_SPK_VOLTAGE);
					spk_voltage_avg = spk_voltage_avg/2;
				}
				if (spk_voltage_avg < OFFSET_PROTECT) {
					AMP_STATUS_REGISTER |= AMP_STATUS_MASK_OFFS;
					//Enable speaker output
					digitalWrite(RELAY_SPK_CMD, 1);
					//Un-mute the amplifier
					cmd_amp_mute(0);
					AMP_STATUS_REGISTER |= AMP_STATUS_MASK_STBY;
				}
		}
}

void cmd_amp_mute(bool state) {
		if (state == 0) {
				digitalWrite(CTRL_MUTE, 0);
				AMP_STATUS_REGISTER &= ~(AMP_STATUS_MASK_MUTE);
		} else {
				digitalWrite(CTRL_MUTE, 1);
				AMP_STATUS_REGISTER |= AMP_STATUS_MASK_MUTE;
		}
}
