all: syscall_cost ctx_cost

clean:
	rm -f syscall_cost ctx_cost

syscall_cost: syscall_cost.c
	gcc -O2 -o syscall_cost syscall_cost.c -Wall

ctx_cost: ctx_cost.c
	gcc -O2 -o ctx_cost ctx_cost.c -Wall