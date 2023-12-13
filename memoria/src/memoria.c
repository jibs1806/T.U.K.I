#include "../includes/memoria.h"

int main(int argc, char **argv){

    char *config_path = argv[1];

	levantar_modulo(config_path);

	manejar_conexiones();
	
	while(1);
	return 0;
}


// UTILS INSTRUCCIONES (SEGMENTOS) ------------------------------------------------------
//crea memoria auxiliar y realoca segmentos ahi
void compactar_memoria(){
	int bytes_recorridos = 0;
	log_info(logger, "Cant sengmetnos en memoria: %d", list_size(segmentos_en_memoria));
	for(int i = 0; i < list_size(segmentos_en_memoria); i++){
		t_segmento* segmento_actual = list_get(segmentos_en_memoria, i);
		log_info(logger, "ID Leido. %d", segmento_actual->id);

		// Muevo lo que estaba escrito en espacio usuario junto con el segmento
		memcpy(memoria_principal + bytes_recorridos, memoria_principal + segmento_actual->direccion_base, segmento_actual->tamanio_segmento);

		bytes_recorridos += segmento_actual->tamanio_segmento;
	}
	agregar_hueco_libre(bytes_recorridos, config_memoria.tam_memoria - bytes_recorridos);
}

void remover_segmento_segun_id(uint32_t id_segmento){
	for(int i = 0; i < list_size(segmentos_en_memoria); i++){
		t_segmento* segm = list_get(segmentos_en_memoria, i);
		if(segm->id == id_segmento){
			list_remove(segmentos_en_memoria, i);
			log_info(logger, "Segmento retirado: %d, base: %d, tam: %d",segm->id, segm->direccion_base, segm->tamanio_segmento);
			return;
		}
	}
}

void  ResultadoCompactacion (void* ptr) {
	t_list* tabla = (t_list*) ptr;
	Segment* segmento;
	for(int i=1;i<config_memoria.cant_segmentos;i++){
		segmento = list_get(tabla,i);
		if(segmento->size!=0) {
			log_info(logger,"PID: %d - Segmento: %d - Base: %p - Tamanio %d",*needed_memory,i,segmento->start,segmento->size);
		}
		(*needed_memory)++;
	}
}



void  DestroySegment (void* ptr) {
	Segment* seleccionado = (Segment*) ptr;
	free(seleccionado);
}


// Devuelve el total de segmentos asignados
void  GetTotalSize (void* ptr) {
	t_list* seleccionado = (t_list*) ptr;
	list_iterate(seleccionado,getEachSize);
}
void  getEachSize (void* ptr) {
	Segment* seleccionado = (Segment*) ptr;
	*needed_memory+=seleccionado->size;
}


void  realocarLista (void* ptr) {
	t_list* seleccionado = (t_list*) ptr;
	list_iterate(seleccionado,realocarSegmento);
}

// Coloca cada segmento al final del anterior
void  realocarSegmento (void* ptr) {
	Segment* seleccionado = (Segment*) ptr;
	void* fin_adjacent= seg_anterior->start +seg_anterior->size;
	if(seleccionado->size!=0){
	void * nueva_pos =(void *)((intptr_t) mem_auxiliar+ (intptr_t) fin_adjacent);
	memcpy(nueva_pos,seleccionado->start,seleccionado->size);
	seleccionado->start=fin_adjacent;
	seg_anterior->start = seleccionado->start;
	seg_anterior->size = seleccionado->size;
	}
}

//agregar y remover seg0 de tablas de segmentos
void  addSeg0 (void* ptr) {
	t_list* seleccionado = (t_list*) ptr;
	list_add_in_index(seleccionado,0,segmento_0);
}
void  removeSeg0 (void* ptr) {
	t_list* seleccionado = (t_list*) ptr;
	list_remove_element(seleccionado, segmento_0);
}



// crea un nuevo proceso y le asigna segmento_0
t_tabla_segmentos_por_proceso* crear_proceso(uint32_t pid_a_crear){
	t_tabla_segmentos_por_proceso* tabla_segmentos_proceso_nuevo = malloc(sizeof(t_tabla_segmentos_por_proceso));
	tabla_segmentos_proceso_nuevo->tabla_segmentos = list_create();
	tabla_segmentos_proceso_nuevo->pid = pid_a_crear;

	list_add(tabla_segmentos_proceso_nuevo->tabla_segmentos, segmento_0);
	log_info(logger,"Agregue el segmento_0 a la tabla de segmentos del proceso");

	list_add(tabla_segmentos_global, tabla_segmentos_proceso_nuevo);
	log_info(logger,"Agregado a tabla_segmentos_global", pid_a_crear);
	
	log_info(logger,"Creacion Proceso PID: %d", pid_a_crear);
	return tabla_segmentos_proceso_nuevo;
}




