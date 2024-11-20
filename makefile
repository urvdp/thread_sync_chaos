CC = gcc
CFLAGS = -std=c99 -lrt -lncurses
TARGET = semaforo
OBJS = main.o vehiculo.o cola.o pantalla.o utils.o
ARG = 123

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)

vehiculo.o: vehiculo.c vehiculo.h
	$(CC) -c vehiculo.c -o vehiculo.o $(CFLAGS)

cola.o: cola.c cola.h
	$(CC) -c cola.c -o cola.o $(CFLAGS)

pantalla.o: pantalla.c pantalla.h
	$(CC) -c pantalla.c -o pantalla.o $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -c utils.c -o utils.o $(CFLAGS)

run:
	./$(TARGET) $(ARG)

clean:
	rm -f *.o

purge:
	rm -f $(TARGET)