CC = /usr/local/bin/mpicc
CFLAGS = -std=c99 -O3 -lm

all: dt_3

#dt_1: dt_1.c
#	$(CC) -o dt_1 dt_1.c $(CFLAGS)

#dt_1o: dt_1o.c
#	$(CC) -o dt_1o dt_1o.c $(CFLAGS)

dt_3: dt_3.c
	$(CC) -o dt_3 dt_3.c $(CFLAGS)

#dt_3_comm: dt_3_comm.c
#	$(CC) -o dt_3_comm dt_3_comm.c $(CFLAGS)

clean:
	rm -f dt_1 dt_1o dt_3 dt_3_comm
