#include "libwasm.h"
#include <read_utils.h>

#define TOP_MASK (1 << 7)

int32_t fetchI32(struct WasmModuleReader* reader) {
    // the largest size of I32 in leb128 representation is 5 bytes. 
    // This is because leb128 numbers must always have a 
    // number of bits which is divisble by 7
    // For the largest possible I32 we will have 35 bits
    // which will end up requiring 40 bits (5 bytes) to store

    uint8_t* data = reader->_data;
    int32_t  ret = 0;

    // initialise with 0 because even if some digits are unused
    // they will not affect the final OR
    uint8_t bits[] = {0, 0, 0, 0, 0};
    for (int i = 0; i <= 4; i++, reader->offset++) {
        if (reader->offset >= reader->size)  {
            reader->offset = UINT32_MAX;
            return 0;
        }

        uint8_t d = *(data + reader->offset);
        if (!(d & TOP_MASK)) {
            reader->offset += 1;
            bits[i] = d & 127;
            break;
        }

        reader->offset += 1;
        bits[i] = d & 127; // 127 is the mask needed to extract lower 7 bits
    }

    ret = (bits[4] << 28) | (bits[3] << 21) | (bits[2] << 14) | (bits[1] << 7) | bits[0];
    return ret;
}

uint32_t fetchU32(struct WasmModuleReader* reader) {
    // the largest size of U32 in leb128 representation is 5 bytes. 
    // This is because the leb128 numbers must always have a number of bits 
    // which is divisble by 7.
    // For the largest possible U32 we will have 35 bits
    // which will end up requiring 40 bits (5 bytes) to store

    uint8_t* data = reader->_data;
    uint32_t  ret = 0;

    // initialise with 0 because even if some digits are unused
    // they will not affect the final OR
    uint8_t bits[] = {0, 0, 0, 0, 0};
    for (int i = 0; i <= 4; i++) {
        if (reader->offset >= reader->size)  {
            reader->offset = UINT32_MAX;
            return 0;
        }

        uint8_t d = *(data + reader->offset);
        if (!(d & TOP_MASK)) {
            bits[i] = d & 127;
            reader->offset += 1;
            break;
        }

        reader->offset += 1;
        bits[i] = d & 127; // 127 is the mask needed to extract lower 7 bits
    }

    ret = (bits[4] << 28) | (bits[3] << 21) | (bits[2] << 14) | (bits[1] << 7) | bits[0];
    return ret;
}


int64_t fetchI64(struct WasmModuleReader* reader) {
    // the largest size of I64 in leb128 representation is 9 bytes. 
    // This is because leb128 numbers must always have a 
    // number of bits which is divisble by 7
    // For the largest possible I64 we will have 70 bits
    // which will end up requiring 72 bits (9 bytes) to store

    uint8_t* data = reader->_data;
    int64_t  ret = 0;

    // initialise with 0 because even if some digits are unused
    // they will not affect the final OR
    uint8_t bits[] = {0, 0, 0, 0, 0, 0, 0 , 0, 0};
    for (int i = 0; i < 9; i++, reader->offset++) {
        if (reader->offset >= reader->size)  {
            reader->offset = UINT32_MAX;
            return 0;
        }

        uint8_t d = *(data + reader->offset);
        if (!(d & TOP_MASK)) {
            bits[i] = d & 127;
            reader->offset += 1;
            break;
        }

        reader->offset += 1;
        bits[i] = d & 127; // 127 is the mask needed to extract lower 7 bits
    }

    ret = (((int64_t)bits[8] << 56) | ((int64_t)bits[7] << 49) | ((int64_t)bits[6] << 42) | ((int64_t)bits[5] << 35)) | 
            ((bits[4] << 28) | (bits[3] << 21) | (bits[2] << 14) | (bits[1] << 7) | bits[0]); 
    return ret;
}

uint64_t fetchU64(struct WasmModuleReader* reader) {
    // the largest size of U64 in leb128 representation is 9 bytes. 
    // This is because leb128 numbers must always have a 
    // number of bits which is divisble by 7
    // For the largest possible U64 we will have 70 bits
    // which will end up requiring 72 bits (9 bytes) to store

    uint8_t* data = reader->_data;
    uint64_t  ret = 0;

    // initialise with 0 because even if some digits are unused
    // they will not affect the final OR
    uint8_t bits[] = {0, 0, 0, 0, 0, 0, 0 , 0, 0};
    for (int i = 0; i < 9; i++, reader->offset++) {
        if (reader->offset >= reader->size)  {
            reader->offset = UINT32_MAX;
            return 0;
        }

        uint8_t d = *(data + reader->offset);
        if (!(d & TOP_MASK)) {
            reader->offset += 1;
            bits[i] = d & 127;
            break;
        }

        reader->offset += 1;
        bits[i] = d & 127; // 127 is the mask needed to extract lower 7 bits
    }

    ret = (((uint64_t)bits[8] << 56) | ((uint64_t)bits[7] << 49) | ((uint64_t)bits[6] << 42) | ((uint64_t)bits[5] << 35)) | 
            ((bits[4] << 28) | (bits[3] << 21) | (bits[2] << 14) | (bits[1] << 7) | bits[0]); 
    return ret;
}

uint8_t fetchRawU8(struct WasmModuleReader* reader) {
    if (reader->offset + 1 >= reader->size) {
        reader->offset = UINT32_MAX;
        return 0;
    }

    uint8_t* buf = (reader->_data + reader->offset);
    reader->offset += 1;
    return *buf;
}

uint32_t fetchRawU32(struct WasmModuleReader* reader) {
    if (reader->offset + 4 >= reader->size) {
        reader->offset = UINT32_MAX;
        return 0;
    }

    uint32_t* buf = (uint32_t*) ((uint8_t*)reader->_data + reader->offset);
    reader->offset += 4;
    return *buf;
}

void skip(struct WasmModuleReader* reader, uint32_t off) {
    if (reader->offset + off >= reader->size) {
        reader->offset = UINT32_MAX;
        return;
    } 

    reader->offset += off;
}
