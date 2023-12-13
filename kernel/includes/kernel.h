#ifndef INCLUDES_KERNEL_H_
#define INCLUDES_KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>
#include <commons/memory.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <serializacion.h>
#include <comunicacion.h>

typedef enum{
	RECURSO_ASIGNADO, // WAIT
	RECURSO_LIBERADO, // SIGNAL
	RECURSO_INVALIDO, // SI PASA => MANDO A EXIT
	EN_ESPERA // LO AGREGO A LA COLA_DE_ESPERA DEL RECURSO
}Res_solicitud_recursos;

typedef struct{
	char* nombre_recurso;
	int instancias_disponibles; // inicialmente es la que esta en el congfig
	t_queue* cola_de_espera; // guarda PCB de los procesos que estan esperando ese recurso
}t_recurso;

typedef struct{
	char* nombre_archivo;
	uint32_t puntero;
	t_queue* cola_de_espera;
	pthread_mutex_t mutex_asignado;
}t_archivo_kernel;

typedef struct{
	int tiempo;
	t_pcb* pcb;
}t_io;

typedef struct{
	char* ip_memoria;
	int puerto_memoria;
	char* ip_cpu;
	int puerto_cpu;
	char* ip_file_system;
	int puerto_file_system;
	int puerto_escucha;
	char* algoritmo;
	int est_inicial;
	int alpha_hrrn;
	int grado_max_multi;
	t_list* recursos; // lista de t_recurso*
}Config_kernel;

// Variables globales
Config_kernel config_kernel;
t_log* logger;
t_config* config;
t_list* tabla_segmentos_global;
t_list* archivos_abiertos_global;
t_list* lista_pcb_global;

int cant_operaciones_mem_fs;

t_archivo_kernel* archivo_nulo;

int socket_memoria;
int socket_file_system;
int socket_cpu;

// Socket propio del kernel
int server_fd;

// SEMAFOROS
// Mutex
pthread_mutex_t mutex_new;
pthread_mutex_t mutex_ready;
pthread_mutex_t mutex_blocked;
pthread_mutex_t mutex_exec;
pthread_mutex_t mutex_exit;
pthread_mutex_t mutex_pid_a_asignar;

pthread_mutex_t mutex_pcb_global;

// Contadores
sem_t cont_new;
sem_t cont_ready;
sem_t cont_exec;
sem_t cont_exit;
sem_t cont_grado_max_multiprog;
sem_t cont_procesador_libre;

sem_t puedo_recibir_de_memoria;
sem_t recepcion_exitosa_de_memoria;

sem_t necesito_enviar_cde;

sem_t operaciones_fs_mem;
//Binarios
sem_t bin1_envio_cde;
sem_t bin2_recibir_cde;



int pid = 0;

int ejecutando = 0;

t_queue* procesosNew;
t_queue* procesosReady;
//t_queue* procesosBlocked; // probablemente no la usemos
t_queue* procesosExec;
t_pcb* pcb_en_ejecucion;
t_queue* procesosExit;

t_recurso* recurso_nulo;

// Prototipos funciones

// UTILS INICIAR MODULO -----------------------------------------------------------------
void levantar_modulo(char* config_path);
void finalizar_modulo();

t_log* iniciar_logger(void);
t_config* iniciar_config(char* config_path);
void levantar_config();
// FIN UTILS INICIAR MODULO -------------------------------------------------------------

// UTILS CONEXIONES ---------------------------------------------------------------------
void manejar_conexion_con_memoria();
void manejar_conexion_con_cpu();
void manejar_conexioncon_file_system();

void establecer_conexiones();

void esperar_consolas();
t_list* recibir_paquete(int);

void recibir_cde_de_cpu();
void enviar_cde_a_cpu();

//void recibir_mensaje_de_memoria();
t_list* recibir_tabla_segmentos_inicial(uint32_t pid);
// FIN UTILS CONEXIONES -----------------------------------------------------------------

// PLANIFICADORES -----------------------------------------------------------------------
void planificadores();
void planificadorLargoPlazo();
void planificadorCortoPlazo();
// FIN PLANIFICADORES -------------------------------------------------------------------

void destruir_tabla_segmentos_por_proceso(t_tabla_segmentos_por_proceso* tabla_segmentos_por_proceso_a_destruir);

// UTLIS CREAR --------------------------------------------------------------------------
t_pcb* crear_pcb(t_list* lista_instrucciones, int socket);
t_cde* crear_cde(t_list* lista_instrucciones);
t_registros inicializar_registros();
// FIN UTILS CREAR ----------------------------------------------------------------------

