#ifndef INCLUDES_FILE_SYSTEM_H_
#define INCLUDES_FILE_SYSTEM_H_

#include <stdio.h>
#include <stdlib.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/bitarray.h"
#include "comunicacion.h"
#include <pthread.h>
#include "serializacion.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>


typedef struct {
	uint32_t  block_size;
	uint32_t block_count;
}superBloque;

typedef struct {
	char*  file_name;
	uint32_t file_size;
	uint32_t  direct_pointer;
	uint32_t indirect_pointer;
}FCB;

typedef struct{
	char* ip_memoria;
	int puerto_memoria;
	int puerto_escucha;
	char *path_superbloque;
	char *path_bitmap;
	char *path_bloques;
	char *path_fcb;
	int retardo_acceso_bloque;
}Config_file_system;

// Variables globales
Config_file_system config_file_system;
t_log* logger;
t_config* config;


int kernel_fd;
int socket_memoria;
int server_fd;

int bitarray_fd;
int archivobloques_fd;
char *archivobloques_pointer;
char *bitarray_pointer;
off_t bitarray_size;
off_t archivobloques_size;
t_bitarray * bitarray;
t_list* fcb_list;
char buscado[64];
superBloque superbloque;


// PROTOTIPOS DE FUNCIONES ----------------------------------------------------
// Inicializaciones
void levantar_modulo();
void finalizar_modulo();
t_log* iniciar_logger();
t_config* iniciar_config();
void levantar_config();

// Conexiones
void conectarse_con_memoria();
void establecer_conexiones();

// Subprogramas generales
int eliminarBloques(int eliminar,FCB* seleccionado);
int crearEstructuras();
int cerrarEstructuras();
int crearArchivo(char* nombre);
int abrirArchivo(char* nombre);
bool igualBuscado(void * ptr);
uint32_t  bloqueLibre();
int liberarBloque(uint32_t bit);
int truncarArchivo(char* nombre, int size);
void * leerArchivo(char* nombre, int pos, int size);


// FIN DE PROTOTIPOS DE FUNCIONES ---------------------------------------------

#endif /* INCLUDES_FILE_SYSTEM_H_ */
