/*
  Archivo: sensor.c
    Autores: Valeria Arenas, Tatiana Vivas, Daniel Leon
    Objetivo: Implementar el sensor que lee datos desde un archivo y los envía por un pipe.
    Módulos o funciones:
    - EjecutarSensor: Ejecuta el sensor que lee datos desde un archivo y los envía por un pipe.
    - main: Función principal que inicia el sensor.
  Fecha de finalización: 17 de mayo de 2024
 */

#include "sensor.h"

#define BUFFER_SIZE 128

/* 
 * Función: EjecutarSensor
 * Parámetros de entrada: argc - número de argumentos
 *                        argv - lista de argumentos
 * Valor de salida: ninguno
 * Descripción: Ejecuta el sensor que lee datos desde un archivo y los envía por un pipe.
 */
void EjecutarSensor(int argc, char *argv[]) {
    char *sensor = NULL, *archivo = NULL, *tiempo = NULL, *pipe_name = NULL;
    char line[100], buffer[128]; 
    int tiempo_espera=0, sensor_type=0, fd;
    float value;
    ssize_t bytes_written; //representar el tamaño de un objeto en bytes o la cantidad de bytes leídos o escritos por funciones de entrada/salida (I/O)

  
    if (argc != 9) {
        fprintf(stderr, "Modo de uso: %s -s [1|2] -t tiempo -f archivo -p pipe\n", argv[0]);
        exit(1);
    }

    
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-s") == 0)
            sensor = argv[i + 1];
        else if (strcmp(argv[i], "-t") == 0)
            tiempo = argv[i + 1];
        else if (strcmp(argv[i], "-f") == 0)
            archivo = argv[i + 1];
        else if (strcmp(argv[i], "-p") == 0)
            pipe_name = argv[i + 1];
    }

    if (!sensor || !archivo || !tiempo || !pipe_name) {
        fprintf(stderr, "Argumentos inválidos.\n");
        fprintf(stderr, "Modo de uso: %s -s [1|2] -t tiempo -f archivo -p pipe\n", argv[0]);
        exit(1);
    }


    tiempo_espera = atoi(tiempo); //entra como cadena de caracteres y se necesita en entero
    sensor_type = atoi(sensor); //para ver si es 1 (t) o 2 (pH)

    printf("Abriendo el pipe...\n");
    fd = open(pipe_name, O_WRONLY); //Para abrir el pipe siempre se hace esto
    if (fd == -1) {
        perror("Error abriendo el pipe");
        exit(1);
    } else {
        printf("Pipe abierto correctamente...\n");
    }

    FILE *file = fopen(archivo, "r"); //Abrir archivo en modo lectura
    if (!file) {
        fprintf(stderr, "Error al abrir el archivo: %s\n", archivo);
        close(fd);
        exit(1);
    } else {
        printf("Archivo abierto correctamente...\n");
    }

  //Bucle para leer archivo y enviar datos por el pipe
    while (fgets(line, sizeof(line), file)) { //leer línea por línea
        value = atof(line);
        snprintf(buffer, sizeof(buffer), "%d:%f\n", sensor_type, value); //permite evitar desbordamientos de buffer al especificar el tamaño máximo del buffer donde se almacenará la cadena formateada
        printf("Sensor envía %d:%f\n", sensor_type, value);

        bytes_written = write(fd, buffer, strlen(buffer));
        if (bytes_written == -1) {
            perror("Error escribiendo en el pipe");
            break;
        }

        sleep(tiempo_espera); //cada lectura espera un tiempo
    }

    fclose(file);
    close(fd);
}

int main(int argc, char *argv[]) {
    EjecutarSensor(argc, argv);
    return 0;
}
