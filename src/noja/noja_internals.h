
typedef struct NOJA_Executable NOJA_Executable;
char            *NOJA_disassemble(NOJA_Executable *exe);
NOJA_Executable *NOJA_assembleString(const char *str);
NOJA_Executable *NOJA_assembleFile(const char *file);
NOJA_Executable *NOJA_compileString(const char *str);
NOJA_Executable *NOJA_compileFile(const char *file);
bool NOJA_compareExecutables(NOJA_Executable *exe1, NOJA_Executable *exe2);
