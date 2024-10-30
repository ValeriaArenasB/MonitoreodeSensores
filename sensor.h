/*
  Archivo: sensor.h  
  Autores: Valeria Arenas, Tatiana Vivas, Daniel Leon
  Objetivo: Declarar las funciones necesarias para el sensor.
  Módulos o funciones:
    - EjecutarSensor: Ejecuta el sensor que lee datos desde un archivo y los envía por un pipe.
  Fecha de finalización: 15 de mayo de 2024
 */

#ifndef SENSOR_H
#define SENSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

void EjecutarSensor(int argc, char *argv[]);

#endif // SENSOR_H
