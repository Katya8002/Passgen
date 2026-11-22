#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>

extern "C" {
#include "passgen.h"
#include "hash.h"
#include "prng.h"
#include "charset.h"
#include "generator.h"
}

static void a_make_test_file(const char *path, long size)
{
    FILE *f = std::fopen(path, "wb");
    ASSERT_NE(f, nullptr);
    for (long i = 0; i < size; i++) {
        unsigned char b = (unsigned char)((i * 131 + 7) ^ (i >> 3));
        std::fputc(b, f);
    }
    std::fclose(f);
}

static pg_config a_make_config(const char *file, const char *key)
{
    pg_config cfg;
    std::memset(&cfg, 0, sizeof cfg);
    cfg.files[0] = file;
    cfg.file_count = 1;
    cfg.key = key;
    cfg.length = PG_DEFAULT_LEN;
    cfg.version = 1;
    cfg.charset = CS_ALL;
    return cfg;
}

static bool a_all_in_set(const char *s, const char *set)
{
    for (; *s; s++)
        if (std::strchr(set, *s) == nullptr)
            return false;
    return true;
}

static bool a_has_any_of(const char *s, const char *set)
{
    for (; *s; s++)
        if (std::strchr(set, *s) != nullptr)
            return true;
    return false;
}

static const char *A_BIG_FILE   = "part_a_big.bin";   
static const char *A_SMALL_FILE = "part_a_small.bin"; 

class PartATestFiles : public ::testing::Environment {
public:
    void SetUp() override
    {
        a_make_test_file(A_BIG_FILE, 50000);
        a_make_test_file(A_SMALL_FILE, 100);
    }
    void TearDown() override
    {
        std::remove(A_BIG_FILE);
        std::remove(A_SMALL_FILE);
    }
};

static ::testing::Environment *const part_a_files_env =
    ::testing::AddGlobalTestEnvironment(new PartATestFiles);


TEST(Hash, EmptyInputReturnsOffsetBasis)
{
    EXPECT_EQ(hash_fnv1a("", 0, FNV_OFFSET_BASIS), 0xcbf29ce484222325ULL);
}

TEST(Hash, KnownValueForA)
{
    EXPECT_EQ(hash_fnv1a("a", 1, FNV_OFFSET_BASIS), 0xaf63dc4c8601ec8cULL);
}

TEST(Hash, ChunkedEqualsWhole)
{
    uint64_t whole = hash_fnv1a("github", 6, FNV_OFFSET_BASIS);
    uint64_t part = hash_fnv1a("git", 3, FNV_OFFSET_BASIS);
    part = hash_fnv1a("hub", 3, part);
    EXPECT_EQ(whole, part);
}

TEST(Hash, SeedIsDeterministic)
{
    EXPECT_EQ(make_seed("github", 1, ""), make_seed("github", 1, ""));
}

TEST(Hash, DifferentKeysGiveDifferentSeeds)
{
    EXPECT_NE(make_seed("github", 1, ""), make_seed("hubgit", 1, ""));
}

TEST(Hash, DifferentVersionsGiveDifferentSeeds)
{
    EXPECT_NE(make_seed("github", 1, ""), make_seed("github", 2, ""));
}

TEST(Hash, MasterChangesSeed)
{
    EXPECT_NE(make_seed("github", 1, ""), make_seed("github", 1, "secret"));
}

TEST(Hash, SeparatorPreventsCollision)
{
    EXPECT_NE(make_seed("ab", 1, "c"), make_seed("a", 1, "bc"));
}

TEST(Hash, NullMasterEqualsEmpty)
{
    EXPECT_EQ(make_seed("github", 1, NULL), make_seed("github", 1, ""));
}


TEST(Prng, SameSeedSameSequence)
{
    pg_prng a, b;
    prng_init(&a, 12345);
    prng_init(&b, 12345);
    for (int i = 0; i < 100; i++)
        EXPECT_EQ(prng_next(&a), prng_next(&b));
}

TEST(Prng, DifferentSeedsDifferentSequence)
{
    pg_prng a, b;
    prng_init(&a, 1);
    prng_init(&b, 2);
    EXPECT_NE(prng_next(&a), prng_next(&b));
}

TEST(Prng, ZeroSeedIsReplaced)
{
    pg_prng p;
    prng_init(&p, 0);
    EXPECT_NE(p.s, 0u);
    EXPECT_NE(prng_next(&p), 0u);
}

TEST(Prng, KnownFirstValues)
{
    pg_prng p;
    prng_init(&p, 12345);
    EXPECT_EQ(prng_next(&p), 0x9857fb32c9efb5e4ULL);
    EXPECT_EQ(prng_next(&p), 0xc0ceba4b4a71bce4ULL);
    EXPECT_EQ(prng_next(&p), 0x1399ce5b8adb52c4ULL);
}

