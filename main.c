#include <libwasm.h>
#include <stdio.h>

int main(int argc, const char* argv[]) {
    Config config = {0};
    Reader reader = {0};

    argv++;
    if (argc < 2) {
        printf("Usage: sample file1 file2 ... fileN\n");
        return 1;
    }

    while (*argv) {
        config.name = *argv;
        int s = createReader(&reader, &config);
        if (s) {
            printf("Error: %s", errString(s));
            return 1;
        }

        s = parseModule(&reader);
        if (s) {
            printf("Error: %s", errString(s));
            return 1;
        }
        
        Module* mod = getModuleFromReader(&reader);
        validateModule(mod);
        destroyReader(&reader);

        argv++;
    }
    return 0;
}
