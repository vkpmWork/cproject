#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_rotator.h"
#include "rotator_ctrl.h"
//#include <iostream>

namespace php_rotator
{
#define EXTENCION_NAME     "rotator"
#define VERSION		   "7.2.1 для РНР7.2.3 от 27.09.2018"

/*
	Версия 1.5.0 - добавлена обработка папки service
	Версия 2.0.0 - выбор треков для ротации случайным образом. Признак random_selection
	Версия 2.0.1 - единообразие в оформлении свойств и методов расширения
	Версия 2.1.0 - последовательная обработка папки service даже при случайнной генерации номера трека
	Версия 2.1.1 - оптимизирована функция записи трека в целевой плейлист
	Версия 2.1.2 - организован счетчик повторов для NEED_TO_SKEEP_RECORD
	Версия 3.0.0 - учет веса трека вес трека. Исключена случайная генерация треков
	Версия 3.0.1 - установлена длина плейлиста по умолчанию для работы по расписанию = 24-м часа
	(Без весов 1 папка - 40 треков, 2 папка - 1000, 3 папка - 1000, история 40 тркеков. Время обработки = 0.109 mc. Тестирование на 4*.php)
	Версия 3.2   - внесены изменения в деструктор в модуле rotator_ctrl.cpр. Изменения коснулись удаления истории эфира
	Версия 4.0   - разработка под PHP7.X 
	Версия 4.1   - отсчет начала формирования плейлиста от текущего времени

	Версия 7.0 - доработка расширения под PHP7
	Версия 7.1 - отладка   расширения для работы по расписанию
    		   - исключен анализ правил папок. search_rules - правилам папок присваиваются общие правила
    	Версия 7.2 - изменение номера версии и компиляция под PHP7.2 
		     добавила флаг set_start_folder. Если переход к новому часу по расписанию, то начинать формирование плейлиста с первой папки
        Версия 7.2.1 - измененр способ проверки наличия сервисной папки. Оставила только IS_TRUE
 */

#define RESULT_PLAYLIST     "result"
#define ORDER_PLAYLIST      "order"

#define PROPERTY_ERROR      "error"
#define PROPERTY_ERROR_LEN  strlen(PROPERTY_ERROR)

//#define PROPERTY_RANDOM     "random_selection"
//#define PROPERTY_RANDOM_LEN strlen(PROPERTY_RANDOM)

#define PROPERTY_MAX        "max_tracks"
#define PROPERTY_MAX_LEN    strlen(PROPERTY_MAX)

#define PROPERTY_DURATION        "default_track_duration"
#define PROPERTY_DURATION_LEN    strlen(PROPERTY_DURATION)

#define PROPERTY_GENERAL         "general_duration"
#define PROPERTY_GENERAL_LEN     strlen(PROPERTY_GENERAL)

#define PROPERTY_AA      "aa"
#define PROPERTY_AA_LEN  strlen(PROPERTY_AA)

zend_class_entry   	       *g_RotatorEntry;
zend_object_handlers 		g_RotatorHandlers;

typedef struct _rotator_object
{
        TRotator_Ctrl	*pRotator_Ctrl;
	zend_object 	zo; /* must be the last element */

} rotator_object;


static inline rotator_object * rotator_fetch_object(zend_object *obj)
{
    return (rotator_object*)((char*)obj - XtOffsetOf(rotator_object, zo));
}

#define Z_ROTATOR_OBJECT_P(zv) rotator_fetch_object(Z_OBJ_P(zv TSRMLS_CC));
//static int le_rotator;
} //namespace php_rptator

using namespace php_rotator;

char* update_error_property(int value)
{
	char *str = NULL;
	switch (value)
	{
		case EMPTY_ERROR    	: str = strdup("Ошибок нет!"); break;
		case TRACK_ERROR 		: str = strdup("Некорректные правила трека"); break;
		case EXECUTOR_ERROR 	: str = strdup("Некорректные правила исполнителя"); break;
		case TRACK_ERROR+EXECUTOR_ERROR : str = strdup("Некорректные правила"); break;
		case SHEDULE_ERROR    	: str = strdup("Ошибка в расписании. Возможно, пустые папки для ротации"); break;
		case INPUT_ARRAY_ERROR	: str = strdup("Какая неприятность, входной массив пустой!"); break;
		case FOLDERS_ERROR 		: str = strdup("Отсутствуют или пустые папки 'folders'"); break;

		default 				: str = strdup("Неизвестная ошибка");
	}
	return str;
}

ZEND_API void rotator_object_dtor(zend_object *object TSRMLS_DC)
{
    zend_objects_destroy_object(object);
}

ZEND_API void rotator_free_storage(zend_object *object TSRMLS_DC)
{

   rotator_object *o =  rotator_fetch_object(object);

    if (o) 
    {
	zend_object_std_dtor(&o->zo);
	delete o->pRotator_Ctrl;
    }

}

