CC=gcc
CFLAGS=-Wall -Wno-unused-value

caltrain: caltrain_runner.c caltrain.c caltrain.h
	$(CC) $(CFLAGS) -o caltrain caltrain_runner.c caltrain.c caltrain.h -lpthread

clean:
	rm  caltrain