# Nombre del archivo: Makefile
# Autores: Valeria Arenas, Tatiana Vivas, Daniel Leon
# Objetivo: Compilar y gestionar el proyecto que incluye los programas del sensor y del monitor.
# Módulos o funciones: Reglas de compilación para sensor, monitor, y limpieza del proyecto.
# Fecha de finalización: 14 de mayo de 2024

CC = gcc
CFLAGS = -pthread -o

all: sensor monitor

sensor: sensor.c sensor.h
	$(CC) sensor.c $(CFLAGS) sensor

monitor: monitor.c monitor.h
	$(CC) monitor.c $(CFLAGS) monitor

clean:
	rm -f sensor monitor