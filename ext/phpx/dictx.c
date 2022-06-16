/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2017 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: zhenkai@staff.weibo.com                                      |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/php_scandir.h"
#include "ext/standard/info.h"
#include "php_dictx.h"

#ifdef __cplusplus
}
#endif

extern int dict_load(const char *full_path);
extern char *dict_find(const char *full_path, const char *key);
extern void dict_check_and_reload(const char *full_path);
extern void dict_scan();

ZEND_DECLARE_MODULE_GLOBALS(dictx)

static int le_dictx;

zend_class_entry *dictx_ce;

PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("dictx.full_path_list", "", PHP_INI_ALL, OnUpdateString, full_path_list, zend_dictx_globals, dictx_globals)
    STD_PHP_INI_ENTRY("dictx.directory", "", PHP_INI_ALL, OnUpdateString, directory, zend_dictx_globals, dictx_globals)
PHP_INI_END()

PHP_FUNCTION(dictx_load) {
    char *arg1 = (char *) 0 ;
    zval args[1];
    int result;
    if(ZEND_NUM_ARGS() != 1 || zend_get_parameters_array_ex(1, args) != SUCCESS) {
        WRONG_PARAM_COUNT;
    }

    if (Z_ISNULL(args[0])) {
        arg1 = (char *) 0;
    } else {
        convert_to_string(&args[0]);
        arg1 = (char *) Z_STRVAL(args[0]);
    }

    result = (int)dict_load((char const *)arg1);

    RETVAL_LONG(result);
}

PHP_FUNCTION(dictx_scan) {
    dict_scan();
}

PHP_METHOD(dictx, __construct) {
    zval *val = NULL;
    long is_load = 0;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|l", &val, &is_load) == FAILURE) {
        RETURN_FALSE;
    }
    convert_to_string(val);
    zend_update_property(Z_OBJCE_P(getThis()), getThis(), DICTX_PROP_NAME, DICTX_PROP_NAME_LEN, val TSRMLS_CC);
    if (is_load > 0) {
        dict_load((char const *)Z_STRVAL_P(val));
    }
}

PHP_METHOD(dictx, find) {
    zval *key;
    char *result = 0;
    long no_reload = 0;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|l", &key, &no_reload) == FAILURE) {
        RETURN_NULL();
    }
    zval *dict_name_zvalp = zend_read_property(dictx_ce, getThis(), DICTX_PROP_NAME, DICTX_PROP_NAME_LEN, 0, NULL);
    int ret = dict_load((const char *) Z_STRVAL_P(dict_name_zvalp));
    convert_to_string(key);
    result = (char *) dict_find(Z_STRVAL_P(dict_name_zvalp), Z_STRVAL_P(key));
    if (!result) {
        RETVAL_NULL();
    } else {
        array_init(return_value);
        char *column = strtok (result, "\t");
        while (column != NULL) {
            add_next_index_string(return_value, column);
            column = strtok(NULL, "\t");
        }
        free(result);
    }

    if (no_reload < 1 && DICTX_G(counter) > DICTX_G(scale)) {
        DICTX_G(counter) = 0;
        DICTX_G(scale) = php_rand() & 0x5F;
        dict_check_and_reload(Z_STRVAL_P(dict_name_zvalp));
    }
    DICTX_G(counter) ++;
}

zend_function_entry dictx_methods[] = {
        PHP_ME(dictx, __construct, NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR)
        PHP_ME(dictx, find, NULL, ZEND_ACC_PUBLIC)
        {NULL, NULL, NULL}
};

static void php_dictx_init_globals(zend_dictx_globals *dictx_globals)
{
    dictx_globals->full_path_list = NULL;
    dictx_globals->directory = NULL;
    dictx_globals->counter = 0;
    dictx_globals->scale = 0;
}

PHP_MINIT_FUNCTION(dictx)
{
    zend_class_entry ce;
    REGISTER_INI_ENTRIES();

    INIT_CLASS_ENTRY(ce, "Dictx", dictx_methods);
    dictx_ce = zend_register_internal_class_ex(&ce, NULL);
    zend_declare_property_null(dictx_ce, DICTX_PROP_NAME, DICTX_PROP_NAME_LEN, ZEND_ACC_PRIVATE TSRMLS_CC);

    // scan directory
    const char *dirname;
    size_t dirlen;
    struct zend_stat dir_sb = {0};
    if ((dirname = DICTX_G(directory)) && (dirlen = strlen(dirname))
    #ifndef ZTS
    && !VCWD_STAT(dirname, &dir_sb) && S_ISDIR(dir_sb.st_mode)
    #endif
    ) {
        int ndir, i;
        char *p, dict_file[MAXPATHLEN];

        struct dirent **namelist;
        if ((ndir = php_scandir(dirname, &namelist, 0, php_alphasort)) > 0) {
            struct zend_stat sb;
            for (i = 0; i < ndir; i++) {
                if (*(namelist[i]->d_name) == '.' || !(p = strrchr(namelist[i]->d_name, '.')) || strcmp(p, ".dict")) {
                    free(namelist[i]);
                    continue;
                }
                snprintf(dict_file, MAXPATHLEN, "%s%c%s", dirname, DEFAULT_SLASH, namelist[i]->d_name);
                if (VCWD_STAT(dict_file, &sb) == 0) {
                    if (S_ISREG(sb.st_mode)) {
                        dict_load(dict_file);
                    }
                }
                free(namelist[i]);
            }
            free(namelist);
        }
    }
//    else {
//        php_error(E_ERROR, "Couldn't opendir '%s'", dirname);
//    }

    // load full_path
    char *full_path;
    if (DICTX_G(full_path_list) != NULL && strlen(DICTX_G(full_path_list)) > 0 ) {
        char *dup = strdup(DICTX_G(full_path_list));
        full_path = strtok (dup,",");
        while (full_path != NULL) {
            dict_load(full_path);
            full_path = strtok(NULL, ",");
        }
        free(dup);
    }
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(dictx)
{
	UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}

PHP_RINIT_FUNCTION(dictx)
{
#if defined(COMPILE_DL_DICTX) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(dictx)
{
	return SUCCESS;
}

PHP_MINFO_FUNCTION(dictx)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "dictx support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}

const zend_function_entry dictx_functions[] = {
	PHP_FE(dictx_load,	NULL)
	PHP_FE(dictx_scan,	NULL)
	PHP_FE_END
};

zend_module_entry dictx_module_entry = {
	STANDARD_MODULE_HEADER,
	"dictx",
	dictx_functions,
	PHP_MINIT(dictx),
	PHP_MSHUTDOWN(dictx),
	PHP_RINIT(dictx),
	PHP_RSHUTDOWN(dictx),
	PHP_MINFO(dictx),
	PHP_DICTX_VERSION,
	STANDARD_MODULE_PROPERTIES
};

// TODO ATTENTION "extern C {}"
BEGIN_EXTERN_C()
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(dictx)
END_EXTERN_C()
