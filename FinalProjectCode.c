#define ADC_BASE	0xFF204000
#define SW_BASE     0xFF200040
#define MPCORE_PRIV_TIMER     0xFFFEC600
#define JTEG_UART 	0xFF201000
#define HEX3_HEX0_BASE        0xFF200020

const int switchNum = 8;
const int secLen = 200000000;
const char hex_code[16] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};



// Define structs
typedef struct a9Timer{
    int load;
    int count;
    int control;
    int status;
}a9Timer;

// Init functions
void initADCAutoWrite();
void zeroArrays(int* adcVals, int* avgCount, int* switchesFlipped, int* avgs);
void readADC(int* switchesFlipped, int* adcVals);
void readSliders(int* switchesFlipped, int* avgCount);
void calcAvgs(int* adcVals, int* avgCount, int* avgs);
void sendData(int* avgs, int* switchesFlipped);
void sendOverSerial(char byte1, char byte2);
void setHex(int binVal, int inputNum);

int main() {
	// init adc
	initADCAutoWrite();

	// init private clock
	volatile a9Timer* const timer = (a9Timer*) MPCORE_PRIV_TIMER;

	volatile int interval = secLen; // to count a second
	timer->load = interval;	
	int status;

	// Starting the clock
	timer->control = 3;

	// To store all adc values and avg counters. Init to 0
	int adcVals[switchNum];
	int avgCount[switchNum];
	int switchesFlipped[switchNum];
	int avgs[switchNum];
	zeroArrays(adcVals, avgCount, switchesFlipped, avgs);

	// Main loop
	while (1) {
		readSliders(switchesFlipped, avgCount);
		readADC(switchesFlipped, adcVals);

		// Flag to see if one second has passed
		status = timer->status;
		
		// Check if timer is done, send data
		if (status == 1) {
			// Resetting status to 1
			timer->status = 1;
			
			// Calculates averages
			calcAvgs(adcVals, avgCount, avgs);
			sendData(avgs, switchesFlipped);
			
			// Reset all arrays
			zeroArrays(adcVals, avgCount, switchesFlipped, avgs);
		}
	}
}

void initADCAutoWrite() {
	volatile int *adcCtlPtr = (int*) (ADC_BASE + 0x04);

	*adcCtlPtr = 0b1; // setting the adc to auto update
}

// Returns the ADC values
void readADC(int* switchesFlipped, int* adcVals) {
	int offset;

	// Checking each switch register
	for (int i = 0; i < switchNum; i++) {
		// Reading the sliders
		if (switchesFlipped[i] == 1) {
			// Giving the correct offset for this register
			offset = 0x04 * i;

			// To read the adc
			volatile int *adcPtr = (int*) (ADC_BASE + offset);
			int adc;
			int adcVal;
			int doneBit;

			// loop until the done bit goes high
			do {
				adc = *adcPtr;
				adcVal = (adc & 0b111111111111);
				doneBit = (adc & (1 << 15)); // Shift by 16 for the simulator
			} while (doneBit == 0);

			adcVals[i] += adcVal; 
		}	
	}
}

// Reads all the sliders
void readSliders(int* switchesFlipped, int* avgCount) {
	volatile int *sliderPtr = (int*) SW_BASE;

	int sliders = *sliderPtr;

	// Assign all the sliders
	for (int i = 0; i < switchNum; i++) {
		// Making sure to shift back to read
		switchesFlipped[i] = (sliders & (0b1 << i)) >> i;

		// Increment count if switch is on
		if (switchesFlipped[i] == 1) {
			avgCount[i]++;
		}
	}
}

// Calculates the averages and clears other arrays
void calcAvgs(int* adcVals, int* avgCount, int* avgs) {
	for (int i = 0; i < switchNum; i++) {
		// Checking for divide by 0
		if (avgCount == 0) {
			avgs[i] = 0;
		}
		else {
			avgs[i] = adcVals[i] / avgCount[i];
		}

		// Clearing averages
		adcVals[i] = 0;
		avgCount[i] = 0;
	}
}

// Packaging and sending the data over RS-232
void sendData(int* avgs, int* switchesFlipped) {
	for (int i = 0; i < switchNum; i++) {
		// Only send if selected
		if (switchesFlipped[i] == 1) {
			// this byte is number, 5 bits starting with the msb of the val
			char byte1 = (i << 5) + ((avgs[i] & 0b111110000000) >> 7);

			// this byte is the rest of the number ending with the lsb, 1 extra space 
			char byte2 = (avgs[i] & 0b1111111) << 1;

			// sending the data over serial
			sendOverSerial(byte1, byte2);
			
			// Temp putting on hex
			int hexVal = formatHex(avgs[i]);
			setHex(hexVal, i);
		}
	}
}

// Sending data through RS-232
void sendOverSerial(char byte1, char byte2) {
	
	int data = byte1 + byte2;

	int *ptr = (int*) JTEG_UART;
	
	ptr++;

	int write_space = *ptr;


	if (write_space != 0xFFFF000) {
		ptr--;
		*ptr = data;	
	}
}

// Zeroing all the data arrays
void zeroArrays(int* adcVals, int* avgCount, int* switchesFlipped, int* avgs){
	for (int i = 0; i < switchNum; i++) {
		adcVals[i] = 0;
		avgCount[i] = 0;
		switchesFlipped[i] = 0;
		avgs[i] = 0;
	}
}

// Sets a hex value
void setHex(int binVal, int inputNum) {
	volatile int *hex_ptr = (int*) (HEX3_HEX0_BASE); // setting the HEX address
	
	// If they want to clear the hex
	if (binVal == -1) {
		*hex_ptr = 0;
	}
	else {
		// Setting the hex with the device num and voltage val
		*hex_ptr = (hex_code[binVal] + (hex_code[inputNum] << 8));
	}
}

// Formatting and sending to hex
int formatHex(int avg) {
	// Calculating the voltage for display purposes
	int ans = (avg*100) / 4095;
	ans /= 10;
	
	return ans;
}