#define ADC_BASE	0xFF204000
#define JP1_BASE    0xFF200060
#define SW_BASE     0xFF200040

void initLedGpio();
void writeLeds(int ledsToWrite);
int readADC();
int readSlider();
int detrLedPos(int adcVal);

int main() {
	// init gpio
	initLedGpio();

	// Read the adc and write to the leds
	while (1) {
		int adcVal = readADC();
		int ledPattern = detrLedPos(adcVal);
		writeLeds(ledPattern);		
	}
}

// Puts GPIO into output mode
void initLedGpio() {
	volatile int *ledCtlPtr = (int*) (JP1_BASE + 0x04);

	*ledCtlPtr = 0b1111111111; // setting the leds to output
}

// Writes all leds
void writeLeds(int ledsToWrite) {
	volatile int *ledPtr = (int*) JP1_BASE;

	*ledPtr = ledsToWrite; // setting the leds
}

// Returns the ADC value
int readADC() {
	int offset;

	// Reading the slider
	if (readSlider() == 0) {
		offset = 0x00;
	}
	else {
		offset = 0x04;
	}

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
	
	return adcVal;
}

// Reads the slider value
int readSlider() {
	volatile int *sliderPtr = (int*) SW_BASE;

	int slider = *sliderPtr;

	// Reading the first bit
	return (slider & 1);
}

// Determines what leds to light up
int detrLedPos(int adcVal) {
	// Determine leds off
	int percentLedsOn = (adcVal*100) / 4095; // what percentage
	int numLedsOn = percentLedsOn / 10;
	int shiftAmount = 10 - numLedsOn;

	return (0b1111111111 >> shiftAmount);
}