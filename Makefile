all: server test_srv clean

server: srv/main.c
	@gcc -O3 -o server srv/main.c

.PHONY:
valgrind: clean server
	@gcc -o server -g3 srv/main.c
	@valgrind ./server 32152 4 4
	@rm -f -- vgcore*

.PHONY: test_srv
test_srv: server
	@./server 32152 4 4

.PHONY: test_clt
test_clt:
	@./client/main.py 127.0.0.1 32152

.PHONY:
clean:
	@rm -f -- server
	@rm -f -- vgcore*
