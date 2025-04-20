

#include <err.h>
#include <stdio.h>
#include <unistd.h>

/*
 * Bootable (BPUN) tape format.
 * Disks can use it as well with a max of 64 words data.  In this case 
 * the bytes are stored in the LSB of the words from beginning of disk.
 * 1kw block should be read at address 0 in memory.
 *
 * A bootable tape consists of nine segments, named A-I.
 *
 * A - Any chars not including '!'
 * B - (optional) octal number terminated by CR (LF ignored).
 * C - (optional) octal number terminated by '!'.
 * D - A '!' delimeter
 * E - Block start address (in memory), two byte, MSB first.
 * F - Word count in G section, two byte, MSB first.
 * G - Words as counted in F section.
 * H - Checksum of G section, one word.
 * I - Action code.  If non-zero, start at address in B, otherwise nothing.
 */


int debug, start, verbose;

int magic, dsize, entry;
char *ofile = "o.bpun";
FILE *ofd;

void rdhdr(void);
void wrwd(int word);
void wrfile(void);
void wrstart(void);
int rd2b(void);
void wrdblk(int addr, int sz);

/*
 * Convert an a.out binary to a BPUN file.
 */
int
main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "dsvo:")) != -1) {
		switch (ch) {
		case 'd': debug = 1; break;
		case 's': start = 1; break;
		case 'v': verbose = 1; break;
		case 'o': ofile = optarg; break;
		default:
			errx(1, "unknown arg '%c'", ch);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 0) {
		if (freopen(argv[0], "r", stdin) == NULL)
			err(1, "freopen %s", argv[0]);
	}

	if ((ofd = fopen(ofile, "w")) == NULL)
		err(1, "fopen %s", ofile);

	rdhdr();

	wrfile();
	return 0;
}

/*
 * Read a.out header.
 */
void
rdhdr(void)
{
	magic = rd2b();
	dsize = rd2b();		/* text size */
	dsize += rd2b();	/* data size */
	rd2b();
	rd2b();
	entry = rd2b();
	rd2b();
	rd2b();
	if (verbose)
		printf("read magic 0%o dsize 0%o\n", magic, dsize);
}

/*
 * Read in a word from file.  Low byte first?
 */
int
rd2b(void)
{
	int rv;

	rv = fgetc(stdin) & 0377;
	rv |= (fgetc(stdin) & 0377) << 8;
	return rv;
}

void
wrfile(void)
{
	int i, cksum;

	/* Just a 0 for A */
	putw(0, ofd);

	/* 42 + CRLF for B */
	putw(('4' << 8) | '2', ofd);
	putw(('\r' << 8) | '\n', ofd);

	/* Start address for C */
	wrwd(entry);

	/* ! == D */
	fputc('!', ofd);

	/* loaded block start address, will be 0, E */
	wrwd(0);

	/* Word count, section F */
	wrwd(dsize);

	/* All words, section G */
	cksum = 0;
	for (i = 0; i < dsize; i++) {
		unsigned short w = rd2b();
		cksum += w;
		wrwd(w);
	}

	/* Checksum, H */
	wrwd(cksum);

	/* Start code, never, I */
	wrwd(0);
}

void
wrwd(int wd)
{
	fputc((wd >> 8) & 0377, ofd);
	fputc(wd & 0377, ofd);
}
