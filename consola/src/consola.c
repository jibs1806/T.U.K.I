#include "../includes/consola.h"

int main(int argc, char **argv)
{
    char *config_path = argv[1];
    char *instruccion_path = argv[2];
    levantar_modulo(config_path);

    crear_array_de_instrucciones(instruccion_path);

    t_list *listaAEnviar = mapearLista();

    t_buffer *buffer = crear_buffer_nuestro();

    enviarLista(buffer, listaAEnviar);

    destruir_buffer_nuestro(buffer);

    destruir_lista(listaAEnviar);

    op_code codigo = recibir_codigo(socket_kernel);

    if (codigo == FIN_PROCESO_CONSOLA)
    {
        log_info(logger, "Fin proceso consola: %d", codigo);
        finalizar_modulo();
        return 0;
    }
    else
    {
        log_info(logger, "Codigo recibido: %d", codigo);
        finalizar_modulo();
        return -2;
    }
}

// SUBPROGRAMAS

t_list *mapearLista()
{

    t_list *listaAEnviar = list_create();

    for (int i = 0; i < instructionCount; i++)
    {
        t_instruction *instruccion = cambiarStruct(instructions[i]);
        list_add(listaAEnviar, instruccion);
    }

    return listaAEnviar;
}

t_instruction *cambiarStruct(Instruction_consola instruccion_a_cambiar)
{

    t_instruction *instruccion_cambiada = NULL;

    instruccion_cambiada = malloc(sizeof(t_instruction));

    instruccion_cambiada->instruccion = instruccion_a_cambiar.instruccion;
    instruccion_cambiada->numero1 = instruccion_a_cambiar.numero1;
    instruccion_cambiada->numero2 = instruccion_a_cambiar.numero2;

    int tam1 = get_tamanio_char_array(instruccion_a_cambiar.string1, 20);
    int tam2 = get_tamanio_char_array(instruccion_a_cambiar.string2, 20);

    instruccion_cambiada->string1 = malloc(tam1);
    for (int i = 0; i < tam1 + 1; i++)
        *(instruccion_cambiada->string1 + i) = instruccion_a_cambiar.string1[i];

    instruccion_cambiada->string2 = malloc(tam2);
    for (int j = 0; j < tam2 + 1; j++)
        *(instruccion_cambiada->string2 + j) = instruccion_a_cambiar.string2[j];

    return instruccion_cambiada;
}

int get_tamanio_char_array(char a[], int tamanioFisico)
{
    int tamanioLogico = 0;

    for (int i = 0; i < tamanioFisico; i++)
    {
        if (a[i] != '\0')
            tamanioLogico++;
    }

    return tamanioLogico;
}

void enviarLista(t_buffer *buffer, t_list *listaAEnviar)
{
    buffer_write_lista_instrucciones(buffer, listaAEnviar);

    enviar_buffer(buffer, socket_kernel);
}

void finalizar_modulo()
{
    log_destroy(logger);
    config_destroy(config);
    close(socket_kernel);
    return;
}

// UTILS INICIAR MODULO -----------------------------------------------------------------
void levantar_modulo(char *config_path)
{
    logger = iniciar_logger();
    config = iniciar_config(config_path);
    levantar_config();
    establecer_conexiones();
}

t_log *iniciar_logger(void)
{
    t_log *nuevo_logger;
    nuevo_logger = log_create("consola.log", "Consola", true, LOG_LEVEL_INFO);

    if (nuevo_logger == NULL)
    {
        printf("Error al crear el logger\n");
        exit(1);
    }

    return nuevo_logger;
}

t_config *iniciar_config(char *config_path)
{
    t_config *nuevo_config;

    nuevo_config = config_create(config_path);

    if (nuevo_config == NULL)
    {
        printf("Error al crear el nuevo config\n");
        exit(2);
    }

    return nuevo_config;
}

void levantar_config()
{
    config_consola.ip_kernel = config_get_string_value(config, "IP_KERNEL");
    config_consola.puerto_kernel = config_get_int_value(config, "PUERTO_KERNEL");
}

// FIN UTILS INICIAR MODULO -------------------------------------------------------------

// UTILS CONEXIONES ---------------------------------------------------------------------
void establecer_conexiones()
{
    socket_kernel = crear_conexion(config_consola.ip_kernel, config_consola.puerto_kernel);
    if (socket_kernel >= 0)
    {
        log_info(logger, "Conectado con kernel");
        // Realizamos el handshake
        enviar_handshake(socket_kernel);
        if (recibir_handshake(socket_kernel) == HANDSHAKE)
        {
            log_info(logger, "Handshake con Kernel realizado.");
        }
        else
            log_info(logger, "Ocurio un error al realizar el handshake con Kernel.");
    }
    else
        log_info(logger, "Error al conectar con kernel");
}
// FIN UTILS CONEXIONES -----------------------------------------------------------------

