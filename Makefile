all: test

commands: commands.c
	gcc -std=gnu99 -O -o commands commands.c

clean:
	rm -f commands

test: commands
	stty -echo cbreak && ./commands
	stty -cbreak echo
