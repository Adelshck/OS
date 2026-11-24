CFLAGS ?= -std=c11 -O2 -Wall -Wextra

.PHONY: all clean

all: libimpl1.so libimpl2.so prog1 prog2

libimpl1.so: impl1.c contracts.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC $(LDFLAGS) -shared -o $@ $< $(LDLIBS) -lm

libimpl2.so: impl2.c contracts.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC $(LDFLAGS) -shared -o $@ $< $(LDLIBS)

prog1: prog1.c commands.c commands.h contracts.h libimpl1.so
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ prog1.c commands.c -L. -Wl,-rpath,'$$ORIGIN' -limpl1 $(LDLIBS)

prog2: prog2.c commands.c commands.h | libimpl1.so libimpl2.so
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ prog2.c commands.c $(LDLIBS) -ldl

clean:
	$(RM) libimpl1.so libimpl2.so prog1 prog2
