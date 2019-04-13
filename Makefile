all: server test clean

server: srv/main.c
	@gcc -O3 -o server srv/main.c

.PHONY:
valgrind: clean server
	@valgrind ./server 32152 12 4
	@rm -f -- vgcore*

.PHONY:
test: server
	@./server 32152 12 4

.PHONY:
clean:
	@rm -f -- server
	@rm -f -- vgcore*
