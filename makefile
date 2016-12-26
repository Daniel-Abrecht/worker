SOURCE += src/main.c
SOURCE += src/worker/utils.c
SOURCE += src/worker/worker.c

OPTIONS += -std=c11 -Wall -Wextra -pedantic -Werror
OPTIONS += -D_GNU_SOURCE -pthread -g
OPTIONS += -Isrc/header/

all: bin/main

bin/main: $(SOURCE)
	gcc $(OPTIONS) $^ -o $@

clean:
	rm bin/main
