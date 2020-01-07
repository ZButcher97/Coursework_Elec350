#include "sample_hardware.hpp"
#include "Networkbits.hpp"




// This is a very short demo that demonstrates all the hardware used in the coursework.
// You will need a network connection set up (covered elsewhere). The host PC should have the address 10.0.0.1



//Threads
Thread thread_Network;
Thread thread_Sampling;
Thread thread_SDCard;
Thread thread_Led;  //declard LED thread
Thread thread_Serial;

void thread_SampleData();
void thread_SDCardWrite();
void thread_LEDFlash(DigitalOut *LED_TOG);
void thread_SerialInterface();

int main() {
    //Greeting
    PC.printf("Testing\n\n");    		
    
    //Power on self test
    post();       

    thread_Sampling.start(thread_SampleData); 
    thread_SDCard.start(thread_SDCardWrite);
    thread_Serial.start(thread_SerialInterface);
    //thread_Led.start(callback(thread_LEDFlash, &greenLED)); 
    PC.puts("Serial Reading Test\n");

    while(true)
	{		        
        ThisThread::sleep_for(1000);
        yellowLED = !yellowLED;

        if(SW1 == 1)
        {
            SD_DEINIT();
        }
	}	
}

void thread_SampleData()
{
    while(true)
    {    
        ThisThread::sleep_for(SAMPLE_RATE);
        if(SAMPLING == 1)
        {            
            char* temp_s;
            char* pressure_s;
            char* light_s;

            char temp_carr[10];
            char pressure_carr[10];
            char light_carr[10];

            double temp = sensor.getTemperature();
            double pressure = sensor.getPressure();
            double light = adcIn.read();
            
            sprintf(temp_carr, "%4.1f", temp);
            sprintf(pressure_carr, "%7.2f", pressure);
            sprintf(light_carr, "%5.3f", light);

            temp_s = (char*)temp_carr;
            pressure_s = (char*)pressure_carr;
            light_s = (char*)light_carr;
        
            char* time = ctime(&seconds);    

            lcd.cls();
            lcd.printf("T:%s ", temp_s);
            lcd.printf("L:%s", light_s);
            lcd.printf("P:%s", pressure_s);

            bufferData.AddToFIFO((char*)"DATA", temp_s, pressure_s, light_s, (char*)"", (char*)"", time);
            if(LOGGING == 1) //Needs mutex
            {
                bufferData.AddToFIFO((char*)"EVENT", (char*)"", (char*)"", (char*)"",(char*)"Sensors Sampled", (char*)"", time);
            }
        }
    }
}

void thread_SDCardWrite()
{
    while(true)
    {
        ThisThread::sleep_for(SDWRITE_RATE); //sleep until time to write to SD        
        //thread_Led.flags_set(0x02);
        SD_WRITE(); //write to sd card
    }
}

void thread_LEDFlash(DigitalOut *LED_TOG)
{
    while(1)
    {
        ThisThread::flags_wait_all(0x02);
        PC.printf("Green LED ON\n");
        *LED_TOG = !*LED_TOG;
        ThisThread::sleep_for(500);
        PC.printf("Green LED OFF\n");
        *LED_TOG = !*LED_TOG;
    }
}


void thread_SerialInterface()
{    
    int Action;    
    while(1)
    {        
        PC.puts("\033[2J");
        PC.puts("\t-- Welcome to out ELEC350 Coursework --\n\n");
        PC.puts("Please select a command from the folowing options:\n");
        Serial_PrintCommandOptions();

        PC.scanf("%d", &Action);   

        PC.printf("Selected Action: %d\n", Action);   
        switch(Action)
        {
            case 1:
                Serial_ReadCurrentReading();
            break;

            case 2:
                Serial_ReadEntireBuffer();
            break;

            case 3:
                Serial_SetSamplingTime();
            break;

            case 4:
                Serial_SetSampling();
            break;

            case 5:
                Serial_SetLogging();
            break;

            case 6:
                Serial_FlushBuffer();
            break;

            case 7:
                Serial_EjectSD();
            break;

            default:
                Serial_InvalidCommand();
            break;
        }
        Action = 0;
    }
}


