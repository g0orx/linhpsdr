UNAME_S := $(shell uname -s)

all:
ifeq ($(UNAME_S), Darwin)
	$(MAKE) -f Makefile.mac
endif
ifeq ($(UNAME_S), Linux)
	$(MAKE) -f Makefile.linux
endif

clean:
ifeq ($(UNAME_S), Darwin)
	$(MAKE) -f Makefile.mac clean
endif
ifeq ($(UNAME_S), Linux)
	$(MAKE) -f Makefile.linux clean
endif

install:
ifeq ($(UNAME_S), Darwin)
	$(MAKE) -f Makefile.mac install
endif
ifeq ($(UNAME_S), Linux)
	$(MAKE) -f Makefile.linux install
endif

