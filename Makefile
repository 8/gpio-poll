# Name of the output file
NAME = gpio-poll

SRCS = $(wildcard *.h) $(wildcard *.c)

all: gpio-poll

gpio-poll: $(SRCS)
	$(CC)  $^ -o $@

clean:
	rm -f gpio-poll
