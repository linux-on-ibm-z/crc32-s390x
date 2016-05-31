/**
 * Based on lib/gen_crc32table.c and lib/crc32defs.h in the Linux kernel.
 * This program has been modified to generate the slicing constants for
 * (bit-reflected) CRC-32 (Ethernet) and CRC-32C (Castagnoli), for a
 * big-endian system (e.g. s390x). 
 */

#include <stdio.h>
#include <inttypes.h>

/*
 * There are multiple 16-bit CRC polynomials in common use, but this is
 * *the* standard CRC-32 polynomial, first popularized by Ethernet.
 * x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x^1+x^0
 */
#define CRCPOLY_LE 0xedb88320
#define CRCPOLY_BE 0x04c11db7

/*
 * This is the CRC-32C polynomial, as outlined by Castagnoli.
 * x^32+x^28+x^27+x^26+x^25+x^23+x^22+x^20+x^19+x^18+x^14+x^13+x^11+x^10+x^9+
 * x^8+x^6+x^0
 */
#define CRC32C_POLY_BE 0x1edc6f41
#define CRC32C_POLY_LE 0x82F63B78

/*
 * How many bits at a time to use.  Valid values are 1, 2, 4, 8, 32 and 64.
 * For less performance-sensitive, use 4 or 8 to save table size.
 * For larger systems choose same as CPU architecture as default.
 * This works well on X86_64, SPARC64 systems. This may require some
 * elaboration after experiments with other architectures.
 */
#define CRC_LE_BITS 64
#define CRC_BE_BITS 64

#define ENTRIES_PER_LINE 4

#if CRC_LE_BITS > 8
# define LE_TABLE_ROWS (CRC_LE_BITS/8)
# define LE_TABLE_SIZE 256
#else
# define LE_TABLE_ROWS 1
# define LE_TABLE_SIZE (1 << CRC_LE_BITS)
#endif

#if CRC_BE_BITS > 8
# define BE_TABLE_ROWS (CRC_BE_BITS/8)
# define BE_TABLE_SIZE 256
#else
# define BE_TABLE_ROWS 1
# define BE_TABLE_SIZE (1 << CRC_BE_BITS)
#endif

static uint32_t crc32table_le[LE_TABLE_ROWS][256];
static uint32_t crc32table_be[BE_TABLE_ROWS][256];
static uint32_t crc32ctable_le[LE_TABLE_ROWS][256];
static uint32_t crc32ctable_be[BE_TABLE_ROWS][256];

/**
 * crc32init_le() - allocate and initialize LE table data
 *
 * crc is the crc of the byte i; other entries are filled in based on the
 * fact that crctable[i^j] = crctable[i] ^ crctable[j].
 *
 */
static void crc32init_le_generic(const uint32_t polynomial,
                                 uint32_t (*tab)[256])
{
        unsigned i, j;
        uint32_t crc = 1;

        tab[0][0] = 0;

        for (i = LE_TABLE_SIZE >> 1; i; i >>= 1) {
                crc = (crc >> 1) ^ ((crc & 1) ? polynomial : 0);
                for (j = 0; j < LE_TABLE_SIZE; j += 2 * i)
                        tab[0][i + j] = crc ^ tab[0][j];
        }

        for (i = 0; i < LE_TABLE_SIZE; i++) {
                crc = tab[0][i];
                for (j = 1; j < LE_TABLE_ROWS; j++) {
                        crc = tab[0][crc & 0xff] ^ (crc >> 8);
                        tab[j][i] = crc;
                }
        }
}

static void crc32init_le(void)
{
        crc32init_le_generic(CRCPOLY_LE, crc32table_le);
}

static void crc32cinit_le(void)
{
        crc32init_le_generic(CRC32C_POLY_LE, crc32ctable_le);
}

/**
 * crc32init_be() - allocate and initialize BE table data
 */
static void crc32init_be_generic(const uint32_t polynomial,
                                 uint32_t (*tab)[256])
{
        unsigned i, j;
        uint32_t crc = 0x80000000;

        tab[0][0] = 0;

        for (i = 1; i < BE_TABLE_SIZE; i <<= 1) {
                crc = (crc << 1) ^ ((crc & 0x80000000) ? polynomial : 0);
                for (j = 0; j < i; j++)
                        tab[0][i + j] = crc ^ tab[0][j];
        }

        for (i = 0; i < BE_TABLE_SIZE; i++) {
                crc = tab[0][i];
                for (j = 1; j < BE_TABLE_ROWS; j++) {
                        crc = tab[0][(crc >> 24) & 0xff] ^ (crc << 8);
                        tab[j][i] = crc;
                }
        }
}