void eliminarSegmento(int id, int index){
	t_list* proceso=list_get(tabla_segmentos_global,id);
	Segment* seg_viejo;
	seg_viejo =(Segment*)list_get(proceso,index);
	liberarSegmento(seg_viejo);
	log_info(logger,"PID: %d - EliminarSeg: %d - Base: %p - TAMANIO: %d",id,index,seg_viejo->start,seg_viejo->size);
	seg_viejo->start = NULL;
	seg_viejo->size =0;
	list_replace(proceso,index,seg_viejo);
}

//algoritmos para comparar
bool Equivalente(void* data){
		Segment* element = (Segment*)data;
		return (element->start == seg_anterior->start);

}

bool FirstFit(void* data){
		Segment* element = (Segment*)data;
		int tamanio = element->size;
		return (tamanio >= *needed_memory);

}

void* BestFit(void* a, void* b) {
    Segment* segA = (Segment*)a;
    Segment* SegB = (Segment*)b;
    return (segA->size > SegB->size) ? a : b;
}

void* WorstFit(void* a, void* b) {
    Segment* segA = (Segment*)a;
    Segment* SegB = (Segment*)b;
    return (segA->size > SegB->size) ? b : a;
}

// printea listas
void printList (void* ptr) {
	t_list* seleccionado = (t_list*) ptr;
	list_iterate(seleccionado,printElement);

}
void printElement(void* ptr) {
	Segment* seleccionado = (Segment*) ptr;
    printf("start: %p\n", seleccionado->start);
    printf("size: %d\n", seleccionado->size);
}

/*
int pedidoSegmento(int size){
	*needed_memory = 0;
	seleccionado = (Segment*) list_find(espacios_libres,FirstFit);
	if(seleccionado){
		agregarSegmento()
	}
	list_iterate(espacios_libres,getEachSize);
	if(size>)
}
*/
// agrega  segmento segun algoritmo elegido
/*
void agregarSegmento(Segment *nuevo){
	*needed_memory = nuevo->size;
	Segment *seleccionado;
	switch(config_memoria.algoritmos_asignacion){
		case FIRST:
		seleccionado = (Segment*)list_remove_by_condition(espacios_libres,FirstFit);
		break;
		case BEST:
		seleccionado = (Segment*)list_get_maximum(espacios_libres,BestFit);
		seg_anterior->start=seleccionado->start;
		seleccionado = (Segment*)list_remove_by_condition(espacios_libres,Equivalente);
		break;
		case WORST:
		seleccionado = (Segment*)list_get_minimum(espacios_libres,WorstFit);
		seg_anterior->start=seleccionado->start;
		seleccionado= (Segment*)list_remove_by_condition(espacios_libres,Equivalente);
		break;
	}
	nuevo->start = seleccionado->start;
	seleccionado->start+=nuevo->size;
	seleccionado->size-=nuevo->size;
	list_add(espacios_libres,seleccionado);
}*/

bool AdyacenteIzquierda(void* data){
		Segment* element = (Segment*)data;
		void* final_seg = element->start+element->size;
		return (seg_anterior->start == final_seg );

}

bool AdyacenteDerecha(void* data){
		Segment* element = (Segment*)data;
		void* final_seg = seg_anterior->start+seg_anterior->size;
		return (element->start == final_seg );

}

// libera  segmento concatenando huecos adyacentes
void liberarSegmento(Segment *viejo){
	seg_anterior->start = viejo->start;
	seg_anterior->size = viejo->size;
	Segment *seg_derecha;
	Segment *seg_izquierda;
	seg_derecha=(Segment*)list_remove_by_condition(espacios_libres,AdyacenteDerecha);
	seg_izquierda=(Segment*)list_remove_by_condition(espacios_libres,AdyacenteIzquierda);
	if(seg_derecha!=NULL && seg_izquierda!=NULL){
		seg_izquierda->size+=seg_derecha->size +viejo->size;
		list_add(espacios_libres,seg_izquierda);
	}else
		if(seg_derecha!=NULL){
		seg_derecha->start-=viejo->size;
		seg_derecha->size+=viejo->size;
		list_add(espacios_libres,seg_derecha);
	}else
		if(seg_izquierda!=NULL){
		seg_izquierda->size+=viejo->size;
		list_add(espacios_libres,seg_izquierda);
	}
	else {
		seg_derecha = malloc(sizeof(Segment));
		seg_derecha->start =viejo->start;
		seg_derecha->size =viejo->size;
		list_add(espacios_libres,seg_derecha);
	}
}
// FIN UTILS INSTRUCCIONES (SEGMENTOS) --------------------------------------------------

