#include "mbed.h"
#include "sample_hardware.hpp"
#include "Networkbits.hpp"

#define RED_DONE 1
#define YELLOW_DONE 2

int SAMPLE_RATE = 1000; //1 second
int SAMPLING = 1;
int SDWRITE_RATE = 10000;   //10 seconds
int LOGGING = 1;

//Digital outputs
DigitalOut onBoardLED(LED1);
DigitalOut redLED(PE_15);
DigitalOut yellowLED(PB_10);
DigitalOut greenLED(PB_11);

//Inputs
DigitalIn  onBoardSwitch(USER_BUTTON);
DigitalIn  SW1(PE_12);
DigitalIn  SW2(PE_14);
Serial pc(USBTX, USBRX);
AnalogIn adcIn(PA_0);

//Environmental Sensor driver
#ifdef BME
BME280 sensor(D14, D15);
#else
BMP280 sensor(D14, D15);
#endif

//LCD Driver (provided via mbed repository)
//RS D9
//E  D8
//D7,6,4,2 are the 4 bit for d4-7
TextLCD lcd(D9, D8, D7, D6, D4, D2); // rs, e, d4-d7

//SD Card
SDBlockDevice sd(PB_5, D12, D13, D10); // mosi, miso, sclk, cs 

FATFileSystem fs("sd", &sd);

Mutex Mutex_SDCard;

time_t seconds = time(NULL);

FIFO bufferData;

Serial PC(USBTX, USBRX);

//POWER ON SELF TEST
void post() 
{
    //POWER ON TEST (POT)
    puts("**********STARTING POWER ON SELF TEST (POST)**********");
    
    //Test LEDs
    puts("ALL LEDs should be blinking");
    for (unsigned int n=0; n<10; n++) {
        redLED    = 1;
        yellowLED = 1;
        greenLED  = 1;
        ThisThread::sleep_for(50);
        redLED    = 0;
        yellowLED = 0;
        greenLED  = 0;     
        ThisThread::sleep_for(50);
    } 
    
    //Output the switch states (hold them down to test)
    printf("SW1: %d\tSW2: %d\n\r", SW1.read(), SW2.read());    
    printf("USER: %d\n\r", onBoardSwitch.read()); 
    
    //Output the ADC
    printf("ADC: %f\n\r", adcIn.read());
    
    //Read Sensors (I2C)
    float temp = sensor.getTemperature();
    float pressure = sensor.getPressure();
    #ifdef BME
    float humidity = sensor.getHumidity();
    #endif
   
    //Display in PuTTY
    printf("Temperature: %5.1f\n", temp);
    printf("Pressure: %5.1f\n", pressure);
    #ifdef BME
    printf("Pressure: %5.1f\n", humidity);
    #endif
    
    //Display on LCD
    redLED = 1;
    lcd.cls();
    lcd.printf("LCD TEST...");
    ThisThread::sleep_for(500);
    redLED = 0;
    
    //Network test (if BOTH switches are held down)
    networktest();

    //Initalise SD Card
    SD_INIT(); 
    
    
    puts("**********POST END**********");
 
}

void errorCode(ELEC350_ERROR_CODE err)
{
    switch (err) {
      case OK:
        greenLED = 1;
        ThisThread::sleep_for(1000);
        greenLED = 0;
        return;                
      case FATAL:
        while(1) {
            redLED = 1;
            ThisThread::sleep_for(100);
            redLED = 0;
            ThisThread::sleep_for(100);
        }
    };
}



//----------------------------------------------------------// FIFO

//constructor
FIFO::FIFO()
	: // protected class attributes
	samplesInBuffer(0),
	spaceInBuffer(FIFO_BUFFER_SIZE-1),
	oldestSample(0),
	newestSample(0)
//Method logic
{
	if(init() == 1)
	{
		printf("FIFO Init Successful");
	} else {
		printf("FIFO Init Failed");
	}
}

//destructor
FIFO::~FIFO()
{
	//flush
}

//initalisation
int FIFO::init()
{
	return 1; //successful
}

