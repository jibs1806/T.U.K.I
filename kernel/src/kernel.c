#include "../includes/kernel.h"

// usleep(5 * 1000000);

t_instruction* inicializar_instruccion(InstructionType codigo, char* str1, char* str2, uint32_t num1, uint32_t num2){
	t_instruction* instr = malloc(sizeof(t_instruction));

	instr -> instruccion = codigo;

	instr->numero1 = num1;
	instr->numero2 = num2;
	int tam1 = 0;
	while(str1[tam1] != NULL)
		tam1++;
	instr->string1 = malloc(tam1);
	strcpy(instr->string1, str1);

	int tam2 = 0;
	while(str1[tam1] != NULL)
		tam2++;
	instr->string2 = malloc(tam2);
	strcpy(instr->string2, str2);

	return instr;
}

t_list* instruccion_prueba(){
	t_list* lista_prueba = list_create();


	list_add(lista_prueba, inicializar_instruccion(F_OPEN, "Videogames", CHAR_VACIO, INT_VACIO, INT_VACIO));
	list_add(lista_prueba, inicializar_instruccion(F_TRUNCATE, "Videogames", CHAR_VACIO, 256, INT_VACIO));
	list_add(lista_prueba, inicializar_instruccion(MOV_IN, "RAX", CHAR_VACIO, 16, INT_VACIO));
	//list_add(lista_prueba, inicializar_instruccion(F_WRITE, "Videogames", CHAR_VACIO, 16, 16));
	//list_add(lista_prueba, inicializar_instruccion(F_READ, "Videogames", CHAR_VACIO, 16, 16));
	list_add(lista_prueba, inicializar_instruccion(F_CLOSE, "Videogames", CHAR_VACIO, INT_VACIO, INT_VACIO));
	list_add(lista_prueba, inicializar_instruccion(EXIT, "v", "v", INT_VACIO, INT_VACIO));	

	return lista_prueba;
}

int main(int argc, char **argv){
	char *config_path = argv[1];
	
	levantar_modulo(config_path);
	
	planificadores();
	
	while(1);
	return 0;
}


// SUBPROGRAMAS
void levantar_modulo(char* config_path){
	logger = iniciar_logger();
	config = iniciar_config(config_path);
	levantar_config();

	incializar_listas();
	incializar_semaforos();
	cant_operaciones_mem_fs = 0; //var para controlar que todas las op entre mem y fs se hayan completado
	inicializar_recurso_nulo();

	establecer_conexiones();
	return;
}

void incializar_listas(){
	procesosNew = queue_create();
	procesosReady = queue_create();
	procesosExec = queue_create();
	procesosExit = queue_create();

	lista_pcb_global = list_create();

	archivos_abiertos_global = list_create();
	inicializar_archivo_nulo();
	return;
}

void incializar_semaforos(){
	// COLAS
	pthread_mutex_init(&mutex_new, NULL);
	pthread_mutex_init(&mutex_ready, NULL);
	pthread_mutex_init(&mutex_blocked, NULL);
	pthread_mutex_init(&mutex_exec, NULL);
	pthread_mutex_init(&mutex_exit, NULL);

	pthread_mutex_init(&mutex_pcb_global, NULL);
	// FIN COLAS
	
	pthread_mutex_init(&mutex_pid_a_asignar, NULL); // para var global pid
	
	//para no agregar mas procesos a READY de los q permite el grado max de multi
	sem_init(&cont_grado_max_multiprog, 0, config_kernel.grado_max_multi); 

	//para postear cuando uno entra a exit y poder matar su consola
	sem_init(&cont_new, 0, 0); 
	sem_init(&cont_ready, 0, 0);
	sem_init(&cont_exec, 0, 0);
	sem_init(&cont_exit, 0, 0);

	sem_init(&bin1_envio_cde, 0, 1);
	sem_init(&bin2_recibir_cde, 0, 0);

	sem_init(&necesito_enviar_cde, 0, 0);

	sem_init(&puedo_recibir_de_memoria, 0, 0);
	sem_init(&recepcion_exitosa_de_memoria, 0, 0);
	// Para ver si el procesador esta libre para mandar un proceso a ejecucion
	sem_init(&cont_procesador_libre, 0, 1);

	sem_init(&operaciones_fs_mem, 0, 1);
}

void finalizar_modulo(){
	log_destroy(logger);
	config_destroy(config);
	return;
}


// UTILS INICIAR MODULO -----------------------------------------------------------------
t_log* iniciar_logger(void)
{
	t_log* nuevo_logger;
	nuevo_logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_INFO);

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

	config_kernel.ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	config_kernel.puerto_memoria = config_get_int_value(config,"PUERTO_MEMORIA");

	config_kernel.ip_cpu = config_get_string_value(config,"IP_CPU");
	config_kernel.puerto_cpu = config_get_int_value(config,"PUERTO_CPU");

	config_kernel.ip_file_system = config_get_string_value(config,"IP_FILESYSTEM");
	config_kernel.puerto_file_system = config_get_int_value(config,"PUERTO_FILESYSTEM");

	config_kernel.puerto_escucha = config_get_int_value(config,"PUERTO_ESCUCHA");
	config_kernel.algoritmo = config_get_string_value(config,"ALGORITMO_PLANIFICACION");
	config_kernel.est_inicial = config_get_int_value(config,"ESTIMACION_INICIAL");
	config_kernel.alpha_hrrn = config_get_int_value(config,"HRRN_ALFA");
	config_kernel.grado_max_multi = config_get_int_value(config,"GRADO_MAX_MULTIPROGRAMACION");

	char **recursos = config_get_array_value(config,"RECURSOS");
	char **instancias = config_get_array_value(config,"INSTANCIAS_RECURSOS");

	config_kernel.recursos = list_create();

	int a = 0;
	while(recursos[a]!= NULL){
		int num = strtol(instancias[a],NULL,10);
		t_recurso* recurso = inicializar_recurso(recursos[a], num);
		list_add(config_kernel.recursos, recurso);
		a++;
	}
	
	
}
// FIN UTILS INICIAR MODULO -------------------------------------------------------------


// UTILS CONEXIONES ---------------------------------------------------------------------
void manejar_conexion_con_memoria(){
	/*
	pthread_t recepcion_mensaje;
	pthread_create(&recepcion_mensaje, NULL, (void*) recibir_mensaje_de_memoria, NULL);
	pthread_detach(recepcion_mensaje);*/
	
}

void manejar_conexion_con_cpu(){
	pthread_t envio_cde;
	pthread_create(&envio_cde, NULL, (void*) enviar_cde_a_cpu, NULL);
	pthread_detach(envio_cde);

	pthread_t recepcion_cde;
	pthread_create(&recepcion_cde, NULL, (void*) recibir_cde_de_cpu, NULL);
	pthread_detach(recepcion_cde);
}

