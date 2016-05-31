CFLAGS=-Icrc32-vpmsum

CRC32_POLY=0x04C11DB7
CRC32C_POLY=0x1EDC6F41

PROGS=  crc32be-vx.o \
	crc32le-vx.o \
	libcrc32_s390x.a \
	crc32-cli \
	crc32_constants \
	crc32_be_constants.h \
	crc32_le_constants.h \
	crc32c_be_constants.h \
	crc32c_le_constants.h \
	crc32_be_test \
	crc32_le_test \
	crc32c_be_test \
	crc32c_le_test \
	crc32_vx_bench \
	crc32_sw_bench

all: $(PROGS)

crc32be-vx.o: crc32be-vx.S
	$(CC) -c crc32be-vx.S

crc32le-vx.o: crc32le-vx.S
	$(CC) -c crc32le-vx.S

libcrc32_s390x.a: crc32-s390x.o crc32be-vx.o crc32le-vx.o
	ar rcs $@ $^ 

crc32-cli: crc32-cli.o libcrc32_s390x.a

crc32_constants.o: crc32-vpmsum/crc32_constants.c
	$(CC) -o $@ -c $^

poly_arithmetic.o: crc32-vpmsum/poly_arithmetic.c
	$(CC) -o $@ -c $^

crcmodel.o: crc32-vpmsum/crcmodel.c
	$(CC) -o $@ -c $^

crc32_constants: crc32_constants.o poly_arithmetic.o crcmodel.o

crc32_be_constants.h: crc32_constants
	./crc32_constants $(CRC32_POLY) > crc32_be_constants.h

crc32_le_constants.h: crc32_constants
	./crc32_constants -r $(CRC32_POLY) > crc32_le_constants.h

crc32c_be_constants.h: crc32_constants
	./crc32_constants $(CRC32C_POLY) > crc32c_be_constants.h

crc32c_le_constants.h: crc32_constants
	./crc32_constants -r $(CRC32C_POLY) > crc32c_le_constants.h

crc32_be_test.o: crc32-stress.c crc32_be_constants.h
	$(CC) $(CFLAGS) -DCRC_VX_FUNC=crc32_be_vx -include crc32_be_constants.h -o $@ -c crc32-stress.c

crc32_le_test.o: crc32-stress.c crc32_le_constants.h
	$(CC) $(CFLAGS) -DCRC_VX_FUNC=crc32_le_vx -include crc32_le_constants.h -o $@ -c crc32-stress.c

crc32c_be_test.o: crc32-stress.c crc32c_be_constants.h
	$(CC) $(CFLAGS) -DCRC_VX_FUNC=crc32c_be_vx -include crc32c_be_constants.h -o $@ -c crc32-stress.c

crc32c_le_test.o: crc32-stress.c crc32c_le_constants.h
	$(CC) $(CFLAGS) -DCRC_VX_FUNC=crc32c_le_vx -include crc32c_le_constants.h -o $@ -c crc32-stress.c

crc32_be_test: crc32_be_test.o crcmodel.o libcrc32_s390x.a

crc32_le_test: crc32_le_test.o crcmodel.o libcrc32_s390x.a

crc32c_be_test: crc32c_be_test.o crcmodel.o libcrc32_s390x.a

crc32c_le_test: crc32c_le_test.o crcmodel.o libcrc32_s390x.a

crc32_sw_bench.o: crc32-bench.c
	$(CC) $(CFLAGS) -DCRC_FUNC=crc32c_le -o $@ -c crc32-bench.c

crc32_vx_bench.o: crc32-bench.c
	$(CC) $(CFLAGS) -DCRC_FUNC=crc32c_le_vx -o $@ -c crc32-bench.c

crc32_sw_bench: crc32_sw_bench.o libcrc32_s390x.a

crc32_vx_bench: crc32_vx_bench.o libcrc32_s390x.a

clean:
	rm -f  *.o $(PROGS)
