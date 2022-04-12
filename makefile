TARGET=UDPtest
OBJ=obj
OBJD=objd
SRC=src
TESTS=testing


CC=gcc
WARN=-Wall -Wextra -Wpedantic -Wconversion -Wunused-variable -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wdouble-promotion -Wunused-function -Wunused-result
CDEFFLAGS=-std=c11 $(WARN)
CFLAGS=-O3 -Wl,--strip-all,--build-id=none,--gc-sections -fno-ident -D NDEBUG
CFLAGSD=-g -O0 -D LOGGING_ENABLE=1
LIB=-lws2_32


default: debug

$(OBJ):
	mkdir $@
$(OBJD):
	mkdir $@
$(TESTS)/bin:
	mkdir $@


srcs = $(wildcard $(SRC)/*.c)
srcs := $(subst $(SRC)/,,$(srcs))

tests = $(wildcard $(TESTS)/*.c)
tests := $(subst $(TESTS)/,,$(tests))
TESTBINS = $(tests:%.c=$(TESTS)/bin/%)

bulk_srcs = $(wildcard $(SRC)/bulk/*.c)


objs_d = $(srcs:%=$(OBJD)/%.o)
objs_r = $(srcs:%=$(OBJ)/%.o)

objs_test = $(subst $(OBJD)/main.c.o,,$(objs_d))


$(OBJ)/%.c.o: $(SRC)/%.c $(OBJ)
	$(CC) -c $< -o $@ $(CDEFFLAGS) $(CFLAGS)

$(OBJD)/%.c.o: $(SRC)/%.c $(OBJD)
	$(CC) -c $< -o $@ $(CDEFFLAGS) $(CFLAGSD)


debug: $(objs_d)
	$(CC) $^ -o deb$(TARGET).exe $(CDEFFLAGS) $(CFLAGSD) $(LIB)

bulkd: bulkd_impl
bulkd_impl: $(bulk_srcs)
	$(CC) $^ -o deb$(TARGET).exe $(CDEFFLAGS) $(CFLAGSD) $(LIB)

release: $(objs_r)
	$(CC) $^ -o $(TARGET).exe $(CDEFFLAGS) $(CFLAGS) $(LIB)

bulkr: bulkr_impl
bulkr_impl: $(bulk_srcs)
	$(CC) $^ -o $(TARGET).exe $(CDEFFLAGS) $(CFLAGS) $(LIB)

$(TESTS)/bin/%: $(TESTS)/%.c $(objs_test)
	$(CC) $(CDEFFLAGS) $^ -o $@ $(LIB) $(CFLAGSD)

test: $(TESTS)/bin $(TESTBINS)
	for test in $(TESTBINS) ; do ./$$test ; done

clean:
	rm -r -f $(OBJ)
	rm -r -f $(OBJD)
	rm -f $(TARGET).exe
	rm -f deb$(TARGET).exe
	rm -r -f $(TESTS)/bin