// UTILS ESTRUCTURAS --------------------------------------------------------------------
// crea segmento_0, memoria principal y agrega segmento_0
int crear_estructuras(){
	memoria_principal = malloc(config_memoria.tam_memoria);
	
	tabla_segmentos_global = list_create();
	huecos_libres = list_create();
	segmentos_en_memoria = list_create();

	segmento_0 = malloc(sizeof(t_segmento));
	segmento_0->id = 0;
	segmento_0->direccion_base = 0;
 	segmento_0->tamanio_segmento = config_memoria.tam_segmento_0;
	
	list_add(segmentos_en_memoria, segmento_0);

	agregar_hueco_libre(segmento_0->tamanio_segmento, config_memoria.tam_memoria - segmento_0->tamanio_segmento);
	
	log_info(logger,"Estructuras creadas");
	return 0;
}
int terminarEstructuras(){
	free(memoria_principal);
	free(segmento_0);
	free(needed_memory);
	free(seg_anterior);
	free(seg_maxsize);
	log_info(logger,"Estructuras terminadas");
	return 0;
}
// FIN UTILS ESTRUCTURAS ----------------------------------------------------------------

// UTILS INICIAR MODULO -----------------------------------------------------------------
void levantar_modulo(char* config_path){
	logger = iniciar_logger();
	config = iniciar_config(config_path);
	levantar_config();
	crear_estructuras();
	establecer_conexiones();
}
void finalizar_modulo(){
	log_destroy(logger);
	config_destroy(config);
	return;
}

t_log* iniciar_logger(void)
{
	t_log* nuevo_logger;
	nuevo_logger = log_create("memoria.log", "Memoria", 1, LOG_LEVEL_INFO);

	if (nuevo_logger == NULL)
	{
		printf("Error al crear el logger\n");
		exit(1);
	}

	return nuevo_logger;
}

t_config* iniciar_config(char* config_path)
{
	t_config* nuevo_config;

	nuevo_config = config_create(config_path);

	if (nuevo_config == NULL){
		printf("Error al crear el nuevo config\n");
		exit(2);
	}

	return nuevo_config;
}

void levantar_config(){
	config_memoria.puerto_escucha = config_get_int_value(config,"PUERTO_ESCUCHA");
	config_memoria.tam_memoria = config_get_int_value(config,"TAM_MEMORIA");
	config_memoria.tam_segmento_0 = config_get_int_value(config,"TAM_SEGMENTO_0");
	config_memoria.cant_segmentos = config_get_int_value(config,"CANT_SEGMENTOS");
	config_memoria.retardo_memoria = config_get_int_value(config,"RETARDO_MEMORIA");
	config_memoria.retardo_compactacion = config_get_int_value(config,"RETARDO_COMPACTACION");
	config_memoria.algoritmos_asignacion = 	selectAlgorithm(config_get_string_value(config,"ALGORITMO_ASIGNACION"));
}
// FIN UTILS INICIAR MODULO -------------------------------------------------------------


Algorithm selectAlgorithm(char* input) {
    if (strcmp(input, "FIRST") == 0) {
        return FIRST;
    } else if (strcmp(input, "BEST") == 0) {
        return BEST;
    } else if (strcmp(input, "WORST") == 0) {
        return WORST;
    } else {
        return -1;
    }
}