//public methods
int FIFO::AddToFIFO(char* _EventType, char* _Tempeture, char* _Pressure, char* _Light, char* _EventMessage, char* _ErrorMessage, char* _Time)
{
    //Data being read and be written to, therefore lock
    FIFO_Mutex.lock();
    //if buffer is full, return error
    if(this->newestSample == this->oldestSample-1)
    {
        //UNLOCK MUTEX!
        FIFO_Mutex.unlock();
        return -1;
    }

    //wrap around buffer index (newestSample)
    if(this->newestSample == FIFO_BUFFER_SIZE-1)
    {
        this->newestSample = 0;
    }

    //if buffer is not full, add new data to buffer and increment newestSample
	this->Data[this->newestSample][0] = _Time;
    this->Data[this->newestSample][1] = _EventType;
    this->Data[this->newestSample][2] = _Tempeture;
    this->Data[this->newestSample][3] = _Pressure;
    this->Data[this->newestSample][4] = _Light;
    this->Data[this->newestSample][5] = _EventMessage;
    this->Data[this->newestSample][6] = _ErrorMessage;
	this->newestSample++;
    FIFO_Mutex.unlock();
    return 1; //return success
    /*
    0 -> Time
    1 -> Event type (Data, Event, Error)
    2 -> Tempeture
    3 -> Pressure
    4 -> Light
    5 -> Event Message
    6 -> Error Message
    */
}



int FIFO::ReadOne(char* *EventTypeOutput, char* *TempetureOutput, char* *PressureOutput, char* *LightOutput, char* *EventMessageOutput, char* *ErrorMessageOutput, char* *TimeOutput)
{    
    //Data being read and be written to, therefore lock
    FIFO_Mutex.lock();
    //if there are no new samples, return -1
	if(this->oldestSample == this->newestSample)
	{      
        //UNLOCK MUTEX!  
        FIFO_Mutex.unlock();
		return -1;
	}
    //wrap around buffer index (oldestSample)
    if(this->oldestSample == FIFO_BUFFER_SIZE-1)
    {
        this->oldestSample = 0;
    }
    //else set return data BY REFERENCE! (hard to reture arrays)
	*TimeOutput = this->Data[this->oldestSample][0];
    *EventTypeOutput = this->Data[this->oldestSample][1];
    *TempetureOutput = this->Data[this->oldestSample][2];
    *PressureOutput = this->Data[this->oldestSample][3];
    *LightOutput = this->Data[this->oldestSample][4];
    *EventMessageOutput = this->Data[this->oldestSample][5];
    *ErrorMessageOutput = this->Data[this->oldestSample][6];
    //PC.printf("Oldest sample index: %d\nNewest Sample Index: %d\n", this->oldestSample, this->newestSample);
    //increment oldestSample
	this->oldestSample++;    
    FIFO_Mutex.unlock();
	return 1; //return success
}

int FIFO::ReadLatestWithoutRemove(char* *TempetureOutput, char* *PressureOutput, char* *LightOutput, char* *TimeOutput)
{
    FIFO_Mutex.lock();
    while(1)
    {
        int Index = this->newestSample;    
        if(Index == this->oldestSample)
        {
            FIFO_Mutex.unlock();
            return -1;
        }

        if(strncmp(this->Data[Index][1], "DATA", 4) == 0)
        {
            *TimeOutput = this->Data[Index][0];
            *TempetureOutput = this->Data[Index][2];
            *PressureOutput = this->Data[Index][3];
            *LightOutput = this->Data[Index][4];
            FIFO_Mutex.unlock();
            return 1;
        }
        else 
        {
            Index -= 1;
        }
    }    
}

int FIFO::ReadBufferWithoutRemoval(char* *EventTypeOutput, char* *TempetureOutput, char* *PressureOutput, char* *LightOutput, char* *EventMessageOutput, char* *ErrorMessageOutput, char* *TimeOutput)
{
    FIFO_Mutex.lock();
    while(1)
    {
        int Index = this->oldestSample;
        if(Index == this->newestSample)
        {            
            FIFO_Mutex.unlock();
            return 1;
        }           
    }
}

//----------------------------------------------------------//


