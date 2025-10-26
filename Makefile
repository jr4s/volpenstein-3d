CC = gcc
CFLAGS = -Iinclude
LDFLAGS = -Llib -lfreeglut -lopengl32 -lglu32
SRC = $(wildcard src/*.c)
OUT = wolfenstein

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

run: all
	./$(OUT)

clean:
	del /Q $(OUT)
