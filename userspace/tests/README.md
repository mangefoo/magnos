# MagnOS Test Programs

This directory contains test programs that are not included in the default build.

## Test Programs

### casetest.c
Tests case-insensitive file operations by opening the same file with different case variations:
- Lowercase: `hello.txt`
- Uppercase: `HELLO.TXT`
- Mixed case: `HeLLo.TxT`

### filetest.c
Tests file I/O operations including reading files and directory listings.

## Building Test Programs

To build a test program manually, use:

```bash
gcc -m32 -ffreestanding -nostdlib -fno-pie -fno-stack-protector \
    -static -Wl,--entry=_start -Wl,-Ttext=0x200000 \
    -o casetest ../crt0.c casetest.c
```

Then copy to the disk image:

```bash
mcopy -i ../../hdd.img casetest ::CASETEST
```

## Why Not Included

These programs are development/testing utilities that don't need to be on every build of the OS. They can be built manually when needed for testing specific functionality.
