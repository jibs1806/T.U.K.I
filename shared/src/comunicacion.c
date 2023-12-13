#include "comunicacion.h"


int iniciar_servidor(t_log* logger, int puerto)
{
	int socket_servidor;

	struct addrinfo hints, *servinfo;// *p

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, itoa(puerto), &hints, &servinfo);

	// Creamos el socket de escucha del servidor

	socket_servidor = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);


	// Asociamos el socket a un puerto

	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);


	// Escuchamos las conexiones entrantes

	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);
	if(socket_servidor != -1)
		log_info(logger, "Listo para escuchar a mi cliente");
	else
		log_info(logger,"No se pudo abrir el servidor correctamente.");
	return socket_servidor;
}

int esperar_cliente(int socket_servidor, t_log* logger)
{
	int socket_cliente;
	socket_cliente = accept(socket_servidor, NULL, NULL);
	return socket_cliente;
}

int crear_conexion(char *ip, int puerto)
{
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, string_itoa(puerto), &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = 0;

	socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo

	connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);


	freeaddrinfo(server_info);

	return socket_cliente;
}

// UTILS BUFFER
t_buffer* recibir_buffer(int socket)
{	
	t_buffer* buffer = crear_buffer_nuestro();

	// Recibo el codigo del buffer
	recv(socket, &(buffer -> codigo), sizeof(uint8_t), MSG_WAITALL);

	// Recibo el tamanio del buffer y reservo espacio en memoria
	recv(socket, &(buffer -> size), sizeof(uint32_t), MSG_WAITALL);
	
	if(buffer->size != 0){
		buffer -> stream = malloc(buffer -> size);

		// Recibo stream del buffer
		recv(socket, buffer -> stream, buffer -> size, MSG_WAITALL);
	}
	return buffer;
}

void enviar_buffer(t_buffer* buffer, int socket){
	// Enviamos el codigo de operacion
    send(socket, &(buffer->codigo), sizeof(uint8_t), 0);

    // Enviamos el tamanio del buffer
    send(socket, &(buffer->size), sizeof(uint32_t), 0);
	
	if (buffer->size != 0){
    	// Enviamos el stream del buffer
    	send(socket, buffer->stream, buffer->size, 0);
	}
}

// UTILS CODIGO
void enviar_codigo(int socket_receptor, op_code codigo){
	send(socket_receptor, &codigo, sizeof(op_code), 0);
}

op_code recibir_codigo(int socket_emisor){
	op_code codigo;

	recv(socket_emisor, &codigo, sizeof(op_code), MSG_WAITALL);

	return codigo;
}

void enviar_handshake(int socket_receptor){
	op_code primer_contacto = HANDSHAKE;
	enviar_codigo(socket_receptor, primer_contacto);
}

op_code recibir_handshake(int socket_emisor){
	return recibir_codigo(socket_emisor);
}
