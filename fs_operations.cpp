#include "fs_operations.h"

bool writeFile(String path, String data, const bool overwrite) {
    char *mode = FILE_WRITE;
    bool create = true;

    if (!LittleFS.begin(true))
        return false;

    if (LittleFS.exists(path)) {
        create = false;
        if (!overwrite)
            mode = FILE_APPEND;
    }

    File file = LittleFS.open(path, mode, create);

    if (!data.endsWith("\n"))
        data += '\n';

    create = file.print(data) == data.length();
    file.flush();
    file.close();

    return create;
}

String readFile(String path) {
    File file;
    String data;

    if (!LittleFS.begin(true))
        return "";

     if (!(file = LittleFS.open(path)))
        return "";

    data = file.readString();
    file.close();
    return data;
}

bool findFile(String path) {
    if (!LittleFS.begin(true))
        return false;

    return LittleFS.exists(path);
}

bool deleteFile(String path){
    if (!LittleFS.begin(true))
        return false;

    return LittleFS.remove(path);
}

bool renameFile(String from, String to) {
    if (!LittleFS.begin(true))
        return false;

    return LittleFS.rename(from, to);
}
