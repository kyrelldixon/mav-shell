mfs: msh.c
	gcc -Wall msh.c -o msh

run: msh
	./msh

clean:
	rm -rf *.o *.out msh
	