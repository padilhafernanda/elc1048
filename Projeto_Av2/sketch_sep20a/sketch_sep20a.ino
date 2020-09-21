#include <Arduino_FreeRTOS.h>
#include <semphr.h>  // add the FreeRTOS functions for Semaphores (or Flags).

// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
// It will be used to ensure only only one Task is accessing this resource at any time.
SemaphoreHandle_t xSerialSemaphore;
SemaphoreHandle_t xControlSemaphore;

float bufferAtual[10];
float bufferMedia[10];
int i=0;
int k=0;


void TaskAnalogRead( void *pvParameters );
void TaskTempAtual( void *pvParameters );
void TaskTempMedia( void *pvParameters );
//TaskHandle_t xAnalogRead;
//TaskHandle_t xTempAtual;
//TaskHandle_t xTempMedia;
//TaskHandle_t xSemaforo;

// the setup function runs once when you press reset or power the board
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
Serial.print("buceta ");
Serial.print("Cuzão ");
Serial.println(i);
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

  if ( xControlSemaphore == NULL )  // Check to confirm that the Control Semaphore has not already been created.
  {
    xControlSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Control Variable
    if ( ( xControlSemaphore ) != NULL )
      xSemaphoreGive( ( xControlSemaphore ) );  // Make the Control variable available for use, by "Giving" the Semaphore.
  }



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
    // read the input on analog pin 0:
    float leitura = (float(analogRead(A0))*5/(1023))/0.01;
           
    // See if we can obtain or "Take" the Control Semaphore.
    // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
    if ( xSemaphoreTake( xControlSemaphore, ( TickType_t ) 5 ) == pdTRUE )
    {
      // We were able to obtain or "Take" the semaphore and can now access the shared resource.
      // We want to have the Control Variable for us alone
      bufferAtual[i]=leitura;
      //i=i+1;
      k=k+1;
      
      if (k==9){
        //vTaskSuspend( NULL );
        //vTaskResume( xTempMedia );
        xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
        //xSemaphoreGive( xControlSemaphore ); // Now free or "Give" the Control Variable for others.
      }
      
      if(i<9)
      {
        //vTaskSuspend( xTempMedia );
        i=i+1;
        xSemaphoreGive( xControlSemaphore ); // Now free or "Give" the Control Variable for others.
      }
    }
         
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
      //xSemaphoreGive( xControlSemaphore ); // Now free or "Give" the Control Variable for others.
      // See if we can obtain or "Take" the Control Semaphore.
      // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
      if ( xSemaphoreTake( xControlSemaphore, ( TickType_t ) 5 ) == pdTRUE )
      {
        //vTaskResume( xAnalogRead );
        float temperaturaAtual;
        if (i!=0)
        { 
          // We were able to obtain or "Take" the semaphore and can now access the shared resource.
          // We want to have the Control Variable for us alone
          temperaturaAtual=bufferAtual[i-1];
          bufferMedia[k-1] = temperaturaAtual;
          i=i-1;        
          // See if we can obtain or "Take" the Serial Semaphore.
          // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
          if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
          {
            // We were able to obtain or "Take" the semaphore and can now access the shared resource.
            // We want to have the Serial Port for us alone, as it takes some time to print,
            // so we don't want it getting stolen during the middle of a conversion.
            // print out the state of the button:
            Serial.print("Temp Atual: ");
            Serial.println(temperaturaAtual);
            Serial.print("Control: ");
            Serial.println(k);
            Serial.println(i);
            xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
          }
        }
        xSemaphoreGive( xControlSemaphore ); // Now free or "Give" the Control Variable for others.  
      }       
  }
}

void TaskTempMedia( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  float media;
  float acumulado;

  for (;;) // A Task shall never return or exit.
  {
    for (int j=0;j<10;j++)
    {
      acumulado = acumulado + bufferMedia[j];            
    } 
    media=acumulado/10;          
    // See if we can obtain or "Take" the Serial Semaphore.
    // If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.
    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
   {
    //vTaskSuspend( xTempAtual );
    // We were able to obtain or "Take" the semaphore and can now access the shared resource.
    // We want to have the Serial Port for us alone, as it takes some time to print,
    // so we don't want it getting stolen during the middle of a conversion.
    // print out the state of the button:
    Serial.print("Media: ");
    Serial.println(media);
    Serial.print("Kaceta: ");
    Serial.println(k);
    media=0;
    acumulado=0;
    xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    xSemaphoreGive( xControlSemaphore );
   }
  }    
}