TEST(Prng, RangeStaysInBounds)
{
    const uint64_t sizes[] = { 1, 2, 3, 7, 10, 72, 1000, 48976 };
    pg_prng p;
    prng_init(&p, 42);
    for (uint64_t n : sizes)
        for (int i = 0; i < 1000; i++)
            EXPECT_LT(prng_range(&p, n), n);
}

TEST(Prng, RangeOfOneIsAlwaysZero)
{
    pg_prng p;
    prng_init(&p, 7);
    for (int i = 0; i < 50; i++)
        EXPECT_EQ(prng_range(&p, 1), 0u);
}

TEST(Prng, RangeOfZeroReturnsZero)
{
    pg_prng p;
    prng_init(&p, 7);
    EXPECT_EQ(prng_range(&p, 0), 0u);
}

TEST(Prng, RangeHitsEveryValue)
{
    int seen[10] = { 0 };
    pg_prng p;
    prng_init(&p, 99);
    for (int i = 0; i < 1000; i++)
        seen[prng_range(&p, 10)] = 1;
    for (int i = 0; i < 10; i++)
        EXPECT_EQ(seen[i], 1) << "value " << i << " never appeared";
}


TEST(Charset, AllTypesGive72)
{
    char alpha[CHARSET_MAX];
    EXPECT_EQ(charset_build(CS_ALL, alpha, sizeof alpha), 72);
    EXPECT_EQ(std::strlen(alpha), 72u);
}

TEST(Charset, NoSymbolsGive62)
{
    char alpha[CHARSET_MAX];
    EXPECT_EQ(charset_build(CS_LOWER | CS_UPPER | CS_DIGITS, alpha, sizeof alpha), 62);
    EXPECT_FALSE(a_has_any_of(alpha, CHARSET_SYMBOLS));
}

TEST(Charset, DigitsOnlyGive10)
{
    char alpha[CHARSET_MAX];
    EXPECT_EQ(charset_build(CS_DIGITS, alpha, sizeof alpha), 10);
    EXPECT_STREQ(alpha, "0123456789");
}

TEST(Charset, EachSingleType)
{
    char alpha[CHARSET_MAX];
    EXPECT_EQ(charset_build(CS_LOWER, alpha, sizeof alpha), 26);
    EXPECT_EQ(charset_build(CS_UPPER, alpha, sizeof alpha), 26);
    EXPECT_EQ(charset_build(CS_SYMBOLS, alpha, sizeof alpha), 10);
}

TEST(Charset, NoFlagsGiveZero)
{
    char alpha[CHARSET_MAX];
    EXPECT_EQ(charset_build(0, alpha, sizeof alpha), 0);
}

TEST(Charset, TooSmallBufferGivesZero)
{
    char alpha[10];
    EXPECT_EQ(charset_build(CS_ALL, alpha, sizeof alpha), 0);
}

TEST(Charset, ByteToCharUsesModulo)
{
    const char *alpha = "abc";
    EXPECT_EQ(byte_to_char(0, alpha, 3), 'a');
    EXPECT_EQ(byte_to_char(4, alpha, 3), 'b');
    EXPECT_EQ(byte_to_char(255, alpha, 3), 'a');  
}

TEST(Charset, PolicyAddsMissingTypes)
{
    char pw[] = "aaaaaaaa";
    pg_prng p;
    prng_init(&p, 1);
    enforce_policy(pw, 8, CS_ALL, &p);
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_LOWER));
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_UPPER));
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_DIGITS));
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_SYMBOLS));
    EXPECT_EQ(std::strlen(pw), 8u);
}

TEST(Charset, PolicyKeepsGoodPassword)
{
    char pw[] = "aB3!aB3!";
    pg_prng p;
    prng_init(&p, 1);
    enforce_policy(pw, 8, CS_ALL, &p);
    EXPECT_STREQ(pw, "aB3!aB3!");
}

TEST(Charset, PolicyWorksForMinimumLength)
{
    char pw[] = "zzzz";
    pg_prng p;
    prng_init(&p, 5);
    enforce_policy(pw, 4, CS_ALL, &p);
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_LOWER));
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_UPPER));
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_DIGITS));
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_SYMBOLS));
}


TEST(Generator, SameInputSamePassword)
{
    pg_config cfg = a_make_config(A_BIG_FILE, "github");
    char a[PG_MAX_LEN + 1], b[PG_MAX_LEN + 1];
    ASSERT_EQ(generate_password(&cfg, a, sizeof a), PG_OK);
    ASSERT_EQ(generate_password(&cfg, b, sizeof b), PG_OK);
    EXPECT_STREQ(a, b);
}

