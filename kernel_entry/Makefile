ifndef POSIX
XENO_CONFIG := /usr/xenomai/bin/xeno-config
CFLAGS := $(shell $(XENO_CONFIG) --posix --cflags) -D__XENO__
LDFLAGS := $(shell $(XENO_CONFIG) --posix --ldflags)
LDFLAGS_EX := -Wl,-rpath=/usr/xenomai/lib
CC := $(shell $(XENO_CONFIG) --cc)
else
CFLAGS := -Wall -D_GNU_SOURCE
LDFLAGS := -lpthread -lrt
LDFLAGS_EX :=
CC := gcc
endif

EXCUTABLE := get_program_name \
	     get_pid \
	     clock_gettime

all: $(EXCUTABLE)

%: %.c
	$(CC) -o $@ $< ../util.c $(CFLAGS) $(LDFLAGS) $(LDFLAGS_EX)

clean:
	rm ${EXCUTABLE}

