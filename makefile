CC = gcc
CFLAGS = -std=c99 -lrt -lncurses
TARGET = semaforo
OBJS = main.o vehiculo.o cola.o
ARG = 123

default: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(CFLAGS)

# Rule to build vehiculo.o
vehiculo.o: vehiculo.c vehiculo.h
	$(CC) -c vehiculo.c -o vehiculo.o $(CFLAGS)

cola.o: cola.c cola.h
	$(CC) -c cola.c -o cola.o $(CFLAGS)

run:
	./$(TARGET) $(ARG)

clean:
	rm -f *.o

purge:
	rm -f $(TARGET)