void manejar_conexion_con_file_system(){
	
}

void establecer_conexiones()
{
	socket_memoria = crear_conexion(config_kernel.ip_memoria, config_kernel.puerto_memoria);
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
	

	
	socket_cpu = crear_conexion(config_kernel.ip_cpu, config_kernel.puerto_cpu);
	if (socket_cpu >= 0){			
		log_info(logger,"Conectado con cpu");

		enviar_handshake(socket_cpu);
		if (recibir_handshake(socket_cpu) == HANDSHAKE){
			log_info(logger, "Handshake con CPU realizado.");

			manejar_conexion_con_cpu();

		}else
			log_info(logger, "Ocurio un error al realizar el handshake con CPU.");
	}
	else
		log_info(logger,"Error al conectar con cpu");
	

	
	socket_file_system = crear_conexion(config_kernel.ip_file_system, config_kernel.puerto_file_system);
	if (socket_file_system >= 0){			
		log_info(logger,"Conectado con FILE SYSTEM");

		enviar_handshake(socket_file_system);
		if (recibir_handshake(socket_file_system) == HANDSHAKE){
			log_info(logger, "Handshake con FILE SYSTEM realizado.");


		}else
			log_info(logger, "Ocurio un error al realizar el handshake con FILE SYSTEM.");
		}
	else
		log_info(logger,"Error al conectar con FILE SYSTEM");

	
	server_fd = iniciar_servidor(logger, config_kernel.puerto_escucha);
	
	pthread_t manejo_consolas;
	pthread_create(&manejo_consolas, NULL, (void *) esperar_consolas, NULL);
	pthread_detach(manejo_consolas);
	
}
// FIN UTILS CONEXIONES -----------------------------------------------------------------


// UTILS CONSOLA ------------------------------------------------------------------------
void esperar_consolas(){
	//thread para esperar clientes
	while(1){
		int socket = esperar_cliente(server_fd, logger);

		if (recibir_handshake(socket) == HANDSHAKE){
			enviar_handshake(socket);
			log_info(logger, "Handshake con Consola realizado.");
			
			pthread_t admitir_consolas;
			pthread_create(&admitir_consolas, NULL, (void *) recepcionar_proceso, (void *) &socket);
			pthread_detach(admitir_consolas);
		}else
			log_info(logger, "Ocurio un error al realizar el handshake con Consola.");

	}
}
// FIN UTILS CONSOLA --------------------------------------------------------------------


// UTILS CPU ----------------------------------------------------------------------------
void recibir_cde_de_cpu(){
	while(1){
		sem_wait(&bin2_recibir_cde);

		t_buffer* buffer = recibir_buffer(socket_cpu);

		pthread_mutex_lock(&mutex_exec);
		pcb_en_ejecucion->cde = buffer_read_cde(buffer);
		pthread_mutex_unlock(&mutex_exec);
	
		destruir_buffer_nuestro(buffer);
		evaluar_instruccion(); // aca posteo necesito_enviar_cde
		sem_post(&bin1_envio_cde);
	}
}

void enviar_cde_a_cpu(){
	while(1){
		sem_wait(&necesito_enviar_cde);
		sem_wait(&bin1_envio_cde);
		t_buffer* buffer = crear_buffer_nuestro();
		buffer->codigo = CONTEXTO_DE_EJECUCION;
		buffer_write_cde(buffer, *(pcb_en_ejecucion->cde));
		enviar_buffer(buffer, socket_cpu);

		destruir_buffer_nuestro(buffer);
		
		destruir_cde(pcb_en_ejecucion->cde);
		sem_post(&bin2_recibir_cde);
	}
}
// FIN UTILS CPU ------------------------------------------------------------------------


// UTILS MEMORIA ------------------------------------------------------------------------
t_list* recibir_tabla_segmentos_inicial(uint32_t pid){
	t_buffer* buffer = crear_buffer_nuestro();
	buffer->codigo = SOLICITUD_INICIO_PROCESO_MEMORIA;
	
	buffer_write_uint32(buffer, pid);
	enviar_buffer(buffer, socket_memoria);
	destruir_buffer_nuestro(buffer);
	
	buffer = recibir_buffer(socket_memoria);
	if(buffer->codigo == INICIO_EXITOSO_PROCESO_MEMORIA){
		t_list* tabla_inicial = buffer_read_tabla_segmentos(buffer);
		destruir_buffer_nuestro(buffer);
		return tabla_inicial;
	}
	else{
		log_info(logger, "Error al inciar procesos en memoria");
		destruir_buffer_nuestro(buffer);
	}

}


// FIN UTILS MEMORIA --------------------------------------------------------------------

// UTILS MOVIMIENTO PCB SEGUROS (MUTEX) -------------------------------------------------
t_pcb* retirar_pcb_de(t_queue* cola, pthread_mutex_t* mutex){
	t_pcb* pcb;
	pthread_mutex_lock(mutex);
	pcb = queue_pop(cola);
	pthread_mutex_unlock(mutex);
	return pcb;
}

void agregar_pcb_a(t_queue* cola, t_pcb* pcb_a_agregar, pthread_mutex_t* mutex){
	pthread_mutex_lock(mutex);
	queue_push(cola, pcb_a_agregar);
	pthread_mutex_unlock(mutex);
}
// FIN UTILS MOVIMIENTO PCB SEGUROS (MUTEX) ---------------------------------------------


// ------------------------------------ TRANSICIONES ------------------------------------
// TRANSICIONES CORTO PLAZO -------------------------------------------------------------
void enviar_de_ready_a_exec(){
	while(1){
		sem_wait(&cont_ready);
		sem_wait(&cont_procesador_libre);

		pthread_mutex_lock(&mutex_exec);
		pcb_en_ejecucion = retirar_pcb_de_ready_segun_algoritmo();
		pcb_en_ejecucion->tiempo_llegada_exec = time(NULL);
		pthread_mutex_unlock(&mutex_exec);

		sem_post(&cont_exec);
		log_info(logger, "PID: %d - Estado anterior: READY - Estado actual: EXEC", pcb_en_ejecucion->cde->pid); //OBLIGATORIO
		
		// Primer post de ese semaforo ya que se envia por primera vez
		sem_post(&necesito_enviar_cde);
	}
}

void enviar_de_exec_a_psedudoblock(char* razon){ // SOLO PARA WAIT, por ARCHIVOS y por (IO -> va a ser un hilo aparte)
	// razones: "nombre de recurso" || "nombre de archivo" |||||| io va aparte
	sem_wait(&cont_exec);
	
	log_info(logger, "PID: %d - Estado anterior: EXEC - Estado actual: BLOCK", pcb_en_ejecucion->cde->pid); //OBLIGATORIO
	log_info("PID: %d -	Bloqueado por: %s", pcb_en_ejecucion->cde->pid, razon);
	pcb_en_ejecucion->tiempo_salida_exec = time(NULL);
	pcb_en_ejecucion = NULL;
	
	sem_post(&cont_procesador_libre);
}

