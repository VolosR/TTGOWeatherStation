#ifndef FS_OPERATIONS_H
#define FS_OPERATIONS_H

#include <LittleFS.h>

bool writeFile(String path, String message, const bool overwrite=false);
String readFile(String path);
bool findFile(String path);
bool deleteFile(String path);
bool renameFile(String from, String to);

#endif // FS_OPERATIONS_H