// UTILS FS
void atender_f_write(t_instruction* instruccion_actual);
void bloquear_pcbs_por_f_write(void* args);
t_archivo_kernel* devolver_archivo_x_nombre(char* nombre_a_buscar);
void inicializar_archivo_nulo();
void cerrar_archivo(char* nombre_a_cerrar);
void sacar_archivo_de_lista(t_list* archivos, char* nombre);
void atender_f_truncate(t_instruction* instruccion);
void bloquear_pcbs_por_f_truncate(void* args);
void atender_f_open(char* nombre_a_abrir);
void atender_f_read(t_instruction* instruccion);
void bloquear_pcbs_por_f_read(void* args);
// FIN UTILS FS

// ------------------------------------ TRANSICIONES ------------------------------------
// TRANSICIONES CORTO PLAZO -------------------------------------------------------------
void enviar_de_pseudoblock_a_ready(t_pcb* pcb); // 
void enviar_de_exec_a_psedudoblock(char* razon_de_block); // Para wait
void enviar_de_exec_a_ready(); // Para yield
void enviar_de_ready_a_exec(); // hilo | se libera cuando no hay ningun proceso en ejecucion
void enviar_de_exec_a_exit(char* razon_de_exit); //
// FIN TRANSICIONES CORTO PLAZO ---------------------------------------------------------

// TRANSICIONES LARGO PLAZO -------------------------------------------------------------
void enviar_de_new_a_ready();
void terminar_procesos();
void recepcionar_proceso(void* argumento);
// FIN TRANSICIONES LARGO PLAZO ---------------------------------------------------------
// ---------------------------------- FIN TRANSICIONES ----------------------------------


// UTILS MOVIMIENTO PCB SEGUROS (MUTEX) -------------------------------------------------
t_pcb* retirar_pcb_de(t_queue* cola, pthread_mutex_t* mutex);
void agregar_pcb_a(t_queue* cola, t_pcb* pcb_a_agregar, pthread_mutex_t* mutex);
// FIN UTILS MOVIMIENTO PCB SEGUROS (MUTEX) ---------------------------------------------


// UTILS PARA SACAR DE READY ------------------------------------------------------------
t_pcb* retirar_pcb_de_ready_segun_algoritmo();
t_pcb* elegido_por_FIFO();
t_pcb* elegido_por_HRRN();
// FIN UTILS PARA SACAR DE READY --------------------------------------------------------

// UTILS HRRN ---------------------------------------------------------------------------
uint32_t calcular_estimacion_proxima_rafaga(t_pcb* pcb);
double calcular_response_ratio(t_pcb *pcb);
bool maximo_response_ratio(t_pcb* pcb1, t_pcb* pcb2);
// FIN UTILS HRRN -----------------------------------------------------------------------


// UTILS RECURSOS -----------------------------------------------------------------------
t_recurso* inicializar_recurso(char* nombre_recu, int instancias_tot); // FUNCIONA
void inicializar_recurso_nulo(); // FUNCIONA
int asignar_instancia_recurso(char* nombre_recurso_a_actualizar, t_pcb* pcb); // FUNCIONA
int liberar_instancia_recurso(char* nombre_recurso_a_liberar, t_pcb* pcb);
void sacar_recurso(t_list* recursos_asignados, char* recurso_a_sacar); // FUNCIONA
void liberar_todos_recursos(t_pcb* pcb);
t_recurso* encontrar_recurso_por_nombre(char* nombre_recurso_a_obtener); // FUNCIONA
// FIN UTILS RECURSOS -------------------------------------------------------------------


// UTILS INSTRUCCIONES ------------------------------------------------------------------
void evaluar_instruccion();
void enviar_solicitud_create_segment(t_instruction* instruccion);
void analizar_respuesta_create_segment();
void enviar_solicitud_delete_segment(t_instruction* instruccion);
void analizar_respuesta_delete_segment(uint32_t id_segm_eliminado);
void administrar_io(t_pcb* pcb_a_dormir, int tiempo_siesta);
void dormir_proceso(void* args);
// FIN UTILS INSTRUCCIONES --------------------------------------------------------------

t_archivo_kernel* devolver_archivo_x_nombre(char* nombre_a_buscar);
t_archivo_kernel* inicializar_archivo(char* nombre);

t_pcb* encontrar_pcb_x_pid(uint32_t pid_a_buscar);

#endif /* INCLUDES_KERNEL_H_ */