void SD_INIT()
{

 if ( sd.init() != 0) {
        printf("Init failed \n");
        lcd.cls();
		lcd.locate(0,0);
        lcd.printf("CANNOT INIT SD");        
        errorCode(FATAL);
    }     
}

void SD_WRITE()
{    
    //declare local variables
    char* EventTypeReturned;
    char* TempetureReturned;
    char* PressureReturned;
    char* LightReturned;
    char* EventMessageReturned;
    char* ErrorMessageReturned;
    char* TimeReturned;

    char* LocalBuffer[FIFO_BUFFER_SIZE-1][7];
    int LocalBufferIndex = 0;
    int Action = 0;
    FILE* fp;
    string Type_Data;
    string Type_Event;
    string Type_Error;

    string DataToWrite;

    
    /*
        retrive data from buffer and place in local buffer
        this is done to minimise time locking global data buffer
    */
    int result = 0;    
    while(1 == bufferData.ReadOne(&EventTypeReturned, &TempetureReturned, &PressureReturned, &LightReturned, &EventMessageReturned, &ErrorMessageReturned, &TimeReturned))
    {
        LocalBuffer[LocalBufferIndex][0] = TimeReturned;
        LocalBuffer[LocalBufferIndex][1] = EventTypeReturned;
        LocalBuffer[LocalBufferIndex][2] = TempetureReturned;
        LocalBuffer[LocalBufferIndex][3] = PressureReturned;
        LocalBuffer[LocalBufferIndex][4] = LightReturned;
        LocalBuffer[LocalBufferIndex][5] = EventMessageReturned;
        LocalBuffer[LocalBufferIndex][6] = ErrorMessageReturned;
        LocalBufferIndex++;
        //printf("Event Type returned: %s\n Data: %s , %s , %s\n Event: %s\n Error: %s\n", LocalBuffer[LocalBufferIndex][1], LocalBuffer[LocalBufferIndex][2], LocalBuffer[LocalBufferIndex][3], LocalBuffer[LocalBufferIndex][4], LocalBuffer[LocalBufferIndex][5], LocalBuffer[LocalBufferIndex][6]);
    }
    
    //Lock SD card to prevent other threads reading whilst writing to the SD card in block
    Mutex_SDCard.lock();
    for(int i = 0; i<LocalBufferIndex; i++) //read all data retrieved from global buffer
    {           
        if(strncmp(LocalBuffer[i][1], "DATA", 4) == 0)
        {
            fp = fopen("/sd/Data.txt", "a+");
            string DataToWrite = "Time: ";
            DataToWrite += LocalBuffer[i][0];

            DataToWrite += "  Tempeture: ";
            DataToWrite += LocalBuffer[i][2];
            DataToWrite += "\n";

            DataToWrite += "  Pressure: ";
            DataToWrite += LocalBuffer[i][3];
            DataToWrite += "\n";
            
            DataToWrite += "  Light: ";
            DataToWrite += LocalBuffer[i][4];
            DataToWrite += "\n\n";

            fprintf(fp, DataToWrite.c_str());

            fclose(fp);
        }
        else if(strncmp(LocalBuffer[i][1], "EVENT", 5) == 0)
        {
            fp = fopen("/sd/Event.txt", "a+");
            string DataToWrite = "Time: ";
            DataToWrite += LocalBuffer[i][0];

            DataToWrite += "  Event: ";
            DataToWrite += LocalBuffer[i][5];
            DataToWrite += "\n\n";

            fprintf(fp, DataToWrite.c_str());
            fclose(fp);
        }
        else
        {
            fp = fopen("/sd/Error.txt", "a+");
            string DataToWrite = "Time: ";
            DataToWrite += LocalBuffer[i][0];

            DataToWrite += "  Error: ";
            DataToWrite += LocalBuffer[i][6];
            DataToWrite += "\n\n";

            fprintf(fp, DataToWrite.c_str());
            fclose(fp);
        }
        
    }     
    Mutex_SDCard.unlock(); 
}


void SD_DEINIT()
{
    //Close down
    sd.deinit();
    printf("Unmounted...\n");
    lcd.cls();
    lcd.printf("Unmounted...\n\n");
}


Mutex mutex_Serial;

