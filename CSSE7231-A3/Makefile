all: jobthing mytee
jobthing: jobthing.c
	gcc -Wall -pedantic -std=gnu99 -g -o $@ $< -L/local/courses/csse2310/lib -lcsse2310a3 -I/local/courses/csse2310/include
mytee: mytee.c
	gcc -Wall -pedantic -std=gnu99 -g -o $@ $< -L/local/courses/csse2310/lib -lcsse2310a3 -I/local/courses/csse2310/include

clean:
	rm jobthing
	rm mytee
