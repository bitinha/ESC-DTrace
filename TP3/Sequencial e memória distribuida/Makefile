CC = gcc
CppC = g++
CFLAGS = -Wall -pedantic -lm -fopenmp -O3 -ftree-vectorize -march=native
CFLAGS_DTRACE = -Wl,--export-dynamic,--strip-all

default: dt_sequencial dt_para dt_pthreads dt_cpp

#dt_otimizado: dt_otimizado.c
#	$(CC) -o dt_otimizado dt_otimizado.c $(CFLAGS)

#dt_para: dt_para.c
#	$(CC) -o dt_para dt_para.c $(CFLAGS)



# re-create new ddt.h if ddt.d file has been changed
ddt.h: ddt.d
	dtrace -h -s ddt.d


# compile dt_otimizado.c
dt_otimizado.o: dt_otimizado.c ddt.h
	$(CC) -c dt_otimizado.c $(CFLAGS)

# have dtrace post-process the object fiiles
ddt_otimizado.o: ddt.d dt_otimizado.o
	dtrace -G -s ddt.d dt_otimizado.o -o ddt_otimizado.o

# link and create executable file "dt_sequencial"
dt_sequencial: ddt_otimizado.o
	$(CC) -o dt_sequencial ddt_otimizado.o dt_otimizado.o ddt.h $(CFLAGS)

	
# compile dt_para.c
dt_para.o: dt_para.c ddt.h
	$(CC) -c dt_para.c $(CFLAGS)

# have dtrace post-process the object fiiles
ddt_para.o: ddt.d dt_para.o
	dtrace -G -s ddt.d dt_para.o -o ddt_para.o

# link and create executable file "dt_para"
dt_para: ddt_para.o
	$(CC) -o dt_para ddt_para.o dt_para.o ddt.h $(CFLAGS)





# compile dt_pthreads.c
dt_pthreads.o: dt_pthreads.c ddt.h
	$(CC) -c dt_pthreads.c $(CFLAGS)

# have dtrace post-process the object files
ddt_pthreads.o: ddt.d dt_pthreads.o
	dtrace -G -s ddt.d dt_pthreads.o -o ddt_pthreads.o

# link and create executable file "dt_pthreads"
dt_pthreads: ddt_pthreads.o
	$(CC) -o dt_pthreads ddt_pthreads.o dt_pthreads.o ddt.h $(CFLAGS)

	

# compile dt.cpp
dt_cpp.o: dt_cpp.cpp ddt.h
	$(CppC) -c dt_cpp.cpp $(CFLAGS) -std=c++11

# have dtrace post-process the object files
ddt_cpp.o: ddt.d dt_cpp.o
	dtrace -G -s ddt.d dt_cpp.o -o ddt_cpp.o

# link and create executable file "dt_cpp"
dt_cpp: ddt_cpp.o
	$(CppC) -o dt_cpp ddt_cpp.o dt_cpp.o ddt.h $(CFLAGS) -std=c++11


	
clean:
	rm -f dt_para dt_sequencial dt_cpp dt_pthreads ddt.h *.o
