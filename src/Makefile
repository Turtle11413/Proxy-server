CC = gcc -lstdc++
CFLAGS = -Wall -Wextra -Werror -std=c++17
SRC = main.cc proxy-server/proxy.cc

ifeq ($(OS),Linux)
  OPEN_CMD=xdg-open
	LEAKS_CMD=valgrind --leak-check=full \
						--show-leak-kinds=all \
						--track-origins=yes
	ADD_LIB=-lgtest -lgtest_main -lpthread -lm
	GCOV_FLAGS=
endif

ifeq ($(OS),Darwin)
	OPEN_CMD=open -a "Google Chrome"
	LEAKS_CMD=leaks -atExit --
	ADD_LIB=-lgtest -lgtest_main -lm
	GCOV_FLAGS=--ignore-errors mismatch
endif

all: run_proxy

proxy:
	@$(CC) $(CFLAGS) $(SRC) -o $@

run_proxy: proxy
	@./proxy 4568 127.0.0.1 5432

rebuild: clean run_proxy

test: clean
	@$(CC) $(CFLAGS) $(ADD_LIB) proxy-server/*.cc tests/*.cc -o $@.out
	@$(LEAKS_CMD) ./$@.out

leaks: clean proxy
	@$(LEAKS_CMD) ./proxy 4568 127.0.0.1 5432

clean:
	@rm -rf *.o *.out proxy
