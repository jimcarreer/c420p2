all: proj2

main.o: main.c
	@gcc -c main.c

customer.o: customer.h customer.c
	@gcc -c customer.c

proj2: main.o customer.o
	@gcc main.o customer.o -lm -o proj2

clean:
	@rm *.o proj2