static void crc32init_be(void)
{
        crc32init_be_generic(CRCPOLY_BE, crc32table_be);
}

static void crc32cinit_be(void)
{
        crc32init_be_generic(CRC32C_POLY_BE, crc32ctable_be);
}

static void output_table(uint32_t (*table)[256], int rows, int len)
{
        int i, j;

        for (j = 0 ; j < rows; j++) {
                if (j == 0)
                        printf("\t");
                printf("{");
                for (i = 0; i < len - 1; i++) {
                        if (i % ENTRIES_PER_LINE == 0)
                                printf("\n\t");
                        printf("0x%8.8x,", table[j][i]);
                        if (i % ENTRIES_PER_LINE < ENTRIES_PER_LINE - 1)
                                printf(" ");
                }
                printf("0x%8.8x\n\t}", table[j][len - 1]);
                if (j < rows - 1)
                        printf(",");
                else
                        printf("\n");
        }
}

static void output_bswap_table(uint32_t (*table)[256], int rows, int len)
{
        int i, j;

        for (j = 0 ; j < rows; j++) {
                if (j == 0)
                        printf("\t");
                printf("{");
                for (i = 0; i < len - 1; i++) {
                        if (i % ENTRIES_PER_LINE == 0)
                                printf("\n\t");
                        printf("0x%8.8x,", __builtin_bswap32(table[j][i]));
                        if (i % ENTRIES_PER_LINE < ENTRIES_PER_LINE - 1)
                                printf(" ");
                }
                printf("0x%8.8x\n\t}", __builtin_bswap32(table[j][len - 1]));
                if (j < rows - 1)
                        printf(",");
                else
                        printf("\n");
        }
}

int main(int argc, char** argv)
{
        printf("/* CRC-32 and CRC-32C slicing-by-8 constants, for use on big-endian systems. */\n");
#if 0
        printf("/* AUTO-GENERATED FILE - DO NOT EDIT */\n\n");
        printf("#include <endian.h>\n");
        printf("#if __BYTE_ORDER == __BIG_ENDIAN\n");
        printf("#  define tobe(x) (x)\n");
        printf("#  define tole(x) __builtin_bswap32(x)\n");
        printf("#else\n");
        printf("#  define tobe(x) __builtin_bswap32(x)\n");
        printf("#  define tole(x) (x)\n");
        printf("#endif\n\n");
#endif
        if (CRC_LE_BITS > 1) {
                crc32init_le();
                printf("static const unsigned int __attribute__((aligned(128))) ");
                printf("crc32table_le[%d][%d] = {\n", LE_TABLE_ROWS, LE_TABLE_SIZE);
                output_bswap_table(crc32table_le, LE_TABLE_ROWS, LE_TABLE_SIZE);
                printf("};\n\n");
        }

        if (CRC_BE_BITS > 1) {
                crc32init_be();
                printf("static const unsigned int __attribute__((aligned(128))) ");
                printf("crc32table_be[%d][%d] = {\n", BE_TABLE_ROWS, BE_TABLE_SIZE);
                output_table(crc32table_be, BE_TABLE_ROWS, BE_TABLE_SIZE);
                printf("};\n\n");
        }

        if (CRC_LE_BITS > 1) {
                crc32cinit_le();
                printf("static const unsigned int __attribute__((aligned(128))) ");
                printf("crc32ctable_le[%d][%d] = {\n", LE_TABLE_ROWS, LE_TABLE_SIZE);
                output_bswap_table(crc32ctable_le, LE_TABLE_ROWS, LE_TABLE_SIZE);
                printf("};\n\n");
        }

        if (CRC_BE_BITS > 1) {
                crc32cinit_be();
                printf("static const unsigned int __attribute__((aligned(128))) ");
                printf("crc32ctable_be[%d][%d] = {\n", BE_TABLE_ROWS, BE_TABLE_SIZE);
                output_table(crc32ctable_be, BE_TABLE_ROWS, BE_TABLE_SIZE);
                printf("};\n\n");
        }

        return 0;
}
