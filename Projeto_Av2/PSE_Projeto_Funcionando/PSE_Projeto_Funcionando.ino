#include <Arduino_FreeRTOS.h>
#include <semphr.h>  /// add the FreeRTOS functions for Semaphores (or Flags).
#include <queue.h>   /// Include queue support
#include <task.h>

/// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
/// It will be used to ensure only only one Task is accessing this resource at any time.
SemaphoreHandle_t xSerialSemaphore;
/// Estrutura que será utilizada para ler dados do sensor
struct pinRead {
  int pin;
  float value;
};
///  Flags para controle das tasks consumidoras da fila de
/// dados do sensor.
int flag;
int k;
///  Vetor que guarda 10 dados lidos do sensor para ser
/// calculada a média pela task TempMedia.
float bufferTemp[10];
///  Handle da fila que a task AnalogRead envia dados
/// lidos do sensor.
QueueHandle_t structQueue;
/// Definição das Tasks
void TaskAnalogRead( void *pvParameters );
void TaskTempAtual( void *pvParameters );
void TaskTempMedia( void *pvParameters );

/// the setup function runs once when you press reset or power the board
void setup() {
  /// initialize serial communication at 9600 bits per second:
  Serial.begin(9600);  
  while (!Serial) {
    ; /// wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
  Serial.print("Iniciando");

  /// Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
  /// because it is sharing a resource, such as the Serial port.
  /// Semaphores should only be used whilst the scheduler is running, but we can set it up here.
  if ( xSerialSemaphore == NULL ){  /// Check to confirm that the Serial Semaphore has not already been created.
    xSerialSemaphore = xSemaphoreCreateMutex();  /// Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  /// Make the Serial Port available for use, by "Giving" the Semaphore.
  }

  /// Cria a fila de dados do sensor.
  structQueue = xQueueCreate(10, sizeof(struct pinRead));

  if (structQueue != NULL) { /// Verifica se a fila foi criada.                             
  /// Cria tarefas que serão executadas independentemente.
  
  xTaskCreate(
    TaskTempAtual
    ,  "TempAtual"  /// A name just for humans
    ,  128  /// This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  /// Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );
  
  xTaskCreate(
    TaskTempMedia
    ,  "TempMedia"  /// A name just for humans
    ,  128  /// This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  /// Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );    

  xTaskCreate(
    TaskAnalogRead
    ,  "AnalogRead"
    ,  128  /// Stack size
    ,  NULL
    ,  2  /// Priority
    ,  NULL );
  }
  /// Now the Task scheduler, which takes over control of scheduling individual Tasks, is automatically started.
}

void loop()
{
  /// Empty. Things are done in Tasks.
}

///*--------------------------------------------------*/
///*---------------------- Tasks ---------------------*/
///*--------------------------------------------------*/

void TaskAnalogRead( void *pvParameters __attribute__((unused)) ){
  for (;;){
    /// Read the input on analog pin 0:
    struct pinRead currentPinRead;
    currentPinRead.pin = 0;
    ///  Codificação dos valores lidos em tensão para temperatura
    /// Fonte: https://portal.vidadesilicio.com.br/lm35-medindo-temperatura-com-arduino/
    currentPinRead.value = (float(analogRead(A0))*5/(1023))/0.01;
    
   /// Post an item on a queue.
   /// https://www.freertos.org/a00117.html
    
    xQueueSend(structQueue, &currentPinRead, portMAX_DELAY);
    vTaskDelay(1);  /// one tick delay (15ms) in between reads for stability
  }
}

void TaskTempAtual( void *pvParameters __attribute__((unused)) ){
 ///Consome dado do buffer se disponível;
  for (;;){
    struct pinRead currentPinRead;
    
     // Read an item from a queue.
     // https://www.freertos.org/a00118.html
     
    if (xQueueReceive(structQueue, &currentPinRead, portMAX_DELAY) == pdPASS) {
      bufferTemp[k]=currentPinRead.value;  
      if(k<10){
       flag=0; 
       if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE ){
        Serial.print("Temp Atual: ");
        Serial.println(currentPinRead.value);  
        Serial.println(k);
        xSemaphoreGive( xSerialSemaphore ); /// Now free or "Give" the Serial Port for others.
        k=k+1;
       }
      } else{ flag=1;}   
    }
  }   
}

void TaskTempMedia( void *pvParameters __attribute__((unused)) ){
  for (;;){
    float media;
    float acumulado;
   
    if(flag==1){ /// Verifica se já foram feitas 10 leituras e armazenamento no buffer.
      /// Cálculo da média dos valores guardados no buffer.
      for (int j=0;j<10;j++){
        acumulado = acumulado + bufferTemp[j]; 
      } 
      media=acumulado/10;
      flag=0; /// Confirma que os dados do buffer foram consumidos e podem ser substituidos.
      k=0;    /// Reseta variável de controle da task TempAtual.
             
      /// See if we can obtain or "Take" the Serial Semaphore.
      /// If the semaphore is not available, wait 5 ticks of the Scheduler to see if it becomes free.  
      if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE ){   
        Serial.print("Media: ");
        Serial.println(media);
        media=0; ///Reinicia as variáveis para um novo cálculo da média.
        acumulado=0;
        xSemaphoreGive( xSerialSemaphore ); /// Now free or "Give" the Serial Port for others.
      }     
    }
    else{flag=0;} /// Se ainda não foram feitas 10 leituras, a média não será calculada.   
  } 
}   
