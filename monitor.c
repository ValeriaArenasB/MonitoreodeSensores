/*
  Archivo: monitor.c
  Autores: Valeria Arenas, Tatiana Vivas, Daniel Leon
  Objetivo: Implementar el monitor que gestiona la recolección y procesamiento de datos de sensores de temperatura y pH.
  Módulos o funciones:
    - InicializarBuffers: Inicializa los buffers y los semáforos.
    - LiberarBuffers: Libera los recursos de los buffers y los semáforos.
    - Recolector: Lee datos desde un pipe y los coloca en los buffers correspondientes (actúa como productor).
    - HiloTemp: Procesa y almacena datos de temperatura.
    - HiloPh: Procesa y almacena datos de pH.
    - EjecutarMonitor: Ejecuta el monitor que recibe datos de sensores y los procesa.
    - main: Función principal que inicia el monitor.
  Fecha de finalización: 19 de mayo de 2024
 */

#include "monitor.h"

#define TEMPERATURA_FLAG "TEMP"
#define PH_FLAG "PH"
#define END_OF_DATA -9999  //cuando se acaban sensores, el monitor tiene que también que acabar

int BUFFER_SIZE;
sbuf_t buffer_temp, buffer_ph;
char *file_temp = NULL, *file_ph = NULL;
int sensors_active = 1;

/*
  Función: InicializarBuffers
  Descripción: Inicializa los buffers y los semáforos. Ningún semáforo se puede compartir entre procesos
 */
void InicializarBuffers() {
    sem_init(&buffer_temp.empty, 0, BUFFER_SIZE); // Indica que todas las posiciones están vacías al inicio. Al final es el tamaño del semáforo. Se le hace un semwait
    sem_init(&buffer_temp.full, 0, 0); // No hay posiciones llenas al inicio. Se le hace un semsignal/sempost
    sem_init(&buffer_temp.mutex, 0, 1); // El buffer está disponible para acceso. Se le hace ambos para asegurar exclusión mútua. Por eso empieza en 1
  
    sem_init(&buffer_ph.empty, 0, BUFFER_SIZE);
    sem_init(&buffer_ph.full, 0, 0);
    sem_init(&buffer_ph.mutex, 0, 1);
}

/*
  Función: LiberarBuffers
  Descripción: Libera los recursos de los buffers y los semáforos. En mismo orden de creación
 */
void LiberarBuffers() {
    sem_destroy(&buffer_temp.empty); 
    sem_destroy(&buffer_temp.full);
    sem_destroy(&buffer_temp.mutex);
  
    sem_destroy(&buffer_ph.empty);
    sem_destroy(&buffer_ph.full);
    sem_destroy(&buffer_ph.mutex);
}

/*
  Función: Recolector
  Parámetros de entrada: arg - nombre del pipe
  Descripción: Lee datos desde un pipe y los coloca en los buffers correspondientes. Actúa como "productor", o sea que genera datos y los coloca en un buffer compartido.
 */
