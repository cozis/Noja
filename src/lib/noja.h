#ifndef NOJA_H
#define NOJA_H
_Bool NOJA_runFile  (const char *file, size_t heap);
_Bool NOJA_runString(const char *str, size_t heap);
_Bool NOJA_runAssemblyFile(const char *file, size_t heap);
_Bool NOJA_runAssemblyString(const char *str, size_t heap);
_Bool NOJA_dumpFileBytecode(const char *file);
_Bool NOJA_dumpStringBytecode(const char *str);
_Bool NOJA_profileFile(const char *file, size_t heap);
_Bool NOJA_profileString(const char *str, size_t heap);
_Bool NOJA_profileAssemblyFile(const char *file, size_t heap);
_Bool NOJA_profileAssemblyString(const char *str, size_t heap);
#endif /* NOJA_H */
