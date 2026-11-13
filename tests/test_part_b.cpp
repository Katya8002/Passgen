#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>
#include <vector>
#include <unistd.h>

extern "C" {
#include "passgen.h"
#include "cli.h"
#include "fileio.h"
}

static pg_status parse(std::initializer_list<const char *> args, pg_config *cfg)
{
    std::vector<char *> argv;
    for (const char *a : args) {
        argv.push_back(const_cast<char *>(a));
    }
    argv.push_back(nullptr);
    return cli_parse((int)args.size(), argv.data(), cfg);
}

TEST(CliParse, DefaultValues)
{
    pg_config cfg;
    ASSERT_EQ(PG_OK, parse({"passgen", "-i", "cat.jpg", "-k", "github"}, &cfg));
    EXPECT_EQ(1, cfg.file_count);
    EXPECT_STREQ("cat.jpg", cfg.files[0]);
    EXPECT_STREQ("github", cfg.key);
    EXPECT_EQ(PG_DEFAULT_LEN, cfg.length);
    EXPECT_EQ(1, cfg.version);
    EXPECT_EQ((int)CS_ALL, cfg.charset);
    EXPECT_EQ(0, cfg.ask_master);
    EXPECT_EQ(0, cfg.show_info);
}

TEST(CliParse, AllFlags)
{
    pg_config cfg;
    ASSERT_EQ(PG_OK, parse({"passgen", "-i", "a.jpg", "-i", "b.mp3", "-k", "bank",
                            "-l", "16", "-v", "2", "-p", "-e"}, &cfg));
    EXPECT_EQ(2, cfg.file_count);
    EXPECT_STREQ("a.jpg", cfg.files[0]);
    EXPECT_STREQ("b.mp3", cfg.files[1]);
    EXPECT_STREQ("bank", cfg.key);
    EXPECT_EQ(16, cfg.length);
    EXPECT_EQ(2, cfg.version);
    EXPECT_EQ(1, cfg.ask_master);
    EXPECT_EQ(1, cfg.show_info);
}

TEST(CliParse, MissingKey)
{
    pg_config cfg;
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "cat.jpg"}, &cfg));
}

TEST(CliParse, MissingFile)
{
    pg_config cfg;
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-k", "github"}, &cfg));
}

TEST(CliParse, NoArguments)
{
    pg_config cfg;
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen"}, &cfg));
}

TEST(CliParse, LengthBoundaries)
{
    pg_config cfg;
    EXPECT_EQ(PG_OK, parse({"passgen", "-i", "a", "-k", "x", "-l", "4"}, &cfg));
    EXPECT_EQ(4, cfg.length);
    EXPECT_EQ(PG_OK, parse({"passgen", "-i", "a", "-k", "x", "-l", "128"}, &cfg));
    EXPECT_EQ(128, cfg.length);
}

TEST(CliParse, LengthOutOfRange)
{
    pg_config cfg;
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-l", "0"}, &cfg));
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-l", "3"}, &cfg));
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-l", "129"}, &cfg));
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-l", "1000"}, &cfg));
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-l", "-5"}, &cfg));
}

TEST(CliParse, LengthNotANumber)
{
    pg_config cfg;
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-l", "abc"}, &cfg));
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-l", "12abc"}, &cfg));
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-l", ""}, &cfg));
}

TEST(CliParse, VersionInvalid)
{
    pg_config cfg;
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-v", "0"}, &cfg));
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-v", "abc"}, &cfg));
}

TEST(CliParse, NoSymbols)
{
    pg_config cfg;
    ASSERT_EQ(PG_OK, parse({"passgen", "-i", "a", "-k", "x", "-S"}, &cfg));
    EXPECT_EQ((int)(CS_LOWER | CS_UPPER | CS_DIGITS), cfg.charset);
}

TEST(CliParse, DigitsOnly)
{
    pg_config cfg;
    ASSERT_EQ(PG_OK, parse({"passgen", "-i", "a", "-k", "x", "-D"}, &cfg));
    EXPECT_EQ((int)CS_DIGITS, cfg.charset);
}