void *Recolector(void *arg) {
    char *pipe_name = (char *)arg;
    int fd = open(pipe_name, O_RDONLY);
    char read_buffer[256];
    float value=0;
    int sensor_type=0;
    
    sem_t *empty, *full;
    sem_t *mutex;
    float *buffer;
    int *in, *out;
  
    if (fd == -1) {
        perror("Error abriendo el pipe en el recolector");
        exit(1);
    }

    while (read(fd, read_buffer, sizeof(read_buffer)) > 0) {  //leer datos desde un pipe, procesarlos y colocarlos en el buffer adecuado (temperatura o pH), mientras haya datos
        if (read_buffer[strlen(read_buffer) - 1] == '\n') { //procesar cada línea
            read_buffer[strlen(read_buffer) - 1] = '\0'; // Eliminar el salto de línea, para que se vea bonito
        }

        char *token = strtok(read_buffer, ":"); //A partir de los : toma esa cadena de caracteres y la coge como el token.
        sensor_type = atoi(token); 
        token = strtok(NULL, ":"); //que se utiliza para dividir una cadena en una serie de tokens, delimitados por uno o más caracteres. Para continuar y en siguiente ejecución volver al inicio
        char *data = token; //para poder manipular y convertir ese valor a un formato numérico adecuado para su posterior procesamiento

        if (sensor_type == 1 || sensor_type == 2) { // Temperatura o pH
            value = atof(data);
            //verificar, si es inválido (negativo, porque es el condición que se comparte) sigue y no se rompe
            if (value < 0) {
                printf("Valor inválido (negativo) recibido: %f\n", value);
                continue;
            }
            //identificar tipo del sensor. SIEMPRE
          
            if (sensor_type == 1) {
              //solo asignar valores a su buffer correspondiente
                empty = &buffer_temp.empty;
                full = &buffer_temp.full;
                mutex = &buffer_temp.mutex;
                buffer = buffer_temp.buf;
                in = &buffer_temp.in;
                out = &buffer_temp.out;
            } else {
                empty = &buffer_ph.empty;
                full = &buffer_ph.full;
                mutex = &buffer_ph.mutex;
                buffer = buffer_ph.buf;
                in = &buffer_ph.in;
                out = &buffer_ph.out;
            }
            
            sem_wait(empty);//restar al buffer.
            sem_wait(mutex);//alguien accede a él

            // Marcar el dato con el flag correspondiente
            buffer[*in] = value; //dato entra en el buffer
            *in = (*in + 1) % BUFFER_SIZE; //una cajita y el espacio. Se asigna en el primer espacio vacío y se suma 1. Se hace ccircular y se asegura que meintras avance, siga dentro de los límites del buffer

            sem_post(mutex); //libera para darle paso al siguiente en la siguiente iteración.
            sem_post(full);
        } else {
            printf("Error: Medida recibida incorrecta.\n");
        }
    }

    // Esperar 10 segundos y luego enviar mensaje de finalización en caso de que ya no tenga mas sensores conectados
    sleep(10);

    // Colocar mensaje de finalización en cada buffer
    sem_wait(&buffer_temp.empty); //se inicializa con el tamaño del buffer (BUFFER_SIZE), y se decrementa cada vez que se produce un elemento (se coloca un dato en el buffer).
    sem_wait(&buffer_temp.mutex);
    buffer_temp.buf[buffer_temp.in] = END_OF_DATA; //marcador de finalización (-9999). el .buf es el buffer circular
    buffer_temp.in = (buffer_temp.in + 1) % BUFFER_SIZE; //espera al menos una posión vacía del buffer, y como es circualr, sigue
    sem_post(&buffer_temp.mutex); //liberar
    sem_post(&buffer_temp.full);

    sem_wait(&buffer_ph.empty);
    sem_wait(&buffer_ph.mutex);
    buffer_ph.buf[buffer_ph.in] = END_OF_DATA;
    buffer_ph.in = (buffer_ph.in + 1) % BUFFER_SIZE;
    sem_post(&buffer_ph.mutex);
    sem_post(&buffer_ph.full);

    sensors_active = 0;
    unlink(pipe_name); //cerrar el pipe para que no se quede el monitor popr siempre esperando por datos que ya no entran
  
    printf("Finalización del procesamiento de medidas.\n");

    close(fd);
    return NULL;
}
//Los hilos son los consumidores

/*
  Función: HiloTemp
  Parámetros de entrada: arg - nombre del archivo de temperatura
  Descripción: Procesa y almacena datos de temperatura.
 */
