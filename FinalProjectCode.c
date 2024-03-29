#define ADC_BASE	0xFF204000
#define SW_BASE     0xFF200040

const int switchNum = 8;

// Init functions
void initADCAutoWrite();
void readADC(int* switchesFlipped, int* adcVals);
void readSlider(int* switchesFlipped, int* avgCount);
void calcAvgs(int* adcVals, int* avgCount, int* avgs);
void sendData(int* avgs, int* switchesFlipped);

int main() {
	// init adc
	initADCAutoWrite();

	// To store all adc values and avg counters. Init to 0
	int[switchNum] adcVals = {0};
	int[switchNum] avgCount = {0};
	int[switchNum] switchesFlipped = {0};
	int[switchNum] avgs = {0};
	int timerDone = 0;

	// Main loop
	while (1) {
		readSliders(switchesFlipped, avgCount);
		readADC(switchesFlipped, adcVals);

		// Check if timer is done, send data
		if (timerDone == 1) {
			// Calculates averages
			calcAvgs(adcVals, avgCount, avgs);
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
	for (i = 0; i < switchNum; i++) {
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

	return adcVals;
}

// Reads all the sliders
void readSliders(int* switchesFlipped, int* avgCount) {
	volatile int *sliderPtr = (int*) SW_BASE;

	int sliders = *sliderPtr;

	// Assign all the sliders
	for (int i = 0; i < switchNum; i++) {
		switchesFlipped[i] = sliders & (0b1 << i);

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
			avgs[i] = adcVals / avgCount;
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

			// something like sendData(byte1, byte2);
		}
	}
}