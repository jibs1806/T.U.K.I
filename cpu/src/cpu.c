#include "../includes/cpu.h"

int main(int argc, char **argv){
    char *config_path = argv[1];

	levantar_modulo(config_path);

	iniciar_modulo();

	while(1);
	return 0;
}


// UTILS INICIAR MODULO -----------------------------------------------------------------
void levantar_modulo(char* config_path){
	logger = iniciar_logger();
	config = iniciar_config(config_path);
	levantar_config();

	inicializar_semaforos();
	establecer_conexiones();
}

void inicializar_semaforos(){
	sem_init(&necesito_enviar_cde, 0, 0);
	sem_init(&leer_siguiente_instruccion, 0 , 0);

	sem_init(&bin1_envio_cde, 0, 0);
	sem_init(&bin2_recibir_cde, 0, 1);
}

void iniciar_modulo(){
	pthread_t recibir_cde_kernel;
	pthread_create(&recibir_cde_kernel, NULL, (void *) recibir_cde, NULL);
	pthread_detach(recibir_cde_kernel);

	pthread_t envio_cde_kernel;
	pthread_create(&envio_cde_kernel, NULL, (void *) enviar_cde, NULL);
	pthread_detach(envio_cde_kernel);

	pthread_t ejecucion;
	pthread_create(&ejecucion, NULL, (void *) ejecutar_proceso, NULL);
	pthread_detach(ejecucion);
}

void finalizar_modulo(){
	log_destroy(logger);
	config_destroy(config);
	return;
}

t_log* iniciar_logger(void)
{
	t_log* nuevo_logger;
	nuevo_logger = log_create("cpu.log", "Cpu", 1, LOG_LEVEL_INFO);

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
	config_cpu.retardo = config_get_int_value(config,"RETARDO_INSTRUCCION");
	config_cpu.ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	config_cpu.puerto_memoria = config_get_int_value(config,"PUERTO_MEMORIA");
	config_cpu.puerto_escucha = config_get_int_value(config,"PUERTO_ESCUCHA");
	config_cpu.tam_max_segmento = config_get_int_value(config,"TAM_MAX_SEGMENTO");
}
// FIN UTILS INICIAR MODULO -------------------------------------------------------------


// UTILS CONEXIONES ---------------------------------------------------------------------
void establecer_conexiones(){
	socket_memoria = crear_conexion(config_cpu.ip_memoria, config_cpu.puerto_memoria);
	if (socket_memoria >= 0){
		log_info(logger,"Conectado con MEMORIA");
		enviar_handshake(socket_memoria);
		if (recibir_handshake(socket_memoria) == HANDSHAKE){
			log_info(logger, "Handshake con MEMORIA realizado.");
		}else
			log_info(logger, "Ocurio un error al realizar el handshake con MEMORIA.");
	}
	else
		log_info(logger,"Error al conectar con MEMORIA");
	
	
	server_fd = iniciar_servidor(logger,config_cpu.puerto_escucha);

	kernel_fd = esperar_cliente(server_fd, logger);
	
	if (kernel_fd >= 0){
		log_info(logger, "Conectado con kernel");
		if (recibir_handshake(kernel_fd) == HANDSHAKE){
			enviar_handshake(kernel_fd);
			log_info(logger, "Handshake con Kernel realizado.");

		}
	}else
		log_info(logger, "Error al conectar con kernel");
}

void recibir_cde(){
	while(1){
		sem_wait(&bin2_recibir_cde);

		t_buffer* buffer = recibir_buffer(kernel_fd);
		log_info(logger, "Recibi buffer.");

		cde_en_ejecucion = 	buffer_read_cde(buffer);
		log_info(logger, "Lei CDE enviado por kernel.");


		destruir_buffer_nuestro(buffer);

		sem_post(&leer_siguiente_instruccion);
		sem_post(&bin1_envio_cde);
	}
}

void enviar_cde(){
	while(1){
		sem_wait(&necesito_enviar_cde);
		sem_wait(&bin1_envio_cde);
		
		t_buffer* buffer = crear_buffer_nuestro();
		buffer_write_cde(buffer, *cde_en_ejecucion);
		enviar_buffer(buffer, kernel_fd);
		log_info(logger, "Envie el cde a kernel");
		
		destruir_buffer_nuestro(buffer);
		
		destruir_cde(cde_en_ejecucion);
		
		sem_post(&bin2_recibir_cde);
	}
}
// UTILS FIN CONEXIONES -----------------------------------------------------------------


// UTILS INSTRUCCIONES (GENERAL) --------------------------------------------------------
void ejecutar_proceso(){
	while(1){
		sem_wait(&leer_siguiente_instruccion);
		int indice = cde_en_ejecucion->program_counter;
		t_instruction* instruccion_actual = list_get(cde_en_ejecucion->lista_de_instrucciones, indice);
		evaluar_instruccion(instruccion_actual);
		cde_en_ejecucion->program_counter++;
	}
}

