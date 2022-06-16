//
// Created by zhenkai on 2022/4/12.
//

#include "process/dict.h"
#include "dictapi.h"

static Dict dict;

int dict_load(const char *full_path) {
    return dict.Load(full_path);
}

char *dict_find(const char *full_path, const char *key) {
    auto line = dict.Find(full_path, key);
    char *line_ptr = NULL;
    if(!line.empty()) {
        line_ptr = (char *)calloc(line.size() + 1, sizeof (char));
        memcpy(line_ptr, line.c_str(), line.size());
    }

    return line_ptr;
}

void dict_check_and_reload(const char *full_path) {
    if (full_path == NULL) {
        return;
    }
    std::string f_path(full_path);
    dict.Check_and_reload(f_path, true);
}

void dict_check_and_reload_all() {
    dict.Check_add_reload_all();
}

void dict_scan() {
    dict.Scan();
}
