#ifndef PATH_H
#define PATH_H
_Bool       Path_IsAbsolute(const char *path);
const char *Path_MakeAbsolute(const char *path, char *buff, size_t buffsize);
#endif /* PATH_H */