// UTILS PARSER -------------------------------------------------------------------------
InstructionType getNextInstruction(FILE *file)
{
    char instruction[20];
    fscanf(file, "%s", instruction);

    if (strcmp(instruction, "SET") == 0)
    {
        return SET;
    }
    else if (strcmp(instruction, "MOV_OUT") == 0)
    {
        return MOV_OUT;
    }
    else if (strcmp(instruction, "WAIT") == 0)
    {
        return WAIT;
    }
    else if (strcmp(instruction, "I/O") == 0)
    {
        return IO;
    }
    else if (strcmp(instruction, "SIGNAL") == 0)
    {
        return SIGNAL;
    }
    else if (strcmp(instruction, "MOV_IN") == 0)
    {
        return MOV_IN;
    }
    else if (strcmp(instruction, "F_OPEN") == 0)
    {
        return F_OPEN;
    }
    else if (strcmp(instruction, "YIELD") == 0)
    {
        return YIELD;
    }
    else if (strcmp(instruction, "F_TRUNCATE") == 0)
    {
        return F_TRUNCATE;
    }
    else if (strcmp(instruction, "F_SEEK") == 0)
    {
        return F_SEEK;
    }
    else if (strcmp(instruction, "CREATE_SEGMENT") == 0)
    {
        return CREATE_SEGMENT;
    }
    else if (strcmp(instruction, "F_WRITE") == 0)
    {
        return F_WRITE;
    }
    else if (strcmp(instruction, "F_READ") == 0)
    {
        return F_READ;
    }
    else if (strcmp(instruction, "DELETE_SEGMENT") == 0)
    {
        return DELETE_SEGMENT;
    }
    else if (strcmp(instruction, "F_CLOSE") == 0)
    {
        return F_CLOSE;
    }
    else if (strcmp(instruction, "EXIT") == 0)
    {
        return EXIT;
    }
    else
    {
        return INVALID;
    }
}
void readIntegerFromFile(FILE *file, int index)
{
    int number;
    fscanf(file, "%d", &number);
    if (index == 1)
        instructions[instructionCount].numero1 = number;
    else
        instructions[instructionCount].numero2 = number;
}
void readNextWordFromFile(FILE *file, int index)
{
    char buffer[20];
    fscanf(file, "%s", buffer);
    if (index == 1)
        strcpy(instructions[instructionCount].string1, buffer);
    else
        strcpy(instructions[instructionCount].string2, buffer);
}

void parseInstructions(FILE *file)
{
    strtok(file, " ");
    int instruccion_actual;
    while (1)
    {
        instruccion_actual = getNextInstruction(file);
        instructions[instructionCount].instruccion = instruccion_actual;
        inicializar_instruccion();
        switch (instruccion_actual)
        {
        case F_READ:
            readNextWordFromFile(file, 1);
            readIntegerFromFile(file, 1);
            readIntegerFromFile(file, 2);
            break;
        case F_WRITE:
            readNextWordFromFile(file, 1);
            readIntegerFromFile(file, 1);
            readIntegerFromFile(file, 2);
            break;
        case SET:
            readNextWordFromFile(file, 1);
            readNextWordFromFile(file, 2);
            break;
        case MOV_IN:
            readNextWordFromFile(file, 1);
            readIntegerFromFile(file, 1);
            break;
        case MOV_OUT:
            readIntegerFromFile(file, 1);
            readNextWordFromFile(file, 1);
            break;
        case F_TRUNCATE:
            readNextWordFromFile(file, 1);
            readIntegerFromFile(file, 1);
            break;
        case F_SEEK:
            readNextWordFromFile(file, 1);
            readIntegerFromFile(file, 1);
            break;
        case CREATE_SEGMENT:
            readIntegerFromFile(file, 1);
            readIntegerFromFile(file, 2);
            break;
        case IO:
            readIntegerFromFile(file, 1);
            break;
        case WAIT:
            readNextWordFromFile(file, 1);
            break;
        case SIGNAL:
            readNextWordFromFile(file, 1);
            break;
        case F_OPEN:
            readNextWordFromFile(file, 1);
            break;
        case F_CLOSE:
            readNextWordFromFile(file, 1);
            break;
        case DELETE_SEGMENT:
            readIntegerFromFile(file, 1);
            break;
        case EXIT:
            instructionCount++;
            return;
            break;
        default:
            break;
        }
        instructionCount++;
    }
}

void match(FILE *file, const char *expected)
{
    char input[MAX_PARAMETER_LENGTH];
    fscanf(file, "%s", input);
    if (strcmp(input, expected) != 0)
    {
        error();
    }
}

void error()
{
    printf("Syntax error\n");
    exit(1);
}

int crear_array_de_instrucciones(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Failed to open file.\n");
        return 1;
    }
    parseInstructions(file);

    fclose(file);
    return 0;
}

void destruir_lista(t_list *lista_instrucciones)
{

    int cant_instrucciones = list_size(lista_instrucciones);
    /*
    for(int i = 0; i < cant_instrucciones; i++){
        t_instruction* instruccion = list_get(lista_instrucciones, i);
        destruir_instruccion(instruccion);
    }
    */
    list_destroy(lista_instrucciones);
}

void inicializar_instruccion()
{
    strcpy(instructions[instructionCount].string1, CHAR_VACIO);
    strcpy(instructions[instructionCount].string2, CHAR_VACIO);
    instructions[instructionCount].numero1 = INT_VACIO;
    instructions[instructionCount].numero2 = INT_VACIO;
}
// FIN UTILS PARSER ---------------------------------------------------------------------
