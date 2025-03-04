#include <libwasm.h>
#include <section.h>

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
    [WASM_INVALID_SECTION_ID] = "Module has section with invalid id\n",
    [WASM_MAX_ERROR] = "Internal error: WASM_MAX_ERROR cannot be reported, possible bug\n",
    [WASM_SECTION_TOO_LARGE] = "Size of builtin section is larger than maximum configured size\n",
    [WASM_CUSTOM_SECTION_TOO_LARGE] = "Size of custom section is larger than maximum configured size\n",
    [WASM_INVALID_FUNCTYPE] = "functype does not start with 0x60\n",
    [WASM_INVALID_TYPEVAL] = "valtype does not lie between 0x7F and 0x7C\n",
    [WASM_TRAILING_BYTES] = "Section has unclaimed trailing bytes\n",
    [WASM_UNSUPPORTED_MULTIVALUE_FUNC] = "Multivalue functions are unsupported\n",
    [WASM_TOO_MANY_PARAMS] = "Any function can only have upto 255 params\n",
    [WASM_TRUNCATED_SECTION] = "Section is smaller than it claims to be\n",
    [WASM_EMPTY_NAME] = "Import/export module name or component name is empty\n",
    [WASM_INVALID_IMPORT_TYPE] = "Import type is not between 0 and 3\n",
    [WASM_TOO_MANY_TABLES] = "Each module may have only 1 table\n",
    [WASM_INVALID_TABLE_ELEMENT_TYPE] = "Tables can only be of function type (0x70)\n",
    [WASM_INVALID_EXPR] = "Code expression must end with 0x0B(end)\n",
    [WASM_INIT_TOO_LONG] = "Init expression for global/data sections cannot be longer than 15 bytes\n",
    [WASM_TOO_MANY_MEMORIES] = "Each module can have only 1 memory\n",
    [WASM_INVALID_GLOBAL_MUTABILITY] = "Globals can have mutability flags equal to 0 or 1\n",
    [WASM_INVALID_MEMORY_INDEX] = "Only memory index 0 can be referenced from data sections\n",
    [WASM_INVALID_TABLE_INDEX] = "Only table index 0 can be referenced from element sections\n",
    [WASM_NO_TYPE] = "Function and/or code section is present but types secrion is absent\n",
    [WASM_FUNCTION_CODE_MISMATCH] = "Number of function indices does not match with number of code bodies\n",
    [WASM_INVALID_TYPE_INDEX] = "Index into type section is invalid\n",
    [WASM_INVALID_LIMIT_TYPE] = "Limit type is not 0(min) or 1 (min-max)\n"
};


const char* errString(int err) {
    if (err >= WASM_MAX_VALIDATION_ERROR || err < 0)
        return "Unknown error code\n";

    return error_to_string[err];
}
