#ifndef INCLUDES_CONSOLA_H_
#define INCLUDES_CONSOLA_H_

#define MAX_INSTRUCTIONS 100
#define MAX_PARAMETERS 10
#define MAX_PARAMETER_LENGTH 50

#include <stdio.h>
#include <stdlib.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/collections/list.h"
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include "serializacion.h"
#include "comunicacion.h"

 typedef struct
{
    int instr;
    int valor;
    int valor2;
    char nombre[32];
    char nombre2[32];

}tablaInstr;


typedef struct{
	char *ip_kernel;
	int puerto_kernel;
}Config_consola;

typedef struct
{
    InstructionType instruccion;
    uint32_t numero1;
    uint32_t numero2;
    char string1[20];
    char string2[20];
} Instruction_consola;

//Variables globales
t_log* logger;
t_config* config;

Config_consola config_consola;

int socket_kernel;
Instruction_consola instructions[MAX_INSTRUCTIONS];
int instructionCount = 0;


// Prototipos funciones
void enviarLista(t_buffer*, t_list*);
void destruir_lista(t_list*);

t_list* mapearLista();
t_instruction* cambiarStruct(Instruction_consola);
int get_tamanio_char_array(char [], int);

// UTILS INICIAR MODULO -----------------------------------------------------------------
void levantar_modulo(char*);
void finalizar_modulo();

t_log* iniciar_logger(void);
t_config* iniciar_config(char* );
void levantar_config();
// FIN UTILS INICIAR MODULO -------------------------------------------------------------

// UTILS CONEXIONES ---------------------------------------------------------------------
void establecer_conexiones();
void conectarse_con_kernel();
// FIN UTILS CONEXIONES -----------------------------------------------------------------

// UTILS PARSER -------------------------------------------------------------------------

InstructionType getNextInstruction(FILE *file);

void readIntegerFromFile(FILE *file, int index);
void readNextWordFromFile(FILE *file, int index);
void parseInstructions(FILE *file);
void match(FILE *file, const char *expected);
void error();
int crearLista(char* filename);

void destruir_lista(t_list* lista_instrucciones);
void inicializar_instruccion();
// FIN UTILS PARSER ---------------------------------------------------------------------

#endif /* INCLUDES_CONSOLA_H_ */
