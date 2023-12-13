#ifndef SERIALIZACION_H_
#define SERIALIZACION_H_

#define INT_VACIO 123456789
#define CHAR_VACIO "v"

#include <stdint.h> // Para uintX_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netdb.h>

#include <time.h>

#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/memory.h>
#include <pthread.h>
#include <semaphore.h>


// STRUCTS NECESARIOS ------------------------------------------------------


typedef enum
{   // GENERALES
	MENSAJE,
	PAQUETE,
    HANDSHAKE,
   
    FIN_PROCESO_CONSOLA, //aviso a consola para finalizacion del procesos
    
	// KERNEL - CPU
    LISTA_INSTRUCCIONES,
    CONTEXTO_DE_EJECUCION,
	
    // FILE SYSTEM - MEMORIA



    // FILE SYSTEM - KERNEL
    

    // MEMORIA - CPU
    SEGMENTATION_FAULT,
    SOLICITUD_MOVE_IN,
    RTA_MOVE_IN,
    SOLICITUD_MOVE_OUT,
    RTA_MOVE_OUT,

    // MEMORIA - KERNEL
	SOLICITUD_INICIO_PROCESO_MEMORIA, // aviso a memoria para que inicialice las estructuras del proceso
    INICIO_EXITOSO_PROCESO_MEMORIA,
    INICIO_FALLIDO_PROCESO_MEMORIA,
    SOLICITUD_CREATE_SEGMENT,
    CREATE_SEGMENT_EXITOSO,
    CREATE_SEGMENT_FALLIDO,
    CREATE_SEGMENT_COMPACTACION,
	COMPACTACION_OK, // Mensaje que envia kernel para habilitar la compactacion de memoria
    SOLICITUD_DELETE_SEGMENT,
    DELETE_SEGMENT_EXITOSO,
    DELETE_SEGMENT_FALLIDO,
	SOLICITUD_FIN_PROCESO_MEMORIA,
    FIN_EXITOSO_PROCESO_MEMORIA,
    FIN_FALLIDO_PROCESO_MEMORIA,

    // KERNEL - FS
    SOLICITUD_ABRIR_ARCHIVO,
	SOLICITUD_CREAR_ARCHIVO,
	SOLICITUD_TRUNCAR_ARCHIVO,
	SOLICITUD_LEER_ARCHIVO,
	SOLICITUD_ESCRIBIR_ARCHIVO,
	F_OPEN_OK,
	F_OPEN_FAIL,
	F_CREATE_OK,
	F_CREATE_FAIL,
	F_TRUNCATE_OK,
	F_TRUNCATE_FAIL,
	F_READ_OK,
	F_READ_FAIL,
	F_WRITE_OK,
	F_WRITE_FAIL,

    // FS - MEMORIA	
    F_READ_MEMORIA,
    F_WRITE_MEMORIA,
    RTA_F_WRITE_MEMORIA
}op_code;



typedef struct
{
	uint32_t size;
	uint32_t offset;
	uint8_t codigo; // Para decir que guarda el buffer (nose si en vez de uint8_t tendria que ser op_code)
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

typedef enum
{
    F_READ,
    F_WRITE,
    SET,
    MOV_IN,
    MOV_OUT,
    F_TRUNCATE,
    F_SEEK,
    CREATE_SEGMENT,
    IO,
    WAIT,
    SIGNAL,
    F_OPEN,
    F_CLOSE,
    DELETE_SEGMENT,
    YIELD,
    EXIT,
    INVALID
} InstructionType;

typedef struct
{
    InstructionType instruccion;
    uint32_t numero1;
    uint32_t numero2;
    char* string1;
    char* string2;
} t_instruction;

typedef struct{
	char AX[5], BX[5], CX[5], DX[5];
	char EAX[9], EBX[9], ECX[9], EDX[9];
	char RAX[17], RBX[17], RCX[17], RDX[17];
}t_registros;

typedef struct{
	uint32_t id;
	uint32_t direccion_base;
	uint32_t tamanio_segmento;
}t_segmento;


typedef struct{
    uint32_t pid;
    t_list* lista_de_instrucciones;
    uint32_t program_counter;
	t_registros registros_cpu;
	t_list* tabla_segmentos; // tabla de t_segmento
}t_cde;

typedef struct{
    t_cde* cde;
    int socket_consola;

    uint32_t estimado_prox_rafaga;
    time_t tiempo_llegada_ready;
    time_t tiempo_llegada_exec;
    time_t tiempo_salida_exec;
    
    t_list* archivos_abiertos;
    t_list* recursos_asignados; // lista de char*
}t_pcb;

typedef struct{
	uint32_t pid;
	t_list* tabla_segmentos; // tabla de t_segmento*
}t_tabla_segmentos_por_proceso;

// FIN DE STRUCTS NECESARIOS ------------------------------------------------------------



// PROTOTIPOS DE FUNCIONES --------------------------------------------------------------

t_paquete* crear_paquete(op_code, t_buffer*);
void enviar_paquete(t_paquete*, int);
void* serializar_paquete(t_paquete*, int);

t_buffer* crear_buffer_nuestro();
void destruir_buffer_nuestro(t_buffer* buffer);

void buffer_write_uint32(t_buffer*, uint32_t);
uint32_t buffer_read_uint32(t_buffer*);

void buffer_write_string(t_buffer*, char*);
char* buffer_read_string(t_buffer*);
char* buffer_read_stringV2(t_buffer*, uint32_t* tam);

void buffer_write_Instruction(t_buffer*, t_instruction);
t_instruction* buffer_read_Instruction(t_buffer*);

void buffer_write_uint8(t_buffer*, uint8_t);
uint8_t buffer_read_uint8(t_buffer*);

void buffer_write_lista_instrucciones(t_buffer*, t_list*);
t_list* buffer_read_lista_instrucciones(t_buffer*);

void buffer_write_Registros(t_buffer*, t_registros);
t_registros buffer_read_Registros(t_buffer*);

void buffer_write_segmento(t_buffer*,t_segmento);
t_segmento* buffer_read_segmento(t_buffer*);

void buffer_write_tabla_segmentos(t_buffer*, t_list*);
t_list* buffer_read_tabla_segmentos(t_buffer*);

void buffer_write_cde(t_buffer*, t_cde);
t_cde* buffer_read_cde(t_buffer*);

void buffer_write_tabla_segmentos_x_procesos(t_buffer* buffer, t_tabla_segmentos_por_proceso* ts_x_p);
t_tabla_segmentos_por_proceso* buffer_read_tabla_segmentos_x_procesos(t_buffer* buffer);

void mostrar_instrucciones(t_log*, t_list*);
void mostrar_instruccion(t_log*, t_instruction*);

// Utils destruir
void destruir_cde(t_cde* cde);
void destruir_pcb(t_pcb* pcb);
void destruir_lista_instrucciones(t_list* lista_instrucciones);
void destruir_instruccion(t_instruction* instruccion);
void destruir_lista_char(t_list* lista_char_asterisco);
// FIN DE PROTOTIPOS DE FUNCIONES ------------------------------------------

t_segmento* encontrar_segmento_por_id(t_list* tabla_segmentos, uint32_t id_segmento_buscado);
t_tabla_segmentos_por_proceso* encontrar_tabla_por_pid(t_list* tabla_de_tabla_x_proceso, uint32_t pid_a_buscar);
#endif /* SERIALIZACION_H_ */