// UTILS CONEXIONES ---------------------------------------------------------------------
void establecer_conexiones()
{
	server_fd = iniciar_servidor(logger, config_memoria.puerto_escucha);
	
	
	cpu_fd = esperar_cliente(server_fd, logger);
	if (cpu_fd >= 0){
		log_info(logger, "Conectado con CPU");
		if (recibir_handshake(cpu_fd) == HANDSHAKE){
			enviar_handshake(cpu_fd);
			log_info(logger, "Handshake con CPU realizado.");			
		}
		else
			log_info(logger, "Ocurio un error al realizar el handshake con CPU.");
	}else
		log_info(logger, "Error al conectar con CPU.");
	

	fileSystem_fd = esperar_cliente(server_fd, logger);
	if (fileSystem_fd >= 0){
		log_info(logger, "Conectado con FILE SYSTEM");
		if (recibir_handshake(fileSystem_fd) == HANDSHAKE){
			enviar_handshake(fileSystem_fd);
			log_info(logger, "Handshake con FYLE SYSTEM realizado.");			
		}
		else
			log_info(logger, "Ocurio un error al realizar el handshake con FYLE SYSTEM.");
	}else
		log_info(logger, "Error al conectar con FYLE SYSTEM.");
	
	
	kernel_fd = esperar_cliente(server_fd, logger);
	if (kernel_fd >= 0){
		log_info(logger, "Conectado con KERNEL");
		if (recibir_handshake(kernel_fd) == HANDSHAKE){
			enviar_handshake(kernel_fd);
			log_info(logger, "Handshake con KERNEL realizado.");
		}
		else
			log_info(logger, "Ocurio un error al realizar el handshake con KERNEL.");
	}else
		log_info(logger, "Error al conectar con KERNEL");
	
}

void manejar_conexiones(){
	
	pthread_t comunicacion_cpu;
	pthread_create(&comunicacion_cpu, NULL, (void* ) manejar_conexion_con_cpu, NULL);
	pthread_detach(comunicacion_cpu);

	pthread_t comunicacion_kernel;
	pthread_create(&comunicacion_kernel, NULL, (void* ) manejar_conexion_con_kernel, NULL);
	pthread_detach(comunicacion_kernel);

	pthread_t comunicacion_fs;
	pthread_create(&comunicacion_fs, NULL, (void* ) manejar_conexion_con_file_system, NULL);
	pthread_detach(comunicacion_fs);
	
}

void manejar_conexion_con_cpu(){
	while(1){
		log_info(logger, "Esperando buffer de cpu");
		t_buffer* buffer = recibir_buffer(cpu_fd);
		log_info(logger, "Buffer recibido por CPU - Codigo: %d", buffer->codigo);
		switch(buffer->codigo){
			case SOLICITUD_MOVE_IN:	
				uint32_t pid = buffer_read_uint32(buffer);
				uint32_t dir_fisica = buffer_read_uint32(buffer);
				uint32_t desplazamiento = buffer_read_uint32(buffer);
				
				destruir_buffer_nuestro(buffer);

				log_info(logger, "Por leer memoria");
				char* valor_leido = (char*) leer_memoria(dir_fisica, desplazamiento);
				
				log_info(logger, "Valor leido: %s", valor_leido);

				buffer = crear_buffer_nuestro();
				buffer_write_string(buffer, valor_leido);
				enviar_buffer(buffer, cpu_fd);
				destruir_buffer_nuestro(buffer);

				log_info(logger, "PID: %d - Accion: LEER - Direccion fisica: %d - Tamanio: %d - Origen: CPU", pid, dir_fisica, desplazamiento);

				break;
			case SOLICITUD_MOVE_OUT:
				pid = buffer_read_uint32(buffer);
				dir_fisica = buffer_read_uint32(buffer);
				int tam = 0;
				char* valor_a_escribir = buffer_read_stringV2(buffer, &tam);
				destruir_buffer_nuestro(buffer);
				escribir_memoria(dir_fisica, valor_a_escribir, tam);
				log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Tamano: %d - Origen: CPU", pid, dir_fisica, tam);

				enviar_codigo(cpu_fd, RTA_MOVE_OUT);
				break;

		}
	}
}

