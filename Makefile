# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread
OBJS = proxy.o sock.o

proxy: $(OBJS)
	$(CC) $(CFLAGS) -o proxy $(OBJS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -MD


clean:
	rm -f *~ *.o *.d proxy compile_commands.json


# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

-include $(OBJS:.o=.d)