buildrun:
	mkdir -p bin/
	gcc -g -lwiringPi -lncurses -lm -Wall -o bin/server.out server.c
	sudo ./bin/server.out

build:
	mkdir -p bin/
	gcc -g -lwiringPi -lncurses -lm -Wall -o bin/server.out server.c

run:
	sudo ./bin/server.out

clean:
	rm -r -f bin/

