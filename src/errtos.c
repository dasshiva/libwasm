#include <libwasm.h>

static const char* error_to_string[] = {
    [WASM_SUCCESS] = "Success\n",
    [WASM_FILE_ACCESS_ERROR] = "Unable to create/access module file\n",
    [WASM_INVALID_MODULE_NAME] = "module name is null\n",
    [WASM_FILE_INVALID_MAGIC] = "module does not have magic \'0asm\'\n",
    [WASM_FILE_INVALID_VERSION] = "module does not have valid version 0x1'",
    [WASM_TRUNCATED_FILE] = "module is truncated, section is too large to fit in module\n",
    [WASM_INVALID_ARG] = "primary argument is null or invalid\n",
    [WASM_OUT_OF_MEMORY] = "system has run out of memory\n",
    [WASM_ARGUMENT_NULL] = "any of the secondary arguments are null\n",
    [WASM_MODULE_TOO_LARGE] = "module size is greater than largest possible size\n",
    [WASM_FILE_READ_ERROR] = "could not read module from disk\n",
};


const char* errString(int err) {
    if (err >= WASM_MAX_ERROR || err < 0)
        return "Unknown error code\n";

    return error_to_string[err];
}