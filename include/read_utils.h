#ifndef __READ_UTILS_H__
#define __READ_UTILS_H__

#include "libwasm.h"

int32_t fetchI32(struct WasmModuleReader* reader);
uint32_t fetchU32(struct WasmModuleReader* reader);
int64_t fetchI64(struct WasmModuleReader* reader);
uint64_t fetchU64(struct WasmModuleReader* reader);
uint8_t fetchRawU8(struct WasmModuleReader* reader);
uint32_t fetchRawU32(struct WasmModuleReader* reader);
void skip(struct WasmModuleReader* reader, uint32_t off);

#endif