void evaluar_instruccion(t_instruction* instruccion){
	switch(instruccion->instruccion){
		case SET:
			log_info(logger, "PID: %d - EJECUTANDO : SET - PARAMETROS: ( %s , %s )", cde_en_ejecucion->pid, instruccion->string1, instruccion->string2);
			ejecutar_set(instruccion->string1,instruccion->string2);			
			sem_post(&leer_siguiente_instruccion);
		break;
		case MOV_IN:
			log_info(logger, "PID: %d - EJECUTANDO : MOVE IN - PARAMETROS: ( %s , %d )", cde_en_ejecucion->pid, instruccion->string1, instruccion->numero1);
			switch(analizar_mov_in_o_out(instruccion->string1, instruccion->numero1)){
				case PUEDE_EJECUTAR:
					ejecutar_move_in(instruccion->string1, instruccion->numero1);			
					sem_post(&leer_siguiente_instruccion);
					break;
				case SEGM_FAULT:
					op_code segm_fault_mov_in = SEGMENTATION_FAULT;
					enviar_codigo(kernel_fd, segm_fault_mov_in);
					sem_post(&necesito_enviar_cde);
					break;
			}
		break;
		case MOV_OUT:
			log_info(logger, "PID: %d - EJECUTANDO : MOVE OUT - PARAMETROS: ( %d , %s )", cde_en_ejecucion->pid, instruccion->numero1, instruccion->string1);
			switch(analizar_mov_in_o_out(instruccion->string1, instruccion->numero1)){
				case PUEDE_EJECUTAR:
					ejecutar_move_out(instruccion->string1, instruccion->numero1);		
					sem_post(&leer_siguiente_instruccion);
					break;
				case SEGM_FAULT:
					op_code segm_fault_mov_out = SEGMENTATION_FAULT;
					enviar_codigo(kernel_fd, segm_fault_mov_out);
					sem_post(&necesito_enviar_cde);
					break;
			}
		break;
		case YIELD:
			log_info(logger, "PID: %d - EJECUTANDO : YIELD - PARAMETROS: (%s)", cde_en_ejecucion->pid, instruccion->string1);
			sem_post(&necesito_enviar_cde);
			break;
		case EXIT:
			log_info(logger, "PID: %d - EJECUTANDO : EXIT", cde_en_ejecucion->pid);
			sem_post(&necesito_enviar_cde);
		break;
		case IO:
			log_info(logger, "PID: %d - EJECUTANDO : IO - PARAMETROS: (%d)", cde_en_ejecucion->pid, instruccion->numero1);
		
			sem_post(&necesito_enviar_cde);
		break;
		case SIGNAL:
			log_info(logger, "PID: %d - EJECUTANDO : SIGNAL - PARAMETROS: (%s)", cde_en_ejecucion->pid, instruccion->string1);

			sem_post(&necesito_enviar_cde);
		break;
		case WAIT:
			log_info(logger, "PID: %d - EJECUTANDO : WAIT - PARAMETROS: (%s)", cde_en_ejecucion->pid, instruccion->string1);
			
			sem_post(&necesito_enviar_cde);
		break;
		case F_OPEN:
			log_info(logger, "PID: %d - EJECUTANDO : F_OPEN - PARAMETROS: (%s - %d - %d)", cde_en_ejecucion->pid, instruccion->string1, instruccion->numero1, instruccion->numero2);

			sem_post(&necesito_enviar_cde);
		break;
		case F_CLOSE:
			log_info(logger, "PID: %d - EJECUTANDO : F_CLOSE - PARAMETROS: (%s)", cde_en_ejecucion->pid, instruccion->string1);

			sem_post(&necesito_enviar_cde);
		break;
		case F_SEEK:
			log_info(logger, "PID: %d - EJECUTANDO : F_SEEK - PARAMETROS: (%s - %d)", cde_en_ejecucion->pid, instruccion->string1, instruccion->numero1);

			sem_post(&necesito_enviar_cde);
		break;
		case F_READ:
			log_info(logger, "PID: %d - EJECUTANDO : F_READ - PARAMETROS: (%s - %d - %d)", cde_en_ejecucion->pid, instruccion->string1, instruccion->numero1, instruccion->numero2);
			instruccion->numero1 = calcular_dir_fisica(instruccion->numero1, instruccion->numero2);	
			sem_post(&necesito_enviar_cde);
		break;
		case F_WRITE:
			log_info(logger, "PID: %d - EJECUTANDO : F_WRITE - PARAMETROS: (%s - %d - %d)", cde_en_ejecucion->pid, instruccion->string1, instruccion->numero1, instruccion->numero2);
			instruccion->numero1 = calcular_dir_fisica(instruccion->numero1, instruccion->numero2);
			sem_post(&necesito_enviar_cde);
		break;
		case F_TRUNCATE:
			log_info(logger, "PID: %d - EJECUTANDO : F_TRUNCATE - PARAMETROS: (%s - %d)", cde_en_ejecucion->pid, instruccion->string1, instruccion->numero1);

			sem_post(&necesito_enviar_cde);
		break;
		case CREATE_SEGMENT:
			log_info(logger, "PID: %d - EJECUTANDO : CREATE_SEGMENT - PARAMETROS: (%d - %d)", cde_en_ejecucion->pid, instruccion->numero1, instruccion->numero2);
			sem_post(&necesito_enviar_cde);
		break;
		case DELETE_SEGMENT:
			log_info(logger, "PID: %d - EJECUTANDO : DELETE_SEGMENT - PARAMETROS: (%d)", cde_en_ejecucion->pid, instruccion->numero1);

			sem_post(&necesito_enviar_cde);
		break;
	}
}
// FIN UTILS INSTRUCCIONES (GENERAL) ----------------------------------------------------