zend_function_entry class_methods[] =
{
	PHP_ME(Rotator,__construct, 	  		NULL, ZEND_ACC_PUBLIC | ZEND_ACC_CTOR)
	PHP_ME(Rotator,__destruct, 		  	NULL, ZEND_ACC_PUBLIC)

	PHP_ME(Rotator, size, 			  	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, is_empty,		  	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, is_ordered, 		  	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, is_error,		  	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, error,			  	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, max_tracks,		  	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, prepare,		  	NULL, ZEND_ACC_PUBLIC)

	PHP_ME(Rotator, default_track_duration, 	NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, create_playlist,  		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, playlist_duration,  		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, general_duration,		NULL, ZEND_ACC_PUBLIC)

	PHP_ME(Rotator, singer_interval,		NULL, ZEND_ACC_PUBLIC)
	PHP_ME(Rotator, track_interval,	    		NULL, ZEND_ACC_PUBLIC)

	PHP_ME(Rotator, set_aaa,		    	NULL, ZEND_ACC_PUBLIC)

	{	NULL, NULL, NULL}
};

PHP_METHOD(Rotator, __construct)
{

    ofstream ofs;

    zval *array = NULL,  *param	= NULL;
    char *s = NULL;

    zval *object = getThis();

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a|z", &array, &param) == FAILURE) 
    {
	RETURN_NULL();
    }

   TRotator_Ctrl *pR  = param == NULL ? new TRotator_Ctrl(array) : Z_TYPE_P(param) == IS_STRING ?  new TRotator_Ctrl(array, Z_STRVAL_P(param)) : Z_TYPE_P(param) == IS_LONG ? new TRotator_Ctrl(array, Z_LVAL_P(param)) : new TRotator_Ctrl(array);

   rotator_object *obj = Z_ROTATOR_OBJECT_P(object);
   if (obj) obj->pRotator_Ctrl  = pR;
}

PHP_METHOD(Rotator, __destruct)
{
    rotator_object *obj = Z_ROTATOR_OBJECT_P(getThis());

    if (obj)
    { 	TRotator_Ctrl *rc = obj->pRotator_Ctrl;
	if (rc) rc->OnDestroy();

    }
}


zend_module_entry rotator_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
		STANDARD_MODULE_HEADER,
#endif
		EXTENCION_NAME,
		NULL,
		PHP_MINIT(rotator),
		PHP_MSHUTDOWN(rotator),
		NULL, //PHP_RINIT(rotator),		/* Replace with NULL if there's nothing to do at request start */
		NULL, //PHP_RSHUTDOWN(rotator),	/* Replace with NULL if there's nothing to do at request end */
		PHP_MINFO(rotator),
#if ZEND_MODULE_API_NO >= 20010901
		VERSION, /* Replace with version number for your extension */
#endif
		STANDARD_MODULE_PROPERTIES };
/* }}} */

#ifdef COMPILE_DL_ROTATOR
ZEND_GET_MODULE(rotator)
#endif

static zend_object *rotator_create_handler(zend_class_entry *type TSRMLS_DC)
{
	    zval *tmp;
	    zval rv;

            rotator_object *obj =  (rotator_object *)ecalloc(1, sizeof(rotator_object) + zend_object_properties_size(type));

	    zend_object_std_init(&obj->zo, type TSRMLS_CC);
	    object_properties_init(&obj->zo, type);

	    obj->zo.handlers = &g_RotatorHandlers;

	    return &obj->zo;
}

PHP_MINIT_FUNCTION(rotator)
{
	zend_class_entry stubClassEntry;

	INIT_CLASS_ENTRY(stubClassEntry, "Rotator", class_methods);

	stubClassEntry.create_object = rotator_create_handler;
	g_RotatorEntry = zend_register_internal_class(&stubClassEntry TSRMLS_CC);

        memcpy(&g_RotatorHandlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));

        g_RotatorHandlers.clone_obj = NULL;
	g_RotatorHandlers.free_obj  = &rotator_free_storage;
	g_RotatorHandlers.dtor_obj  = &rotator_object_dtor;

	g_RotatorHandlers.offset    = XtOffsetOf(rotator_object, zo);

	if (g_RotatorEntry) return SUCCESS;

	return FAILURE;

}
/* }}} */
/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(rotator)
{
	/* uncomment this line if you have INI entries */
//    UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}

PHP_MINFO_FUNCTION(rotator)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "rotator support", "enabled");
	php_info_print_table_header(2, "Version", VERSION);
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini*/
//	DISPLAY_INI_ENTRIES();
	/**/
}

PHP_METHOD(Rotator, size)
{
    long len = -1;

    TRotator_Ctrl *rc   = NULL;
    rotator_object *obj = Z_ROTATOR_OBJECT_P(getThis());

    if (obj)
    { 	rc = obj->pRotator_Ctrl;
    	if (rc) len = rc->size();
    }

    RETURN_LONG(len);
}

PHP_METHOD(Rotator, is_empty)
{
    bool value = true;
   rotator_object *obj = Z_ROTATOR_OBJECT_P(getThis());

    if (obj)
    { 	TRotator_Ctrl *rc = obj->pRotator_Ctrl;
    	if (rc) value = rc->isempty();
    }

    RETURN_BOOL(value);
}

