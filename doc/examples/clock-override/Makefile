# Copyright (C) 2013  Jérémie Galarneau <jeremie.galarneau@efficios.com>
# Copyright (C) 2014  Mathieu Desnoyers <mathieu.desnoyers@efficios.com>
#
# THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
# OR IMPLIED. ANY USE IS AT YOUR OWN RISK.
#
# Permission is hereby granted to use or copy this program for any
# purpose, provided the above notices are retained on all copies.
# Permission to modify the code and to distribute modified code is
# granted, provided the above notices are retained, and a notice that
# the code was modified is included with the above copyright notice.
#
# This Makefile is not using automake so that users may see how to build
# a program with tracepoint provider probes as stand-alone shared objects.
#
# This makefile is purposefully kept simple to support GNU and BSD make.

ifdef AM_CC
	CC = $(AM_CC)
endif

LIBS = -ldl	# On Linux
#LIBS = -lc	# On BSD
LOCAL_CPPFLAGS += -I.

all: lttng-ust-clock-override-example.so

lttng-ust-clock-override-example.o: lttng-ust-clock-override-example.c
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(AM_CPPFLAGS) \
		$(AM_CFLAGS) -fpic -c -o $@ $<

lttng-ust-clock-override-example.so: lttng-ust-clock-override-example.o
	$(CC) -shared -Wl,--no-as-needed -o $@ $(LDFLAGS) $(CFLAGS) \
		$(AM_LDFLAGS) $(AM_CFLAGS) lttng-ust-clock-override-example.o

.PHONY: clean
clean:
	rm -f *.o *.so
