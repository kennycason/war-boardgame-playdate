CC      = clang
CFLAGS  = -std=c99 -Wall -Wextra -Wno-unused-function -g -O0 -I ../Source
SRC     = ../Source/piece.c ../Source/board.c ../Source/move.c ../Source/game.c \
          test_piece.c test_board.c test_move.c test_game.c main.c
OUT     = run_tests

.PHONY: all run clean
all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)
