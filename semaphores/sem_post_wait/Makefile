
XENO_CONFIG := /usr/xenomai/bin/xeno-config
CFLAGS := $(shell $(XENO_CONFIG) --posix --cflags)
LDFLAGS := $(shell $(XENO_CONFIG) --posix --ldflags) 
LDFLAGS_EX := -Wl,-rpath=/usr/xenomai/lib
CC := $(shell $(XENO_CONFIG) --cc)

EXCUTABLE := sem_post_wait

all: $(EXCUTABLE)

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS) $(LDFLAGS_EX)
