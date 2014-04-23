#define ARM_MATH_CM4
#include <arm_math.h>

int SAMPLE_RATE_HZ = 250;              // Sample rate of the audio in hertz.
const int FFT_SIZE = 256;               // Size of the FFT. Realistically can only be at most 256
// without running out of memory for buffers and other state.

const int RADAR_INPUT_PIN = 0;
const int ANALOG_READ_RESOLUTION = 10;  // Bits of resolution for the ADC.
const int ANALOG_READ_AVERAGING = 1;    // Number of samples to average with each ADC reading.
const int MAX_CHARS = 65; // Max size of the input command buffer

IntervalTimer samplingTimer;
float samples[FFT_SIZE * 2];
float magnitudes[FFT_SIZE];
float newMag[FFT_SIZE-1];
int sampleCounter = 0;
uint32_t testIndex = 0;
float32_t maxValue;
char commandBuffer[MAX_CHARS];

void setup()
{
    Serial.begin(115200);

    // Set up ADC and audio input.
    pinMode(RADAR_INPUT_PIN, INPUT);
    analogReadResolution(ANALOG_READ_RESOLUTION);
    analogReadAveraging(ANALOG_READ_AVERAGING);

    // Clear the input command buffer
  memset(commandBuffer, 0, sizeof(commandBuffer));

    samplingBegin();
}

int val;

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

        //     if (true) {
        //   for (int i = 1; i < FFT_SIZE; ++i) {
        //     Serial.println(magnitudes[i]);
        //   }
        //   Serial.println("---------------");
        // }
        
        for (int i = 1; i < FFT_SIZE/2; ++i) {
        	newMag[i] = magnitudes[i];
        }

        arm_max_f32(newMag, FFT_SIZE, &maxValue, &testIndex);
        // Serial.println(testIndex);

        samplingBegin();
    }

    // Parse any pending commands.
    parserLoop();
    radarLoop();

}


void radarLoop()
{
    // Serial.println(testIndex);
    // delay(250);
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

////////////////////////////////////////////////////////////////////////////////
// COMMAND PARSING FUNCTIONS
// These functions allow parsing simple commands input on the serial port.
// Commands allow reading and writing variables that control the device.
//
// All commands must end with a semicolon character.
//
// Example commands are:
// GET SAMPLE_RATE_HZ;
// - Get the sample rate of the device.
// SET SAMPLE_RATE_HZ 400;
// - Set the sample rate of the device to 400 hertz.
//
////////////////////////////////////////////////////////////////////////////////

void parserLoop()
{
    // Process any incoming characters from the serial port
    while (Serial.available() > 0)
    {
        char c = Serial.read();
        // Add any characters that aren't the end of a command (semicolon) to the input buffer.
        if (c != ';')
        {
            c = toupper(c);
            strncat(commandBuffer, &c, 1);
        }
        else
        {
            // Parse the command because an end of command token was encountered.
            parseCommand(commandBuffer);
            // Clear the input buffer
            memset(commandBuffer, 0, sizeof(commandBuffer));
        }
    }
}

// Macro used in parseCommand function to simplify parsing get and set commands for a variable
#define GET_AND_SET(variableName) \
    else if (strcmp(command, "GET " #variableName) == 0) { \
        Serial.println(variableName); \
    } \
    else if (strstr(command, "SET " #variableName " ") != NULL) { \
        variableName = (typeof(variableName)) atof(command+(sizeof("SET " #variableName " ")-1)); \
    }

void parseCommand(char *command)
{
    if (strcmp(command, "GET MAGNITUDES") == 0)
    {
        for (int i = 0; i < FFT_SIZE; ++i)
        {
        	Serial.print(i);
        	Serial.print(" :");
            Serial.println(magnitudes[i]);
        }
    }
    else if (strcmp(command, "GET SAMPLES") == 0)
    {
        for (int i = 0; i < FFT_SIZE * 2; i += 2)
        {
        	Serial.print(i);
        	Serial.print(" :");
            Serial.println(samples[i]);
        }
    }
    else if (strcmp(command, "GET FFT_SIZE") == 0)
    {
        Serial.println(FFT_SIZE);
    } else if (strcmp(command, "GET MAX") == 0)
    {
    	Serial.print("max : ");
    	Serial.print(testIndex);
        Serial.print(" ");
        Serial.println(maxValue);
    }

    GET_AND_SET(SAMPLE_RATE_HZ)
    // GET_AND_SET(SPECTRUM_MIN_DB)
    // GET_AND_SET(SPECTRUM_MAX_DB)

    // // Update spectrum display values if sample rate was changed.
    // if (strstr(command, "SET SAMPLE_RATE_HZ ") != NULL)
    // {
    //     spectrumSetup();
    // }

    
}