mfs: mfs.c
	gcc -Wall mfs.c -o mfs

run: mfs
	./mfs

clean:
	rm -rf *.o *.out mfs