void enviar_de_exec_a_ready(){ // SOLO PARA YIELD (se llama al ejecutar yield)
	sem_wait(&cont_exec);
	agregar_pcb_a(procesosReady, pcb_en_ejecucion, &mutex_ready);
	pcb_en_ejecucion->tiempo_salida_exec = time(NULL);
	pcb_en_ejecucion->tiempo_llegada_ready = time(NULL);

	log_info(logger, "PID: %d - Estado anterior: EXEC - Estado actual: READY", pcb_en_ejecucion->cde->pid); //OBLIGATORIO
	
	pcb_en_ejecucion = NULL;
	
	sem_post(&cont_procesador_libre);
	sem_post(&cont_ready);

	// log_info(logger, "Cola ready %s :");
	// mostrar_pids(pcb_a_ready->cde->lista_de_instrucciones);
}

void enviar_de_pseudoblock_a_ready(t_pcb* pcb){
	pcb->tiempo_llegada_ready = time(NULL);
	agregar_pcb_a(procesosReady, pcb, &mutex_ready);
	log_info(logger, "PID: %d - Estado anterior: BLOCK - Estado actual: READY", pcb->cde->pid); //OBLIGATORIO
	log_info(logger, "Cola ready - %s - ", config_kernel.algoritmo); //OBLIGATORIO

	sem_post(&cont_ready);
}
void enviar_de_exec_a_exit(char* razon){
	sem_wait(&cont_exec);

	
	pthread_mutex_lock(&mutex_pcb_global);
	list_remove_element(lista_pcb_global, pcb_en_ejecucion);
	pthread_mutex_unlock(&mutex_pcb_global);

	agregar_pcb_a(procesosExit, pcb_en_ejecucion, &mutex_exit);
	pcb_en_ejecucion->tiempo_salida_exec = time(NULL);
	log_info(logger, "PID: %d - Estado anterior: EXEC - Estado actual: EXIT", pcb_en_ejecucion->cde->pid); //OBLIGATORIO
	log_info(logger, "Finaliza el proceso %d - Motivo: %s", pcb_en_ejecucion->cde->pid, razon); // OBLIGATORIO
	
	pcb_en_ejecucion = NULL;
	
	sem_post(&cont_procesador_libre); // se libera el procesador
	sem_post(&cont_exit); // se agrega uno a procesosExit
	sem_post(&cont_grado_max_multiprog); // Habilita uno para que venga de NEW a READY
}
// FIN TRANSICIONES CORTO PLAZO ---------------------------------------------------------

// TRANSICIONES LARGO PLAZO -------------------------------------------------------------
void enviar_de_new_a_ready(){
	while(1){
		sem_wait(&cont_new);
		sem_wait(&cont_grado_max_multiprog);
		
		t_pcb* pcb_a_ready = retirar_pcb_de(procesosNew, &mutex_new);

		pcb_a_ready->cde->tabla_segmentos = recibir_tabla_segmentos_inicial(pcb_a_ready->cde->pid);

		pcb_a_ready->tiempo_llegada_ready = time(NULL);
		pcb_a_ready->estimado_prox_rafaga = calcular_estimacion_proxima_rafaga(pcb_a_ready);
		agregar_pcb_a(procesosReady, pcb_a_ready, &mutex_ready);
		
		// Agrego el pcb a la lista global de pids
		pthread_mutex_lock(&mutex_pcb_global);
		list_add(lista_pcb_global, pcb_a_ready);
		pthread_mutex_unlock(&mutex_pcb_global);

		log_info(logger, "PID: %d - Estado anterior: NEW - Estado actual: READY", pcb_a_ready->cde->pid); //OBLIGATORIO
		sem_post(&cont_ready);
	}	
}

void recepcionar_proceso(void* arg){
	int* p = (int*) arg;
	int socket_a_atender = *p;

	// Recibe lista de instrucciones de la consola y arma el PCB
	t_buffer* buffer = recibir_buffer(socket_a_atender);

	t_list* lista_instrucciones = buffer_read_lista_instrucciones(buffer);

	destruir_buffer_nuestro(buffer);

	t_pcb* pcb_recibido = crear_pcb(lista_instrucciones, socket_a_atender);

	agregar_pcb_a(procesosNew, pcb_recibido, &mutex_new);
	log_info(logger, "Se crea el proceso %d en NEW", pcb_recibido->cde->pid); // OBLIGATORIO
	sem_post(&cont_new);
}

void terminar_procesos(){
	while(1){
		// Semaforo contador de cuantos procesos hay en exit
		sem_wait(&cont_exit);
		t_pcb* pcb = retirar_pcb_de(procesosExit, &mutex_exit);

		// Liberar recursos asignados
		liberar_todos_recursos(pcb);

		// Avisar a memoria para liberar estructuras		
			// Primero le envio el codigo de solicitid de fin + el pid a destruir por "parametro"
		
		t_buffer* buffer = crear_buffer_nuestro();

		buffer->codigo = SOLICITUD_FIN_PROCESO_MEMORIA;
		
		buffer_write_uint32(buffer, pcb->cde->pid);

		enviar_buffer(buffer, socket_memoria);
		
		destruir_buffer_nuestro(buffer);
		
		// Ahora recibo que la destruccion fue EXITOSO	
		op_code codigo = recibir_codigo(socket_memoria);
		if(codigo == FIN_EXITOSO_PROCESO_MEMORIA){
				// Si fue exitoso liberar memoria => avisar a consola para matar conexion, etc
			op_code terminar_consola = FIN_PROCESO_CONSOLA;
			enviar_codigo(pcb->socket_consola, terminar_consola);
		
			destruir_pcb(pcb);
		
		}
		else
			log_info(logger, "Error al liberar estructuras de memoria al terminar proceso %d", pcb->cde->pid);
		
	}
}
// FIN TRANSICIONES LARGO PLAZO ---------------------------------------------------------
// ---------------------------------- FIN TRANSICIONES ----------------------------------

// PLANIFICADORES -----------------------------------------------------------------------
void planificadores(){
	planificadorLargoPlazo();
	planificadorCortoPlazo();
}

void planificadorLargoPlazo(){
	pthread_t hiloNew;
	pthread_t hiloExit;

	pthread_create(&hiloNew, NULL, (void *) enviar_de_new_a_ready, NULL);
	pthread_create(&hiloExit, NULL, (void *) terminar_procesos, NULL);

	pthread_detach(hiloNew);
	pthread_detach(hiloExit);
}

void planificadorCortoPlazo(){ 
	pthread_t poner_en_ejecucion;
	pthread_create(&poner_en_ejecucion, NULL, (void *) enviar_de_ready_a_exec, NULL);
	pthread_detach(poner_en_ejecucion);

	// Solo tiene 1 hilo el corto plazo
	// enviar_de_exec_a_pseudoblock se llama cuando se bloquea un proc
	// enviar_de_exec_a_ready solo para YIELD
	// enviar_de_pseudoblock_a_ready cuando se libera un recurso, etc (se desbloquea el proc)
	// enviar_de_exec_a_exit cuando finaliza x error o por EXIT ;)

}

// FIN PLANIFICADORES -------------------------------------------------------------------


// UTILS RECURSOS -----------------------------------------------------------------------
t_recurso* inicializar_recurso(char* nombre_recu, int instancias_tot){
	t_recurso* recurso = malloc(sizeof(t_recurso));
	int tam = 0;

	while(nombre_recu[tam] != NULL)
		tam++;

	recurso -> nombre_recurso  = malloc(tam);
	strcpy(recurso -> nombre_recurso, nombre_recu);

	recurso -> instancias_disponibles = instancias_tot;
	recurso -> cola_de_espera = queue_create();
	
	return recurso;
}

void inicializar_recurso_nulo(){
	recurso_nulo = inicializar_recurso("NULO", 0);
}

int asignar_instancia_recurso(char* nombre_recurso_a_asignar, t_pcb* pcb){
	t_recurso* recurso = encontrar_recurso_por_nombre(nombre_recurso_a_asignar);

	if(strcmp(recurso -> nombre_recurso, recurso_nulo -> nombre_recurso) == 0)
		return RECURSO_INVALIDO;
	// Si sigue de largo de este if(el de arriba) es porque es distinto de recurso_nulo

	// Por mas que no asigne, tengo que restar las instancias ya que seria como "reservarlo"	

	if(recurso->instancias_disponibles <= 0){
		list_add(recurso->cola_de_espera, pcb);
		recurso->instancias_disponibles--;
		return EN_ESPERA;
	}
	// Si sigue de largo de este if(el de arriba) es porque hay instancias disponibles

	recurso->instancias_disponibles--;
	int tam = 0;
	while(recurso -> nombre_recurso[tam] != NULL)
		tam++;

	// Necesito que &nombre != &(recurso -> nombre_recurso), porque despues hago free(nombre)
	char* nombre = malloc(tam);
	strcpy(nombre, recurso -> nombre_recurso);
	list_add(pcb->recursos_asignados, nombre);
	return RECURSO_ASIGNADO;
}

int liberar_instancia_recurso(char* nombre_recurso_a_liberar, t_pcb* pcb){
	t_recurso* recurso = encontrar_recurso_por_nombre(nombre_recurso_a_liberar);

	// Suponemos que todos los recursos a liberar fueron asignados antes ya que no
	// aclara que sucederia en caso de que se quiera liberar un recurso que no
	// fue asignado con anterioridad

	if(strcmp(recurso -> nombre_recurso, recurso_nulo -> nombre_recurso) == 0)
		return RECURSO_INVALIDO;
	else{
		recurso->instancias_disponibles++;
		sacar_recurso(pcb -> recursos_asignados, nombre_recurso_a_liberar);
		
		// El igual va ya que si era (-1), con el ++ de arriba lo transforme en 0 y
		// => hay un recurso que estaba solicito el proceso que se libero
		if(recurso->instancias_disponibles <= 0){
			t_pcb* pcb_a_desbloquear = queue_pop(recurso -> cola_de_espera);
			asignar_instancia_recurso(nombre_recurso_a_liberar, pcb_a_desbloquear);
			enviar_de_pseudoblock_a_ready(pcb_a_desbloquear);
		}
		return RECURSO_LIBERADO;
	}
}

void sacar_recurso(t_list* recursos_asignados, char* recurso_a_sacar){
	for(int i = 0; i < list_size(recursos_asignados); i++){
		char* recurso = list_get(recursos_asignados, i);
		if(strcmp(recurso, recurso_a_sacar) == 0){
			recurso = list_remove(recursos_asignados, i);
			free(recurso);
			return RECURSO_LIBERADO;
		}
	}
}

void liberar_todos_recursos(t_pcb* pcb){
	int cant_rec_asignados = list_size(pcb -> recursos_asignados);
	for(int i = 0; i < cant_rec_asignados; i++){
		char* recurso_a_liberar = list_get(pcb -> recursos_asignados, i);
		liberar_instancia_recurso(recurso_a_liberar, pcb);
	}
}

t_recurso* encontrar_recurso_por_nombre(char* nombre_recurso_a_obtener){
	// En caso de que no exista el recurso (si sale del for) devuelve el recurso_nulo
	for(int i=0; i< list_size(config_kernel.recursos) ; i++){
		t_recurso* recurso = list_get(config_kernel.recursos,i);
		if(strcmp(nombre_recurso_a_obtener, recurso->nombre_recurso) == 0)
			return recurso;
	}
	return recurso_nulo;
}

// FIN UTILS RECURSOS -------------------------------------------------------------------


// UTILS PARA SACAR DE READY ------------------------------------------------------------
t_pcb* retirar_pcb_de_ready_segun_algoritmo(){
	if (strcmp(config_kernel.algoritmo, "FIFO") == 0)
		return elegido_por_FIFO();
	else if(strcmp(config_kernel.algoritmo, "HRRN") == 0)
		return elegido_por_HRRN();
	
	log_info(logger, "Algoritmo no reconocido.");
	exit(3);
}

t_pcb* elegido_por_FIFO(){
	return retirar_pcb_de(procesosReady, &mutex_ready);
}

t_pcb* elegido_por_HRRN(){
	pthread_mutex_lock(&mutex_ready);
	list_sort(procesosReady->elements, (void*)maximo_response_ratio);
	pthread_mutex_unlock(&mutex_ready);

	t_pcb* pcb_elegido = retirar_pcb_de(procesosReady, &mutex_ready);
	
	return pcb_elegido;
}
// FIN UTILS PARA SACAR DE READY --------------------------------------------------------


// UTILS HRRN ---------------------------------------------------------------------------
uint32_t calcular_estimacion_proxima_rafaga(t_pcb* pcb){
	time_t tiempo_actual = time(NULL);
	uint32_t estimacion_proxima_rafaga = -1;
	double tiempo_ejecucion_actual = difftime(tiempo_actual, pcb->tiempo_llegada_exec);

	estimacion_proxima_rafaga = (config_kernel.alpha_hrrn * tiempo_ejecucion_actual) + (1 - config_kernel.alpha_hrrn) * pcb->estimado_prox_rafaga;

	return estimacion_proxima_rafaga;
}

double calcular_response_ratio(t_pcb *pcb){
	double wait_time = -1; // cuanto tiempo espero en ready
	uint32_t expected_next_round = -1; // cuanto tiempo se estima que va a tardar la proxima rafaga de ejecucion
	double rr = -1; // response ratio

	time_t tiempo_actual = time(NULL);

	wait_time = difftime(tiempo_actual, pcb->tiempo_llegada_ready); //funcion q resta dos time_t y devuelve double
	expected_next_round = calcular_estimacion_proxima_rafaga(pcb);

	rr = (pcb->estimado_prox_rafaga + wait_time * 1000) / pcb->estimado_prox_rafaga;
	return rr;
}

bool maximo_response_ratio(t_pcb* pcb1, t_pcb* pcb2){
	if (calcular_response_ratio(pcb1) > calcular_response_ratio(pcb2))
		return true;
	else
		return false;
}
// FIN UTILS HRRN -----------------------------------------------------------------------


// UTLIS CREAR --------------------------------------------------------------------------
t_cde* crear_cde(t_list* instrucciones){
	t_cde* cde = malloc(sizeof(t_cde));

	pthread_mutex_lock(&mutex_pid_a_asignar);
	cde->pid = pid;
	pid++;
	pthread_mutex_unlock(&mutex_pid_a_asignar);
	
	cde->lista_de_instrucciones = instrucciones;
	cde->program_counter = 0;
	cde->registros_cpu = inicializar_registros();
    cde->tabla_segmentos = list_create();

	return cde;
}

t_pcb* crear_pcb(t_list *instrucciones, int socket_con){
	t_pcb* pcb = malloc(sizeof(t_pcb));
	
	pcb->cde = crear_cde(instrucciones);
	
	pcb->estimado_prox_rafaga = config_kernel.est_inicial;
	pcb->tiempo_llegada_ready = -1;

	pcb->archivos_abiertos = list_create();
	pcb->recursos_asignados = list_create();
	pcb->socket_consola = socket_con;
	return pcb;
}


t_registros inicializar_registros(){
	t_registros registros;

	for(int i=0;i<4;i++){
		registros.AX[i] = 'x';
		registros.BX[i] = 'x';
		registros.CX[i] = 'x';
		registros.DX[i] = 'x';
	}

	registros.AX[4] = '\0';
	registros.BX[4] = '\0';
	registros.CX[4] = '\0';
	registros.DX[4] = '\0';

	
	for(int i=0;i<8;i++){
		registros.EAX[i] = 'x';
		registros.EBX[i] = 'x';
		registros.ECX[i] = 'x';
		registros.EDX[i] = 'x';
	}
	registros.EAX[8] = '\0';
	registros.EBX[8] = '\0';
	registros.ECX[8] = '\0';
	registros.EDX[8] = '\0';


	for(int i=0;i<16;i++){
		registros.RAX[i] = 'x';
		registros.RBX[i] = 'x';
		registros.RCX[i] = 'x';
		registros.RDX[i] = 'x';
	}
	registros.RAX[16] = '\0';
	registros.RBX[16] = '\0';
	registros.RCX[16] = '\0';
	registros.RDX[16] = '\0';

	return registros;
}
// FIN UTILS CREAR ----------------------------------------------------------------------


// UTILS INSTRUCCIONES ------------------------------------------------------------------
void evaluar_instruccion(){
	// La evalua cuando vuelve de CPU
	int indice = pcb_en_ejecucion->cde->program_counter - 1;
	t_list* lista_instrucciones = pcb_en_ejecucion->cde->lista_de_instrucciones;
	t_instruction* instruccion_actual = list_get(lista_instrucciones, indice);
	switch(instruccion_actual->instruccion){
		case F_READ:
			atender_f_read(instruccion_actual);
			break;
		case F_WRITE:
			atender_f_write(instruccion_actual);
			break;
		case F_TRUNCATE:
			atender_f_truncate(instruccion_actual);
			break;
		case F_SEEK:
			t_archivo_kernel* archivo = devolver_archivo_x_nombre(instruccion_actual->string1);
			archivo->puntero = instruccion_actual->numero1;
			log_info(logger, "PID: %d - Actualizar puntero Archivo: %s - Puntero %d", pcb_en_ejecucion->cde->pid, archivo->nombre_archivo, archivo->puntero);
			sem_post(&necesito_enviar_cde);
			break;
		case CREATE_SEGMENT:
			// Se mantiene el proc en exec
			enviar_solicitud_create_segment(instruccion_actual);
			analizar_respuesta_create_segment();
			break;
		case DELETE_SEGMENT:
			// Se mantiene el proc en exec
			enviar_solicitud_delete_segment(instruccion_actual);
			analizar_respuesta_delete_segment(instruccion_actual->numero1);
			break;
		case IO:
			log_info(logger,"PID: %d - Bloqueado por: %s", pcb_en_ejecucion->cde->pid, "IO"); // OBLIGATORIO
			log_info(logger,"PID: %d - Ejecuta IO: %d", pcb_en_ejecucion->cde-> pid, instruccion_actual->numero1); // OBLIGATORIO
			administrar_io(pcb_en_ejecucion, instruccion_actual->numero1);
			break;
		case WAIT:
			switch(asignar_instancia_recurso(instruccion_actual -> string1, pcb_en_ejecucion)){
				case RECURSO_INVALIDO:
					enviar_de_exec_a_exit("INVALID_RESOURCE");
					break;
				case EN_ESPERA:
					enviar_de_exec_a_psedudoblock(instruccion_actual -> string1);
					break;
				case RECURSO_ASIGNADO:
					t_recurso* recurso = encontrar_recurso_por_nombre(instruccion_actual->string1);
					log_info(logger, "PID: %d - Wait: %s - Instancias: %d", pcb_en_ejecucion->cde->pid, recurso -> nombre_recurso, recurso -> instancias_disponibles);
					sem_post(&necesito_enviar_cde);
					break;
			}
			break;
		case SIGNAL:
			switch(liberar_instancia_recurso(instruccion_actual -> string1, pcb_en_ejecucion)){
				case RECURSO_INVALIDO:
					enviar_de_exec_a_exit("INVALID_RESOURCE");
					break;
				case RECURSO_LIBERADO:
					t_recurso* recurso = encontrar_recurso_por_nombre(instruccion_actual->string1);
					log_info(logger, "PID: %d - Signal: %s - Instancias: %d", pcb_en_ejecucion -> cde -> pid, recurso -> nombre_recurso, recurso -> instancias_disponibles);
					sem_post(&necesito_enviar_cde);
					break;
			}
			break;
		case F_OPEN:
		// Verificar tabla global de archivos abiertos
				// Si no se encuentra el archivo a leer, se consulta a FS si existe el archivo o no
					// Si no existe => se le solicita a FS que lo cree con tam 0
					// Si existe =>  ???
					// En ambos casos se agrega a la tabla global de archivos abiertos y a la tabla de archivos abiertos del proceso
				// Si se encuentra en la tabla global de archivos abiertos, se le agrega una entrada a la tabla de archivos abiertos por proceso con el puntero en la pos 0.
			log_info(logger, "Entre a f_open");
			atender_f_open(instruccion_actual->string1);
			sem_post(&necesito_enviar_cde);
			break;
		case F_CLOSE:
			cerrar_archivo(instruccion_actual->string1);
			sem_post(&necesito_enviar_cde);
			break;
		case YIELD:
			enviar_de_exec_a_ready();
			break;
		case EXIT:
			enviar_de_exec_a_exit("SUCCES");
			break;
		default:
			log_info(logger, "Error: Instruccion no reconocida.");
			break;
	}
}


// UTILS INSTRUCCIONES FS ---------------------------------------------------------------
void atender_f_write(t_instruction* instruccion_actual){
	// Solicita al fs q escriba ene l archivo desde la direccion fisica pasada por parametro n cantidad de bytes indicada
	// Bloqueado hasta que el FS informe finalizacion de la operacion
	// SIEMPRE SE PASAN PUNTEROS Y TAMANIOS VALIDOS
	// lo que lee fs: 
	//	char *nombre_archivo_escribir = buffer_read_string(buffer);
	//	uint32_t posicion_escribir = buffer_read_uint32(buffer);
	//	uint32_t tamanio_escribir = buffer_read_uint32(buffer);
	//	uint32_t direccion_memoria_escribir = buffer_read_uint32(buffer);
	//	uint32_t pid_escribir = buffer_read_uint32(buffer);

	t_buffer* buffer = crear_buffer_nuestro();
	buffer->codigo = SOLICITUD_ESCRIBIR_ARCHIVO;

	t_archivo_kernel* archivo_a_escribir = devolver_archivo_x_nombre(instruccion_actual->string1);

	
	buffer_write_string(buffer, instruccion_actual->string1); // escribo el nombre de archivo a escribir
	buffer_write_uint32(buffer, archivo_a_escribir->puntero); // verificar que puntero sea uint32!!!!!
	// escribi la direcciona memoria donde hay que escribir
	buffer_write_uint32(buffer, instruccion_actual->numero2); // escribo el tamanio a escribir
	buffer_write_uint32(buffer, instruccion_actual->numero1); // escribo la posicion a escribir

	buffer_write_uint32(buffer, pcb_en_ejecucion->cde->pid); // escribo el pid del procesos q solicito f_write;

	if(cant_operaciones_mem_fs == 0)
		sem_wait(&operaciones_fs_mem);
	cant_operaciones_mem_fs++;

	enviar_buffer(buffer, socket_file_system);
	destruir_buffer_nuestro(buffer);

	log_info(logger, "PID: %d - Escribir Archivo: %s - Puntero %d - Direccion Memoria %d - Tamanio %d", pcb_en_ejecucion->cde->pid, archivo_a_escribir->nombre_archivo, archivo_a_escribir->puntero, instruccion_actual->numero1, instruccion_actual->numero2);

	//bloquea proceso hasta que fs nos avise q finalizo operacion
	t_pcb* pcb_a_bloquear = pcb_en_ejecucion;
	enviar_de_exec_a_psedudoblock(instruccion_actual->string1); // INDIRECTAMTENTE POSTEO necesito_enviar_cde
	
	// Espera la rta de fs pero mientras el kernel cambia de proceso
	pthread_t esperar_fin_fs;
	pthread_create(&esperar_fin_fs, NULL, (void* ) bloquear_pcbs_por_f_write, (void* ) pcb_a_bloquear);
	pthread_detach(esperar_fin_fs);
}

void bloquear_pcbs_por_f_write(void* args){
	t_pcb* pcb_a_desbloquear = (t_pcb*) args;
	
	op_code codigo = recibir_codigo(socket_file_system);
	
	log_info(logger, "Codigo recibido en bloquear_pcbspor_f_write: %d", codigo);

	if(codigo == F_WRITE_OK){
		enviar_de_pseudoblock_a_ready(pcb_a_desbloquear);
		cant_operaciones_mem_fs--;
		if(cant_operaciones_mem_fs == 0)
			sem_post(&operaciones_fs_mem);
	}
	else
		log_info(logger, " Error, fallo F_write recibido");
}

t_archivo_kernel* devolver_archivo_x_nombre(char* nombre_a_buscar){
	for(int i = 0; i < list_size(archivos_abiertos_global); i++){
		t_archivo_kernel* archivo = list_get(archivos_abiertos_global, i);
		if(strcmp(nombre_a_buscar, archivo->nombre_archivo) == 0)
			return archivo;
	}
	return archivo_nulo;
}

void inicializar_archivo_nulo(){
	archivo_nulo = malloc(sizeof(t_archivo_kernel));
	archivo_nulo->nombre_archivo = malloc(4);
	strcpy(archivo_nulo->nombre_archivo, "NULO");
}

void cerrar_archivo(char* nombre_a_cerrar){
	// Lo tengo que sacar de la lista de archivos abiertos de cada PCB
	// Si no hay ningun proceso en cola de espera, lo saco de la lista global de archivos abiertos
	sacar_archivo_de_lista(pcb_en_ejecucion->archivos_abiertos, nombre_a_cerrar);
	t_archivo_kernel* archivo = devolver_archivo_x_nombre(nombre_a_cerrar);
	if(queue_is_empty(archivo->cola_de_espera)){
		// Si nadie esta esperando que liberen este archivo
		list_remove_element(archivos_abiertos_global, archivo);
		log_info(logger, "PID: %d - Cerrar Archivo: %s", pcb_en_ejecucion->cde->pid, archivo->nombre_archivo);
	}else{
		t_pcb* pcb = queue_pop(archivo->cola_de_espera);
		list_add(pcb->archivos_abiertos, archivo);
		archivo->puntero = 0;
		enviar_de_pseudoblock_a_ready(pcb);
	}
}

void sacar_archivo_de_lista(t_list* archivos, char* nombre){
	for(int i = 0; i < list_size(archivos); i++){
		t_archivo_kernel* archivo = list_get(archivos, i);
		if(strcmp(nombre, archivo->nombre_archivo) == 0)
			list_remove(archivos, i);
	}
}

