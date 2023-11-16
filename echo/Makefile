.PHONY: clean

CC := clang
CFLAGS = -O2 -Wall -I .
OBJS_CLIENT = csapp.o echoclient.o
OBJS_SERVER = csapp.o echoserveri.o echo.o
LIB = -lpthread

main: echoclient echoserver

echoclient: $(OBJS_CLIENT) 
	$(CC) $(CFLAGS) $^ -o $@ $(LIB)

echoserver: $(OBJS_SERVER) 
	$(CC) $(CFLAGS) $^ -o $@ $(LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -MD

clean:
	rm -f *~ *.o *.d main compile_commands.json

-include $(OBJS:.o=.d)