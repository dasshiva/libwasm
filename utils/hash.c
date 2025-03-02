#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define FNV1A_OFFSET_BASIS 0xCBF29CE484222325UL
#define FNV1A_PRIME        0x00000100000001B3UL

// FNV1-A hash implementation
uint64_t hash(const char* s) {
    uint64_t ret = FNV1A_OFFSET_BASIS;
    while (*s) {
        ret ^= *s;
        ret *= FNV1A_PRIME;
        s++;
    }

    return ret;
}

// Very simple program to generate hashes from builtin section names
int main(int argc, const char* argv[]) {
    if (argc != 3) {
        printf("Usage: <hash-file>.inc <output>.h\n");
        return 1;
    }

    FILE* infile = fopen(argv[1], "r");
    if (!infile) {
        printf("%s not found\n", argv[1]);
        return 1;
    }

    // We don't care if output file exists or not as we overwrite it if it exists anyway
    FILE* ofile = fopen(argv[2], "w");
    if (!ofile) {
        // But we do care if someone messed up the folder perms so bad
        // that they made the generated directory inaccessible
        printf("Could not open for writing %s\n", argv[2]);
        return 1;
    }

    fprintf(ofile, "#ifndef __HASHES_H__\n");
    fprintf(ofile, "#define __HASHES_H__\n");
    char name[70] = {0};

    while (1) {
        int ret = fscanf(infile, "%s\n", name);
        if (ret == EOF)
            break; 

        int len = strlen(name);
        if (!len) 
            continue;
        else {
            if (len >= 2) {
                if (name[0] == '/' && name[1] == '/') break; // this is a comment
                else {
                    uint64_t h = hash(name);
                    if (!h) {
                        printf("Hashing error: Hash is 0");
                        break;
                    }

                    fprintf(ofile, "#define WASM_HASH_%s (0x%lxUL)\n", name, h);
                }
                
            }

            else {
                printf("Name %s is too small: does not generate enough entropy", name);
                break;
            }
        
        }
    }

    fprintf(ofile, "#endif\n");
    return 0;
}