LIBS = -lm
OBJS = textreader.o demo.o
main:  $(OBJS)
	gcc -o demo $(OBJS) $(LIBS)
clean:
	rm -fv demo $(OBJS)