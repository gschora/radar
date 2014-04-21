#define ARM_MATH_CM4
#include <arm_math.h>

int SAMPLE_RATE_HZ = 125000;              // Sample rate of the audio in hertz.
const int FFT_SIZE = 256;               // Size of the FFT. Realistically can only be at most 256
// without running out of memory for buffers and other state.

const int RADAR_INPUT_PIN = 0;
const int ANALOG_READ_RESOLUTION = 10;  // Bits of resolution for the ADC.
const int ANALOG_READ_AVERAGING = 1;    // Number of samples to average with each ADC reading.

IntervalTimer samplingTimer;
float samples[FFT_SIZE * 2];
float magnitudes[FFT_SIZE];
int sampleCounter = 0;

void setup()
{
    Serial.begin(115200);

    // Set up ADC and audio input.
    pinMode(RADAR_INPUT_PIN, INPUT);
    analogReadResolution(ANALOG_READ_RESOLUTION);
    analogReadAveraging(ANALOG_READ_AVERAGING);

    samplingBegin();
}

int val;
int freq = 0;

void loop()
{
    // Calculate FFT if a full sample is available.
    if (samplingIsDone())
    {
        // Run FFT on sample data.
        arm_cfft_radix4_instance_f32 fft_inst;
        arm_cfft_radix4_init_f32(&fft_inst, FFT_SIZE, 0, 1);
        arm_cfft_radix4_f32(&fft_inst, samples);
        // Calculate magnitude of complex numbers output by the FFT.
        arm_cmplx_mag_f32(samples, magnitudes, FFT_SIZE);

        // // Restart audio sampling.
        
        // arm_rfft_fast_instance_f32 fft_inst;
        // arm_rfft_fast_init_f32(&fft_inst, fftLen);
        // arm_rfft_fast_f32(&fft_inst,inSamples,outSamples,0,1);
        freq = 0;
        getHighestFrequency();
        

        samplingBegin();
    }

    // Parse any pending commands.
    // parserLoop();
    radarLoop();

}

void getHighestFrequency(){

	for (int i = 1; i < FFT_SIZE/2; ++i) {
		int m = (int)magnitudes[i]/1000;
		if (m > freq){
			freq = m;
		}
	}
}

void radarLoop(){
	Serial.println(freq);
	delay(250);
}

////////////////////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Compute the average magnitude of a target frequency window vs. all other frequencies.
void windowMean(float *magnitudes, int lowBin, int highBin, float *windowMean, float *otherMean)
{
    *windowMean = 0;
    *otherMean = 0;
    // Notice the first magnitude bin is skipped because it represents the
    // average power of the signal.
    for (int i = 1; i < FFT_SIZE / 2; ++i)
    {
        if (i >= lowBin && i <= highBin)
        {
            *windowMean += magnitudes[i];
        }
        else
        {
            *otherMean += magnitudes[i];
        }
    }
    *windowMean /= (highBin - lowBin) + 1;
    *otherMean /= (FFT_SIZE / 2 - (highBin - lowBin));
}

////////////////////////////////////////////////////////////////////////////////
// SAMPLING FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

void samplingCallback()
{
    // Read from the ADC and store the sample data
    samples[sampleCounter] = (float32_t)analogRead(RADAR_INPUT_PIN);
    // Complex FFT functions require a coefficient for the imaginary part of the input.
    // Since we only have real data, set this coefficient to zero.
    samples[sampleCounter + 1] = 0.0;
    // Update sample buffer position and stop after the buffer is filled
    sampleCounter += 2;
    if (sampleCounter >= FFT_SIZE * 2)
    {
        samplingTimer.end();
    }
}

void samplingBegin()
{
    // Reset sample buffer position and start callback at necessary rate.
    sampleCounter = 0;
    samplingTimer.begin(samplingCallback, 1000000 / SAMPLE_RATE_HZ);
}

boolean samplingIsDone()
{
    return sampleCounter >= FFT_SIZE * 2;
}