TEST(Generator, DifferentKeysDifferentPasswords)
{
    pg_config c1 = a_make_config(A_BIG_FILE, "github");
    pg_config c2 = a_make_config(A_BIG_FILE, "hubgit");
    char a[PG_MAX_LEN + 1], b[PG_MAX_LEN + 1];
    ASSERT_EQ(generate_password(&c1, a, sizeof a), PG_OK);
    ASSERT_EQ(generate_password(&c2, b, sizeof b), PG_OK);
    EXPECT_STRNE(a, b);
}

TEST(Generator, DifferentVersionsDifferentPasswords)
{
    pg_config c1 = a_make_config(A_BIG_FILE, "github");
    pg_config c2 = a_make_config(A_BIG_FILE, "github");
    c2.version = 2;
    char a[PG_MAX_LEN + 1], b[PG_MAX_LEN + 1];
    ASSERT_EQ(generate_password(&c1, a, sizeof a), PG_OK);
    ASSERT_EQ(generate_password(&c2, b, sizeof b), PG_OK);
    EXPECT_STRNE(a, b);
}

TEST(Generator, PasswordHasRequestedLength)
{
    const int lengths[] = { PG_MIN_LEN, 8, 16, 64, PG_MAX_LEN };
    char pw[PG_MAX_LEN + 1];
    for (int len : lengths) {
        pg_config cfg = a_make_config(A_BIG_FILE, "github");
        cfg.length = len;
        ASSERT_EQ(generate_password(&cfg, pw, sizeof pw), PG_OK);
        EXPECT_EQ((int)std::strlen(pw), len);
    }
}

TEST(Generator, DigitsOnly)
{
    pg_config cfg = a_make_config(A_BIG_FILE, "monobank");
    cfg.charset = CS_DIGITS;
    cfg.length = 6;
    char pw[PG_MAX_LEN + 1];
    ASSERT_EQ(generate_password(&cfg, pw, sizeof pw), PG_OK);
    EXPECT_TRUE(a_all_in_set(pw, CHARSET_DIGITS));
}

TEST(Generator, NoSymbols)
{
    pg_config cfg = a_make_config(A_BIG_FILE, "github");
    cfg.charset = CS_LOWER | CS_UPPER | CS_DIGITS;
    cfg.length = 32;
    char pw[PG_MAX_LEN + 1];
    ASSERT_EQ(generate_password(&cfg, pw, sizeof pw), PG_OK);
    EXPECT_FALSE(a_has_any_of(pw, CHARSET_SYMBOLS));
}

TEST(Generator, AllTypesPresent)
{
    pg_config cfg = a_make_config(A_BIG_FILE, "github");
    char pw[PG_MAX_LEN + 1];
    ASSERT_EQ(generate_password(&cfg, pw, sizeof pw), PG_OK);
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_LOWER));
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_UPPER));
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_DIGITS));
    EXPECT_TRUE(a_has_any_of(pw, CHARSET_SYMBOLS));
}

TEST(Generator, SmallFileWithoutHeaderSkip)
{
    pg_config cfg = a_make_config(A_SMALL_FILE, "github");
    char pw[PG_MAX_LEN + 1];
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_OK);
    EXPECT_EQ(std::strlen(pw), 8u);
}

TEST(Generator, SmallFileFullLengthUsesEveryByte)
{
    pg_config cfg = a_make_config(A_SMALL_FILE, "github");
    cfg.length = 100;
    char pw[PG_MAX_LEN + 1];
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_OK);
    EXPECT_EQ(std::strlen(pw), 100u);
}

TEST(Generator, PasswordLongerThanFileFails)
{
    pg_config cfg = a_make_config(A_SMALL_FILE, "github");
    cfg.length = 101;
    char pw[PG_MAX_LEN + 1];
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_ERR_FILE_SMALL);
}

TEST(Generator, MissingFileFails)
{
    pg_config cfg = a_make_config("no_such_file.bin", "github");
    char pw[PG_MAX_LEN + 1];
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_ERR_FILE_OPEN);
}

TEST(Generator, BadLengthFails)
{
    pg_config cfg = a_make_config(A_BIG_FILE, "github");
    char pw[PG_MAX_LEN + 1];
    cfg.length = 3;
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_ERR_ARGS);
    cfg.length = 129;
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_ERR_ARGS);
}

TEST(Generator, TooSmallBufferFails)
{
    pg_config cfg = a_make_config(A_BIG_FILE, "github");
    char pw[8];   /* потрібно 8 + 1 для '\0' */
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_ERR_ARGS);
}

TEST(Generator, NullArgumentsFail)
{
    pg_config cfg = a_make_config(A_BIG_FILE, "github");
    char pw[PG_MAX_LEN + 1];
    EXPECT_EQ(generate_password(NULL, pw, sizeof pw), PG_ERR_ARGS);
    EXPECT_EQ(generate_password(&cfg, NULL, sizeof pw), PG_ERR_ARGS);
    cfg.key = NULL;
    EXPECT_EQ(generate_password(&cfg, pw, sizeof pw), PG_ERR_ARGS);
}
