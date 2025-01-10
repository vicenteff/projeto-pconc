all: old-photo-parallel-A old-photo-parallel-B 

old-photo-parallel-A: 
	gcc -Wall old-photo-parallel-A.c image-lib.c image-lib.h -g -o old-photo-parallel-A -lgd

old-photo-parallel-B: 
	gcc -Wall old-photo-parallel-B.c image-lib.c image-lib.h old-photo-parallel-B-functions.c -g -o old-photo-parallel-B -lgd

clean:
	rm -rf old_photo_PAR_A old_photo_PAR_B
	rm old-photo-parallel-A old-photo-parallel-B 
	
