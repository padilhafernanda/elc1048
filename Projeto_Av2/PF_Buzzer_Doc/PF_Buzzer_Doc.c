//-------------------------------------------------------------------------//
//                  Universidade Federal de Santa Maria                    //
//                   Curso de Engenharia de Computação                     //
//                ELC 1048 - Projeto de Sistemas Embarcados                //
//                                                                         //
//   Programador:                                                          //
//       Fernanda Hamdan Padilha (2019520380)                              //
//   Professor:                                                            //
//       Carlos Henrique Barriquello                                       //
//   Versão: 3.0 - Data: 21/09/2020                                        //
//=========================================================================//
//                         Descrição do Programa                           //
//=========================================================================//
//  Projeto desenvolvido para avaliação 2 da disciplina.                   //
//  Monitoramento da temperatura atual, armazenamento em buffer e cálculo  //
//  da média dos últimos 10 dados.                                         //
//  Utilizando a biblioteca FreeRTOS para Arduino.                         //
//  Sensor: LM35                                                           //
//  Saída: Porta serial e Buzzer                                           //
//-------------------------------------------------------------------------//

#include <Arduino_FreeRTOS.h>
#include <semphr.h>  
#include <queue.h>  
#include <task.h>

/// Declaração da mutex Semaphore Handle que vai controlar a Serial Port.
/// Garante que apenas uma tarefa controla a serial a cada vez.
SemaphoreHandle_t xSerialSemaphore;
/// Estrutura utilizada para ler dados do sensor.
struct pinRead {
  ///Pino lido da placa.
  int pin; 
  /// Valor lido da placa.
  float value; 
};
/// Controla o buffer para cálculo da Média.
int flag; 
/// Contador de preenchimento do buffer da Média.
int k; 
/// Variável de controle do buzzer.   
int i; 

///  Vetor que guarda 10 dados lidos do sensor para ser
/// calculada a média pela task TempMedia.
float bufferTemp[10];

/// Indica a porta digital ligada ao buzzer
const int pinBuzzer = 11; 

///  Handle da fila que a task AnalogRead envia dados
/// lidos do sensor.
QueueHandle_t structQueue;


void TaskAnalogRead( void *pvParameters );
void TaskTempAtual( void *pvParameters );
void TaskTempMedia( void *pvParameters );
void TaskBuzzer( void *pvParameters );

/// Função que executa quando liga a placa ou aperta o botão reset.
void setup() 
{
  /// Inicia a comunicação serial a 9600 bits por segundo.
  Serial.begin(9600);  
  while (!Serial) {
    ; /// Espera a porta serial conectar. 
  }
  Serial.print("Iniciando rotina"); /// Confirma que a conexão foi estabelecida.(Debug)

  pinMode(pinBuzzer, OUTPUT);

  if ( xSerialSemaphore == NULL ){  /// Checa se o semáforo da porta serial já não foi criado.
    xSerialSemaphore = xSemaphoreCreateMutex();  /// Cria a mutex que controla a porta serial. 
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  /// Torna a porta serial disponível, "dando" o semáforo.
  }

  /// Cria a fila de dados do sensor.
  structQueue = xQueueCreate(10, sizeof(struct pinRead));

  if (structQueue != NULL) { /// Verifica se a fila foi criada.                             
  /// Cria tarefas que serão executadas independentemente.
  
  xTaskCreate(TaskTempAtual,"TempAtual",128,NULL,2,NULL ); /// Cria a tarefa para consumir dados da fila.
  
  xTaskCreate(TaskTempMedia,"TempMedia",128,NULL,2,NULL ); /// Cria a tarefa para cálculo da média.   

  xTaskCreate(TaskAnalogRead,"AnalogRead",128,NULL,2,NULL);/// Cria a tarefa produtora de dados da fila.

  xTaskCreate(TaskBuzzer,"BuzzerTone",128,NULL,2,NULL);/// Cria a tarefa que emite sons no buzzer.
  }
  /// Agora, o escalonador de tarefas, que assume o controle do escalonamento de tarefas individuais, é iniciado automaticamente.
}

void loop()
{
  /// Vazio. Tudo é feito nas tarefas.
}

///*--------------------------------------------------*/
///*---------------------- Tasks ---------------------*/
///*--------------------------------------------------*/