void manejar_conexion_con_kernel(){
	while(1){
		log_info(logger, "Esperando buffer de kernel");
		t_buffer* buffer_recibido = recibir_buffer(kernel_fd);
		log_info(logger, "Codigo recibido por kernel: %d", buffer_recibido->codigo);
		switch(buffer_recibido->codigo){
			case SOLICITUD_INICIO_PROCESO_MEMORIA:
				uint32_t pid_a_crear = buffer_read_uint32(buffer_recibido);
				destruir_buffer_nuestro(buffer_recibido);
				log_info(logger, "PID recibido: %d", pid_a_crear);

				t_tabla_segmentos_por_proceso* tabla_segmentos_inicial_nueva = crear_proceso(pid_a_crear);

				t_buffer* buffer_a_enviar = crear_buffer_nuestro();
				buffer_write_tabla_segmentos(buffer_a_enviar, tabla_segmentos_inicial_nueva->tabla_segmentos);
				buffer_a_enviar->codigo = INICIO_EXITOSO_PROCESO_MEMORIA;
				
				enviar_buffer(buffer_a_enviar, kernel_fd);
				log_info(logger, "Buffer enviado");
				destruir_buffer_nuestro(buffer_a_enviar);

				break;
			case SOLICITUD_FIN_PROCESO_MEMORIA:

				uint32_t pid_a_destruir = buffer_read_uint32(buffer_recibido);
				destruir_buffer_nuestro(buffer_recibido);
				t_tabla_segmentos_por_proceso* tabla_a_destruir = encontrar_tabla_por_pid(tabla_segmentos_global, pid_a_destruir);

				log_info(logger, "Elminacion de proceso PID: %d", tabla_a_destruir->pid);
				destruir_tabla_segmentos_por_proceso(tabla_a_destruir);

				op_code fin_proceso = FIN_EXITOSO_PROCESO_MEMORIA;
				enviar_codigo(kernel_fd, fin_proceso);
				break;
			case SOLICITUD_CREATE_SEGMENT:
				uint32_t pid_a_aniadir_segmn = buffer_read_uint32(buffer_recibido);
				uint32_t id_segmento = buffer_read_uint32(buffer_recibido);
				uint32_t tamanio_segmento = buffer_read_uint32(buffer_recibido);
				destruir_buffer_nuestro(buffer_recibido);
				t_tabla_segmentos_por_proceso* tabla_segmentos_proceso = encontrar_tabla_por_pid(tabla_segmentos_global, pid_a_aniadir_segmn);
				
				switch(atender_create_segment(pid_a_aniadir_segmn, id_segmento, tamanio_segmento)){
					case EXITOSO:
						// Le envio a kernel q fue exitoso y el segmento creado
						buffer_a_enviar = crear_buffer_nuestro();
						buffer_a_enviar->codigo = CREATE_SEGMENT_EXITOSO;
						
						log_info(logger, "Por encontrar segmento con id: %d", id_segmento);
						t_segmento* segm_a_enviar = encontrar_segmento_por_id(tabla_segmentos_proceso->tabla_segmentos, id_segmento);
						log_info(logger, "Encontre segmento con id: %d", segm_a_enviar->id);

						buffer_write_segmento(buffer_a_enviar, (*segm_a_enviar));
						enviar_buffer(buffer_a_enviar, kernel_fd);

						destruir_buffer_nuestro(buffer_a_enviar);

						break;
					case OUT_OF_MEMORY:
						buffer_a_enviar = crear_buffer_nuestro();
						buffer_a_enviar->codigo = CREATE_SEGMENT_FALLIDO;
						enviar_buffer(buffer_a_enviar, kernel_fd);

						destruir_buffer_nuestro(buffer_a_enviar);
						break;
					case COMPACTACION:

						log_info(logger, "Solicitud de compactacion"); // OBLIGATORIO
						buffer_a_enviar = crear_buffer_nuestro();
						buffer_a_enviar->codigo = CREATE_SEGMENT_COMPACTACION;
						// faltan cosaaas
						enviar_buffer(buffer_a_enviar, kernel_fd);
						destruir_buffer_nuestro(buffer_a_enviar);

						if(recibir_codigo(kernel_fd) == COMPACTACION_OK){
							compactar_memoria();
							crear_segmento(tabla_segmentos_proceso, id_segmento, tamanio_segmento);
						}

						buffer_a_enviar = crear_buffer_nuestro();
						uint32_t cant_listas = list_size(tabla_segmentos_global);

						buffer_write_uint32(buffer_a_enviar, cant_listas);

						for(int i = 0; i < list_size(tabla_segmentos_global); i++){
							t_tabla_segmentos_por_proceso* t_s_x_proceso = list_get(tabla_segmentos_global, i);
							buffer_write_tabla_segmentos_x_procesos(buffer_a_enviar, t_s_x_proceso);
						}
						enviar_buffer(buffer_a_enviar, kernel_fd);

						break;
				}
				break;
			case SOLICITUD_DELETE_SEGMENT:
				uint32_t pid_a_deletear_segm = buffer_read_uint32(buffer_recibido);
				uint32_t id_segmento_a_eliminar = buffer_read_uint32(buffer_recibido);
				
				atender_delete_segment(pid_a_deletear_segm, id_segmento_a_eliminar);
				
				buffer_a_enviar = crear_buffer_nuestro(buffer_a_enviar);
				buffer_a_enviar->codigo = DELETE_SEGMENT_EXITOSO;

				t_tabla_segmentos_por_proceso* ts_x_proceso = encontrar_tabla_por_pid(tabla_segmentos_global, pid_a_deletear_segm);

				buffer_write_tabla_segmentos(buffer_a_enviar, ts_x_proceso->tabla_segmentos);

				enviar_buffer(buffer_a_enviar, kernel_fd);
				destruir_buffer_nuestro(buffer_a_enviar);
				break;
			default:
				log_info(logger, "Default");
				break;
			}
			
	}		
}

