CC = gcc
AR = ar
OPTS =
WARN = -Wall

all: libnvbdf.a bdfbanner

libnvbdf.a: nvbdflib.o 
	$(AR) r libnvbdf.a nvbdflib.o

nvbdflib.o: nvbdflib.c
	$(CC) -c -o nvbdflib.o nvbdflib.c $(OPTS) $(WARN)

bdfbanner: bdfbanner.c
	$(CC) -o bdfbanner bdfbanner.c libnvbdf.a $(OPTS) $(WARN)

clean:
	rm -f nvbdflib.o libnvbdf.a bdfbanner



