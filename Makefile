all: server clean

server: srv/main.c
	@gcc -o server srv/main.c
	@./server 32152 12 4

.PHONY:
clean:
	@rm -f -- server