void manejar_conexion_con_file_system(){
	while(1){
		log_info(logger, "Esperando buffer de fyle system");
		t_buffer* buffer_recibido = recibir_buffer(fileSystem_fd);
		log_info(logger, "Codigo recibido por file system: %d", buffer_recibido->codigo);
		switch(buffer_recibido->codigo){
			case F_READ_MEMORIA :
				uint32_t pid = buffer_read_uint32(buffer_recibido);
				uint32_t dir_fisica = buffer_read_uint32(buffer_recibido);
				int tam = 0;
				char* string_a_escribir = buffer_read_stringV2(buffer_recibido, &tam);
				destruir_buffer_nuestro(buffer_recibido);
				escribir_memoria(dir_fisica, string_a_escribir, tam);
				
				log_info(logger, "PID: %d - Accion: ESCRIBIR - Direccion fisica: %d - Tamano: %d - Origen: FS", pid, dir_fisica, tam);
				
				buffer_recibido = crear_buffer_nuestro();
				buffer_recibido->codigo = F_READ_OK;
				enviar_buffer(buffer_recibido, fileSystem_fd);
				destruir_buffer_nuestro(buffer_recibido);
			break;
			case F_WRITE_MEMORIA:
				uint32_t pid_write = buffer_read_uint32(buffer_recibido);
				uint32_t tamanio = buffer_read_uint32(buffer_recibido);
				uint32_t dir_fisica_write = buffer_read_uint32(buffer_recibido);
				destruir_buffer_nuestro(buffer_recibido);

				log_info(logger, "Por leer memoria");
				char* valor_leido = (char*) leer_memoria(dir_fisica_write, tamanio);
				
				log_info(logger, "Valor leido: %s", valor_leido);


				log_info(logger, "PID: %d - Accion: LEER - Direccion fisica: %d - Tamanio: %d - Origen: FS", pid_write, dir_fisica_write, tamanio);
				
				t_buffer* buffer = crear_buffer_nuestro();
				buffer->codigo = RTA_F_WRITE_MEMORIA;
				buffer_write_string(buffer, valor_leido);
				enviar_buffer(buffer, fileSystem_fd);
				destruir_buffer_nuestro(buffer);
			break;
		}
	}
}
// FIN UTILS CONEXIONES -----------------------------------------------------------------

// UTILS ESCRITURA Y LECTURA ------------------------------------------------------------
// Puede ser que haya que agregar el '\0' en algun lado al leer char*
void* leer_memoria(uint32_t dir_fisica, uint32_t tamanio){
	char* lectura = malloc(tamanio + 1);

	memcpy(lectura, memoria_principal + dir_fisica, tamanio);

	*(lectura + tamanio) = '\0';
	
	return lectura;
}

void escribir_memoria(uint32_t dir_fisica, char* valor_a_escribir, uint32_t tamanio){
	memcpy(memoria_principal + dir_fisica, valor_a_escribir, tamanio);
}
// FIN UTILS ESCRITURA Y LECTURA --------------------------------------------------------


void destruir_tabla_segmentos_por_proceso(t_tabla_segmentos_por_proceso* tabla_segmentos_por_proceso_a_destruir){
	list_remove_element(tabla_segmentos_global, tabla_segmentos_por_proceso_a_destruir);
	list_destroy(tabla_segmentos_por_proceso_a_destruir->tabla_segmentos);
	free(tabla_segmentos_por_proceso_a_destruir);
	return;
}

// UTILS INSTRUCCIONES ------------------------------------------------------------------
// Supongo que el segmento ya existe - FALTA PROBAR
void atender_delete_segment(uint32_t pid, uint32_t id_segm_a_eliminar){
	t_tabla_segmentos_por_proceso* tabla = encontrar_tabla_por_pid(tabla_segmentos_global, pid);
	t_list* tabla_segm = tabla->tabla_segmentos;
	
	for(int i = 0; i < list_size(tabla_segm); i++){
		t_segmento* segm = list_get(tabla_segm, i);
		if(segm->id == id_segm_a_eliminar){
			log_info(logger, "Segmento encontrado");
			agregar_hueco_libre(segm->direccion_base, segm->tamanio_segmento);
			log_info(logger, "Hueco libre - Base : %d - Tam: %d", segm->direccion_base, segm->tamanio_segmento);
			union_huecos_libres_contiguos(0);
			list_remove_element(segmentos_en_memoria, segm);
			list_remove(tabla_segm, i);
			return;
		}
	}
}

