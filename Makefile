UNAME_S := $(shell uname -s)

all:
ifeq ($(UNAME_S), Darwin)
	make -f Makefile.mac
endif
ifeq ($(UNAME_S), Linux)
	make -f Makefile.linux
endif

clean:
ifeq ($(UNAME_S), Darwin)
	make -f Makefile.mac clean
endif
ifeq ($(UNAME_S), Linux)
	make -f Makefile.linux clean
endif

install:
ifeq ($(UNAME_S), Darwin)
	make -f Makefile.mac install
endif
ifeq ($(UNAME_S), Linux)
	make -f Makefile.linux install
endif