TEST(CliParse, NoSymbolsAndDigitsTogether)
{
    pg_config cfg;
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-S", "-D"}, &cfg));
}

TEST(CliParse, UnknownFlag)
{
    pg_config cfg;
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "-z"}, &cfg));
}

TEST(CliParse, ExtraArgument)
{
    pg_config cfg;
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "a", "-k", "x", "extra"}, &cfg));
}

TEST(CliParse, FourFilesOkFiveFail)
{
    pg_config cfg;
    EXPECT_EQ(PG_OK, parse({"passgen", "-i", "1", "-i", "2", "-i", "3", "-i", "4",
                            "-k", "x"}, &cfg));
    EXPECT_EQ(4, cfg.file_count);
    EXPECT_EQ(PG_ERR_ARGS, parse({"passgen", "-i", "1", "-i", "2", "-i", "3", "-i", "4",
                                  "-i", "5", "-k", "x"}, &cfg));
}

TEST(CliParse, CanBeCalledRepeatedly)
{
    pg_config cfg;
    ASSERT_EQ(PG_OK, parse({"passgen", "-i", "a", "-k", "first", "-l", "20"}, &cfg));
    EXPECT_EQ(20, cfg.length);
    ASSERT_EQ(PG_OK, parse({"passgen", "-i", "b", "-k", "second"}, &cfg));
    EXPECT_STREQ("second", cfg.key);
    EXPECT_EQ(PG_DEFAULT_LEN, cfg.length);
    EXPECT_EQ(1, cfg.file_count);
}

TEST(CliParse, HelpPrintsUsageAndExits)
{
    pg_config cfg;
    EXPECT_EXIT(parse({"passgen", "-h"}, &cfg), ::testing::ExitedWithCode(0), "Usage");
}

TEST(StatusMessage, EveryCodeHasText)
{
    EXPECT_STREQ("ok", pg_status_message(PG_OK));
    EXPECT_STREQ("invalid arguments", pg_status_message(PG_ERR_ARGS));
    EXPECT_STREQ("cannot open file", pg_status_message(PG_ERR_FILE_OPEN));
    EXPECT_STREQ("file is too small", pg_status_message(PG_ERR_FILE_SMALL));
    EXPECT_STREQ("read error", pg_status_message(PG_ERR_READ));
    EXPECT_STREQ("out of memory", pg_status_message(PG_ERR_MEMORY));
}

class TempFile {
public:
    explicit TempFile(size_t size)
    {
        char tmpl[] = "/tmp/passgen_test_XXXXXX";
        int fd = mkstemp(tmpl);
        EXPECT_NE(-1, fd);
        path_ = tmpl;
        for (size_t i = 0; i < size; i++) {
            data_.push_back((char)(i % 256));
        }
        if (fd != -1) {
            if (size > 0) {
                EXPECT_EQ((ssize_t)size, write(fd, data_.data(), size));
            }
            close(fd);
        }
    }
    ~TempFile() { std::remove(path_.c_str()); }
    const char *path() const { return path_.c_str(); }
    const std::string &data() const { return data_; }

private:
    std::string path_;
    std::string data_;
};

TEST(FileOpen, NonexistentFile)
{
    pg_file f;
    EXPECT_EQ(PG_ERR_FILE_OPEN, file_open("/tmp/passgen_no_such_file.bin", &f));
    EXPECT_EQ(nullptr, f.fp);
}

TEST(FileOpen, EmptyFile)
{
    TempFile tf(0);
    pg_file f;
    EXPECT_EQ(PG_ERR_FILE_SMALL, file_open(tf.path(), &f));
    EXPECT_EQ(nullptr, f.fp);
}

TEST(FileOpen, TooSmallFile)
{
    TempFile tf(PG_MIN_FILE_SIZE - 1);
    pg_file f;
    EXPECT_EQ(PG_ERR_FILE_SMALL, file_open(tf.path(), &f));
}

