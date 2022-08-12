#ifndef NOJA_H
#define NOJA_H
_Bool NOJA_runFile  (const char *file);
_Bool NOJA_runString(const char *str);
_Bool NOJA_runAssemblyFile(const char *file);
_Bool NOJA_runAssemblyString(const char *str);
_Bool NOJA_dumpFileBytecode(const char *file);
_Bool NOJA_dumpStringBytecode(const char *str);
#endif /* NOJA_H */