void *HiloTemp() {
  
    float temp=0;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20];
  
    FILE *file = fopen(file_temp, "a"); //append para que no se sobreescribe
    if (file == NULL) {
        perror("Error al abrir el archivo de temperatura");
        exit(1);
    }

    while (1) {
        sem_wait(&buffer_temp.full); //resta tamaño de lo que entra (región crítica)
        sem_wait(&buffer_temp.mutex); //exclusión mutua

        temp = buffer_temp.buf[buffer_temp.out];
        buffer_temp.out = (buffer_temp.out + 1) % BUFFER_SIZE; //in era para recibir, entonces out para escribir (sacarlo)

        sem_post(&buffer_temp.mutex);
        sem_post(&buffer_temp.empty);

        if (temp == END_OF_DATA) { //si el buffer (escritura) se acabó, break
            break;
        }
        //validaciones y escritura de fechas. Así se pone:)
        if (temp >= 20.0 && temp <= 31.6) {
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info); //formato fechas que toca hacer así

            fprintf(file, "[%s] %f\n", timestamp, temp);
            fflush(file); // Vaciar el buffer inmediatamente
        } else {
            printf("Temperatura fuera de rango: %f\n", temp);
        }
    }

    fclose(file);
    return NULL;
}

/*
 * Función: HiloPh
 * Parámetros de entrada: arg - nombre del archivo de pH
 * Descripción: Procesa y almacena datos de pH.
 */
void *HiloPh() {
    float ph=0;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[20];
    
  
    FILE *file = fopen(file_ph, "a");
    if (file == NULL) {
        perror("Error al abrir el archivo de pH");
        exit(1);
    }

    while (1) {
        sem_wait(&buffer_ph.full);
        sem_wait(&buffer_ph.mutex);

         ph = buffer_ph.buf[buffer_ph.out];
        buffer_ph.out = (buffer_ph.out + 1) % BUFFER_SIZE;

        sem_post(&buffer_ph.mutex);
        sem_post(&buffer_ph.empty);

        if (ph == END_OF_DATA) {
            break;
        }

        if (ph >= 6.0 && ph <= 8.0) {
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

            fprintf(file, "[%s] %f\n", timestamp, ph);
            fflush(file); // Vaciar el buffer inmediatamente
        } else {
            printf("PH fuera de rango: %f\n", ph);
        }
    }

    fclose(file);
    return NULL;
}

/*
  Función: EjecutarMonitor
  Parámetros de entrada: argc - número de argumentos
                         argv - lista de argumentos
  Descripción: Ejecuta el monitor que recibe datos de sensores y los procesa.
 */
void EjecutarMonitor(int argc, char *argv[]) {
    char *pipe_name;
    pthread_t recolector_thread, ph_thread, temperatura_thread;
  
    if (argc != 9) {
        fprintf(stderr, "Modo de uso: %s -b buffer_size -t temp_file -h ph_file -p pipe_name\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-b") == 0) {
            BUFFER_SIZE = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0) {
            file_temp = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0) {
            file_ph = argv[++i];
        }
    }

    if (!file_temp || !file_ph) {
        fprintf(stderr, "Error al abrir archivo.\n");
        exit(1);
    }

    pipe_name = NULL;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            pipe_name = argv[++i];
        }
    }

    if (mkfifo(pipe_name, 0666) == -1) {
          if (errno == EEXIST) {
              // Si el pipe ya existe, eliminarlo
              if (unlink(pipe_name) == -1) {
                  perror("unlink");
                  exit(EXIT_FAILURE);
              }
              // Intentar crear el pipe nuevamente
              if (mkfifo(pipe_name, 0666) == -1) {
                  perror("mkfifo");
                  exit(EXIT_FAILURE);
              }
          } else {
              perror("mkfifo");
              exit(EXIT_FAILURE);
          }
      }


    printf("Inicializando semáforos...\n");
    InicializarBuffers();

    printf("Buffers inicializados\n");

    
    pthread_create(&recolector_thread, NULL, Recolector, pipe_name); //1. id almacen del hilo, 2. objeto de atributos para crear el hilo pero ya se hizo, 3. rutina inicio o sea funicón a la que corresponde, y 4. apunta al único arg que se necesita para la rutina.
    pthread_create(&ph_thread, NULL, HiloPh, NULL);
    pthread_create(&temperatura_thread, NULL, HiloTemp, NULL);

    pthread_join(recolector_thread, NULL);
    pthread_join(ph_thread, NULL);
    pthread_join(temperatura_thread, NULL);

    LiberarBuffers();
}

int main(int argc, char *argv[]) {
    
    EjecutarMonitor(argc, argv);
    return 0;
}
