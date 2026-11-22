#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <string>

extern "C" {
#include "passgen.h"
#include "fileio.h"
#include "hash.h"
#include "sha256.h"
#include "entropy.h"
#include "charset.h"
#include "generator.h"
}

static std::string ext_hex(const uint8_t *d, size_t n) {
    static const char *digits = "0123456789abcdef";
    std::string s;
    for (size_t i = 0; i < n; i++) {
        s += digits[d[i] >> 4];
        s += digits[d[i] & 0x0f];
    }
    return s;
}

static std::string ext_sha_hex(const std::string &msg) {
    uint8_t out[SHA256_SIZE];
    sha256(msg.data(), msg.size(), out);
    return ext_hex(out, SHA256_SIZE);
}

enum ExtFill { EXT_PATTERN_A, EXT_PATTERN_B, EXT_CONSTANT, EXT_UNIFORM };

static void ext_make_file(const char *path, long size, ExtFill fill) {
    FILE *f = std::fopen(path, "wb");
    ASSERT_NE(f, nullptr);
    for (long i = 0; i < size; i++) {
        unsigned char b = 0;
        switch (fill) {
        case EXT_PATTERN_A: b = (unsigned char)((i * 131 + 7) ^ (i >> 3)); break;
        case EXT_PATTERN_B: b = (unsigned char)((i * 197 + 61) ^ (i >> 5)); break;
        case EXT_CONSTANT:  b = 0xFF; break;                
        case EXT_UNIFORM:   b = (unsigned char)(i % 256); break; 
        }
        std::fputc(b, f);
    }
    std::fclose(f);
}

static pg_config ext_config(const char *key) {
    pg_config cfg;
    std::memset(&cfg, 0, sizeof cfg);
    cfg.key = key;
    cfg.length = PG_DEFAULT_LEN;
    cfg.version = 1;
    cfg.charset = CS_ALL;
    return cfg;
}

static void ext_add_file(pg_config *cfg, const char *path) {
    cfg->files[cfg->file_count++] = path;
}

static double ext_entropy_of(const char *path)
{
    pg_file f;
    if (file_open(path, &f) != PG_OK)
        return -100.0;
    double h = file_entropy(&f);
    file_close(&f);
    return h;
}

static const char *EXT_FILE_A = "part_a_ext_a.bin";     
static const char *EXT_FILE_B = "part_a_ext_b.bin";    
static const char *EXT_FILE_SMALL = "part_a_ext_small.bin"; 
static const char *EXT_FILE_WHITE = "part_a_ext_white.bin";
static const char *EXT_FILE_UNI = "part_a_ext_uni.bin";  

class PartAExtFiles : public ::testing::Environment {
public:
    void SetUp() override
    {
        ext_make_file(EXT_FILE_A, 40000, EXT_PATTERN_A);
        ext_make_file(EXT_FILE_B, 30000, EXT_PATTERN_B);
        ext_make_file(EXT_FILE_SMALL, 100, EXT_PATTERN_A);
        ext_make_file(EXT_FILE_WHITE, 4096, EXT_CONSTANT);
        ext_make_file(EXT_FILE_UNI, 2048, EXT_UNIFORM);
    }
    void TearDown() override
    {
        std::remove(EXT_FILE_A);
        std::remove(EXT_FILE_B);
        std::remove(EXT_FILE_SMALL);
        std::remove(EXT_FILE_WHITE);
        std::remove(EXT_FILE_UNI);
    }
};

static ::testing::Environment *const part_a_ext_env =
    ::testing::AddGlobalTestEnvironment(new PartAExtFiles);