TEST(FileOpen, MinimumSizeIsAccepted)
{
    TempFile tf(PG_MIN_FILE_SIZE);
    pg_file f;
    ASSERT_EQ(PG_OK, file_open(tf.path(), &f));
    EXPECT_EQ(PG_MIN_FILE_SIZE, f.size);
    file_close(&f);
}

TEST(FileOpen, DirectoryIsRejected)
{
    pg_file f;
    EXPECT_NE(PG_OK, file_open("/tmp", &f));
    EXPECT_EQ(nullptr, f.fp);
}

TEST(FileByteAt, ReadsCorrectBytes)
{
    TempFile tf(300);
    pg_file f;
    ASSERT_EQ(PG_OK, file_open(tf.path(), &f));
    EXPECT_EQ(300, f.size);

    unsigned char b = 99;
    EXPECT_EQ(PG_OK, file_byte_at(&f, 0, &b));
    EXPECT_EQ(0, b);
    EXPECT_EQ(PG_OK, file_byte_at(&f, 255, &b));
    EXPECT_EQ(255, b);
    EXPECT_EQ(PG_OK, file_byte_at(&f, 256, &b));
    EXPECT_EQ(0, b);
    EXPECT_EQ(PG_OK, file_byte_at(&f, 299, &b));
    EXPECT_EQ(43, b);
    file_close(&f);
}

TEST(FileByteAt, ReadsInAnyOrder)
{
    TempFile tf(300);
    pg_file f;
    ASSERT_EQ(PG_OK, file_open(tf.path(), &f));
    unsigned char b = 0;
    EXPECT_EQ(PG_OK, file_byte_at(&f, 200, &b));
    EXPECT_EQ(200, b);
    EXPECT_EQ(PG_OK, file_byte_at(&f, 10, &b));
    EXPECT_EQ(10, b);
    EXPECT_EQ(PG_OK, file_byte_at(&f, 200, &b));
    EXPECT_EQ(200, b);
    file_close(&f);
}

TEST(FileByteAt, OutOfRangeOffset)
{
    TempFile tf(100);
    pg_file f;
    ASSERT_EQ(PG_OK, file_open(tf.path(), &f));
    unsigned char b = 0;
    EXPECT_EQ(PG_OK, file_byte_at(&f, 99, &b));
    EXPECT_EQ(PG_ERR_READ, file_byte_at(&f, 100, &b));
    EXPECT_EQ(PG_ERR_READ, file_byte_at(&f, 5000, &b));
    EXPECT_EQ(PG_ERR_READ, file_byte_at(&f, -1, &b));
    file_close(&f);
}

TEST(FileByteAt, NullArguments)
{
    TempFile tf(100);
    pg_file f;
    ASSERT_EQ(PG_OK, file_open(tf.path(), &f));
    unsigned char b = 0;
    EXPECT_EQ(PG_ERR_READ, file_byte_at(nullptr, 0, &b));
    EXPECT_EQ(PG_ERR_READ, file_byte_at(&f, 0, nullptr));
    file_close(&f);
}

TEST(FileClose, SafeToCallTwice)
{
    TempFile tf(100);
    pg_file f;
    ASSERT_EQ(PG_OK, file_open(tf.path(), &f));
    file_close(&f);
    EXPECT_EQ(nullptr, f.fp);
    file_close(&f);
    file_close(nullptr);
}

TEST(FileOpen, FileIsNotModified)
{
    TempFile tf(500);
    pg_file f;
    ASSERT_EQ(PG_OK, file_open(tf.path(), &f));
    unsigned char b = 0;
    for (long i = 0; i < f.size; i++) {
        ASSERT_EQ(PG_OK, file_byte_at(&f, i, &b));
    }
    file_close(&f);

    std::ifstream in(tf.path(), std::ios::binary);
    std::string after((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(tf.data(), after);
}
