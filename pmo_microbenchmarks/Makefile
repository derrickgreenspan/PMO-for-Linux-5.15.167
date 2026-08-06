all:
	@echo "Compiling microbenchmarks"
	mkdir -p bin
	gcc -DDETACH=2 -DPMO -mcmodel=large -c  pmalloc.c -o pmalloc_PMO.o
	g++ -DP=8 2dConv.cc -mcmodel=large pmalloc_PMO.o -o bin/2dConv_8threads_PMO.out
	g++ -DP=8 LU.cc pmalloc_PMO.o -o bin/LU_8threads_PMO.out
	g++ -DP=8 gauss.cc pmalloc_PMO.o -o bin/gauss_8threads_PMO.out
	g++ -DP=8 TMM.cc pmalloc_PMO.o -o bin/tmm_8threads_PMO.out

clean:
	rm -rf bin
	rm *.o
