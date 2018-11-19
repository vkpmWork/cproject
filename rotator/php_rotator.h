#ifndef PHP_ROTATOR_H
#define PHP_ROTATOR_H

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "Zend/zend_hash.h"

extern zend_module_entry rotator_module_entry;
#define phpext_rotator_ptr &rotator_module_entry

#ifdef PHP_WIN32
#	define PHP_ROTATOR_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_ROTATOR_API __attribute__ ((visibility("default")))
#else
#	define PHP_ROTATOR_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif


PHP_MINIT_FUNCTION(rotator);
PHP_MSHUTDOWN_FUNCTION(rotator);
PHP_RINIT_FUNCTION(rotator);
PHP_RSHUTDOWN_FUNCTION(rotator);
PHP_MINFO_FUNCTION(rotator);

PHP_METHOD(Rotator,__construct);
PHP_METHOD(Rotator,__destruct);

PHP_METHOD(Rotator, create_playlist);
PHP_METHOD(Rotator, size);
PHP_METHOD(Rotator, is_empty);				/* целевой плейлист пустой */
PHP_METHOD(Rotator, is_ordered);			/* установлен порядок обхода папок */
PHP_METHOD(Rotator, is_error);				/* true = ошибки в структуре входного плейлиста или в формировании целевого плейлиста*/
PHP_METHOD(Rotator, error);					/* текст ошибки */
PHP_METHOD(Rotator, max_tracks);			/* максимальное количество треков в целевом плейлисте */
PHP_METHOD(Rotator, default_track_duration);/* длительность трека, если не задано во входном плейлисте */
PHP_METHOD(Rotator, prepare);
PHP_METHOD(Rotator, playlist_duration); 	/* фактическая длительность плейлиста */
PHP_METHOD(Rotator, general_duration);      /* длительность плейлиста */
PHP_METHOD(Rotator, singer_interval);		/* заданные правила исполнителя*/
PHP_METHOD(Rotator, track_interval);		/* заданные правила трека */

PHP_METHOD(Rotator, set_aaa);		/* заданные правила трека */
/*
    Declare any global variables you may need between the BEGIN
    and END macros here:

*/

/*
ZEND_BEGIN_MODULE_GLOBALS(rotator)
ZEND_END_MODULE_GLOBALS(rotator)
*/

#ifdef ZTS
#define ROTATOR_G(v) TSRMG(rotator_globals_id, zend_rotator_globals *, v)
#else
#define ROTATOR_G(v) (rotator_globals.v)
#endif

#endif	/* PHP_ROTATOR_H */


