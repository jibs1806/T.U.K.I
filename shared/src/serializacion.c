#include "serializacion.h"

t_paquete* crear_paquete(op_code codigo, t_buffer* buffer)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo;
	paquete->buffer = buffer;
	return paquete;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int tamanio_paquete_serializado = paquete->buffer->size + sizeof(uint8_t) + sizeof(uint32_t);
	void* paquete_serializado = serializar_paquete(paquete, tamanio_paquete_serializado);

	send(socket_cliente, paquete_serializado, tamanio_paquete_serializado, 0);

	free(paquete_serializado);
}

void* serializar_paquete(t_paquete* paquete, int tam_paquete)
{
	// Esta funcion, mete al paquete en un stream para que se pueda enviar
	void* paquete_serializado = malloc(tam_paquete);
	int desplazamiento = 0;

	// Codigo de operacion
	memcpy(paquete_serializado + desplazamiento, &(paquete->codigo_operacion), sizeof(uint8_t));
	desplazamiento+= sizeof(int);

	// Tamanio del stream
	memcpy(paquete_serializado + desplazamiento, &(paquete->buffer->size), sizeof(uint32_t));
	desplazamiento+= sizeof(int);

	// Stream en si
	memcpy(paquete_serializado + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return paquete_serializado;
}

t_buffer* crear_buffer_nuestro(){
	t_buffer* b = malloc(sizeof(t_buffer));
	b->size = 0;
	b->stream = NULL;
	b->offset = 0;
	b->codigo = PAQUETE;
	return b;
}

void destruir_buffer_nuestro(t_buffer* buffer){
	free(buffer->stream);
	free(buffer);
	return;
}

// UINT32
void buffer_write_uint32(t_buffer* buffer, uint32_t entero){
	buffer->stream = realloc(buffer->stream, buffer->size + sizeof(uint32_t));

	memcpy(buffer->stream + buffer->size, &entero, sizeof(uint32_t));
	buffer->size += sizeof(uint32_t);
}

uint32_t buffer_read_uint32(t_buffer *buffer){
	uint32_t entero;

	memcpy(&entero, buffer->stream + buffer->offset, sizeof(uint32_t));
	buffer->offset += sizeof(uint32_t);

	return entero;
}

// UINT8
void buffer_write_uint8(t_buffer* buffer, uint8_t entero){
	buffer->stream = realloc(buffer->stream, buffer->size + sizeof(uint8_t));

	memcpy(buffer->stream + buffer->size, &entero, sizeof(uint8_t));
	buffer->size += sizeof(uint8_t);
}

uint8_t buffer_read_uint8(t_buffer *buffer){
	uint8_t entero;

	memcpy(&entero, buffer->stream + buffer->offset, sizeof(uint8_t));
	buffer->offset += sizeof(uint8_t);

	return entero;
}

//STRING
void buffer_write_string(t_buffer* buffer, char* cadena){
	uint32_t tam = 0;

	while(cadena[tam] != NULL)
		tam++;
	
	buffer_write_uint32(buffer,tam);

	buffer->stream = realloc(buffer->stream, buffer->size + tam);

	memcpy(buffer->stream + buffer->size, cadena , tam);
	buffer->size += tam;
}


char* buffer_read_string(t_buffer *buffer){
	int tam = buffer_read_uint32(buffer);
	char* cadena = malloc(tam + 1);

	memcpy(cadena, buffer->stream + buffer->offset, tam);
	buffer->offset += tam;

	return cadena;
}

char* buffer_read_stringV2(t_buffer* buffer, uint32_t* tam){
	(*tam) = buffer_read_uint32(buffer);
	char* cadena = malloc((*tam) + 1);

	memcpy(cadena, buffer->stream + buffer->offset, (*tam));
	buffer->offset += (*tam);

	*(cadena + (*tam)) = '\0';

	return cadena;
}


//INSTRUCTION
void buffer_write_Instruction(t_buffer* buffer, t_instruction instruccion){
	buffer_write_uint8(buffer, instruccion.instruccion);
	buffer_write_uint32(buffer, instruccion.numero1);
	buffer_write_uint32(buffer, instruccion.numero2);
	buffer_write_string(buffer, instruccion.string1);
	buffer_write_string(buffer, instruccion.string2);
}

// Tiene que ser puntero para poder despues manejarlo en las listas

t_instruction* buffer_read_Instruction(t_buffer* buffer){
	t_instruction* inst = malloc(sizeof(t_instruction));

	inst->instruccion = buffer_read_uint8(buffer);
	
	inst->numero1 = buffer_read_uint32(buffer);
	inst->numero2 = buffer_read_uint32(buffer);
	
	int tam1 = 0;
	int tam2 = 0;

	char* s1 = buffer_read_stringV2(buffer, &tam1);
	char* s2 = buffer_read_stringV2(buffer, &tam2);
	
	inst -> string1 = malloc(tam1);
	for(int i = 0; i < tam1 + 1; i++){
		*(inst -> string1 + i) = *(s1 + i);
	}
	
	inst -> string2 = malloc(tam2);
	for(int i = 0; i < tam2 + 1; i++){
		*(inst -> string2 + i) = *(s2 + i);
	}

	free(s1);
	free(s2);
	
	return inst;
}


// LISTA DE INSTRUCCIONES
void buffer_write_lista_instrucciones(t_buffer* buffer, t_list* lista_de_instrucciones){

	
	buffer -> codigo = LISTA_INSTRUCCIONES;
	// Hay que ver el orden en que escribimos y leemos
	// Primero guardo el tamanio de la lista, que lo necesito para cuando lea la lista
	uint32_t cant_instrucciones = list_size(lista_de_instrucciones);
	buffer_write_uint32(buffer, cant_instrucciones);

	for(int i = 0; i < cant_instrucciones; i++){
		t_instruction* inst = list_get(lista_de_instrucciones,i);
		buffer_write_Instruction(buffer, *inst);
	}
}

t_list* buffer_read_lista_instrucciones(t_buffer* buffer){
	t_list* lista_instrucciones = list_create();

	uint32_t cant_instrucciones = buffer_read_uint32(buffer);

	for(int i = 0; i < cant_instrucciones; i++){
		t_instruction* instr = buffer_read_Instruction(buffer);
		list_add(lista_instrucciones, instr);
	}

	return lista_instrucciones;
}


//REGISTROS

void buffer_write_Registros(t_buffer* buffer, t_registros registros){
	buffer_write_string(buffer, registros.AX);
	buffer_write_string(buffer, registros.BX);
	buffer_write_string(buffer, registros.CX);
	buffer_write_string(buffer, registros.DX);

	buffer_write_string(buffer, registros.EAX);
	buffer_write_string(buffer, registros.EBX);
	buffer_write_string(buffer, registros.ECX);
	buffer_write_string(buffer, registros.EDX);

	buffer_write_string(buffer, registros.RAX);
	buffer_write_string(buffer, registros.RBX);
	buffer_write_string(buffer, registros.RCX);
	buffer_write_string(buffer, registros.RDX);
}

t_registros buffer_read_Registros(t_buffer* buffer){
	t_registros regis;
	int* tam = malloc(sizeof(int));


	strcpy(regis.AX, buffer_read_stringV2(buffer, tam));
	strcpy(regis.BX, buffer_read_stringV2(buffer, tam));
	strcpy(regis.CX, buffer_read_stringV2(buffer, tam));
	strcpy(regis.DX, buffer_read_stringV2(buffer, tam));

	strcpy(regis.EAX, buffer_read_stringV2(buffer, tam));
	strcpy(regis.EBX, buffer_read_stringV2(buffer, tam));
	strcpy(regis.ECX, buffer_read_stringV2(buffer, tam));
	strcpy(regis.EDX, buffer_read_stringV2(buffer, tam));

	strcpy(regis.RAX, buffer_read_stringV2(buffer, tam));
	strcpy(regis.RBX, buffer_read_stringV2(buffer, tam));
	strcpy(regis.RCX, buffer_read_stringV2(buffer, tam));
	strcpy(regis.RDX, buffer_read_stringV2(buffer, tam));

	return regis;
}

// SEGMENTOS
void buffer_write_segmento(t_buffer* buffer, t_segmento segm){
	buffer_write_uint32(buffer, segm.id);
	buffer_write_uint32(buffer, segm.direccion_base);
	buffer_write_uint32(buffer, segm.tamanio_segmento);
}


t_segmento* buffer_read_segmento(t_buffer* buffer){
	t_segmento* seg = malloc(sizeof(t_segmento));

	seg->id = buffer_read_uint32(buffer);
	seg->direccion_base = buffer_read_uint32(buffer);
	seg->tamanio_segmento = buffer_read_uint32(buffer);

	return seg;
}

// TABLA DE SEGMENTOS
void buffer_write_tabla_segmentos(t_buffer* buffer, t_list* tabla_segmentos){

	// Hay que ver el orden en que escribimos y leemos

	// Primero guardo la cantidad de nodos de la lista, que lo necesito para cuando lea la lista
	uint32_t cant_segmentos = list_size(tabla_segmentos);
	buffer_write_uint32(buffer, cant_segmentos);

	for(int i = 0; i < cant_segmentos; i++){
		t_segmento* segm = list_get(tabla_segmentos, i);
		buffer_write_segmento(buffer, *segm);
	}
}

t_list* buffer_read_tabla_segmentos(t_buffer* buffer){
	t_list* tabla_segmentos = list_create();

	uint32_t cant_segmentos = buffer_read_uint32(buffer);

	for(int i = 0; i < cant_segmentos; i++){
		t_segmento* segm = buffer_read_segmento(buffer);
		list_add(tabla_segmentos, segm);
	}

	return tabla_segmentos;
}




// CONTEXTO DE EJECUCION
void buffer_write_cde(t_buffer* buffer, t_cde cde){
	buffer -> codigo = CONTEXTO_DE_EJECUCION;

	buffer_write_uint32(buffer, cde.pid);
	buffer_write_lista_instrucciones(buffer, cde.lista_de_instrucciones);
	buffer_write_uint32(buffer, cde.program_counter);
	buffer_write_Registros(buffer, cde.registros_cpu);
	buffer_write_tabla_segmentos(buffer, cde.tabla_segmentos);
}

t_cde* buffer_read_cde(t_buffer* buffer){
	t_cde* cde;

	cde = malloc(sizeof(t_cde));
	
	cde->pid = buffer_read_uint32(buffer);
	cde->lista_de_instrucciones = buffer_read_lista_instrucciones(buffer);
	cde->program_counter = buffer_read_uint32(buffer);;
	cde->registros_cpu = buffer_read_Registros(buffer);
	cde->tabla_segmentos = buffer_read_tabla_segmentos(buffer);

	return cde;
}

// TABLA DE SEGMENTOS POR PROCESO
void buffer_write_tabla_segmentos_x_procesos(t_buffer* buffer, t_tabla_segmentos_por_proceso* ts_x_p){
	buffer_write_uint32(buffer, ts_x_p->pid);
	buffer_write_tabla_segmentos(buffer, ts_x_p->tabla_segmentos);
}

t_tabla_segmentos_por_proceso* buffer_read_tabla_segmentos_x_procesos(t_buffer* buffer){
	t_tabla_segmentos_por_proceso* ts_x_proc = malloc(sizeof(t_tabla_segmentos_por_proceso));

	ts_x_proc->pid = buffer_read_uint32(buffer);
	ts_x_proc->tabla_segmentos = buffer_read_tabla_segmentos(buffer);

	return ts_x_proc;
}

// UTILS DE LECTURA DE INSTRUCCIONES ----------------------------------------------------
void mostrar_instrucciones(t_log* logger,t_list* lista_instrucciones){
	for(int i = 0; i < list_size(lista_instrucciones); i++){
		t_instruction* instruccion = list_get(lista_instrucciones, i);
		mostrar_instruccion(logger, instruccion);
	}
	return;	
}

void  mostrar_instruccion(t_log* logger, t_instruction* instruccion){
	log_info(logger, "Codigo de instruccion: %d ", instruccion->instruccion);
        
	if (instruccion->numero1 != INT_VACIO)
        log_info(logger, "Numero1: %d ", instruccion->numero1);
    if (instruccion->numero2 != INT_VACIO)
        log_info(logger, "Numero2: %d ", instruccion->numero2);
    if (strcmp(instruccion->string1, CHAR_VACIO) != 0)
        log_info(logger, "String1: %s ", instruccion->string1);
    if (strcmp(instruccion->string2, CHAR_VACIO) != 0)
        log_info(logger, "String2: %s ", instruccion->string2);

}
// FIN UTILS DE LECTURA DE INSTRUCCIONES ------------------------------------------------


// UTILS DESTRUIR -----------------------------------------------------------------------
void destruir_cde(t_cde* cde){
	destruir_lista_instrucciones(cde -> lista_de_instrucciones);
	list_destroy(cde -> tabla_segmentos);
	free(cde);
}

void destruir_pcb(t_pcb* pcb){
	destruir_cde(pcb -> cde);
	//destruir_lista_char(pcb -> recursos_asignados);
	//destruir_lista_char(pcb -> archivos_abiertos);
	list_destroy(pcb->recursos_asignados);
	list_destroy(pcb->archivos_abiertos);
	free(pcb);
}

void destruir_lista_instrucciones(t_list* lista_instrucciones){
	t_instruction* instruccion;
	for(int i = 0; i < list_size(lista_instrucciones); i++){
		instruccion = list_get(lista_instrucciones, i);
		destruir_instruccion(instruccion);
	}
	list_destroy(lista_instrucciones);
}

void destruir_instruccion(t_instruction* instruccion){
	free(instruccion -> string1);
	free(instruccion -> string2);
	free(instruccion);
}

void destruir_lista_char(t_list* lista_char_asterisco){
	for(int i = 0; i < list_size(lista_char_asterisco); i++){
		char* palabra = list_get(lista_char_asterisco, i);
		free(palabra);
	}
	list_destroy(lista_char_asterisco);
}


// FIN UTILS DESTRUIR -------------------------------------------------------------------

// Tabla_segmentos es una lista de t_segmento*
t_segmento* encontrar_segmento_por_id(t_list* tabla_segmentos, uint32_t id_segmento_buscado){
	for(int i = 0; i < list_size(tabla_segmentos); i++){
		
		t_segmento* segm = list_get(tabla_segmentos, i);
		
		if (segm->id == id_segmento_buscado){
			return segm;
		}
	}
}

// Supongo que existe la tabla del proceso a buscar
t_tabla_segmentos_por_proceso* encontrar_tabla_por_pid(t_list* tabla_de_tabla_x_proceso, uint32_t pid_a_buscar){
	for(int i = 0; i < list_size(tabla_de_tabla_x_proceso); i++){
		
		t_tabla_segmentos_por_proceso* tabla = list_get(tabla_de_tabla_x_proceso, i);
		
		if (tabla->pid == pid_a_buscar){
			return tabla;
		}
	}
}