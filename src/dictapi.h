//
// Created by zhenkai on 2022/4/12.
//

#ifndef DICTX_DICTAPI_H
#define DICTX_DICTAPI_H


#ifdef __cplusplus
extern "C" {
#endif

int dict_load(const char *full_path);
char *dict_find(const char *full_path, const char *key);
void dict_check_and_reload(const char *full_path);
void dict_check_and_reload_all();
void dict_scan();

#ifdef __cplusplus
}
#endif


#endif //DICTX_DICTAPI_H
