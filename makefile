SOURCE += src/worker/utils.c
SOURCE += src/worker/worker.c

TARGET = bin/worker.a

OPTIONS += -std=c11 -Wall -Wextra -pedantic -Werror
OPTIONS += -D_GNU_SOURCE -pthread -O3
OPTIONS += -Isrc/header/

OBJECTS = $(SOURCE:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar scr $@ $^

clean:
	rm -f $(TARGET) $(OBJECTS)

%.o: %.c
	gcc $(OPTIONS) -c $^ -o $@
