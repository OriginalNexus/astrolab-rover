buildrun:
	mkdir -p bin/
	gcc -g -o bin/server.out -lwiringPi -lm -Wall server.c -lncurses
	sudo ./bin/server.out

build:
	mkdir -p bin/
	gcc -g -o bin/server.out -lwiringPi -lm -Wall server.c -lncurses

run:
	sudo ./bin/server.out

clean:
	rm -r -f bin/

