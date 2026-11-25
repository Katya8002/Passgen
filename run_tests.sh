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

m1=$($BIN -i $PHOTO -i samples/white.bmp -k github)
m2=$($BIN -i samples/white.bmp -i $PHOTO -k github)
if [ -n "$m1" ] && [ "$m1" = "$m2" ] && [ "$m1" != "$p1" ]; then
    pass "two files: order does not matter, differs from one file"
else
    fail "two files: order does not matter, differs from one file"
fi

expect_fail "five files" -i $PHOTO -i $PHOTO -i $PHOTO -i $PHOTO -i $PHOTO -k github

pm1=$(echo secret | $BIN -i $PHOTO -k github -p)
pm2=$(echo secret | $BIN -i $PHOTO -k github -p)
pm3=$(echo other | $BIN -i $PHOTO -k github -p)
if [ -n "$pm1" ] && [ "$pm1" = "$pm2" ] && [ "$pm1" != "$pm3" ] && [ "$pm1" != "$p1" ]; then
    pass "-p master password changes the result"
else
    fail "-p master password changes the result"
fi

info_white=$($BIN -i samples/white.bmp -k github -e 2>&1 >/dev/null)
if [[ $info_white == *low* ]]; then
    pass "-e warns about weak file"
else
    fail "-e warns about weak file"
fi

info_photo=$($BIN -i $PHOTO -k github -e 2>&1 >/dev/null)
if [[ $info_photo != *low* ]] && [[ $info_photo == *entropy* ]]; then
    pass "-e does not warn about good file"
else
    fail "-e does not warn about good file"
fi

out_e=$($BIN -i $PHOTO -k github -e 2>/dev/null)
if [ "$out_e" = "$p1" ]; then
    pass "-e keeps stdout as the password only"
else
    fail "-e keeps stdout as the password only"
fi

if [ $FAILED -eq 0 ]; then
    echo "ALL TESTS PASSED"
else
    echo "SOME TESTS FAILED"
fi

exit $FAILED
