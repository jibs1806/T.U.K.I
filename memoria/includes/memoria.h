#ifndef INCLUDES_MEMORIA_H_
#define INCLUDES_MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commons/log.h"
#include "commons/config.h"
#include "commons/collections/list.h"
#include "serializacion.h"
#include "comunicacion.h"

typedef enum{
	EXITOSO,
	OUT_OF_MEMORY,
	COMPACTACION
} Rta_crear_segmento;

typedef enum {
    FIRST,
    BEST,
    WORST
} Algorithm;

typedef struct{
    uint32_t id_segmento;
    uint32_t size;
	void* start;
} Segment;

/*
typedef struct{
	uint32_t pid;
	t_list* tabla_segmentos; // tabla de t_segmento*
}t_tabla_segmentos_por_proceso;
*/

typedef struct{
	int puerto_escucha;
	int tam_memoria;
	int tam_segmento_0;
	int cant_segmentos;
	int retardo_memoria;
	int retardo_compactacion;
	int algoritmos_asignacion;
}Config_memoria;

typedef struct{
	int direccion_base;
	int tamanio;
}t_hueco;



// Variables globales
Config_memoria config_memoria;
t_log* logger;
t_config* config;

int server_fd;

int cpu_fd;
int fileSystem_fd;
int kernel_fd;

void *memoria_principal;

t_list* espacios_libres; // No va mas
t_list* tabla_segmentos_global; // lista de t_tabla_segmentos_por_proceso*
t_list* huecos_libres; // lista de t_hueco*
t_list* segmentos_en_memoria; // lista de todos los t_segmento* generados

t_segmento* segmento_0;
int *needed_memory;
Segment *seg_anterior;
int *seg_maxsize;
void* mem_auxiliar;

// Prototipos funciones
// UTILS INICIAR MODULO -----------------------------------------------------------------
void levantar_modulo(char* config_path);
void finalizar_modulo();
t_log* iniciar_logger(void);
t_config* iniciar_config(char* config_path);
void levantar_config();;
// FIN UTILS INICIAR MODULO -------------------------------------------------------------

void  ResultadoCompactacion (void* ptr);
void  DestroyList (void* ptr);
void  DestroySegment (void* ptr);
void  GetTotalSize (void* ptr);
void  getEachSize (void* ptr);
void  realocarSegmento (void* ptr);
void  realocarLista (void* ptr);
void compactar_memoria();
void  addSeg0 (void* ptr);
void  removeSeg0 (void* ptr);
Algorithm selectAlgorithm(char* input);
int crear_estructuras();
int terminarEstructuras();
bool FirstFit(void* data);
bool AdyacenteIzquierda(void* data);
bool AdyacenteDerecha(void* data);
void printElement(void* ptr);
void printList (void* ptr);
void crearSegmento(int id,int index, int size);
void eliminarSegmento(int id, int index);
void agregarSegmento(Segment *nuevo);
void liberarSegmento(Segment *viejo);
t_tabla_segmentos_por_proceso* crear_proceso(uint32_t pid_a_crear);
void *pedidoLectura(int *direccion, int size);
int pedidoEscritura(int *direccion, int size, void*nuevo_dato);

// UTILS TABLA DE SEGMENTOS -------------------------------------------------------------
void destruir_tabla_segmentos_por_proceso(t_tabla_segmentos_por_proceso*);
// FIN UTILS TABLA DE SEGMENTOS ---------------------------------------------------------

// UTILS CONEXIONES ---------------------------------------------------------------------
void manejar_conexion_con_kernel();
void manejar_conexion_con_cpu();
void manejar_conexion_con_file_system();
void levantar_modulo();
void establecer_conexiones();
// FIN UTILS CONEXIONES -----------------------------------------------------------------

// UTILS ESCRITURA Y LECTURA ------------------------------------------------------------
void* leer_memoria(uint32_t dir_fisica, uint32_t tamanio);
void escribir_memoria(uint32_t dir_fisica, char* valor_a_escribir, uint32_t tamanio);
// FIN UTILS ESCRITURA Y LECTURA --------------------------------------------------------

t_hueco* retirar_hueco_por_base(uint32_t);
void remover_segmento_segun_id(uint32_t id_segmento);
// UTILS INSTRUCCIONES ------------------------------------------------------------------
void atender_delete_segment(uint32_t pid, uint32_t id_segm_a_eliminar);
Rta_crear_segmento atender_create_segment(uint32_t pid, uint32_t id_segmento, uint32_t tamanio_segmento);
// FIN UTILS INTRUCCIONES ---------------------------------------------------------------

// UTILS ORDENAR LISTA SEGMENTOS --------------------------------------------------------
bool por_menor_base_segmento(t_segmento* segm1, t_segmento* segm2);
// FIN UTILS ORDENAR LISTA SEGMENTOS ----------------------------------------------------

// UTILS ORDENAR LISTA HUECOS -----------------------------------------------------------
bool por_menor_base(t_hueco* hl1, t_hueco* hl2);
bool de_menor_a_mayor_tamanio(t_hueco* hueco1, t_hueco* hueco2);
// FIN UTILS ORDENAR LISTA HUECOS -------------------------------------------------------

// UTILS HUECOS LIBRE -------------------------------------------------------------------
void agregar_hueco_libre(uint32_t base, uint32_t tam);
void union_huecos_libres_contiguos(uint32_t posicion);
// FIN DE UTILS HUECOS LIBRES -----------------------------------------------------------

#endif /* INCLUDES_MEMORIA_H_ */