PHP_METHOD(Rotator, is_ordered)
{
   bool value = false;
   rotator_object *obj = Z_ROTATOR_OBJECT_P(getThis());

    if (obj)
    { 	TRotator_Ctrl *rc	= obj->pRotator_Ctrl;
    	if (rc) value 		= rc->ordered();
    }

	RETURN_BOOL(value);
}

PHP_METHOD(Rotator, max_tracks)
{
	zval *This = getThis();
	zval rv;

	zval* value = zend_read_property(Z_OBJCE_P(This), This, PROPERTY_MAX, PROPERTY_MAX_LEN, 1 , &rv TSRMLS_CC);

	if (value && Z_TYPE_P(value) == IS_LONG) RETURN_LONG(Z_LVAL_P(value));

	RETURN_NULL();
}

PHP_METHOD(Rotator, is_error)
{
    bool value = true;
   rotator_object *obj = Z_ROTATOR_OBJECT_P(getThis());

    if (obj)
    { 	TRotator_Ctrl *rc = obj->pRotator_Ctrl;
    	if (rc) value = rc->iserror();
    }

    RETURN_BOOL(value);
}

PHP_METHOD(Rotator, error)
{
	zval *This = getThis();

	zval rv;
	zval *value = zend_read_property(Z_OBJCE_P(This), This, PROPERTY_ERROR, PROPERTY_ERROR_LEN, 1, &rv TSRMLS_CC);
	if (value && Z_TYPE_P(value) == IS_STRING) RETURN_STR(Z_STR_P(value));

	RETURN_NULL();
}

PHP_METHOD(Rotator, prepare)
{
   bool value = false;
   zval *This = getThis();

   TRotator_Ctrl *rc   = NULL;
   rotator_object *obj = Z_ROTATOR_OBJECT_P(This);

    rc = obj->pRotator_Ctrl;
    if (rc)
    {
    	value =  rc->prepare();

		char *s =  update_error_property(rc->get_error());
		if (s)
		{	zend_update_property_string(Z_OBJCE_P(This), This, PROPERTY_ERROR, PROPERTY_ERROR_LEN, s TSRMLS_CC);
			free(s);
		}
    }
	RETURN_BOOL(value);
}

PHP_METHOD(Rotator, default_track_duration)

{
	zval* This = getThis();
	zval rv;
	zval* value = zend_read_property(Z_OBJCE_P(This), This, PROPERTY_DURATION, PROPERTY_DURATION_LEN, 1, &rv TSRMLS_CC);

	if (value && Z_TYPE_P(value) == IS_STRING) RETURN_STR(Z_STR_P(value));
	RETURN_NULL();
}

PHP_METHOD(Rotator, create_playlist)
{

    zval *play_list =  NULL;
    zval *This		= getThis();

    TRotator_Ctrl *rc   = NULL;
    rotator_object *obj = Z_ROTATOR_OBJECT_P(This);

    zval rv;

    rc = obj->pRotator_Ctrl;
    if (rc)	play_list =  rc->create_playlist();

    if (play_list) RETURN_ZVAL(play_list, 1, 0);

    RETURN_NULL();

}

PHP_METHOD(Rotator, playlist_duration)
{
	/* фактическая длительность плейлиста */
    long int value = 0;
    rotator_object *obj = Z_ROTATOR_OBJECT_P(getThis());

    if (obj)
    { 	TRotator_Ctrl *rc = obj->pRotator_Ctrl;
    	if (rc) value =  rc->get_playlist_duration();
    }

    RETURN_LONG(value);
}

PHP_METHOD(Rotator, general_duration)
{
	zval rv;
	zval *This = getThis();
	zval* m_general = zend_read_property(Z_OBJCE_P(This), This, PROPERTY_GENERAL, PROPERTY_GENERAL_LEN, 1, &rv TSRMLS_CC);

        if (m_general && Z_TYPE_P(m_general) == IS_STRING) RETURN_STR(Z_STR_P(m_general));
	RETURN_NULL();
}

PHP_METHOD(Rotator, singer_interval)
{
  	/* установленная длительность плейлиста */
        rotator_object *obj = Z_ROTATOR_OBJECT_P(getThis());

        if (obj)
        {
        	TRotator_Ctrl *rc = obj->pRotator_Ctrl;
        	if (rc) RETURN_LONG(rc->get_singer_interval());
  	}

        RETURN_NULL();
}

PHP_METHOD(Rotator, track_interval)
{
    	/* установленная длительность плейлиста */
        rotator_object *obj = Z_ROTATOR_OBJECT_P(getThis());
        if (obj)
        {
        	TRotator_Ctrl *rc = obj->pRotator_Ctrl;
        	if (rc) RETURN_LONG(rc->get_track_interval());
  	}

        RETURN_NULL();
}

PHP_METHOD(Rotator, set_aaa)
{
    	/* установленная длительность плейлиста */
		zval *This = getThis();

		zval aa;
		ZVAL_LONG(&aa, 1010);

		zend_update_static_property(Z_OBJCE_P(This), PROPERTY_AA, PROPERTY_AA_LEN, &aa TSRMLS_CC);
}

/*
 * Local variables:
 *
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */

