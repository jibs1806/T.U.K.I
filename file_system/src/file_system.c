#include "../includes/file_system.h"


int main(void){

	levantar_modulo();
	crearEstructuras();
	
	manejar_conexion_con_kernel();
	/*
	crearArchivo("Videogames");

	uint32_t tam = 0;
	char *nombre_archivo_escribir = "Videogames";
	uint32_t posicion_escribir = 0;
	uint32_t tamanio_escribir = 16;
	uint32_t direccion_memoria_escribir = 300;
	uint32_t pid_escribir = 15;

	truncarArchivo("Videogames", 256);
	atender_solicitud_f_write(nombre_archivo_escribir, posicion_escribir, tamanio_escribir, direccion_memoria_escribir, pid_escribir);
	*/
	while(1);
	return 0;
}

// SUBPROGRAMAS



int escribirArchivo(char* nombre, int pos, int size, void*datos){
	int offset=0;
		strcpy(buscado,nombre);
		FCB* seleccionado = (FCB*)list_find(fcb_list,igualBuscado);
		int tam_restante = seleccionado->file_size - pos ;
			if(size < tam_restante){
				div_t start_pos = div(pos, superbloque.block_size);
				int nro_bloque = start_pos.quot;
				int tam_escrito = superbloque.block_size - start_pos.rem;
				void * bloque_destino;
				if(nro_bloque == 0){
						bloque_destino =archivobloques_pointer +seleccionado->direct_pointer*superbloque.block_size+start_pos.rem;
						usleep(config_file_system.retardo_acceso_bloque);
						memcpy(bloque_destino,datos,tam_escrito);
						offset=offset+tam_escrito;
						tam_restante = size- tam_escrito;
					    log_info(logger,"Escribir Archivo: %s - Puntero: %d - Memoria: %p - Tamaño: %d", seleccionado->file_name,nro_bloque,bloque_destino,tam_escrito);

					}
				void * bloque_punteros =archivobloques_pointer +seleccionado->indirect_pointer*superbloque.block_size;
				uint32_t* destination = (uint32_t*)bloque_punteros;
					if(nro_bloque>0){
						usleep(config_file_system.retardo_acceso_bloque);
						bloque_destino =archivobloques_pointer +destination[nro_bloque]*superbloque.block_size+start_pos.rem;
						usleep(config_file_system.retardo_acceso_bloque);
						memcpy(bloque_destino,datos,tam_escrito);
						offset=offset+tam_escrito;
						tam_restante = size- tam_escrito;
					    log_info(logger,"Escribir Archivo: %s - Puntero: %d - Memoria: %p - Tamaño: %d", seleccionado->file_name,nro_bloque,bloque_destino,tam_escrito);
					}
					if(tam_restante>0){
							int bloques_restantes= tam_restante/superbloque.block_size;
								  for(int i = nro_bloque;i<bloques_restantes;i++){
									usleep(config_file_system.retardo_acceso_bloque);
									  bloque_destino =archivobloques_pointer +destination[i]*superbloque.block_size;
										usleep(config_file_system.retardo_acceso_bloque);
									  memcpy(bloque_destino,datos+offset,superbloque.block_size);
									  offset=offset+superbloque.block_size;
									  tam_restante=tam_restante-superbloque.block_size;
									    log_info(logger,"Escribir Archivo: %s - Puntero: %d - Memoria: %p - Tamaño: %d", seleccionado->file_name,i,bloque_destino,superbloque.block_size);

								  }
									usleep(config_file_system.retardo_acceso_bloque);
								  bloque_destino =archivobloques_pointer +destination[bloques_restantes]*superbloque.block_size;
									usleep(config_file_system.retardo_acceso_bloque);
								  memcpy(bloque_destino,datos+offset,tam_restante);
								    log_info(logger,"Escribir Archivo: %s - Puntero: %d - Memoria: %p - Tamaño: %d", seleccionado->file_name,bloques_restantes+1,bloque_destino,tam_restante);

						}
					int sync = msync(archivobloques_pointer, archivobloques_size, MS_SYNC);
					if(sync == -1) printf("Error syncing archivobloques");
				return 0;
			} else return -1;

}

// lee archivo desde pos el size pedido
void * leerArchivo(char* nombre, int pos, int size){
	void* leido=malloc(size);
	int offset=0;
	strcpy(buscado,nombre);
	FCB* seleccionado = (FCB*)list_find(fcb_list,igualBuscado);
	int tam_restante = seleccionado->file_size - pos ;
	if(size < tam_restante){
	div_t start_pos = div(pos, superbloque.block_size);
	int nro_bloque = start_pos.quot;
	int tam_leido = superbloque.block_size - start_pos.rem;
	void * bloque_leido;
	if(nro_bloque == 0){
		usleep(config_file_system.retardo_acceso_bloque);
		bloque_leido =archivobloques_pointer +seleccionado->direct_pointer*superbloque.block_size+start_pos.rem;
		usleep(config_file_system.retardo_acceso_bloque);
		memcpy(leido,bloque_leido,tam_leido);
		offset=offset+tam_leido;
		tam_restante = size- tam_leido;
	    log_info(logger,"Leer Archivo: %s - Puntero: %d - Memoria: %p - Tamaño: %d", seleccionado->file_name,nro_bloque,bloque_leido,tam_leido);

	}
	void * bloque_punteros =archivobloques_pointer +seleccionado->indirect_pointer*superbloque.block_size;
	uint32_t* destination = (uint32_t*)bloque_punteros;
	if(nro_bloque>0){
		usleep(config_file_system.retardo_acceso_bloque);
		bloque_leido =archivobloques_pointer +destination[nro_bloque]*superbloque.block_size+start_pos.rem;
		usleep(config_file_system.retardo_acceso_bloque);
		memcpy(leido,bloque_leido,tam_leido);
		offset=offset+tam_leido;
		tam_restante = size- tam_leido;
	    log_info(logger,"Leer Archivo: %s - Puntero: %d - Memoria: %p - Tamaño: %d", seleccionado->file_name,nro_bloque,bloque_leido,tam_leido);
	}
	if(tam_restante>0){
		int bloques_restantes= tam_restante/superbloque.block_size;
			  for(int i = nro_bloque;i<bloques_restantes;i++){
				usleep(config_file_system.retardo_acceso_bloque);
				  bloque_leido =archivobloques_pointer +destination[i]*superbloque.block_size;
					usleep(config_file_system.retardo_acceso_bloque);
				  memcpy(leido+offset,bloque_leido,superbloque.block_size);
				  offset=offset+superbloque.block_size;
				  tam_restante=tam_restante-superbloque.block_size;
				    log_info(logger,"Leer Archivo: %s - Puntero: %d - Memoria: %p - Tamaño: %d", seleccionado->file_name,nro_bloque,bloque_leido,superbloque.block_size);

			  }
				usleep(config_file_system.retardo_acceso_bloque);
			  bloque_leido =archivobloques_pointer +destination[bloques_restantes]*superbloque.block_size;
				usleep(config_file_system.retardo_acceso_bloque);
			  memcpy(leido+offset,bloque_leido,tam_restante);
			    log_info(logger,"Leer Archivo: %s - Puntero: %d - Memoria: %p - Tamaño: %d", seleccionado->file_name,bloques_restantes+1,bloque_leido,tam_restante);

	}
	int sync = msync(archivobloques_pointer, archivobloques_size, MS_SYNC);
	if(sync == -1) printf("Error syncing archivobloques");
	return leido;
	}
	else return -1;

}
//devuelve bloque libre del bitarray
uint32_t bloqueLibre(){
	uint32_t offset=-1;
	for(uint32_t i = 0;i<bitarray_size;i++){
			if(bitarray_test_bit(bitarray,i)==0){
				bitarray_set_bit(bitarray,i);
				offset = i;
				i= bitarray_size;
			}
			}
	int sync = msync(bitarray_pointer, bitarray_size, MS_SYNC);
	if(sync == -1) printf("Error syncing bitarray");
    log_info(logger,"Acceso a Bitmap - Bloque: %d - Estado: %d", offset,1);
	return offset;
}

//libera bloque del bitarray
int liberarBloque(uint32_t bit){
	if(bitarray_test_bit(bitarray,bit)==0) {
		bitarray_clean_bit(bitarray,bit);
	}
	int sync = msync(bitarray_pointer, bitarray_size, MS_SYNC);
	if(sync == -1) printf("Error syncing bitarray");
    log_info(logger,"Acceso a Bitmap - Bloque: %d - Estado: %d", bit,0);

	return 0;
}

//agrega una cantidad de  bloques aoartir del ultimo bloque
int agregarBloques(int agregar, FCB* seleccionado){
	void * bloque_punteros =archivobloques_pointer +seleccionado->indirect_pointer*superbloque.block_size;
	int pos_nuevos = (seleccionado->file_size/superbloque.block_size);
	int ultimo = pos_nuevos + agregar;
    uint32_t* destination = (uint32_t*)bloque_punteros;
	for(int i =pos_nuevos;i<ultimo;i++){
		destination[i]= bloqueLibre();
	}
	return 0;

}
//elimina una cantidad de bloques apartir del ultimo bloque
int eliminarBloques(int eliminar,FCB* seleccionado){
	void * bloque_punteros =archivobloques_pointer +seleccionado->indirect_pointer*superbloque.block_size;
	int pos_viejos = (seleccionado->file_size/superbloque.block_size)-2;
	int ultimo = pos_viejos -eliminar;
	  uint32_t* destination = (uint32_t*)bloque_punteros;
		for(int i =pos_viejos;i>ultimo;i--){
			liberarBloque(destination[i]);
		}
		return 0;
}

// reduce o extiende fcb segun lo pedido
int truncarArchivo(char* nombre, int size){
	strcpy(buscado,nombre);
	FCB* seleccionado = (FCB*)list_remove_by_condition(fcb_list,igualBuscado);
	if(seleccionado){
	int bloques_restantes = (size +superbloque.block_size -1)/superbloque.block_size;
	int tam_viejo=seleccionado->file_size/superbloque.block_size;
	if(bloques_restantes==0){
			liberarBloque(seleccionado->direct_pointer);
		}
	if(bloques_restantes<2){
				liberarBloque(seleccionado->indirect_pointer);
			}
	if(tam_viejo==0&&bloques_restantes>0){
		seleccionado->direct_pointer = bloqueLibre();
		bloques_restantes = bloques_restantes-1;
	}
	if(tam_viejo<2&&bloques_restantes>0){
			seleccionado->indirect_pointer = bloqueLibre();
		}

	if(tam_viejo<bloques_restantes){
		int agregar = bloques_restantes-tam_viejo;
		agregarBloques(agregar,seleccionado);
	}else if(tam_viejo>bloques_restantes){
		int eliminar = tam_viejo-bloques_restantes;
		eliminarBloques(eliminar,seleccionado);
	}
	seleccionado->file_size= bloques_restantes*superbloque.block_size;
    char file_path[256];
	snprintf(file_path, sizeof(file_path), "%s/%s.dat", config_file_system.path_fcb, nombre);
	FILE * fcb_file = fopen(file_path,"w");
	fprintf(fcb_file, "NOMBRE_ARCHIVO=%s\n", seleccionado->file_name);
	       fprintf(fcb_file, "TAMANIO_ARCHIVO=%u\n", seleccionado->file_size);
	       fprintf(fcb_file, "PUNTERO_DIRECTO=%u\n", seleccionado->direct_pointer);
	       fprintf(fcb_file, "PUNTERO_INDIRECTO=%u\n", seleccionado->indirect_pointer);
	       fclose(fcb_file);
	list_add(fcb_list,seleccionado);
    log_info(logger,"Truncar Archivo: %s - Tamanio: %d", nombre,size);
	return 0;
	}

}

// busca FCB por nombre
bool igualBuscado(void * ptr){
	FCB* seleccionado = (FCB*) ptr;
	if (strcmp(seleccionado->file_name,buscado)==0){
		return 1;
	} else return 0;

}
// verifica existencia de archivo
int abrirArchivo(char* nombre){
	strcpy(buscado,nombre);
	bool resultado = list_any_satisfy(fcb_list,igualBuscado);
	if(resultado==1){
    log_info(logger,"Abrir Archivo: %s", nombre);}
	else{
	    log_info(logger,"Archivo no existe");
	}
	return resultado;

}
//crea archivo fcb  y lo carga a la lista de FCBs
int crearArchivo(char* nombre){
	FCB *fcb_nuevo;
	fcb_nuevo=malloc(sizeof(FCB));
	fcb_nuevo->file_name= malloc(sizeof(nombre));
    char file_path[256];
    strcpy(fcb_nuevo->file_name, nombre);
	fcb_nuevo->file_size=0;
    snprintf(file_path, sizeof(file_path), "%s/%s.dat", config_file_system.path_fcb, nombre);
	FILE * fcb_file = fopen(file_path,"w");
	 if (!fcb_file) {
	        printf("Error creating the file.\n");
	        return-1;
	    }
    fprintf(fcb_file, "NOMBRE_ARCHIVO=%s\n", fcb_nuevo->file_name);
       fprintf(fcb_file, "TAMANIO_ARCHIVO=%u\n", fcb_nuevo->file_size);
       fprintf(fcb_file, "PUNTERO_DIRECTO=%u\n", fcb_nuevo->direct_pointer);
       fprintf(fcb_file, "PUNTERO_INDIRECTO=%u\n", fcb_nuevo->indirect_pointer);
       fclose(fcb_file);
       list_add(fcb_list,fcb_nuevo);
       log_info(logger,"Crear archivo: %s", nombre);
	return 0;

}

//abre superbloque, bitarray, FCBs y archivo de bloques
int crearEstructuras(){
	//superbloque
	t_config* superbloque_config;
	superbloque_config = config_create(config_file_system.path_superbloque);
	superbloque.block_size= (uint32_t)config_get_int_value(superbloque_config,"BLOCK_SIZE");
	superbloque.block_count= (uint32_t)config_get_int_value(superbloque_config,"BLOCK_COUNT");
	//bitarray
	bitarray_fd = open(config_file_system.path_bitmap, O_RDWR);
	if (bitarray_fd==-1) {
		FILE * bitarray_file = fopen(config_file_system.path_bitmap,"w");
	    size_t num_bytes = superbloque.block_count / 8;
	    unsigned char* buffer = (unsigned char*)malloc(num_bytes);
	    for (size_t i = 0; i < num_bytes; i++) {
	           buffer[i] = 0x00;
	       }
	    fwrite(buffer, sizeof(unsigned char), num_bytes, bitarray_file);
	    free(buffer);
	    fclose(bitarray_file);
	    bitarray_fd = open(config_file_system.path_bitmap, O_RDWR);
	}
    bitarray_size = lseek(bitarray_fd, 0, SEEK_END);
    bitarray_pointer = mmap(NULL, bitarray_size, PROT_READ | PROT_WRITE, MAP_SHARED, bitarray_fd, 0);
    if (bitarray_pointer == MAP_FAILED) printf("Error mapeando bitarray");
    bitarray =bitarray_create(bitarray_pointer,bitarray_size);

	//fcb
    char file_path[256];
    t_config* config_fcb;
    FCB *read_fcb;
    fcb_list = list_create();
    DIR* directory = opendir(config_file_system.path_fcb);
    struct dirent* entry;
    while ((entry = readdir(directory)) != NULL) {
            if (entry->d_type == DT_REG) { // Check if it's a regular file
            	read_fcb = malloc(sizeof(FCB));
            	read_fcb->file_name = malloc(sizeof(config_get_string_value(config_fcb,"NOMBRE_ARCHIVO")));
            	snprintf(file_path, sizeof(file_path), "%s/%s", config_file_system.path_fcb, entry->d_name);
                config_fcb= config_create(file_path);
                read_fcb->file_name = config_get_string_value(config_fcb,"NOMBRE_ARCHIVO");
                read_fcb->file_size = (uint32_t)config_get_int_value(config_fcb,"TAMANIO_ARCHIVO");
                read_fcb->direct_pointer = (uint32_t)config_get_int_value(config_fcb,"PUNTERO_DIRECTO");
                read_fcb->indirect_pointer = (uint32_t)config_get_int_value(config_fcb,"PUNTERO_INDIRECTO");
                list_add(fcb_list,read_fcb);
            }
    }
    closedir(directory);

    //archivo de bloques

    archivobloques_fd = open(config_file_system.path_bloques, O_RDWR);
    	if (archivobloques_fd==-1) {
    		FILE * archivobloques_file = fopen(config_file_system.path_bloques,"w");
    	    size_t total_size = superbloque.block_count * superbloque.block_size;
    	    unsigned char* buffer = (unsigned char*)malloc(total_size);
    	    fwrite(buffer, sizeof(unsigned char), total_size, archivobloques_file);
    	    free(buffer);
    	    fclose(archivobloques_file);
    	    archivobloques_fd = open(config_file_system.path_bloques, O_RDWR);
    	}
	
	archivobloques_size = lseek(archivobloques_fd, 0, SEEK_END);
	archivobloques_pointer = mmap(NULL, archivobloques_size, PROT_READ | PROT_WRITE, MAP_SHARED, bitarray_fd, 0);
	if (archivobloques_pointer == MAP_FAILED) printf("Error mapeando archivobloques");
	
	log_info(logger,"Estructuras creadas");
    return 0;
}

// cierra mmaps de bitarray y archivo de bloques
int cerrarEstructuras(){
	int sync = msync(bitarray_pointer, bitarray_size, MS_SYNC);
	    if(sync == -1) printf("Error syncing bitarray");
	    int unmap = munmap(bitarray_pointer, bitarray_size);
	    if(unmap == -1) printf("Error unmapping bitarray");
	    close(bitarray_fd);

	    sync = msync(archivobloques_pointer, archivobloques_size, MS_SYNC);
	    	    if(sync == -1) printf("Error syncing archivobloques");
	    	     unmap = munmap(archivobloques_pointer, archivobloques_size);
	    	    if(unmap == -1) printf("Error unmapping archivobloques");
	    	    close(archivobloques_fd);
	    log_info(logger,"Estructuras terminadas");

	    return 0;
}

void manejar_conexion_con_kernel()
{
		while (1)
		{
			t_buffer *buffer = recibir_buffer(kernel_fd);
			log_info(logger, "Buffer recibido por kernel arriba del switch: %d", buffer->codigo);
			switch (buffer->codigo)
			{
			case SOLICITUD_ABRIR_ARCHIVO:

				char *nombre_archivo_abrir = buffer_read_string(buffer);
				uint32_t resultado_abrir = abrirArchivo(nombre_archivo_abrir);
				destruir_buffer_nuestro(buffer);

				buffer = crear_buffer_nuestro();
				if (resultado_abrir == 0)
				{
					buffer->codigo = F_OPEN_OK;
				}
				else
				{
					uint32_t resultado_crear = crearArchivo(nombre_archivo_abrir);
					if(resultado_crear==0){
						buffer->codigo = F_OPEN_OK;
					}else  	
						buffer->codigo = F_OPEN_FAIL;

				}

				enviar_buffer(buffer, kernel_fd);

				log_info(logger, "Buffer enviado");
				destruir_buffer_nuestro(buffer);
				break;

			case SOLICITUD_TRUNCAR_ARCHIVO:

				uint32_t tam = 0;
				char* nombre_archivo_truncar = buffer_read_stringV2(buffer, &tam);
				uint32_t tamanio_truncar = buffer_read_uint32(buffer);
				uint32_t resultado_truncar = truncarArchivo(nombre_archivo_truncar, tamanio_truncar);
				destruir_buffer_nuestro(buffer);

				buffer = crear_buffer_nuestro();
				if (resultado_truncar == 0)
					buffer->codigo = F_TRUNCATE_OK;
				else
					buffer->codigo = F_TRUNCATE_FAIL;

				enviar_buffer(buffer, kernel_fd);

				log_info(logger, "Buffer enviado");
				destruir_buffer_nuestro(buffer);

				break;

			case SOLICITUD_LEER_ARCHIVO:
				tam = 0;
				char *nombre_archivo_leer = buffer_read_stringV2(buffer, &tam);
				uint32_t posicion_leer = buffer_read_uint32(buffer);
				uint32_t tamanio_leer = buffer_read_uint32(buffer);
				uint32_t direccion_memoria_leer = buffer_read_uint32(buffer);
				uint32_t pid_leer = buffer_read_uint32(buffer);

				destruir_buffer_nuestro(buffer);

				char *resultado_leer = leerArchivo(nombre_archivo_leer, posicion_leer, tamanio_leer);
				
				buffer->codigo = F_READ_MEMORIA;
				
				buffer_write_uint32(buffer, pid_leer);
				buffer_write_uint32(buffer, direccion_memoria_leer);
				buffer_write_string(buffer, resultado_leer);
				enviar_buffer(buffer, socket_memoria);
				destruir_buffer_nuestro(buffer);

				buffer = recibir_buffer(socket_memoria);
				log_info(logger, "Buffer recibido");
				if(buffer->codigo == F_READ_OK){
					enviar_buffer(buffer, kernel_fd);
					log_info(logger, "Buffer enviado");

				}
				destruir_buffer_nuestro(buffer);

				break;

			case SOLICITUD_ESCRIBIR_ARCHIVO:
				tam = 0;
				char *nombre_archivo_escribir = buffer_read_stringV2(buffer, &tam);
				uint32_t posicion_escribir = buffer_read_uint32(buffer);
				uint32_t tamanio_escribir = buffer_read_uint32(buffer);
				uint32_t direccion_memoria_escribir = buffer_read_uint32(buffer);
				uint32_t pid_escribir = buffer_read_uint32(buffer);

				destruir_buffer_nuestro(buffer);
				atender_solicitud_f_write(nombre_archivo_escribir, posicion_escribir, tamanio_escribir, direccion_memoria_escribir, pid_escribir);

			default:

				log_info(logger, "Default");

				break;
			}
		}
}

void levantar_modulo(){
	logger = iniciar_logger();
	config = iniciar_config();
	levantar_config();
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
	nuevo_logger = log_create("fileSystem.log", "FileSystem", 1, LOG_LEVEL_INFO);

	if (nuevo_logger == NULL)
	{
		printf("Error al crear el logger\n");
		exit(1);
	}

	return nuevo_logger;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config;

	nuevo_config = config_create("fileSystem.config");

	if (nuevo_config == NULL){
		printf("Error al crear el nuevo config\n");
		exit(2);
	}

	return nuevo_config;
}

void levantar_config(){
	config_file_system.ip_memoria = config_get_string_value(config,"IP_MEMORIA");
	config_file_system.puerto_memoria = config_get_int_value(config,"PUERTO_MEMORIA");
	config_file_system.puerto_escucha = config_get_int_value(config,"PUERTO_ESCUCHA");
	config_file_system.path_superbloque = config_get_string_value(config,"PATH_SUPERBLOQUE");
	config_file_system.path_bitmap = config_get_string_value(config,"PATH_BITMAP");
	config_file_system.path_bloques = config_get_string_value(config,"PATH_BLOQUES");
	config_file_system.path_fcb = config_get_string_value(config,"PATH_FCB");
	config_file_system.retardo_acceso_bloque = config_get_int_value(config,"RETARDO_ACCESO_BLOQUE");
}

void establecer_conexiones()
{
	
	socket_memoria = crear_conexion(config_file_system.ip_memoria, config_file_system.puerto_memoria);
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
	
	
	server_fd = iniciar_servidor(logger,config_file_system.puerto_escucha);
	
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

void atender_solicitud_f_write(char* nombre_archivo_escribir, uint32_t posicion_escribir, uint32_t tamanio_escribir, uint32_t direccion_memoria_escribir, uint32_t pid_escribir){
	
	t_buffer* buffer = crear_buffer_nuestro();
	
	buffer->codigo = F_WRITE_MEMORIA;
	buffer_write_uint32(buffer, pid_escribir);
	buffer_write_uint32(buffer, tamanio_escribir);
	buffer_write_uint32(buffer, direccion_memoria_escribir);

	log_info(logger, "Por enviar buffer");
	enviar_buffer(buffer, socket_memoria);
	destruir_buffer_nuestro(buffer);

	buffer = recibir_buffer(socket_memoria);
	log_info(logger, "Buffer recibido por memoria en soli f_write: %d", buffer->codigo);
	char *datos_escribir;
	if(buffer->codigo==RTA_F_WRITE_MEMORIA){
		uint32_t tam = 0;
		datos_escribir = buffer_read_stringV2(buffer, &tam);
	}
	destruir_buffer_nuestro(buffer);

	uint32_t resultado_escribir = escribirArchivo(nombre_archivo_escribir, posicion_escribir, tamanio_escribir, datos_escribir);
	log_info(logger, "Ya escribri en el archivo");

	resultado_escribir = 0;

	log_info(logger, "Resultado escribir: %d", resultado_escribir);

	buffer = crear_buffer_nuestro();

	if (resultado_escribir == 0)
		buffer->codigo = F_WRITE_OK;
	else
		buffer->codigo = F_WRITE_FAIL;

	log_info(logger, "Resultado F_WRITE: %d", buffer->codigo);

	enviar_buffer(buffer, kernel_fd);
	log_info(logger, "Buffer enviado");

	destruir_buffer_nuestro(buffer);
}