void atender_f_truncate(t_instruction* instruccion){
	// Solicitar al FS q actualice al nuevo tam pasado x param
	// Bloq proc hasta que fs avise informe de la finalizacion del proc
	// lo que lee fs1: char* nombre_archivo_truncar = buffer_read_string(buffer);
	// lo que lee fs2: uint32_t tamanio_truncar = buffer_read_uint32(buffer);
		
	t_buffer* buffer = crear_buffer_nuestro();
	buffer->codigo = SOLICITUD_TRUNCAR_ARCHIVO;

	buffer_write_string(buffer, instruccion->string1); // escribo el nombre de archivo a truncar
	buffer_write_uint32(buffer, instruccion->numero1); // escribo el tamanio nuevo del archivo

	if(cant_operaciones_mem_fs == 0)
		sem_wait(&operaciones_fs_mem);
	cant_operaciones_mem_fs++;


	enviar_buffer(buffer, socket_file_system);

	destruir_buffer_nuestro(buffer);

	log_info(logger, "PID: %d - Archivo: %s - Tamanio: %d", pcb_en_ejecucion->cde->pid, instruccion->string1, instruccion->numero1);

	//bloqueadr proceso hasta que fs nos avise q finalizo operacion
	t_pcb* pcb_a_bloquear = pcb_en_ejecucion;

	enviar_de_exec_a_psedudoblock(instruccion->string1);

	log_info(logger, "PID del pcb a enviar a ready %d", pcb_a_bloquear->cde->pid);

	pthread_t esperar_fin_fs;
	pthread_create(&esperar_fin_fs, NULL, (void* ) bloquear_pcbs_por_f_truncate, (void* ) pcb_a_bloquear);
	pthread_detach(esperar_fin_fs);


}

void bloquear_pcbs_por_f_truncate(void* args){
	t_pcb* pcb_a_desbloquear = (t_pcb*) args;
	
	log_info(logger, "PID del pcb a enviar a ready %d", pcb_a_desbloquear->cde->pid);

	op_code codigo = recibir_codigo(socket_file_system);
	
	log_info(logger, "Codigo recibido en bloquear_pcbspor_f_truncate: %d", codigo);

	if(codigo == F_TRUNCATE_OK){
		enviar_de_pseudoblock_a_ready(pcb_a_desbloquear);
		cant_operaciones_mem_fs--;
		if(cant_operaciones_mem_fs == 0)
			sem_post(&operaciones_fs_mem);
	}
	else
		log_info(logger, " Error, fallo F_truncate recibido");
}

void atender_f_open(char* nombre_a_abrir){
	// Verificar tabla global de archivos abiertos
				// Si no se encuentra el archivo a leer, se consulta a FS si existe el archivo o no
					// Si no existe => se le solicita a FS que lo cree con tam 0
					// Si existe =>  ???
					// En ambos casos se agrega a la tabla global de archivos abiertos y a la tabla de archivos abiertos del proceso
				// Si se encuentra en la tabla global de archivos abiertos, se le agrega una entrada a la tabla de archivos abiertos por proceso con el puntero en la pos 0.
	t_archivo_kernel* archivo = devolver_archivo_x_nombre(nombre_a_abrir);
	log_info(logger, "PID: %d - Abrir Archivo: %s", pcb_en_ejecucion->cde->pid, nombre_a_abrir);
	

	if(strcmp(archivo->nombre_archivo, archivo_nulo->nombre_archivo) == 0){
		// El archivo no se encuentra en la tabla global de archivos abiertos
		t_buffer* buffer = crear_buffer_nuestro();
		buffer->codigo = SOLICITUD_ABRIR_ARCHIVO;
		buffer_write_string(buffer, nombre_a_abrir);
		enviar_buffer(buffer, socket_file_system);

		destruir_buffer_nuestro(buffer);
		buffer = recibir_buffer(socket_file_system);
		if(buffer->codigo == F_OPEN_OK){
			archivo = inicializar_archivo(nombre_a_abrir);
			list_add(pcb_en_ejecucion->archivos_abiertos, archivo);
			list_add(archivos_abiertos_global, archivo);
		}
	}else{
		// El archivo se encuentra en la tabla global de archivos abiertos => ya lo esta usando otro proceso
		queue_push(archivo->cola_de_espera, pcb_en_ejecucion);
		enviar_de_exec_a_psedudoblock(nombre_a_abrir);
	}
}

void atender_f_read(t_instruction* instruccion){
		// Solicitar al fs q lea desde el puntero del arch pasado x param n cantidad de bytes indicada
		/// Grabarlo en al direccion fisica pasada x param
		// En block hasta que el fs informe que finaliza la operacion
		// SIEMPRE SE PASAN PUNTEROS Y TAMANIOS VALIDOS
		// lo que lee fs: 
		//char *nombre_archivo_leer = buffer_read_string(buffer);
		//	uint32_t posicion_leer = buffer_read_uint32(buffer);
		//	uint32_t tamanio_leer = buffer_read_uint32(buffer);
		//	uint32_t direccion_memoria_leer = buffer_read_uint32(buffer);
		//	uint32_t pid_leer = buffer_read_uint32(buffer);

		t_buffer* buffer = crear_buffer_nuestro;

		buffer->codigo = SOLICITUD_LEER_ARCHIVO;

		buffer_write_string(buffer, instruccion->string1); // escribe el nombre de archivo
		
		t_archivo_kernel* archivo = devolver_archivo_x_nombre(instruccion->string1); 
		
		buffer_write_uint32(buffer, archivo->puntero); //escribo la posicion a leer

		buffer_write_uint32(buffer, instruccion->numero2); // escribo tamanio a leer
		buffer_write_uint32(buffer, instruccion->numero1); // escribo direcc de memoria a leer
		
		buffer_write_uint32(buffer, pcb_en_ejecucion->cde->pid); // esvribo el pid

		log_info(logger, "PID: %d - Leer Archivo: %s - Puntero %d - Direccion Memoria %d - Tamanio %d", pcb_en_ejecucion->cde->pid, archivo->nombre_archivo, archivo->puntero, instruccion->numero1, instruccion->numero2);

		if(cant_operaciones_mem_fs == 0)
			sem_wait(&operaciones_fs_mem);
		cant_operaciones_mem_fs++;
		
		enviar_buffer(buffer, socket_file_system);

		t_pcb* pcb_a_desbloquear = pcb_en_ejecucion;

		// Aca lo "BLOQUEO"
		enviar_de_exec_a_psedudoblock(instruccion->string1); // INDIRECTAMTENTE POSTEO necesito_enviar_cde

		pthread_t esperar_fin_fs;
		pthread_create(&esperar_fin_fs, NULL, (void* ) bloquear_pcbs_por_f_read, (void* ) pcb_a_desbloquear);
		pthread_detach(esperar_fin_fs);
}

void bloquear_pcbs_por_f_read(void* args){
	t_pcb* pcb_a_desbloquear = (t_pcb*) args;
	
	t_buffer* buffer = recibir_buffer(socket_file_system);
	
	log_info(logger, "Codigo recibido en bloquear_f_read: %d", buffer->codigo);
	if(buffer->codigo == F_OPEN_OK){
		enviar_de_pseudoblock_a_ready(pcb_a_desbloquear);
		cant_operaciones_mem_fs--;
		if(cant_operaciones_mem_fs == 0)
			sem_post(&operaciones_fs_mem);
	}
	else
		log_info(logger, " Error, fallo F_truncate recibido");
	destruir_buffer_nuestro(buffer);
}

