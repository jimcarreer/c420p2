all: iQ

main.o: main.c
	@gcc -c main.c

customer.o: customer.h customer.c
	@gcc -c customer.c

simout.o: simout.h simout.c
    @gcc -c simout.c

iQ: main.o customer.o simout.o
	@gcc main.o simout.o customer.o -lm -lcurses -lpthread -o iQ

debug: main.o customer.o
	@gcc main.o customer.o -g -lm -lpthread -o debug

clean:
	@rm *.o proj2