Rta_crear_segmento atender_create_segment(uint32_t pid, uint32_t id_segmento, uint32_t tamanio_segmento){
	t_tabla_segmentos_por_proceso* tabla_segmentos_de_proceso = encontrar_tabla_por_pid(tabla_segmentos_global, pid);
	log_info(logger, "PID encontrado: %d", tabla_segmentos_de_proceso->pid);
	int tam_libre_actual = tamanio_libre();
	int tam_libre_max_contiguo = tamanio_libre_max_contiguo();
	log_info(logger, "Tam libre actual: %d", tam_libre_actual);
	log_info(logger, "Tam max contiguo: %d", tam_libre_max_contiguo);
	if (tam_libre_actual >= tamanio_segmento){
		if (tam_libre_max_contiguo >= tamanio_segmento){
			// Si entra aca, entonces existe un espacio en donde entra el segmento
			log_info(logger, "Por crear segmento");
			crear_segmento(tabla_segmentos_de_proceso, id_segmento, tamanio_segmento);
			return EXITOSO;
		}else{
			log_info(logger, "Entre a compactacion");
			return COMPACTACION;
		}
	}else{
		log_info(logger, "Entre a OUT OF MEMORY");
		return OUT_OF_MEMORY;
	}
	
	
}
// FIN UTILS INTRUCCIONES ---------------------------------------------------------------


t_hueco* retirar_hueco_por_base(uint32_t base_a_retirar){
	for(int i = 0; i < list_size(huecos_libres); i++){
		t_hueco* hueco = list_get(huecos_libres, i);
		if(hueco->direccion_base == base_a_retirar){
			hueco = list_remove(huecos_libres, i);	
			return hueco;
		}
	}
}

// Crea el segmento y actualiza la lista de huecos libres
void crear_segmento(t_tabla_segmentos_por_proceso* t_segm_x_proceso, uint32_t id_segm_a_crear, uint32_t tam_segm_a_crear){
	// switch best, first worst
	t_segmento* segm_nuevo = malloc(sizeof(t_segmento));
	
	segm_nuevo->id = id_segm_a_crear;
	segm_nuevo->tamanio_segmento = tam_segm_a_crear;
	
	// Necesito encontrar la base donde voy a colocar el segmento nuevo y dsps lo agrego a la tabla de segmentos x proceso
	
	switch (config_memoria.algoritmos_asignacion){
	case BEST:
		log_info(logger, "Entre a best");
		segm_nuevo->direccion_base = obtener_base_segun_BEST(segm_nuevo->tamanio_segmento);
		break;
	case FIRST:
		segm_nuevo->direccion_base = obtener_base_segun_FIRST(segm_nuevo->tamanio_segmento);
		break;
	case WORST:
		segm_nuevo->direccion_base = obtener_base_segun_WORST(segm_nuevo->tamanio_segmento);
		break;
	default:
		break;
	}
	
	// Obtengo la base del segmento nuevo -> retiro el hueco que tenga esa posicion
		// -> Creo un hueco cuya base sea (base + desplazamiento) del segmento nuevo
			// -> hago la pseudo compactacion
	t_hueco* hueco_ocupado = retirar_hueco_por_base(segm_nuevo->direccion_base);
	
	 log_info(logger, "PID: %d - Crear Segmento: %d - Base: %d - TAMAÑO: %d", t_segm_x_proceso->pid, id_segm_a_crear, segm_nuevo->direccion_base, segm_nuevo->tamanio_segmento);


	// Falta chequear que no se cree un hueco de tamanio 0
	uint32_t base_hueco_nuevo = segm_nuevo->direccion_base + hueco_ocupado->direccion_base;
	uint32_t tamanio_hueco_nuevo = hueco_ocupado->tamanio - segm_nuevo->tamanio_segmento;
	agregar_hueco_libre(base_hueco_nuevo, tamanio_hueco_nuevo);
	list_add_sorted(t_segm_x_proceso->tabla_segmentos, segm_nuevo, (void*) por_menor_base_segmento);
	list_add_sorted(segmentos_en_memoria, segm_nuevo, (void* )por_menor_base_segmento);
	return;
}


