/*
 * Result of a boredom
 *
 * lets try to implement a dumb brute forcing alghorithm for sha256
 * we'll search for a hash knowing a preimage
 * TODO: general collisions search
 */

#include <stdatomic.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#define IS(arg, str) strcmp(arg, str) == 0

#define CANDIDATE_BUFSIZE       4096
#define CANDIDATE_FIRST_CHAR    32   // space
#define CANDIDATE_LAST_CHAR     126  // ~
#define NTHREADS_DEFAULT        16
#define NTHREADS_MAX            64

#define SHA256_DIGEST_SIZE  32  // SHA-256 produces 32 raw bytes (256 bits)
#define SHA256_CHAR_SIZE    64

#define perror_exit(msg)                  \
    do {                                  \
        perror(msg);                      \
        exit(EXIT_FAILURE);               \
    } while(0)

#define error_exit(...)                   \
    do {                                  \
        fprintf(stderr, __VA_ARGS__);     \
        exit(EXIT_FAILURE);               \
    } while (0)

typedef unsigned long long bigint_t;

typedef struct {
    size_t len;
    char   buf[CANDIDATE_BUFSIZE];
} candidate_t;

typedef struct {
    char* target;
    bool  generate;
    int   benchmark,
          nthreads;
} args_t;

typedef struct {
    unsigned char* target;
    int nthread, nthreads;
} bf_targs_t;

typedef struct {
    int nthread, nthreads;
    bigint_t nhashes;
} bm_targs_t;

atomic_int bf_is_found = 0;
atomic_int bm_is_done = 0;

#define DBL_INT_ADD(a,b,c) if (a > 0xffffffff - (c)) ++b; a += c;
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

typedef struct {
	unsigned char data[64];
	unsigned int datalen;
	unsigned int bitlen[2];
	unsigned int state[8];
} SHA256_CTX;

unsigned int k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

