#ifndef __sample_hardware__
#define __sample_hardware__

//#define BME
#ifdef BME
#include "BME280.h"
#else
#include "BMP280.h"
#endif
#include "TextLCD.h"
#include "SDBlockDevice.h"
#include "FATFileSystem.h"

#define FIFO_BUFFER_SIZE 100

enum ELEC350_ERROR_CODE {OK, FATAL};

extern DigitalOut onBoardLED;
extern DigitalOut redLED;
extern DigitalOut yellowLED;
extern DigitalOut greenLED;

extern DigitalIn  onBoardSwitch;
extern DigitalIn  SW1;
extern DigitalIn  SW2;
extern Serial PC;
extern AnalogIn adcIn;

#ifdef BME
extern BME280 sensor;
#else
extern BMP280 sensor;
#endif

extern TextLCD lcd;
extern SDBlockDevice sd;

extern void post();
extern void errorCode(ELEC350_ERROR_CODE err);

extern FATFileSystem fs;

extern time_t seconds;

extern int SAMPLE_RATE; //1 second
extern int SAMPLING;
extern int SDWRITE_RATE;   //10 seconds
extern int LOGGING;

void SD_INIT();
void SD_WRITE();
void SD_DEINIT();

void Serial_PrintCommandOptions();
void Serial_ReadCurrentReading();
void Serial_ReadEntireBuffer();
void Serial_SetSamplingTime();
void Serial_SetSampling();
void Serial_SetLogging();
void Serial_FlushBuffer();
void Serial_EjectSD();
void Serial_InvalidCommand();

class FIFO
{
	//public methods and attributes
	public:
		//constructor
		FIFO();
	
		//destructor
		virtual ~FIFO();
	
		//initlisation
		int init(void);
	
		//public methods
		int AddToFIFO(char* _EventType, char* _Tempeture, char* _Pressure, char* _Light, char* _EventMessage, char* _ErrorMessage, char* _Time);
		int ReadOne(char* *EventTypeOutput, char* *TempetureOutput, char* *PressureOutput, char* *LightOutput, char* *EventMessageOutput, char* *ErrorMessageOutput, char* *TimeOutput);
        int ReadLatestWithoutRemove(char* *TempetureOutput, char* *PressureOutput, char* *LightOutput, char* *TimeOutput);
        int ReadBufferWithoutRemoval(char* *EventTypeOutput, char* *TempetureOutput, char* *PressureOutput, char* *LightOutput, char* *EventMessageOutput, char* *ErrorMessageOutput, char* *TimeOutput);

	//protected methods and attributes
	protected:
		int samplesInBuffer;
		int spaceInBuffer;
		int oldestSample;
		int newestSample;        
		char* Data[FIFO_BUFFER_SIZE][7];
        //float Data[FIFO_BUFFER_SIZE][7];
        /*
            1 -> Time
            2 -> Event type (Data, Event, Error)
            3 -> Tempeture
            4 -> Pressure
            5 -> Light
            6 -> Event Message
            7 -> Error Message
         */
        Mutex FIFO_Mutex;
};

extern FIFO bufferData;

#endif