t_archivo_kernel* inicializar_archivo(char* nombre){
	t_archivo_kernel* a = malloc(sizeof(t_archivo_kernel));

	a->cola_de_espera = queue_create();
	pthread_mutex_init(&a->mutex_asignado, NULL);
	a->puntero = 0;
	
	int tam = 0;
	while(nombre[tam] != NULL)
		tam++;
	a->nombre_archivo = malloc(tam+1);

	strcpy(a->nombre_archivo, nombre);
	return a;
}
// FIN UTILS INSTRUCCIONES FS -----------------------------------------------------------

void enviar_solicitud_create_segment(t_instruction* instruccion){
	// Enviar solicitud
	t_buffer* buffer = crear_buffer_nuestro();
	buffer->codigo = SOLICITUD_CREATE_SEGMENT; // le envio el pid, el segm_id y el tam del seg
	buffer_write_uint32(buffer, pcb_en_ejecucion->cde->pid);
	buffer_write_uint32(buffer, instruccion->numero1); // segm_id
	buffer_write_uint32(buffer, instruccion->numero2); // tam_segm
	
	enviar_buffer(buffer, socket_memoria);

	destruir_buffer_nuestro(buffer);
}

void analizar_respuesta_create_segment(){
	t_buffer* buffer_recibido = recibir_buffer(socket_memoria);
	switch (buffer_recibido->codigo){
		case CREATE_SEGMENT_EXITOSO:
			t_segmento* segm_a_agregar = buffer_read_segmento(buffer_recibido);
			destruir_buffer_nuestro(buffer_recibido);
			list_add(pcb_en_ejecucion->cde->tabla_segmentos, segm_a_agregar);
			log_info(logger, "PID: %d - Crear Segmento - Id: %d - Tamanio: %d", pcb_en_ejecucion->cde->pid, segm_a_agregar->id, segm_a_agregar->tamanio_segmento); // OBLIGATORIO			
			sem_post(&necesito_enviar_cde);
			break;
		case CREATE_SEGMENT_COMPACTACION:
			// Validar que no haya operaciones ejecutando entre FS y memoria


			log_info(logger, "Compactacion: Esperando Fin de Operaciones de FS");
			sem_wait(&operaciones_fs_mem);
			log_info(logger, "Compactacion: Se solicito compactacion");

			op_code permiso_compact = COMPACTACION_OK;
			enviar_codigo(socket_memoria, permiso_compact);
			destruir_buffer_nuestro(buffer_recibido);

			// Buffer con todos los segmentos
			buffer_recibido = recibir_buffer(socket_memoria);

			uint32_t cant_listas = buffer_read_uint32(buffer_recibido);

			for(int i = 0; i < cant_listas; i++){
				t_tabla_segmentos_por_proceso* t_s_x_proceso = buffer_read_tabla_segmentos_x_procesos(buffer_recibido);
				t_pcb* pcb = encontrar_pcb_x_pid(t_s_x_proceso->pid);
				
				list_destroy(pcb->cde->tabla_segmentos);
				pcb->cde->tabla_segmentos = t_s_x_proceso->tabla_segmentos;
			}

			log_info(logger, "Compactacion: se finalizo el proceso de compactacion"); // OBLIGATORIO
			sem_post(&operaciones_fs_mem);
			sem_post(&necesito_enviar_cde);
			break;
		case CREATE_SEGMENT_FALLIDO:
			// por out of memory entonces se va a exit
			enviar_de_exec_a_exit("Out of memory");
			destruir_buffer_nuestro(buffer_recibido);
			break;
	}
}

void destruir_tabla_segmentos_por_proceso(t_tabla_segmentos_por_proceso* tabla_segmentos_por_proceso_a_destruir){
	list_destroy(tabla_segmentos_por_proceso_a_destruir->tabla_segmentos);
	free(tabla_segmentos_por_proceso_a_destruir);
	return;
}

t_pcb* encontrar_pcb_x_pid(uint32_t pid_a_buscar){
	for(int i = 0; i < list_size(lista_pcb_global); i++){
		t_pcb* pcb = list_get(lista_pcb_global, i);
		if(pcb->cde->pid == pid_a_buscar)
			return pcb;
	}
}

void enviar_solicitud_delete_segment(t_instruction* instruccion){
	// Enviar solicitud
	t_buffer* buffer = crear_buffer_nuestro();
	buffer->codigo = SOLICITUD_DELETE_SEGMENT; 
	// le envio el pid del procesos q tiene q eliminar un segm y el id del segmento a eliminar de ese proceso
	buffer_write_uint32(buffer, pcb_en_ejecucion->cde->pid);
	buffer_write_uint32(buffer, instruccion->numero1);
	
	enviar_buffer(buffer, socket_memoria);

	destruir_buffer_nuestro(buffer);
}

void analizar_respuesta_delete_segment(uint32_t id_segmento){
	t_buffer* buffer_recibido = recibir_buffer(socket_memoria);
	switch (buffer_recibido->codigo){
		case DELETE_SEGMENT_EXITOSO:
			pcb_en_ejecucion->cde->tabla_segmentos = buffer_read_tabla_segmentos(buffer_recibido);
			log_info(logger, "PID: %d - Eliminar Segmento - Id Segmento: %d", pcb_en_ejecucion->cde->pid, id_segmento);
			sem_post(&necesito_enviar_cde);
			break;
		case DELETE_SEGMENT_FALLIDO:
			log_info(logger, "Delete segment fallido");
			break;
		default: 
			log_info(logger, "Entre a default en analizar_respuesta_delete_segment");
	}
	destruir_buffer_nuestro(buffer_recibido);
}

void administrar_io(t_pcb* pcb_a_dormir, int tiempo_siesta){
	t_io* datos_io = malloc(sizeof(t_io));

	datos_io -> pcb = pcb_a_dormir;
	datos_io -> tiempo = tiempo_siesta;

	pthread_t bloquear_proceso_por_io;
	pthread_create(&bloquear_proceso_por_io, NULL, (void *) dormir_proceso, (void*) datos_io);
	pthread_detach(bloquear_proceso_por_io);
}

void dormir_proceso(void* args){
	t_io* datos_io = (t_io*) args;
	enviar_de_exec_a_psedudoblock("IO");
	usleep(datos_io->tiempo * 1000000);	
	enviar_de_pseudoblock_a_ready(datos_io -> pcb);
}

// FIN UTILS INSTRUCCIONES --------------------------------------------------------------
