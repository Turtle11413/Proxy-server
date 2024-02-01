CC = gcc -lstdc++
CFLAGS = -Wall -Wextra -Werror -std=c++17
SRC = main.cc proxy-server/proxy.cc
HOST = 127.0.0.1

all: run_proxy

proxy:
	@$(CC) $(CFLAGS) $(SRC) -o $@

run_proxy: proxy
	@./proxy $(HOST) 5432 4568

clean:
	@rm -rf *.o *.out proxy
