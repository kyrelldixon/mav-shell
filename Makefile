mfs:
	gcc -Wall mfs.c -o mfs

run:
	./mfs

clean:
	rm -rf *.o *.out mfs