// UTILS INSTRUCCIONES (PARTICULAR) -----------------------------------------------------
Rta_mov_in_o_out analizar_mov_in_o_out(char* registro, uint32_t dir_logica){
	
	int num_seg = floor(dir_logica / config_cpu.tam_max_segmento);
	
	t_segmento* segm = encontrar_segmento_por_id(cde_en_ejecucion->tabla_segmentos, num_seg); 
	int desplaz_segmento = dir_logica % segm->tamanio_segmento;

	int tamanio_reg = tamanioRegistro(registro);

	if(desplaz_segmento + tamanio_reg > segm->tamanio_segmento){
		log_info(logger, "PID: %d - Error SEG_FAULT- Segmento: %d - Offset: %d - Tamanio: %d", cde_en_ejecucion->pid, num_seg, desplaz_segmento, segm->tamanio_segmento);
		return SEGM_FAULT;
	}
	
	return PUEDE_EJECUTAR;
}


void ejecutar_set(char* registro, char* valor){
	usleep(config_cpu.retardo);
	if (strcmp(registro,"AX") == 0)  
		strcpy(cde_en_ejecucion->registros_cpu.AX, valor);
	if (strcmp(registro,"BX") == 0)  
		strcpy(cde_en_ejecucion->registros_cpu.BX, valor);
	if (strcmp(registro,"CX") == 0)  
		strcpy(cde_en_ejecucion->registros_cpu.CX, valor);
	if (strcmp(registro,"DX") == 0)  
		strcpy(cde_en_ejecucion->registros_cpu.DX, valor);
	if (strcmp(registro,"EAX") == 0) 
		strcpy(cde_en_ejecucion->registros_cpu.EAX, valor);
	if (strcmp(registro,"EBX") == 0) 
		strcpy(cde_en_ejecucion->registros_cpu.EBX, valor);
	if (strcmp(registro,"ECX") == 0) 
		strcpy(cde_en_ejecucion->registros_cpu.ECX, valor);
	if (strcmp(registro,"EDX") == 0) 
		strcpy(cde_en_ejecucion->registros_cpu.EDX, valor);
	if (strcmp(registro,"RAX") == 0) 
		strcpy(cde_en_ejecucion->registros_cpu.RAX, valor);
	if (strcmp(registro,"RBX") == 0) 
		strcpy(cde_en_ejecucion->registros_cpu.RBX, valor);
	if (strcmp(registro,"ECX") == 0) 
		strcpy(cde_en_ejecucion->registros_cpu.RCX, valor);
	if (strcmp(registro,"RDX") == 0)
		strcpy(cde_en_ejecucion->registros_cpu.RDX, valor);
}

uint32_t tamanioRegistro(char* registro){
	if(strcmp(registro,"AX") == 0 || strcmp(registro,"BX") == 0 || strcmp(registro,"CX") == 0 || strcmp(registro,"DX") == 0)
		return 4;
	else if(strcmp(registro,"EAX") == 0 || strcmp(registro,"EBX") == 0 || strcmp(registro,"ECX") == 0 || strcmp(registro,"EDX") == 0)
			return 8;
		else if(strcmp(registro,"RAX") == 0 || strcmp(registro,"RBX") == 0 || strcmp(registro,"RCX") == 0 || strcmp(registro,"RDX") == 0)
				return 16;
	else return -1;
}