void Serial_PrintCommandOptions()
{
    mutex_Serial.lock();
    PC.puts("\t1: Read Current Reading\n");
    PC.puts("\t2: Read Entire buffer\n");
    PC.puts("\t3: Set Sampling Time\n");
    PC.puts("\t4: Set Sampling On or Off\n");
    PC.puts("\t5: Set Logging On or Off\n");
    PC.puts("\t6: Flush Buffer\n");
    PC.puts("\t7: Eject SD Card\n");
    mutex_Serial.unlock();
}

void Serial_ReadCurrentReading()
{
    char* TimeReturned;
    char* TempetureReturned;
    char* PressureReturned;
    char* LightReturned;
    bufferData.ReadLatestWithoutRemove(&TempetureReturned, &PressureReturned, &LightReturned, &TimeReturned );
    PC.puts("Lastest Data Reading:\n");

    PC.puts("\tTime:");
    PC.puts(TimeReturned);
    PC.puts("\n");

    PC.puts("\tTempeture:");
    PC.puts(TempetureReturned);
    PC.puts("\n");

    PC.puts("\tPressure:");
    PC.puts(PressureReturned);
    PC.puts("\n");

    PC.puts("\tLight:");
    PC.puts(LightReturned);
    PC.puts("\n");
}

void Serial_ReadEntireBuffer()
{
    PC.puts("READ ENTIRE BUFFER\n");
}

void Serial_SetSamplingTime()
{
    PC.puts("SET SAMPLING TIME\n");
}

void Serial_SetSampling()
{    
    int Action;
    int Exit = 0;
    
    while(Exit == 0)
    {
        Action = 0;
        PC.puts("\033[2J");
        PC.puts("Sampling is currently ");
        if(SAMPLING == 1)
        {
            PC.puts("On\n");
        }
        else
        {
            PC.puts("Off\n");
        }
        PC.puts("Please a command\n");
        PC.puts("1 - Turn Sampling On\n");
        PC.puts("2 - Turn Sampling Off\n");
        PC.puts("3 - Return without changing\n");
        PC.scanf("%d", &Action);
        switch(Action)
        {
            case 1:
                SAMPLING = 1;
                PC.puts("Sampling Turned On\n");
                Exit = 1;
            break;

            case 2:
                SAMPLING = 0;
                PC.puts("Sampling Turned Off\n");
                Exit = 1;
            break;

            case 3:
                PC.puts("Exiting without changing\n");
                Exit = 1;
            break;

            default:
                PC.puts("Please enter a valid command\n");
                PC.scanf("%d", &Action);
            break;
        }
    }
    PC.puts("Press any key to return\n");
    PC.scanf("%d", &Action);
}

void Serial_SetLogging()
{
    int Action;
    int Exit = 0;
    
    while(Exit == 0)
    {
        Action = 0;
        PC.puts("\033[2J");
        PC.puts("Logging is currently ");
        if(LOGGING == 1)
        {
            PC.puts("On\n");
        }
        else
        {
            PC.puts("Off\n");
        }
        PC.puts("Please a command\n");
        PC.puts("1 - Turn Loging On\n");
        PC.puts("2 - Turn Logging Off\n");
        PC.puts("3 - Return without changing\n");
        PC.scanf("%d", &Action);
        switch(Action)
        {
            case 1:
                LOGGING = 1;
                PC.puts("Logging Turned On\n");
                Exit = 1;
            break;

            case 2:
                LOGGING = 0;
                PC.puts("Logging Turned Off\n");
                Exit = 1;
            break;

            case 3:
                PC.puts("Exiting without changing\n");
                Exit = 1;
            break;

            default:
                PC.puts("Please enter a valid command\n");
                PC.scanf("%d", &Action);
            break;
        }
    }
    PC.puts("Press any key to return\n");
    PC.scanf("%d", &Action);
}

void Serial_FlushBuffer()
{
    PC.puts("FLUSH BUFFER\n");
}

void Serial_EjectSD()
{
    PC.puts("EJECT SD\n");
}

void Serial_InvalidCommand()
{
    PC.puts("Invalid command...\n");
}

