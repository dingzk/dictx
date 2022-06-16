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
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifndef PHP_DICTX_H
#define PHP_DICTX_H

extern zend_module_entry dictx_module_entry;
#define phpext_dictx_ptr &dictx_module_entry

#define PHP_DICTX_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_DICTX_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_DICTX_API __attribute__ ((visibility("default")))
#else
#	define PHP_DICTX_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define DICTX_PROP_NAME "full_path"
#define DICTX_PROP_NAME_LEN (sizeof(DICTX_PROP_NAME) - 1)


ZEND_BEGIN_MODULE_GLOBALS(dictx)
    char *full_path_list;
    char *directory;
    long  counter;
    long  scale;
ZEND_END_MODULE_GLOBALS(dictx)

#define DICTX_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(dictx, v)

#if defined(ZTS) && defined(COMPILE_DL_DICTX)
ZEND_TSRMLS_CACHE_EXTERN()
#endif

#endif	/* PHP_DICTX_H */
