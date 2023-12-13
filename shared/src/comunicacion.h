#ifndef COMUNICACION_H_
#define COMUNICACION_H_


#include <sys/socket.h>
#include <netdb.h>
#include <stdint.h> // Para uintX_t
#include <commons/log.h>
#include <unistd.h>
#include "serializacion.h"


// STRUCTS NECESARIOS ------------------------------------------------------



// FIN DE STRUCTS NECESARIOS -----------------------------------------------



// PROTOTIPOS DE FUNCIONES -------------------------------------------------

int iniciar_servidor(t_log*, int);
int crear_conexion(char*, int);

void enviar_buffer(t_buffer* buffer, int socket);
t_buffer* recibir_buffer(int socket);

void enviar_codigo(int socket_receptor, op_code codigo_a_enviar);
op_code recibir_codigo(int socket_emisor);

void enviar_handshake(int socket_receptor);
op_code recibir_handshake(int socket_emisor);
// FIN DE PROTOTIPOS DE FUNCIONES ------------------------------------------

#endif /* COMUNICACION_H_ */