uint32_t calcular_dir_fisica(uint32_t dir_logica, uint32_t tamanio){
	uint32_t tam_max_segmento = config_cpu.tam_max_segmento;
	
	uint32_t num_seg = floor(dir_logica/tam_max_segmento);
	t_segmento* segmento = encontrar_segmento_por_id(cde_en_ejecucion->tabla_segmentos, num_seg);
	
	uint32_t desplaz_segmento = dir_logica % segmento->tamanio_segmento;
	
	return segmento->direccion_base + desplaz_segmento;
}


// Lee el contenido de la direccion logica en el registro
void ejecutar_move_in(char* nombre_registro, uint32_t dir_logica){
	uint32_t tamanio = tamanioRegistro(nombre_registro);
	uint32_t dir_fisica = calcular_dir_fisica(dir_logica, tamanio);
	uint32_t num_seg = floor(dir_logica / config_cpu.tam_max_segmento);

	t_buffer* buffer = crear_buffer_nuestro();

	buffer->codigo = SOLICITUD_MOVE_IN;
	buffer_write_uint32(buffer, cde_en_ejecucion->pid);
	buffer_write_uint32(buffer, dir_fisica); // La va usar memoria como indice para escribir en la memoria principal
	buffer_write_uint32(buffer, tamanio);
	enviar_buffer(buffer, socket_memoria);

	destruir_buffer_nuestro(buffer);

	buffer = recibir_buffer(socket_memoria);
	uint32_t tam = 0;
	char* valor_leido = buffer_read_stringV2(buffer, &tam);
	destruir_buffer_nuestro(buffer);

	log_info(logger, "PID: %d - Accion: LEER - Segmento: %d - Direccion Fisica: %d - Valor: %s", cde_en_ejecucion->pid, num_seg, dir_fisica, valor_leido);
	
	ejecutar_set(nombre_registro, valor_leido);
}

// Escribe el contenido del registro en la direccion logica
void ejecutar_move_out(char* registro, uint32_t dir_logica){  
	uint32_t tamanio = tamanioRegistro(registro);
	uint32_t dir_fisica = calcular_dir_fisica(dir_logica,tamanio);
	int num_seg = floor(dir_logica / config_cpu.tam_max_segmento);

	t_buffer* buffer = crear_buffer_nuestro();
	
	log_info(logger, "Por obtener el contenido de %s", registro);
	char* valor_a_escribir = devolver_contenido_registro(registro);
	log_info(logger, "Contenido: %s = %s", registro, valor_a_escribir);


	buffer->codigo = SOLICITUD_MOVE_OUT;
	buffer_write_uint32(buffer, cde_en_ejecucion->pid);
	buffer_write_uint32(buffer, dir_fisica);
	buffer_write_string(buffer, valor_a_escribir);

	enviar_buffer(buffer, socket_memoria);
	destruir_buffer_nuestro(buffer);

	if(recibir_codigo(socket_memoria) == RTA_MOVE_OUT)
		log_info(logger, "PID: %d - Accion: ESCRIBIR - Segmento: %d - Direccion Fisica: %d - Valor: %s", cde_en_ejecucion->pid, num_seg, dir_fisica, valor_a_escribir);
	else
		log_info(logger, "Error al recibir respuesta de mov_out");

}


char* devolver_contenido_registro(char* nombre_registro){
	if (strcmp(nombre_registro, "AX") == 0)
		return cde_en_ejecucion->registros_cpu.AX;

	if (strcmp(nombre_registro, "BX") == 0)
		return cde_en_ejecucion->registros_cpu.BX;

	if (strcmp(nombre_registro, "CX") == 0)
		return cde_en_ejecucion->registros_cpu.CX;

	if (strcmp(nombre_registro, "DX") == 0)
		return cde_en_ejecucion->registros_cpu.DX;

	if (strcmp(nombre_registro, "EAX") == 0)
		return cde_en_ejecucion->registros_cpu.EAX;

	if (strcmp(nombre_registro, "EBX") == 0)
		return cde_en_ejecucion->registros_cpu.EBX;

	if (strcmp(nombre_registro, "ECX") == 0)
		return cde_en_ejecucion->registros_cpu.ECX;

	if (strcmp(nombre_registro, "EDX") == 0)
		return cde_en_ejecucion->registros_cpu.EDX;

	if (strcmp(nombre_registro, "RAX") == 0)
		return cde_en_ejecucion->registros_cpu.RAX;

	if (strcmp(nombre_registro, "RBX") == 0)
		return cde_en_ejecucion->registros_cpu.RBX;

	if (strcmp(nombre_registro, "RCX") == 0)
		return cde_en_ejecucion->registros_cpu.RCX;

	if (strcmp(nombre_registro, "RDX") == 0)
		return cde_en_ejecucion->registros_cpu.RDX;
}
// FIN UTILS INSTRUCCIONES --------------------------------------------------------------