void SHA256Transform(SHA256_CTX *ctx, unsigned char data[])
{
	unsigned int a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for (; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void SHA256Init(SHA256_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen[0] = 0;
	ctx->bitlen[1] = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

void SHA256Update(SHA256_CTX *ctx, unsigned char data[], unsigned int len)
{
	for (unsigned int i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			SHA256Transform(ctx, ctx->data);
			DBL_INT_ADD(ctx->bitlen[0], ctx->bitlen[1], 512);
			ctx->datalen = 0;
		}
	}
}

void SHA256Final(SHA256_CTX *ctx, unsigned char hash[])
{
	unsigned int i = ctx->datalen;

	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;
		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;
		while (i < 64)
			ctx->data[i++] = 0x00;
		SHA256Transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	DBL_INT_ADD(ctx->bitlen[0], ctx->bitlen[1], ctx->datalen * 8);
	ctx->data[63] = ctx->bitlen[0];
	ctx->data[62] = ctx->bitlen[0] >> 8;
	ctx->data[61] = ctx->bitlen[0] >> 16;
	ctx->data[60] = ctx->bitlen[0] >> 24;
	ctx->data[59] = ctx->bitlen[1];
	ctx->data[58] = ctx->bitlen[1] >> 8;
	ctx->data[57] = ctx->bitlen[1] >> 16;
	ctx->data[56] = ctx->bitlen[1] >> 24;
	SHA256Transform(ctx, ctx->data);

	for (i = 0; i < 4; ++i) {
		hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}

void SHA256(char* data, size_t len, unsigned char* dest) 
{
	SHA256_CTX ctx;

	SHA256Init(&ctx);
	SHA256Update(&ctx, (unsigned char*)data, len);
	SHA256Final(&ctx, dest);
}

int strtoi(const char* in)
{
    if (in == NULL)
        error_exit("Parsing error: number expected");

    errno = 0;
    char* end;
    long out = strtol(in, &end, 10);

    if (errno != 0)
        perror_exit("Parsing error");
    if (end == in || *end != '\0')
        error_exit("Parsing error: Expected a valid number\n");
    if (out < INT_MIN || out > INT_MAX)
        error_exit("Parsing error: Out of int range\n");

    return (int)out;
}

void hex_to_bytes(const char* hex, unsigned char* out) 
{
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++)
        sscanf(hex + 2*i, "%2hhx", &out[i]);
}

void print_bytes_as_hex(unsigned char* bytes) 
{
    for (int i = 0; i < SHA256_DIGEST_SIZE; i++) {
        printf("%02x", bytes[i]);
    }

    printf("\n");
}

// NTHREADS = 5
// step = NTHREADS
// c c c c c c c c c c c c c 
// ^         ^
//   ^         ^
//     ^         ^
//       ^         ^
//         ^         ^

void next_candidate(candidate_t* c, int step)
{
    int base = CANDIDATE_LAST_CHAR - CANDIDATE_FIRST_CHAR + 1;
    int carry = step;
    int i = (int)c->len - 1;

    while (carry > 0) {
        if (i < 0) {
            if (c->len + 1 >= CANDIDATE_BUFSIZE) 
                return;

            memmove(c->buf + 1, c->buf, c->len + 1);
            c->buf[0] = CANDIDATE_FIRST_CHAR;
            c->len++;
            i = 0;
        }

        int digit = c->buf[i] - CANDIDATE_FIRST_CHAR;
        int sum = digit + carry;
		
        c->buf[i] = CANDIDATE_FIRST_CHAR + (sum % base);
        carry = sum / base;
        i--;
    }

    c->buf[c->len] = '\0';
}

int generate(char* input) 
{
    unsigned char hash[SHA256_DIGEST_SIZE];
    size_t len = strlen(input);

    SHA256(input, len, hash);
    print_bytes_as_hex(hash);
    return 0;
}

void* th_brute_force(void* arg) 
{
    candidate_t  cand = {0};
    bf_targs_t*  targs = (bf_targs_t*)arg;
    
    unsigned char* target = targs->target;

    cand.buf[0] = CANDIDATE_FIRST_CHAR + (targs->nthread - 1);
    cand.buf[1] = '\0';
    cand.len = 1;

    while (!atomic_load(&bf_is_found)) {
        unsigned char hash[SHA256_DIGEST_SIZE];
        SHA256(cand.buf, cand.len, hash);

        if (memcmp(target, hash, SHA256_DIGEST_SIZE) == 0) {
            printf("\nThread: %i\n", targs->nthread);
            printf("-== Found: %s ==-\n", cand.buf);

            print_bytes_as_hex(hash);
            atomic_store(&bf_is_found, 1);
            return NULL;
        } 

        next_candidate(&cand, targs->nthreads);

        if (cand.len >= CANDIDATE_BUFSIZE) {
            printf("End reached! Unbelievable!\n");
            break;
        }
    }
    
    return NULL;
}

void* th_benchmark(void* arg) 
{
    candidate_t cand = {0};
    bm_targs_t* targs = (bm_targs_t*)arg;
    
    cand.buf[0] = CANDIDATE_FIRST_CHAR + (targs->nthread - 1);
    cand.buf[1] = '\0';
    cand.len = 1;

    while (!atomic_load(&bm_is_done)) {
        unsigned char hash[SHA256_DIGEST_SIZE];
        SHA256(cand.buf, cand.len, hash);

        next_candidate(&cand, targs->nthreads);
        targs->nhashes++;

        if (cand.len >= CANDIDATE_BUFSIZE) {
            printf("End reached! Unbelievable!\n");
            break;
        }
    }
    
    return NULL;
}

int brute_force(char* target, int nthreads) 
{
    unsigned char target_bytes[SHA256_DIGEST_SIZE];
    bf_targs_t    targs[NTHREADS_MAX];
    pthread_t     threads[NTHREADS_MAX];

    if (strlen(target) != SHA256_CHAR_SIZE)
        error_exit("Invalid hash size, expected to be %i, got %zu\n", SHA256_CHAR_SIZE, strlen(target));

    hex_to_bytes(target, target_bytes);

    for (int i = 0; i < nthreads; i++) {
        targs[i] = (bf_targs_t){
            .target = target_bytes, 
            .nthread = i + 1,
            .nthreads = nthreads
        };

        pthread_create(&threads[i], NULL, th_brute_force, &targs[i]);
    }

    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}

int benchmark(int nseconds, int nthreads) 
{
    bm_targs_t targs[NTHREADS_MAX];
    pthread_t  threads[NTHREADS_MAX];

    if (nthreads > NTHREADS_MAX)
        error_exit("No more than %i threads.\n", NTHREADS_MAX);

    for (int i = 0; i < nthreads; i++) {
        targs[i] = (bm_targs_t){
            .nthread = i + 1,
            .nthreads = nthreads,
            .nhashes = 0
        };

        pthread_create(&threads[i], NULL, th_benchmark, &targs[i]);
    }

    sleep(nseconds);
    atomic_store(&bm_is_done, 1);

    for (int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("%-8s %12s %18s\n\n", "Thread", "Hashes", "Hashes/second");
    
    bigint_t total = 0, total_hps = 0;

    for (int i = 0; i < nthreads; i++) {
        bm_targs_t arg = targs[i];
        bigint_t hps = arg.nhashes / nseconds;
        char thread[16];

        snprintf(thread, sizeof(thread), "T%02d", arg.nthread);
        printf("%-8s %12llu %18llu\n", thread, arg.nhashes, hps);
        
        total_hps += hps;
        total += arg.nhashes;
    }

    printf("\n%-8s %12llu %18llu\n", "Total:", total, total_hps);
    return 0;
}

void help(const char* program) 
{
    printf("Usage: %s [-g|--generate] [-b SECONDS|--benchmark SECONDS]                  \n"
            "         [-t [NTHREADS]|--threads [NTHREADS]] TARGET                       \n"
            "                                                                           \n"
            " A toy brute forcer for sha256, benchmarking tool.                         \n"
            "                                                                           \n"
            " -g, --generate    Just generate sha256 hash for given TARGET              \n"
            " -b, --benchmark   Benchmark mode, Calculate hashes for given SECONDS,     \n"
            " -h, --help        Show this message and exit                              \n"
            " -t, --threads     Number of threads to use, no more than 64,              \n"
            "                   defaults to 16                                          \n"
            "                                                                           \n"
            "  TARGET:          Input data or sha256 hash                               \n"
            ,program);
        
    exit(EXIT_FAILURE);
} 

args_t parse_args(int argc, char** argv) 
{
    args_t out = {0};
    out.nthreads = NTHREADS_DEFAULT;

	if (argc < 2)
		help(argv[0]);

    for (char** itr = argv + 1; *itr != NULL; itr++) {
        char* arg = *itr;

        if (IS(arg, "--")) {
            if (*(++itr) != NULL) 
                out.target = *itr;
            break;
        } 
        
        if (IS(arg, "-h") || IS(arg, "--help"))
            help(argv[0]);
        else if (IS(arg, "-b") || IS(arg, "--benchmark"))
            out.benchmark = strtoi(*(++itr));
        else if (IS(arg, "-t") || IS(arg, "--threads"))
            out.nthreads = strtoi(*(++itr));
        else if (IS(arg, "-g") || IS(arg, "--generate"))
            out.generate = true;
        else
            out.target = arg;        
    }

    return out;
}

int main(int argc, char** argv)
{
    args_t args = parse_args(argc, argv);

    if (args.generate)
        return generate(args.target);
    else if (args.benchmark)
        return benchmark(args.benchmark, args.nthreads);
    else
        return brute_force(args.target, args.nthreads);
}