/// Tarefa que lê dados do sensor.
void TaskAnalogRead( void *pvParameters __attribute__((unused)) )
{ 
  for (;;){
    struct pinRead currentPinRead;
    currentPinRead.pin = 0;
    ///  Codificação dos valores lidos em tensão para temperatura.
    /// Fonte: https://portal.vidadesilicio.com.br/lm35-medindo-temperatura-com-arduino/
    currentPinRead.value = (float(analogRead(A0))*5/(1023))/0.01;
    
   /// Posta um item na fila.
   /// https://www.freertos.org/a00117.html
    
    xQueueSend(structQueue, &currentPinRead, portMAX_DELAY);
    vTaskDelay(1);  /// Um tick de atraso (15ms) entre as leituras para estabilidade.
  }
}

///Tarefa que consome dado da fila se disponível;
void TaskTempAtual( void *pvParameters __attribute__((unused)) )
{
  for (;;){
    struct pinRead currentPinRead;

     /// Read an item from a queue.
     /// https://www.freertos.org/a00118.html
    if (xQueueReceive(structQueue, &currentPinRead, portMAX_DELAY) == pdPASS) {
      bufferTemp[k]=currentPinRead.value;  
      if(k<10){ /// Verifica se ainda não foram armazenados 10 dados no buffer da média.
       i = k; /// A variável de controle do buzzer recebe contador do buffer. 
       flag=0; /// Caso não, flag continua em 0.
       if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE ){ 
        /// Se o semáforo estiver disponível, a tarefa consegue o controle da porta serial.
        Serial.print("Temp Atual: "); /// Comunica o valor lido da fila.
        Serial.println(currentPinRead.value);  
        Serial.println(k); /// Posição do buffer no momento.
        xSemaphoreGive( xSerialSemaphore ); /// Libera a porta serial. 
        k=k+1;   /// Incrementa a variável de controle do buffer.
       }
      } else{   /// Caso o contador atinja 10,
        i=0;    /// reseta a variável de controle do buzzer para evitar leitura do buffer.
        flag=1; /// Altera a flag e sinaliza que a média pode ser calculada.
        }   
    }
  }   
}

///Tarefa que consome o buffer para cálculo da média.
void TaskTempMedia( void *pvParameters __attribute__((unused)) )
{
  for (;;){
	/// Variável que guarda a média.
    float media; 
	/// Variável que guarda o acumulado dos dados do buffer.
    float acumulado; 
   
    if(flag==1){ /// Verifica se a flag foi alterada para 1.
      /// Executa laço para cálculo da média dos valores guardados no buffer.
      for (int j=0;j<10;j++){
        acumulado = acumulado + bufferTemp[j]; 
      } 
      media=acumulado/10;
     
      flag=0; /// Reseta a flag para confirmar que os dados do buffer foram consumidos e podem ser substituidos.
      k=0;    /// Reseta variável de controle do buffer.
     
      if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE ){   
        /// Verifica se a porta serial está disponível.
        /// Caso obtenha o controle do semáforo, 
        Serial.print("Media: "); /// comunica o valor da média pela porta serial.
        Serial.println(media);
        media=0; /// Reseta a variável da média.
        acumulado=0;/// Reseta variável do acumulado.
        xSemaphoreGive( xSerialSemaphore ); /// Libera a porta serial.
      }     
    }
    else{flag=0;i=k;} /// Se ainda não foram feitas 10 leituras, a média não será calculada.   
  } 
}   

///Tarefa que controla o buzzer.
void TaskBuzzer( void *pvParameters __attribute__((unused)) )
{  
  /// Fonte:http://www.squids.com.br/arduino/index.php/projetos-arduino/projetos-squids/basico/137-projeto-36-controlando-frequencia-de-um-buzzer-com-potenciometro
  for (;;) 
  {
	///Frequência tocada no buzzer.  
    int frequency;     
	/// Variável para guardar o valor consumido do buffer.
    int atual; 
    if (i>0){
      atual=  bufferTemp[i]; /// Consome dado do buffer.
      if(atual>29){ /// Caso a temperatura atinja valor superior a 29, codifica os valores de temperatura
      frequency = map(atual, 30, 80, 0, 2500); /// entre 30 e 80 para valores de frequencia entre 0 e 2500.
      tone(pinBuzzer, frequency);  /// Buzzer emite som para cada leitura acima de 29.
      } /// Se não for detectada temperatura superior a 29, o som do buzzer apenas será alterado na chamada de noTone().
    }
    else { /// Se não há dados no buffer (k=0), buzzer não consome dados.
      i=0; /// Reseta variável de controle do buzzer.
      noTone(pinBuzzer); /// Silencia o buzzer até a próxima chamada de tone().
    }
  }
}
