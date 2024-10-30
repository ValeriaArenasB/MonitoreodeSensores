/*
  Archivo: monitor.h
  Autores: Valeria Arenas, Tatiana Vivas, Daniel Leon
  Objetivo: Declarar las funciones y estructuras necesarias para el monitor
  Módulos o funciones:
    - InicializarBuffers: Inicializa los buffers y los semáforos.
    - LiberarBuffers: Libera los recursos de los buffers y los semáforos.
    - Recolector: Lee datos desde un pipe y los coloca en los buffers correspondientes.
    - HiloTemp: Procesa y almacena datos de temperatura.
    - HiloPh: Procesa y almacena datos de pH.
    - EjecutarMonitor: Ejecuta el monitor que recibe datos de sensores y los procesa.
  Fecha de finalización: 17 de mayo de 2024
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define BUFFER_MAX 100


typedef struct {
    float buf[BUFFER_MAX];
    int in;
    int out;
    sem_t full;
    sem_t empty;
    sem_t mutex;
} sbuf_t;

extern int BUFFER_SIZE;

void InicializarBuffers();
void LiberarBuffers();
void *Recolector(void *arg);
void *HiloTemp();
void *HiloPh();
void EjecutarMonitor(int argc, char *argv[]);

#endif // MONITOR_H
