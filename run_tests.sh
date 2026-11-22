#!/bin/bash

BIN=./passgen
PHOTO=samples/photo.jpg
FAILED=0

pass() {
    echo "PASS: $1"
}

fail() {
    echo "FAIL: $1"
    FAILED=1
}

expect_fail() {
    name=$1
    shift
    "$BIN" "$@" >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        pass "$name"
    else
        fail "$name"
    fi
}

p1=$($BIN -i $PHOTO -k github)
p2=$($BIN -i $PHOTO -k github)
if [ -n "$p1" ] && [ "$p1" = "$p2" ]; then
    pass "same input gives same password"
else
    fail "same input gives same password"
fi

p3=$($BIN -i $PHOTO -k hubgit)
if [ "$p1" != "$p3" ]; then
    pass "different key gives different password"
else
    fail "different key gives different password"
fi

p4=$($BIN -i $PHOTO -k github -v 2)
if [ "$p1" != "$p4" ]; then
    pass "different version gives different password"
else
    fail "different version gives different password"
fi

p5=$($BIN -i $PHOTO -k github -l 16)
if [ ${#p5} -eq 16 ]; then
    pass "length 16"
else
    fail "length 16"
fi

p6=$($BIN -i $PHOTO -k github -l 20 -D)
if [[ $p6 =~ ^[0-9]+$ ]]; then
    pass "-D gives digits only"
else
    fail "-D gives digits only"
fi

p7=$($BIN -i $PHOTO -k github -l 20 -S)
if [[ $p7 =~ ^[A-Za-z0-9]+$ ]]; then
    pass "-S gives no symbols"
else
    fail "-S gives no symbols"
fi

before=$(shasum -a 256 "$PHOTO" | cut -d' ' -f1)
$BIN -i $PHOTO -k github >/dev/null
after=$(shasum -a 256 "$PHOTO" | cut -d' ' -f1)
if [ "$before" = "$after" ]; then
    pass "file is not modified"
else
    fail "file is not modified"
fi

expect_fail "missing file" -i samples/none.bin -k github
expect_fail "empty file" -i samples/empty.bin -k github
expect_fail "tiny file" -i samples/tiny.bin -k github
expect_fail "no key" -i $PHOTO
expect_fail "length 0" -i $PHOTO -k github -l 0
expect_fail "length 1000" -i $PHOTO -k github -l 1000
expect_fail "length abc" -i $PHOTO -k github -l abc
expect_fail "-S with -D" -i $PHOTO -k github -S -D

if [ $FAILED -eq 0 ]; then
    echo "ALL TESTS PASSED"
else
    echo "SOME TESTS FAILED"
fi

exit $FAILED