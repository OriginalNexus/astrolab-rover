buildrun:
	mkdir -p bin/
	gcc -g -o bin/server.out -lwiringPi -lncurses -lm -Wall server.c
	sudo ./bin/server.out

build:
	mkdir -p bin/
	gcc -g -o bin/server.out -lwiringPi -lncurses -lm -Wall server.c

run:
	sudo ./bin/server.out

clean:
	rm -r -f bin/

