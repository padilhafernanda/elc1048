#include <Arduino_FreeRTOS.h>
#include <semphr.h>  // add the FreeRTOS functions for Semaphores (or Flags).
// Include queue support
#include <queue.h>

// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
// It will be used to ensure only only one Task is accessing this resource at any time.
SemaphoreHandle_t xSerialSemaphore;


// Define a struct
struct pinRead {
  int pin;
  float value;
};

/* 
 * Declaring a global variable of type QueueHandle_t 
 * 
 */
QueueHandle_t structQueue;
QueueHandle_t structBuffer;
//float bufferMedia[10];
int k=0;

void TaskAnalogRead( void *pvParameters );
void TaskTempAtual( void *pvParameters );
void TaskTempMedia( void *pvParameters );
TaskHandle_t xTempMedia;
// the setup function runs once when you press reset or power the board
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
Serial.print("buceta ");
Serial.print("Cuzão ");
Serial.println(k);

//vTaskSuspend( xTempMedia );
  // Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
  // because it is sharing a resource, such as the Serial port.
  // Semaphores should only be used whilst the scheduler is running, but we can set it up here.
  if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
  }


  
  structQueue = xQueueCreate(10, // Queue length
                              sizeof(struct pinRead) // Queue item size
                              );

  structBuffer = xQueueCreate(10, // Queue length
                              sizeof(float) // Queue item size
                              );

  if (structQueue != NULL) {                              
  // Now set up two Tasks to run independently.
  xTaskCreate(
    TaskTempAtual
    ,  "TempAtual"  // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

    
  xTaskCreate(
    TaskTempMedia
    ,  "TempMedia"  // A name just for humans
    ,  128  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );    


  xTaskCreate(
    TaskAnalogRead
    ,  "AnalogRead"
    ,  128  // Stack size
    ,  NULL
    ,  2  // Priority
    ,  NULL );
  }
  // Now the Task scheduler, which takes over control of scheduling individual Tasks, is automatically started.
}

void loop()
{
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


void TaskAnalogRead( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  for (;;)
  {
    // Read the input on analog pin 0:
    struct pinRead currentPinRead;
    currentPinRead.pin = 0;
    currentPinRead.value = (float(analogRead(A0))*5/(1023))/0.01;
     /**
     * Post an item on a queue.
     * https://www.freertos.org/a00117.html
     */
    xQueueSend(structQueue, &currentPinRead, portMAX_DELAY);
    vTaskDelay(1);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskTempAtual( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  /*
    Consome dado do buffer se disponível;
  */
  for (;;) // A Task shall never return or exit.
  {
    
    struct pinRead currentPinRead;
    float bufferTemp[10];
    int ptT=&bufferTemp;
    /**
     * Read an item from a queue.
     * https://www.freertos.org/a00118.html
     */
    if (xQueueReceive(structQueue, &currentPinRead, portMAX_DELAY) == pdPASS) {
      bufferTemp[k]=currentPinRead.value;
      
      k=k+1;
      if(k<11){  
      if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
      {
        
        
        //k=k+1;
        Serial.print("Temp Atual: ");
        Serial.println(currentPinRead.value);  
        Serial.println(k);  
        //Serial.println(bufferTemp[0]);     
        xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
       }
      }
    }
      if (k==11){
        k=k-1;
        ptT=&bufferTemp;
        xQueueSend(structBuffer, &ptT, portMAX_DELAY);    
        ptT=ptT+1;
        
      }
    
    }   
  }

void TaskTempMedia( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  float bufferMedia[10];
  int ptM=&bufferMedia;
  int t=0;
  float media;
  float acumulado;
  for (;;) // A Task shall never return or exit.
  {
    if(xQueueReceive(structBuffer, &bufferMedia, portMAX_DELAY) == pdPASS){
      t=t+1;
      if (t==9){
        ptM=&bufferMedia;
        for (int j=0;j<10;j++)
          {
            acumulado = acumulado + bufferMedia[j]; 
            ptM=ptM+1;           
          } 
          media=acumulado/10;
                
      // See if we can obtain or "Take" the Serial Semaphore.
      // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
      
      if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
      {         
        Serial.print("Media: ");
        Serial.println(media);
        Serial.print("bostona: ");
        Serial.println(t);
        Serial.println(bufferMedia[0]);
        media=0;
        acumulado=0;
        k=0;
        t=0;
        xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
      }

     }
    } 
  
  }   
}