// UTILS ORDENAR LISTA SEGMENTOS --------------------------------------------------------
bool por_menor_base_segmento(t_segmento* segm1, t_segmento* segm2){
	return segm1->direccion_base < segm2->direccion_base;
}
// FIN UTILS ORDENAR LISTA SEGMENTOS ----------------------------------------------------


// FALTA TESTEAR!!!!
int obtener_base_segun_BEST(uint32_t tamanio_segm_a_crear){
	int base_a_devolver;
	list_sort(huecos_libres, (void* ) de_menor_a_mayor_tamanio);

	for(int i = 0; i < list_size(huecos_libres); i++){ // itero la lista hasta encontrar un hueco libre en el que entre el segm a crear
		t_hueco* hueco1 = list_get(huecos_libres, i);
		
		if(hueco1->tamanio >=  tamanio_segm_a_crear){
			base_a_devolver = hueco1->direccion_base;
			break;
		} //no se devuelve error porque se verifica antes de que va a haber un hueco libre con ese tamanio
	}
	log_info(logger, "Base obtenida: %d", base_a_devolver);	
	list_sort(huecos_libres, (void* ) por_menor_base);
	
	return base_a_devolver;
}


int obtener_base_segun_FIRST(uint32_t tamanio_segm_a_crear){
	for(int i = 0; i < list_size(huecos_libres); i++){
		t_hueco* hueco1 = list_get(huecos_libres, i);

		if (hueco1->tamanio >= tamanio_segm_a_crear){
			return hueco1->direccion_base;
		}
	}

	return -1;
}

int obtener_base_segun_WORST(uint32_t tamanio_segm_a_crear){
	int base = -1;
	
	list_sort(huecos_libres, (void* ) de_menor_a_mayor_tamanio);
	
	for(int i = list_size(huecos_libres); i > 0; i--){ // CAMBIAR HAY QUE ORDENAR POR TAMANIO
		t_hueco* hueco1 = list_get(huecos_libres, i);

		if (hueco1->tamanio >= tamanio_segm_a_crear){
			base = hueco1->direccion_base;
			break;
		}
	}
	
	

	return base;
}

// Devuelve el tamanio libre total de la memoria (no toma en cuenta si es contiguo o no)
int tamanio_libre(){
	int tam_total = 0;
	for(int i = 0; i < list_size(huecos_libres); i++){
		t_hueco* hueco = list_get(huecos_libres, i);
		tam_total += hueco->tamanio;
	}
	return tam_total;
}

int tamanio_libre_max_contiguo(){
	int tam_max = 0;
	for(int i = 0; i < list_size(huecos_libres); i++){
		t_hueco* hueco = list_get(huecos_libres, i);
		if(hueco->tamanio > tam_max)
			tam_max = hueco->tamanio;
	}
	return tam_max;
}

// UTILS ORDENAR LISTA HUECOS -----------------------------------------------------------
bool de_menor_a_mayor_tamanio(t_hueco* hueco1, t_hueco* hueco2){
	return hueco1->tamanio < hueco2->tamanio;
}

bool por_menor_base(t_hueco* hueco1, t_hueco* hueco2){
	return hueco1->direccion_base < hueco2->direccion_base;
}
// FIN UTILS ORDENAR LISTA HUECOS -------------------------------------------------------

// UTILS HUECOS LIBRE -------------------------------------------------------------------
void agregar_hueco_libre(uint32_t base, uint32_t tam){
	t_hueco* hueco_libre = malloc(sizeof(t_hueco));
	hueco_libre->direccion_base = base;
	hueco_libre->tamanio = tam;

	list_add_sorted(huecos_libres, hueco_libre, (void* )por_menor_base);
}

// FALTA PROBAR
void union_huecos_libres_contiguos(uint32_t posicion){
	if(posicion + 1 < list_size(huecos_libres)){
		t_hueco* hueco1 = list_get(huecos_libres, posicion);
		t_hueco* hueco2 = list_get(huecos_libres, posicion + 1);
		if(hueco1->direccion_base + hueco1->tamanio == hueco2->direccion_base){
			agregar_hueco_libre(hueco1->direccion_base, hueco1->tamanio + hueco2->tamanio);
			list_remove(huecos_libres, posicion);
			list_remove(huecos_libres, posicion + 1);
			union_huecos_libres_contiguos(posicion);
		}else
			union_huecos_libres_contiguos(posicion + 1);
	}	
}
// FIN DE UTILS HUECOS LIBRES -----------------------------------------------------------