TEST(Sha256, Abc)
{
    EXPECT_EQ(ext_sha_hex("abc"),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(Sha256, Empty)
{
    EXPECT_EQ(ext_sha_hex(""),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(Sha256, TwoBlockMessage)
{
    EXPECT_EQ(ext_sha_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST(Sha256, BlockBoundaries)
{
    EXPECT_EQ(ext_sha_hex(std::string(55, 'x')),
              "d5e285683cd4efc02d021a5c62014694958901005d6f71e89e0989fac77e4072");
    EXPECT_EQ(ext_sha_hex(std::string(56, 'x')),
              "04c26261370ee7541549d16dee320c723e3fd14671e66a099afe0a377c16888e");
    EXPECT_EQ(ext_sha_hex(std::string(64, 'x')),
              "7ce100971f64e7001e8fe5a51973ecdfe1ced42befe7ee8d5fd6219506b5393c");
}

TEST(Sha256, MillionAInChunks)
{
    std::string chunk(1000, 'a');
    sha256_ctx ctx;
    uint8_t out[SHA256_SIZE];
    sha256_init(&ctx);
    for (int i = 0; i < 1000; i++)
        sha256_update(&ctx, chunk.data(), chunk.size());
    sha256_final(&ctx, out);
    EXPECT_EQ(ext_hex(out, SHA256_SIZE),
              "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST(Sha256, ChunkedEqualsWhole)
{
    const char *msg = "The quick brown fox jumps over the lazy dog";
    uint8_t whole[SHA256_SIZE], parts[SHA256_SIZE];
    sha256(msg, std::strlen(msg), whole);

    sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, msg, 10);
    sha256_update(&ctx, msg + 10, 0);
    sha256_update(&ctx, msg + 10, std::strlen(msg) - 10);
    sha256_final(&ctx, parts);

    EXPECT_EQ(ext_hex(whole, SHA256_SIZE), ext_hex(parts, SHA256_SIZE));
}

TEST(Stretch, ZeroIterationsKeepsKey)
{
    uint8_t key[SHA256_SIZE], copy[SHA256_SIZE];
    sha256("abc", 3, key);
    std::memcpy(copy, key, SHA256_SIZE);
    stretch_key(key, 0);
    EXPECT_EQ(std::memcmp(key, copy, SHA256_SIZE), 0);
}

TEST(Stretch, OneIterationIsSha256OfKey)
{
    uint8_t key[SHA256_SIZE], expected[SHA256_SIZE];
    sha256("abc", 3, key);
    sha256(key, SHA256_SIZE, expected);
    stretch_key(key, 1);
    EXPECT_EQ(std::memcmp(key, expected, SHA256_SIZE), 0);
}

TEST(Stretch, IsDeterministicAndDependsOnCount)
{
    uint8_t a[SHA256_SIZE], b[SHA256_SIZE], c[SHA256_SIZE];
    sha256("abc", 3, a);
    std::memcpy(b, a, SHA256_SIZE);
    std::memcpy(c, a, SHA256_SIZE);
    stretch_key(a, 1000);
    stretch_key(b, 1000);
    stretch_key(c, 1001);
    EXPECT_EQ(std::memcmp(a, b, SHA256_SIZE), 0);
    EXPECT_NE(std::memcmp(a, c, SHA256_SIZE), 0);
}

TEST(Seed, KnownValue)
{
    EXPECT_EQ(make_seed("github", 1, ""), 0xeb1e3a30d2496250ULL);
}

TEST(Entropy, ConstantFileIsZero)
{
    EXPECT_NEAR(ext_entropy_of(EXT_FILE_WHITE), 0.0, 1e-9);
}

TEST(Entropy, UniformFileIsEight)
{
    EXPECT_NEAR(ext_entropy_of(EXT_FILE_UNI), 8.0, 1e-9);
}

TEST(Entropy, WhiteFileIsBelowThreshold)
{
    EXPECT_LT(ext_entropy_of(EXT_FILE_WHITE), ENTROPY_LOW);
}

TEST(Entropy, VariedFileIsAboveThreshold)
{
    double h = ext_entropy_of(EXT_FILE_A);
    EXPECT_GT(h, ENTROPY_LOW);
    EXPECT_LE(h, 8.0);
}

TEST(Entropy, NullFileReturnsError)
{
    EXPECT_LT(file_entropy(NULL), 0.0);
}

TEST(Entropy, PasswordBits)
{
    EXPECT_NEAR(password_bits(8, 72), 49.36, 0.01);   
    EXPECT_NEAR(password_bits(6, 10), 19.93, 0.01); 
    EXPECT_NEAR(password_bits(16, 62), 95.27, 0.01); 
}

TEST(Entropy, PasswordBitsBadArgs)
{
    EXPECT_EQ(password_bits(0, 72), 0.0);
    EXPECT_EQ(password_bits(8, 1), 0.0);
    EXPECT_EQ(password_bits(-1, 0), 0.0);
}

TEST(MultiFile, SameInputSamePassword)
{
    pg_config cfg = ext_config("github");
    ext_add_file(&cfg, EXT_FILE_A);
    ext_add_file(&cfg, EXT_FILE_B);
    char a[PG_MAX_LEN + 1], b[PG_MAX_LEN + 1];
    ASSERT_EQ(generate_password(&cfg, a, sizeof a), PG_OK);
    ASSERT_EQ(generate_password(&cfg, b, sizeof b), PG_OK);
    EXPECT_STREQ(a, b);
}

TEST(MultiFile, DiffersFromSingleFile)
{
    pg_config one = ext_config("github");
    ext_add_file(&one, EXT_FILE_A);
    pg_config two = ext_config("github");
    ext_add_file(&two, EXT_FILE_A);
    ext_add_file(&two, EXT_FILE_B);
    one.length = two.length = 32;
    char a[PG_MAX_LEN + 1], b[PG_MAX_LEN + 1];
    ASSERT_EQ(generate_password(&one, a, sizeof a), PG_OK);
    ASSERT_EQ(generate_password(&two, b, sizeof b), PG_OK);
    EXPECT_STRNE(a, b);
}

TEST(MultiFile, OrderDoesNotMatter)
{
    pg_config ab = ext_config("github");
    ext_add_file(&ab, EXT_FILE_A);
    ext_add_file(&ab, EXT_FILE_B);
    pg_config ba = ext_config("github");
    ext_add_file(&ba, EXT_FILE_B);
    ext_add_file(&ba, EXT_FILE_A);
    char a[PG_MAX_LEN + 1], b[PG_MAX_LEN + 1];
    ASSERT_EQ(generate_password(&ab, a, sizeof a), PG_OK);
    ASSERT_EQ(generate_password(&ba, b, sizeof b), PG_OK);
    EXPECT_STREQ(a, b);
}

TEST(MultiFile, PositionsLimitedBySmallestFile)
{
    pg_config cfg = ext_config("github");
    ext_add_file(&cfg, EXT_FILE_A);
    ext_add_file(&cfg, EXT_FILE_SMALL);
    char pw[PG_MAX_LEN + 1];

    cfg.length = 100;  
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_OK);
    EXPECT_EQ(std::strlen(pw), 100u);

    cfg.length = 101;
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_ERR_FILE_SMALL);
}

TEST(MultiFile, MissingSecondFileFails)
{
    pg_config cfg = ext_config("github");
    ext_add_file(&cfg, EXT_FILE_A);
    ext_add_file(&cfg, "no_such_file.bin");
    char pw[PG_MAX_LEN + 1];
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_ERR_FILE_OPEN);
}

TEST(MultiFile, MaxFourFiles)
{
    pg_config cfg = ext_config("github");
    char pw[PG_MAX_LEN + 1];
    for (int i = 0; i < PG_MAX_FILES; i++)
        ext_add_file(&cfg, EXT_FILE_A);
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_OK);

    cfg.file_count = PG_MAX_FILES + 1;
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_ERR_ARGS);
}
