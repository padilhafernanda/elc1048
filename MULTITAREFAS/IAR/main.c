#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "multitarefas.h"

/*
 * Prototipos das tarefas
 */

void tarefa(void);
//void tarefa_1(void);
//void tarefa_2(void);
//void tarefa_teste(void);
//void tarefa_per(void);
//void produtor(void);
//void consumidor(void);

/*
 * Configuracao dos tamanhos das pilhas
 */
#define TAM_PILHA_1		(TAM_MINIMO_PILHA + 24)
#define TAM_PILHA_2		(TAM_MINIMO_PILHA + 24)
//#define TAM_PILHA_3		(TAM_MINIMO_PILHA + 24)
//#define TAM_PILHA_4		(TAM_MINIMO_PILHA + 24)
#define TAM_PILHA_OCIOSA	(TAM_MINIMO_PILHA + 24)
//#define TAM_BUFFER 10

/*
 * Declaracao das pilhas das tarefas
 */
//uint32_t PILHA_TAREFA_1[TAM_PILHA_1];
//uint32_t PILHA_TAREFA_2[TAM_PILHA_2];
//uint32_t PILHA_TAREFA_3[TAM_PILHA_3];
//uint32_t PILHA_TAREFA_4[TAM_PILHA_4];
//uint32_t pilha_produtor[TAM_PILHA_1];
//uint32_t pilha_consumidor[TAM_PILHA_2];
//semaforo_t cheio;
//semaforo_t vazio;
//uint32_t buffer[TAM_BUFFER];
//uint8_t i=0;
//uint8_t f=0;
//uint32_t PILHA_TAREFA_OCIOSA[TAM_PILHA_OCIOSA];
uint32_t PILHAS_TAREFAS[PRIORIDADE_MAXIMA][TAM_PILHA_1];
uint32_t PILHA_TAREFA_OCIOSA[TAM_PILHA_OCIOSA];
/*
 * Funcao principal de entrada do sistema
 */
int main(void)
{
	
	/* Criacao das tarefas */
	/* Parametros: ponteiro, nome, ponteiro da pilha, tamanho da pilha, prioridade da tarefa */
	
  int i = 1;  
  for(i = 1 ; i <= PRIORIDADE_MAXIMA ; i++){
  CriaTarefa(tarefa, "Tarefa", (stackptr_t)PILHAS_TAREFAS+(i-1)*TAM_PILHA_1, TAM_PILHA_1, i);
    
  }

  
  
	//CriaTarefa(produtor, "Produtor", pilha_produtor, TAM_PILHA_1, 1);
	
	//CriaTarefa(consumidor, "Consumidor", pilha_consumidor, TAM_PILHA_2, 2);
        
       // cheio.contador=0;
       // cheio.tarefaEsperando=NULL;
       // vazio.contador=TAM_BUFFER;
       // vazio.tarefaEsperando=NULL;
	
        //CriaTarefa(tarefa_teste, "Tarefa Teste", PILHA_TAREFA_3, TAM_PILHA_3, 3);
        
        //CriaTarefa(tarefa_per, "Tarefa Periodica", PILHA_TAREFA_4, TAM_PILHA_4, 4);
	/* Cria tarefa ociosa do sistema */
	CriaTarefa(tarefa_ociosa,"Tarefa ociosa", PILHA_TAREFA_OCIOSA, TAM_PILHA_OCIOSA, 0);
	
	/* Configura marca de tempo */
	ConfiguraMarcaTempo();   
	
	/* Inicia sistema multitarefas */
	IniciaMultitarefas();
	
	/* Nunca chega aqui */
	while (1)
	{
	}
}


int contador = 0;

void tarefa(){
  prioridade_t prioridade_tarefa = TCB[tarefa_atual].prioridade;
  if(prioridade_tarefa > 1)
    TarefaSuspende(TCB[tarefa_atual].prioridade);
  
  while(true){
        
    contador++;
    
    if(prioridade_tarefa < PRIORIDADE_MAXIMA){
      TarefaContinua(prioridade_tarefa+1);
    }
    
    if(prioridade_tarefa > 1){
      TarefaSuspende(prioridade_tarefa);
    }
    
  }
}

/* Tarefas de exemplo que usam funcoes para suspender/continuar as tarefas */
//void tarefa_1(void)
//{
//	volatile uint16_t a = 0;
//	for(;;)
//	{
//		a++;
//		TarefaContinua(2);
//              TarefaContinua(3);
//	}
//}

//void tarefa_2(void)
//{
//	volatile uint16_t b = 0;
//	for(;;)
//	{
//		b++;
//		TarefaSuspende(2);	
//	}
//}

//void tarefa_teste(void)
//{
//	volatile uint16_t c = 0;
//	for(;;)
//	{
//		c++;
//		TarefaSuspende(3);	
//	}
//}

//void tarefa_per(void)
//{
//	volatile uint16_t d = 0;
//	for(;;)
//	{
//		d++;
//		TarefaEspera(10);	
//	}
//}

//int produz()
//{
//  return rand()%20;
//}
//void produtor ()
//{
//  while(true)
//  {
//    SemaforoAguarda(&vazio);
//    buffer[f]=produz();
//    f = (f+1)%TAM_BUFFER;
//    SemaforoLibera(&cheio);       
//  }
//} 
//int consome(int valor)
//{
//  return valor;
//}

//void consumidor ()
//{
//  while(true)
//  {
//    SemaforoAguarda(&cheio);
//    consome(buffer[i]);
//    i=(i+1)%TAM_BUFFER;
//    SemaforoLibera(&vazio);
// }
//}
