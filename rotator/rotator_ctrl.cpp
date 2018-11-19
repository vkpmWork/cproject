#include "rotator_ctrl.h"
#include <time.h>
#include <stdexcept>
#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "Zend/zend_hash.h"

//#define DEBUG

#ifdef DEBUG
#include <iostream>
#endif

namespace php_rotator
{

#define DEFAULT_RULES      "rules"
#define DEFAULT_FOLDERS    "folders"
#define DEFAULT_TRACKS     "tracks"
#define DEFAULT_HISTORY    "history"
#define DEFAULT_ORDER      "order"
#define DEFAULT_SCHEDULE   "schedule"

#define DEFAULT_SINGER     "executor"
#define DEFAULT_TITLE      "title"

#define DEFAULT_DURATION   "duration"
#define DEFAULT_BYPASS     "bypass"

#define DEFAULT_SERVICE	   "service"
#define DEFAULT_STATISTICS "statistics"

#define START_INDEX 	             0

#define DEFAULT_SINGER_RESTRICTION   0 /* исполнитель может быть в эфире через Х треков;// исполнитель повторяется не чаще одного раз в 3 часа */
#define DEFAULT_TRACK_RESTRICTION    0 /* трек может быть в эфире через Х треков; // трек        повторяется не чаще одного раз в 6 часа */
#define DEFAULT_RETURN_INTERVAL      0 /* вернуться в папку через Х треков , count_executor и 'count_track' игнорируются */

#define NEED_TO_CHECK_IN_HISTORY    0x02
#define NEED_TO_SKIP_RECORD         0x03
#define ADD_TO_PLAYLIST		        0x00
#define NEED_TO_CONTINUE	        0x01

#define MAX_INCREASE_COUNTER        3

#define FOLDER_RULES		    0 // правила папаки
#define COMMON_RULES		    1 // общие правила

/* надо переделать на функцию со многими аргументами. Подать один аргумент. Иначе не понимается ключ */
int on_erase_item(zval **data TSRMLS_DC)
{
	return ZEND_HASH_APPLY_REMOVE;
}

int apply_on_history_item(zval **data TSRMLS_DC)
{
            zval tmp;

	    ZVAL_DUP(&tmp, *data);

	    /* Выводим тип и значение данных */
	    if( Z_TYPE(tmp) == IS_STRING)
	    {
	        zend_printf("  =>  ");
	        ZEND_WRITE(Z_STRVAL(tmp), Z_STRLEN(tmp));
	    }
	    else if(Z_TYPE(tmp) == IS_LONG) zend_printf(" Type: int, Value: %ld  ", Z_LVAL(tmp));
	    	 else if(Z_TYPE(tmp) == IS_NULL) zend_printf(" Type: null, Value: NULL  ");
	    	 	  else if(Z_TYPE(tmp) == IS_ARRAY)  zend_printf(" Type: array  ");

	        /* Уничтожаем временную переменную */
       zval_dtor(&tmp);

      return ZEND_HASH_APPLY_KEEP;
}

int apply_on_history(zval **data TSRMLS_DC)
{
	zend_hash_apply(HASH_OF(*data), (apply_func_t) apply_on_history_item TSRMLS_DC);
	return ZEND_HASH_APPLY_KEEP;
}


int on_erase_history(zval *pData, int num_args , va_list args, zend_hash_key *hash_key)
{
		long *argument1 = va_arg(args, long*);

		if (Z_TYPE_P(pData) != IS_ARRAY) return ZEND_HASH_APPLY_KEEP;

		HashTable *ht 	 = Z_ARRVAL_P(pData);
		if (ht && (hash_key->h > *argument1))
		{
			//zend_hash_apply(ht, (apply_func_t)on_erase_item TSRMLS_CC );
			return ZEND_HASH_APPLY_REMOVE;
		}
		return ZEND_HASH_APPLY_KEEP;
}

int on_erase_play_list(zval **data TSRMLS_DC)
{
		return ZEND_HASH_APPLY_REMOVE;
}

void TRotator_Ctrl::on_init(zval* arr TSRMLS_CC)
{

#ifdef DEBUG
//	if (!ofs.is_open()) ofs.open("/var/tmp/rotator/rotator_log.txt", ofstream::out | ofstream::app);
	if (!ofs.is_open()) ofs.open("rotator_log.txt", ofstream::out | ofstream::app);

	if (ofs.is_open())
	{
		time_t timer;
		time(&timer);

		ofs << endl << "								Begining! Now is "  << ctime(&timer) << endl;
		ofs << "Версия 8.Х" << endl;
	}
#endif
/*	HashTable *ht = HASH_OF(arr);
	if (ht)
	{
		size_t i;
		Bucket p;
		zval   val;

		for (i = 0; i < ht->nTableSize; i++)
		{
			p = ht->arData[i];
			val = p.val;
			if (Z_TYPE(val) == IS_STRING) ofs << "sval = " << Z_STRVAL(val) << endl;
			else if (Z_TYPE(val) == IS_LONG) ofs << "lval" << Z_LVAL(val) << endl;
			     else if (Z_TYPE(val) == IS_ARRAY) ofs << "array= " <<  ZSTR_VAL(p.key) << endl;
		}
	}

	int v = zend_hash_num_elements(HASH_OF(arr));
	ofs << "value = " << v << endl;
*/
	max_ht_size	= 0;
	sHistory.hash_history = NULL;
	pTimer 		      = NULL;

	m_error = (arr == NULL);
	if (m_error) return;

	Array  = *arr;

	pTimer = new TTimer(general_duration);
	if (!pTimer) return;

	array_init(&play_list);
        playlist   = HASH_OF(&play_list);

        zval tmp;
	ZVAL_UNDEF(&tmp);
        ZVAL_DUP(&tmp, &Array);
	int value = zend_hash_num_elements(HASH_OF(&Array));

	if (!Z_ISUNDEF(tmp)) value = zend_hash_num_elements(HASH_OF(&tmp));

	if (!array_size(HASH_OF(&tmp)))
	{
		m_error = INPUT_ARRAY_ERROR;
		zval_dtor(&tmp);
		return;
	}

    /* сначала определяем непустые папки с треками, уже потом правила обхода и расписание */
    sRules =  search_rules(&tmp, COMMON_RULES); /* общие правила */
//    ofs << "First init Rule_singer = " << sRules.m_singer_limit << ", Rule_track = " << sRules.m_track_limit << ", Max_rule = "  << sRules.m_max_rules <<  endl;

    if (!search_folders(&tmp, true)) return;

    m_schedule = search_schedule(&tmp);
    if (!m_schedule) m_order = search_order(&tmp);

//    search_rules(&tmp);
    search_history(&tmp);

    zval_dtor(&tmp);

    track_duration = NULL;
    m_playlist_construction_complete = false;
    m_special_conditions 	     = set_special_conditions();
}

bool	TRotator_Ctrl::set_special_conditions()
{
	return max_tracks || m_schedule || general_duration;
}

bool    TRotator_Ctrl::prepare()
{
	if (m_error) return false;
#ifdef DEBUG
	if (ofs.is_open()) ofs << "In Prepare" << endl;
#endif

    zval tmp;
    ZVAL_DUP(&tmp, &Array);	

    if (!array_size(HASH_OF(&tmp)))
	{
		m_error = INPUT_ARRAY_ERROR;
		zval_dtor(&tmp);
		return false;
	}

    if (m_schedule)
    {
    	if (m_playlist_construction_complete) sSchedule.it = sSchedule.schedule.end();
    	create_schedule_order();
    }
    else if (m_playlist_construction_complete) m_order = search_order(&tmp);

	if (m_playlist_construction_complete)
	{
		for (HFolders::iterator it = sFolders.hfolders.begin(); it != sFolders.hfolders.end(); it++)
		(*it).second.index = search_next_hash_index((*it).second.ht, START_INDEX TSRMLS_CC);

		pTimer->clear_playlist_duration();
		m_playlist_construction_complete = false;
		m_error		 = EMPTY_ERROR;
	}

	zval_dtor(&tmp);
	return true;
}

TRotator_Ctrl::TRotator_Ctrl(zval* array)
			 : max_tracks(0)
			 , general_duration(NULL)
			 , track_duration(NULL)
			 , m_order(false)
			 , m_schedule(false)
			 , m_special_conditions(false)
			 , m_error(EMPTY_ERROR)

{
	on_init(array TSRMLS_CC);
}

TRotator_Ctrl::TRotator_Ctrl(zval* array, int max_tracks)
						: max_tracks(max_tracks)
						, general_duration(NULL)
		        			, track_duration(NULL)
						, m_order(false)
						, m_schedule(false)
						, m_special_conditions(false)
						, m_error(EMPTY_ERROR)
{

	on_init(array TSRMLS_CC);
}

TRotator_Ctrl::TRotator_Ctrl(zval* array, char* g_duration)
						: max_tracks(0)
						, general_duration(NULL)
		        			, track_duration(NULL)
						, m_order(false)
						, m_schedule(false)
						, m_special_conditions(false)
						, m_error(EMPTY_ERROR)
{
	general_duration = strdup(g_duration);
	on_init(array TSRMLS_CC);
#ifdef DEBUG
	if (ofs.is_open()) ofs << "Setup : general_duration = " << general_duration << endl;
#endif
}


TRotator_Ctrl::~TRotator_Ctrl()
{
}

void TRotator_Ctrl::OnDestroy()
{

#ifdef DEBUG
	if (ofs.is_open()) ofs << "Destructor" << endl;
#endif

	char *value = NULL;
	for (MHistory::iterator it = mHistory.begin(); it != mHistory.end(); it++)
	{
		
		if ((*it).second) 
		{
			value = ((*it).second)->singer;
			if (value) {efree(value); value = NULL;}
			
			value = ((*it).second)->title;
			if (value) { efree(value); value = NULL;}

			delete (*it).second;
			(*it).second  = NULL;
		} 
	}

#ifdef DEBUG
#endif

 	if (pTimer)		{ delete pTimer; pTimer = NULL;}
	if (track_duration) 	{ free(track_duration);   track_duration   = NULL; }
	if (general_duration)	{ free(general_duration); general_duration = NULL; }

#ifdef DEBUG
		if (ofs.is_open())
		{
			ofs << "Destructor end" << endl;
			ofs.close();
		}
#endif
}

bool  TRotator_Ctrl::playlist_construction_completed()
{
    bool aa = (max_tracks > 0) && (size() >= max_tracks);
    bool bb = (m_schedule || general_duration) && pTimer->is_24hours_completed();

//   cout  << "max_tracks = " << max_tracks << "; size() = " << size() << endl;// << "?; general_duration = " << general_duration << endl;
//   cout << "aa = " << aa <<"; bb = " << bb << endl;
    aa =  aa || bb;

#ifdef DEBUG
	if (aa && ofs.is_open()) ofs << "Stopped!!" << endl;
#endif
    return  aa;
}

bool TRotator_Ctrl::playlist_size_increase()
{
	int sz = this->size();
#ifdef DEBUG
	if (ofs.is_open()) ofs << "sFolders.m_increase_counter = " << sFolders.m_increase_counter + 1 << endl;
#endif

	if (sz > sFolders.m_previos_playlist_size)
	{
		sFolders.m_previos_playlist_size = sz;
		sFolders.m_increase_counter      = 0;
	}
	else
		if (++sFolders.m_increase_counter > MAX_INCREASE_COUNTER)
		{
			if (sRules.m_singer_limit >= sz) m_error  = EXECUTOR_ERROR;
			if (sRules.m_track_limit  >= sz) m_error |= TRACK_ERROR;
			return false;
		}
	return true;
}

long  TRotator_Ctrl::size()
{
	long value = 0;
	if (playlist) value = zend_hash_num_elements(playlist);

	return value;
}

bool  TRotator_Ctrl::isempty()
{
	return this->size() == 0;
}

void TRotator_Ctrl::get_node_hash_index()
{
}

int TRotator_Ctrl::array_size(HashTable *ht)
{
    return ht != NULL ? zend_hash_num_elements(ht) : 0;
}

void  TRotator_Ctrl::find_data(zval **data, char *argument)
{
}

void TRotator_Ctrl::add_to_playlist(zval *data, long statistics)
{
	if (data == NULL)
	{
	#ifdef DEBUG
		if (ofs.is_open()) ofs << "ERROR!!!!!!!!!!!!!!! add_to_playlist(zval **data) == NULL" << endl;
	#endif

		return;
	}
	zval array, array1;

	ZVAL_ZVAL(&array, data, 1, 0);
	add_next_index_zval(&play_list, &array);

#ifdef DEBUG
    	if (ofs.is_open())
    	{
   	    	ofs << "добавить запись в playlist" << endl;
    	}
#endif

	HashTable *ht = Z_ARRVAL_P(data);

        if (ht && array_size(ht))
        {
                                zval *h_singer =  find_data_in_node(ht, (char*)DEFAULT_SINGER); 
                                zval *h_title  =  find_data_in_node(ht, (char*)DEFAULT_TITLE);
				    
                                
				if (h_singer && h_title)
				{
				    THistory *H = new THistory;
                            	    if (H)
                            	    {
					char* m_data = Z_STRVAL_P(h_singer);
					int sz = Z_STRLEN_P(h_singer);
					if (m_data) H->singer = estrndup(m_data, sz); //strdup(Z_STRVAL_P(h_singer));
                                        
					m_data = Z_STRVAL_P(h_title);
					sz     = Z_STRLEN_P(h_title);
					if (m_data) H->title  = estrndup(m_data, sz); //strdup(Z_STRVAL_P(h_title));

                                        H->statistics = ++statistics;
					mHistory[mHistory.size()] = H;
                            	    }
				}
				

        }

	sFolders.m_increase_counter = 0;     /* сбросили счетчик повторов NEED_TOSKIP_RECORD */

    	zval *duration = find_data_in_node(HASH_OF(data), (char*)DEFAULT_DURATION);

    	char *sd = NULL;

    	if (duration &&  Z_TYPE_P(duration) == IS_STRING && Z_STRLEN_P(duration) > 0 ) sd = strdup(Z_STRVAL_P(duration));

    	if (pTimer)
    	{
		pTimer->append_playlist_duration(sd);
		if (m_schedule && pTimer->remained_interval(sd) <= 0) 
		{ 
		    create_schedule_order();
		    set_start_folder = true;
		}
    	}
    	if (sd) free(sd);
}

void TRotator_Ctrl::add_to_playlist(zval *singer, zval *track, zval *duration)
{
}


zval *TRotator_Ctrl::get_node_ptr(HashTable* ht, char *node_name)
{
	if (!ht) return NULL;

        zend_hash_internal_pointer_reset(ht);
	
	zval *arr_data = zend_hash_str_find(ht, node_name, strlen(node_name) TSRMLS_CC);
	if ( arr_data  && Z_TYPE_P(arr_data) == IS_ARRAY) return arr_data;

	return NULL;
}

zval *TRotator_Ctrl::get_node_ptr(zval *z_node, char *node_name)
{
    if (!z_node) return NULL;

    return get_node_ptr(HASH_OF(z_node), node_name);
}

zval* TRotator_Ctrl::find_data_in_node(HashTable *ht, char *str)
{
	zval *arr_data = NULL;

	if (ht)
	{
		zend_hash_internal_pointer_reset(ht);
		arr_data = zend_hash_str_find(ht, str, strlen(str));
	}
	return 	arr_data;
}


zval* TRotator_Ctrl::find_data_in_node(zval *z_node, char *str)
{
	if (!z_node) return NULL;

	return find_data_in_node(HASH_OF(z_node), str TSRMLS_CC);
}


TRules TRotator_Ctrl::search_rules(zval* z_node, bool common_rules)
{
	TRules r;
	if (z_node == NULL) return r;

	if (common_rules == COMMON_RULES)
	{
		r.m_singer_limit    = DEFAULT_SINGER_RESTRICTION;
		r.m_track_limit     = DEFAULT_TRACK_RESTRICTION;
		r.m_return_interval = DEFAULT_RETURN_INTERVAL;
	}
	else
	{
		r.m_singer_limit    = sRules.m_singer_limit;
		r.m_track_limit     = sRules.m_track_limit;
		r.m_return_interval = sRules.m_return_interval;
	}


	HashTable *ht = Z_ARRVAL_P(z_node);
	if (ht && common_rules)
	{
        	zend_hash_internal_pointer_reset(ht);
        	zval *zval_rules = get_node_ptr(ht, (char*)DEFAULT_RULES);

        	zval* tmp = find_data_in_node(zval_rules, (char*)"singer_interval");
        	if (tmp)
        	{
                	if (Z_TYPE_P(tmp) == IS_STRING) r.m_singer_limit = atoi(Z_STRVAL_P(tmp));
                	else if (Z_TYPE_P(tmp) == IS_LONG) r.m_singer_limit = Z_LVAL_P(tmp);
        	}

        	tmp       = find_data_in_node(zval_rules, (char*)"track_interval");
        	if (tmp)
        	{
                	if (Z_TYPE_P(tmp) == IS_STRING) r.m_track_limit = atoi(Z_STRVAL_P(tmp));
                	else if (Z_TYPE_P(tmp) == IS_LONG) r.m_track_limit = Z_LVAL_P(tmp);
        	}

        	tmp       = find_data_in_node(zval_rules, (char*)"folder_interval");
        	if (tmp)
        	{
                	if (Z_TYPE_P(tmp) == IS_STRING) r.m_return_interval = atoi(Z_STRVAL_P(tmp));
                	else if (Z_TYPE_P(tmp) == IS_LONG) r.m_return_interval = Z_LVAL_P(tmp);
        	}

	}

	r.m_max_rules = r.m_singer_limit < r.m_track_limit ? r.m_track_limit : r.m_singer_limit;

#ifdef DEBUG
                if (ofs.is_open()) ofs << "Rules. singer limit    =  " << r.m_singer_limit << endl;
                if (ofs.is_open()) ofs << "Rules. track  limit    =  " << r.m_track_limit << endl;
                if (ofs.is_open()) ofs << "Rules. folder interval =  " << r.m_return_interval << endl;
#endif
      return r;	
}

unsigned long TRotator_Ctrl::search_next_hash_index(HashTable *ht, unsigned long index)
{
	int i   = 0;
	int cnt = array_size(ht);

	for ( i = index; i < cnt; i++)
		if (zend_hash_index_exists(ht, i))
		{
			index = i;
			break;
		}

	if (i == cnt) index = (unsigned long)i;
	return index;
}

void TRotator_Ctrl::search_tracks(zval* z_node, string key_word)
{

	if (Z_ISUNDEF(*z_node)) return;

	HashTable *ht = Z_ARRVAL_P(z_node);

	if (!ht) return;

	bool is_service = false;
	zval *arr_data  = NULL;

	zend_hash_internal_pointer_reset(ht);
	zval *zval_tracks = get_node_ptr(ht, (char*)DEFAULT_TRACKS);

	if (zval_tracks && (Z_TYPE_P(zval_tracks) == IS_ARRAY))
	{

			THFolders    hf;
			hf.ht  	   = Z_ARRVAL_P(zval_tracks);
			hf.size	   = array_size(hf.ht);
			hf.statistics  = 1;

			if (hf.size)
			{
				hf.bypass  = false;
				hf.index   = search_next_hash_index(hf.ht, START_INDEX);

				arr_data = zend_hash_str_find (ht, DEFAULT_SERVICE , sizeof(DEFAULT_SERVICE)-1);

				is_service = (arr_data != NULL) && (Z_TYPE_P(arr_data) == IS_TRUE);

//				Удалено в версии 7.2.1
//				if (is_service)
//				  	is_service = (bool)Z_LVAL_P(arr_data);

				hf.service                  = is_service;
				sFolders.hfolders[key_word] = hf;
				sFolders.folders[key_word]  = true;
				increase_max_ht_size(hf.size);
#ifdef DEBUG
				if (ofs.is_open())
				{	ofs <<  key_word  << " max_ht_size =	" << max_ht_size << endl;
					ofs <<  "Service forder  =	" << is_service << endl;
				}
#endif

			}
	}

}

bool TRotator_Ctrl::search_folders(zval *arr, bool is_init)
{
    if (!arr) return false;
    bool ret_value = true;

    zval		*arr_value;
    zend_string 	*key;
    unsigned long    	num_index;

    zval *zval_folders = get_node_ptr(arr, (char*)DEFAULT_FOLDERS TSRMLS_CC);
    if (!zval_folders) return false;

    HashTable *ht = Z_ARRVAL_P(zval_folders);

    if (ht == NULL || zend_hash_num_elements(ht) == 0) return false;

    ZEND_HASH_FOREACH_KEY_VAL(ht, num_index, key, arr_value)
    {
	if (key && arr_value)
	{
   		string m_str = zend_string_to_std_string(key);
		search_tracks (arr_value, m_str TSRMLS_CC);
         	if (is_init && sFolders.hfolders.count(m_str.c_str()))
		{
			TRules r = search_rules(arr_value, FOLDER_RULES);
			mRules[m_str] = r; 
#ifdef DEBUG
			if (ofs.is_open())
			{
					ofs << "Rules Key = " << m_str;
                			ofs << ":: Rules. singer limit    =  " << r.m_singer_limit;
                			ofs << " Rules. track  limit    =  " << r.m_track_limit;
                			ofs << " Rules. folder interval =  " << r.m_return_interval << endl << endl;
			}
#endif

/*			int m_service = 0;
			int m_size    = 0;

			m_service  = sFolders.hfolders[m_str].service;
			m_size     = sFolders.hfolders[m_str].size;

			if ((m_service == true) || (r.m_singer_limit <= m_size && r.m_track_limit <= m_size))
			{
				mRules[m_str] = r; 
#ifdef DEBUG
				if (ofs.is_open())
				{
					ofs << "Rules Key = " << m_str << " Service = " << m_service << " Size = " << m_size;
                			ofs << ":: Rules. singer limit    =  " << r.m_singer_limit;
                			ofs << " Rules. track  limit    =  " << r.m_track_limit;
                			ofs << " Rules. folder interval =  " << r.m_return_interval << endl << endl;
				}
#endif
			}
			else
			{
			    m_error |= RULES_ERROR;
			    ret_value = false;
			    break;
			}
*/
		}
	}
    } ZEND_HASH_FOREACH_END();

    
    return ret_value;
}

void  TRotator_Ctrl::search_history(zval *arr)
{

	/* определяем указатель на начало папки history */
	//char * key = NULL;

    	zval 	 *pData, *h_singer = NULL, *h_title = NULL;
    	HashTable *ht 	   = NULL;
    	int m_statistics_value = 0;

    	zend_string *key;
    	unsigned long    num_index;

	sHistory.last_init_index = -1;
	if (arr == NULL) return;

	zval *tmp = get_node_ptr(arr, (char*)DEFAULT_HISTORY TSRMLS_CC);
	if (tmp)
	{
	    sHistory.hash_history = Z_ARRVAL_P(tmp);
	    ZEND_HASH_FOREACH_KEY_VAL(sHistory.hash_history, num_index, key, pData)
    	    {
		if (pData && (Z_TYPE_P(pData) == IS_ARRAY))
		{
    			ht = Z_ARRVAL_P(pData);

    			if (ht && array_size(ht))
			{
				h_singer =  find_data_in_node(ht, (char*)DEFAULT_SINGER); /* executor */
				if (!h_singer) continue;

    				h_title  =  find_data_in_node(ht, (char*)DEFAULT_TITLE);
				if (!h_title) continue;

				m_statistics_value = calc_statistics_in_history(h_title); /* посчитаем вес в истории */

				
				THistory *H = new THistory;
				if (H)
				{
					char *m_singer = Z_STRVAL_P(h_singer);
					char *m_title  = Z_STRVAL_P(h_title);
#ifdef DEBUG
					ofs << "Добавить в историю" << m_singer << "  " << m_title << endl;
#endif
					if (m_singer) H->singer = estrndup(m_singer, Z_STRLEN_P(h_singer));
					if (m_title)  H->title  = estrndup(m_title , Z_STRLEN_P(h_title ));

					H->statistics = m_statistics_value;
					mHistory[num_index] = H;
				}
				

			}
    		}
	    }ZEND_HASH_FOREACH_END();

	}
}

int TRotator_Ctrl::calc_statistics_in_history(zval *str)
{
	HashTable *ht    = NULL;
	int m_counter    = 0;
	zval *pData      = NULL;
	zval  *tmp_title = NULL;

	zend_string    *key = NULL;
    	unsigned long  num_index;

    ZEND_HASH_FOREACH_KEY_VAL(sHistory.hash_history, num_index, key, pData)
    {

	if (pData && (Z_TYPE_P(pData) != IS_ARRAY)) continue;

    	ht = Z_ARRVAL_P(pData);

    	if (ht && array_size(ht))
	{
    		 tmp_title = find_data_in_node(ht, (char*)DEFAULT_TITLE);

    		 if (tmp_title && Z_TYPE_P(tmp_title) == IS_STRING)
    		 {
    			m_counter += (int) is_equal(str, tmp_title);
    		 }
	}

    } ZEND_HASH_FOREACH_END();		

    return m_counter;
}

bool TRotator_Ctrl::search_order(zval * arr)
{
    sOrder.folders_order.clear();

    if (!arr) return false;

    zval 		*arr_value;
    zend_string		*key = NULL;
    unsigned long  	 num_index = 0;

    HashTable 		*ht  = NULL;
    string 			 s;

    HFolders::iterator it;
    THFolders  hf;

    max_ht_size = 0;

    zval *order    = get_node_ptr(arr, (char*)DEFAULT_ORDER TSRMLS_CC);
    if (order) 	ht = Z_ARRVAL_P(order);

    if (ht == NULL ||zend_hash_num_elements(ht) == 0) 
    {
		it = sFolders.hfolders.begin();
		while(it != sFolders.hfolders.end())
		{
			sOrder.folders_order[num_index++] = (*it).first;
			THFolders  hf = (*it).second;
			int aa = (int) array_size(hf.ht);
			increase_max_ht_size(aa);

			it++;
		}
#ifdef DEBUG
    		if (ofs.is_open())
    		{
    			ofs << "\nОпределяем порядок обхода папок по индексу" << endl;
			for (Order::iterator it	= sOrder.folders_order.begin(); it != sOrder.folders_order.end(); it++)
				ofs << (*it).first << " папка " << (*it).second << endl;

    			ofs << "sOrder.folders_order.size() = " << sOrder.folders_order.size() << endl;
    		}

#endif
		
		return sOrder.folders_order.size() > 0;
    }

    ZEND_HASH_FOREACH_KEY_VAL(ht, num_index, key, arr_value)
    {
    	/* Выводим тип индекса элемента и название/значение */

	if (arr_value  && Z_TYPE_P(arr_value) == IS_STRING )
    	{
    		s = Z_STRVAL_P(arr_value);
		it = sFolders.hfolders.find(s);
    		
		if (it != sFolders.hfolders.end())
		{
			sOrder.folders_order[num_index] = s;

			THFolders  hf = (*it).second;
			int aa = (int) array_size(hf.ht);
			increase_max_ht_size(aa);
		}
#ifdef DEBUG
    		else  if (ofs.is_open()) ofs <<  "folder = " << s  << " in ORDER not found" << endl;
#endif
    	}
    } ZEND_HASH_FOREACH_END();

#ifdef DEBUG
    if (ofs.is_open())
    {
    		ofs << "\nОпределяем порядок обхода папок по order" << endl;
		for (Order::iterator it	= sOrder.folders_order.begin(); it != sOrder.folders_order.end(); it++)
				ofs << (*it).first << " папка " << (*it).second << endl;

    			ofs << "sOrder.folders_order.size() = " << sOrder.folders_order.size() << endl;
    }

#endif

    return sOrder.folders_order.size() > 0;
}

bool 	TRotator_Ctrl::search_schedule(zval * arr)
{
	if (arr == NULL) return false;

	zend_string	 *key;
	zval 		 *data  = NULL;
	HashTable        *ht1  = NULL;
	Order		  schedule_item;
	string		  s;

	unsigned long sch_index, order_index;

	zval *value    = get_node_ptr(arr, (char*)DEFAULT_SCHEDULE  TSRMLS_CC);

        if (value == NULL || Z_TYPE_P(value) != IS_ARRAY) 
	{
		return false;
	}	

        HashTable *ht = HASH_OF(value);

       if (ht == NULL || zend_hash_num_elements(ht) == 0) return false;


       for (	zend_hash_internal_pointer_reset(ht);
		zend_hash_has_more_elements(ht) == SUCCESS;
		zend_hash_move_forward(ht)
   	   )
	{


				if (zend_hash_get_current_key(ht, &key, &sch_index) == HASH_KEY_IS_LONG)
				{
					data = zend_hash_get_current_data(ht);
					if (data == NULL || Z_TYPE_P(data) != IS_ARRAY) continue;
#ifdef DEBUG
					if (ofs.is_open()) ofs <<  "Schedule hour number = " << sch_index << endl;
#endif
					ht1 = Z_ARRVAL_P(data);
					schedule_item.clear();

					int counter = 0;
					for (	zend_hash_internal_pointer_reset(ht1);
							zend_hash_has_more_elements(ht1) == SUCCESS;
							zend_hash_move_forward(ht1)
						)
					{
						if (zend_hash_get_current_key(ht1, &key, &order_index) == HASH_KEY_IS_LONG)
						{
							data = zend_hash_get_current_data(ht1);
							if (data == NULL || Z_TYPE_P(data) != IS_STRING) continue;

							s = Z_STRVAL_P(data);
				    			if (sFolders.folders.count(s))
				    			{
#ifdef DEBUG
				    				if (ofs.is_open()) ofs <<  counter << " => " << s << endl;
#endif
				    				schedule_item[counter++] = s;
				    			}
#ifdef DEBUG
				    			else  if (ofs.is_open()) ofs <<  "folder = " << s  << " in SCHEDULE not found" << endl;
#endif

						}

					}

					if(schedule_item.size()) sSchedule.schedule[sch_index] = schedule_item;
				}
	}

//    sSchedule.it = sSchedule.schedule.end();

    int current_hour = pTimer->get_current_hour();

    int m_hour = 0;
    Schedule::iterator it;
    for (it = sSchedule.schedule.begin(); it != sSchedule.schedule.end(); it++)
    {
	if (current_hour < (*it).first) break;
	m_hour = (*it).first;
    }

    sSchedule.it = sSchedule.schedule.find(m_hour);

    if (sSchedule.it != sSchedule.schedule.end())
    {
    	int m_interval = it != sSchedule.schedule.end() ? (*it).first - (*sSchedule.it).first : DEFAULT_DAY - (*sSchedule.it).first;
#ifdef DEBUG
    	if (ofs.is_open()) 
    	{
		ofs << "Current hour for creating schedule is " << current_hour << endl;
		ofs << "Длительность интервала " << m_interval  << endl;
    	} 	
#endif

    	pTimer->set_interval(m_interval);
	pTimer->define_playlist_duration(m_hour);
	
	sOrder.folders_order = (*sSchedule.it).second;
	m_order = sOrder.folders_order.size();

#ifdef DEBUG
	if (ofs.is_open())
	{
		ofs <<  " ---Правила ротации по расписанию! Интервал = " << m_hour << " час"<< endl;
		for(Order::iterator it = sOrder.folders_order.begin(); it != sOrder.folders_order.end(); it++)
			ofs <<  (*it).first <<  " => " << (*it).second << endl;
	}
#endif

    }


    return (sSchedule.schedule.size() > 0);// && (sSchedule.it != sSchedule.schedule.end());//create_schedule_order();
}

bool    TRotator_Ctrl::create_schedule_order()
{
	if (sSchedule.it == sSchedule.schedule.end()) sSchedule.it = sSchedule.schedule.begin();
	else sSchedule.it++;

	if (sSchedule.it == sSchedule.schedule.end()) return false;

	sOrder.folders_order = (*sSchedule.it).second;

	m_order = sOrder.folders_order.size();

	Schedule::iterator  it_next = sSchedule.it;
	it_next++;

	int aa = it_next != sSchedule.schedule.end() ? (*it_next).first - (*sSchedule.it).first : DEFAULT_DAY - (*sSchedule.it).first;
	pTimer->set_interval(aa);

#ifdef DEBUG
	if (ofs.is_open())
	{
		ofs <<  " ---Правила ротации по расписанию! Интервал = " << aa << " час"<< endl;
		for(Order::iterator it = sOrder.folders_order.begin(); it != sOrder.folders_order.end(); it++)
			ofs <<  (*it).first <<  " => " << (*it).second << endl;
	}
#endif
	return m_order;
}


TRules TRotator_Ctrl::get_current_rules(char *folder)
{
	return mRules.count(folder) ? mRules[folder] : sRules;
}

bool TRotator_Ctrl::is_equal(zval *str1, zval *str2)
{
	if (str1 == NULL || str2 == NULL) return true; 		/* true - пропускаем трек */
	if (Z_STRLEN_P(str1) == 0 || Z_STRLEN_P(str2) == 0) return true; /* true - пропускаем трек */	
	return strncmp(Z_STRVAL_P(str1), Z_STRVAL_P(str2), Z_STRLEN_P(str2)) == 0;
}

/*
int  TRotator_Ctrl::random_selection(int value)
{
	return rand() % value;
}
*/
int  TRotator_Ctrl::find_record_in_history(zval *tmp_singer, zval *tmp_title, int m_rule_singer, int m_rule_track, int m_max_rule, int &m_folder_statistics)
{
	int   fl_ok = ADD_TO_PLAYLIST;

	if (!mHistory.size()) return fl_ok;

	char *m_singer = Z_STRVAL_P(tmp_singer);
	char *m_title  = Z_STRVAL_P(tmp_title);

	long  count_of_tracks_between_equal = -1;
	int   m_statistics 		    =  0;	

	THistory *H;

	for (MHistory::reverse_iterator it = mHistory.rbegin(); it != mHistory.rend(); it++)
	{

		H = (*it).second;

	        count_of_tracks_between_equal++;

		if (strcmp(H->singer, m_singer) != 0) continue;

#ifdef DEBUG
		if (ofs.is_open())
		{
		 	ofs << "		Совпадение В ИСТОРИИ по исполнителю " << endl;
	    		ofs << "	Проверка правила исполнителя count_of_tracks_between_equal >= m_rule_singer " << count_of_tracks_between_equal << " >= " << m_rule_singer << endl;
			ofs << "	Вес папки = " << m_statistics << endl;	
		}
#endif
		if (count_of_tracks_between_equal >= m_rule_singer)
		{
			/* правило исполнителя выполнено, проверим правило трека */
			fl_ok  = title_statistics_in_history(m_title, m_rule_track, m_statistics);
			if (fl_ok == ADD_TO_PLAYLIST && m_statistics > 0)
			{
					// проверим вес трека и папки
	    		    		if (m_statistics > m_folder_statistics) fl_ok = NEED_TO_SKIP_RECORD;
#ifdef DEBUG
	    		    		if (ofs.is_open()) 	ofs << "	Проверка веса    трека	track_statistics <  m_folder_statistics : " << m_statistics << " VS " << m_folder_statistics << endl;
#endif
			}

		}
		else fl_ok = NEED_TO_SKIP_RECORD;

		break;

	}

	return fl_ok;
}

int   TRotator_Ctrl::title_statistics_in_history(char *m_title, int m_rule_track, int &stat)
{
       int  count_of_tracks_between_equal = -1;
       THistory *H  = NULL;

       int  flag = ADD_TO_PLAYLIST;
       stat      = 0;

       for (MHistory::reverse_iterator it = mHistory.rbegin(); it != mHistory.rend(); it++)
        {
                count_of_tracks_between_equal++;
 
                H = (*it).second;

                if (strcmp(H->title, m_title) != 0) continue;

                if (count_of_tracks_between_equal >= m_rule_track) 
		{
			stat = H->statistics;
#ifdef DEBUG
                	if (ofs.is_open())
                	{
                        	ofs << "        	Совпадение В ИСТОРИИ по треку " << endl;
                        	ofs << "Выполнено правило трека count_of_tracks_between_equal > m_rule_track " << count_of_tracks_between_equal << " > " << m_rule_track << endl;
				ofs << "Вес трека = " << endl;
	        	}
#endif
		}
		else
		{
			flag = NEED_TO_SKIP_RECORD;	
#ifdef DEBUG
                	if (ofs.is_open())
                	{
                        	ofs << "        	Совпадение В ИСТОРИИ по треку " << endl;
                        	ofs << "        Count_of_tracks_between_equal <= m_rule_track " << count_of_tracks_between_equal << " <= " << m_rule_track << endl;
	        	}
#endif
		}

		break;
	}
	return flag;
}

/*
int  TRotator_Ctrl::find_record_in_history(HashTable *ht, zval *tmp_singer, zval *tmp_title, int m_rule_singer, int m_rule_track, int m_max_rule, int &m_folder_statistics)
{
#ifdef DEBUG
		if (ofs.is_open()) ofs <<  "ищем запись в истории эфира\n" << endl;
#endif
	/* ищем запись в истории эфира 
	zval 		*data = NULL, tmp, *zval_ht = NULL;
	long 		count_of_tracks_between_equal = 0;
	long 		index = -1;

	int         	ht_size = array_size(ht);
	int  	    	fl_ok = ADD_TO_PLAYLIST;
	
	if (!ht_size)
	{
		return fl_ok;
	}
	for (	zend_hash_internal_pointer_end(ht);
			zend_hash_has_more_elements(ht) == SUCCESS;
			zend_hash_move_backwards(ht)
		)
	{

		        fl_ok = NEED_TO_CONTINUE;

		       /* Получаем данные и копируем их во временную переменную 
			data = 	zend_hash_get_current_data(ht);

		        //tmp = *data;
//		        zval_copy_ctor(&tmp);
//		        INIT_PZVAL(&tmp);
   		        ZVAL_DUP(&tmp, data);	

		        if (Z_TYPE(tmp) == IS_ARRAY)
		        {

		        	zval_ht = find_data_in_node(&tmp, (char*)DEFAULT_SINGER);
		    		if (zval_ht && Z_TYPE_P(zval_ht) == IS_STRING)
		    		{
#ifdef DEBUG
		    			if (ofs.is_open()) ofs << "Singer in history = " << Z_STRVAL_P(zval_ht) << endl;
#endif
		    			if (is_equal(zval_ht, tmp_singer))
		    			{
#ifdef DEBUG
		    				if (ofs.is_open())
		    				{
		    					ofs << "		Совпадение В ИСТОРИИ по исполнителю " << endl;
	    						ofs << "	Проверка правила исполнителя count_of_tracks_between_equal >= m_rule_singer " << count_of_tracks_between_equal << " >= " << m_rule_singer << endl;
		    				}
#endif
		    				if (count_of_tracks_between_equal >= m_rule_singer)
		    				{
		    					/* правило исполнителя выполнено, проверим правило трека 

		    					zval_ht = find_data_in_node(&tmp, (char*)DEFAULT_TITLE TSRMLS_CC);

		    		    			if (is_equal(zval_ht, tmp_title))
		    					{	/* нашли совпадение по треку 
#ifdef DEBUG
		    		    				if (ofs.is_open())
		    		    				{	ofs << "		Совпадение В ИСТОРИИ по треку " << endl;
    		    							ofs << "	Проверка правила треков	count_of_tracks_between_equal >= m_rule_track : " 		 << count_of_tracks_between_equal << " >= " << m_rule_track << endl;
		    		    				}
#endif

		    		    				if (count_of_tracks_between_equal >= m_rule_track)
		    		    				{	/* проверим вес трека 
     	    		    						zval_ht = find_data_in_node(&tmp, (char*)DEFAULT_STATISTICS TSRMLS_CC);
	        		    					index = (zval_ht && Z_TYPE_P(zval_ht) == IS_LONG) ? Z_LVAL_P(zval_ht) : -1;
	    		    						fl_ok = index < m_folder_statistics ? ADD_TO_PLAYLIST : NEED_TO_SKIP_RECORD;
#ifdef DEBUG
	    		    						if (ofs.is_open()) 	ofs << "	Проверка веса    трека	index  <  m_folder_statistics : " << index << " VS " << m_folder_statistics << endl;
#endif
		    		    				}
		    		    				else fl_ok = NEED_TO_SKIP_RECORD;
		    		    			}
		    				}
		    				else fl_ok = NEED_TO_SKIP_RECORD;

		    			}

		    		}

		        }
		        zval_dtor(&tmp);
		        if (fl_ok != NEED_TO_CONTINUE) break;

		        count_of_tracks_between_equal++;
	}	//for

	if (fl_ok == NEED_TO_CONTINUE) fl_ok = ADD_TO_PLAYLIST;
	return fl_ok;
}

*/
int TRotator_Ctrl::check_current_record(HashTable *ht, int m_ht_size, zval *tmp_singer, zval *tmp_title, int m_rule_singer, int m_rule_track, int m_max_rule, int &delta)
{
	/* ищем запись в уже сформированном плейлисте */

	zend_string     *key  = NULL;
	zval 		*data = NULL, tmp, *zval_ht = NULL;
	unsigned long 		 num_index;

	long count_of_tracks_between_equal = delta;
	int fl_ok 						   = NEED_TO_CONTINUE;

#ifdef DEBUG
	if (ofs.is_open())
	{	ofs << "					check_current_record!" << endl;
		ofs << "Count_of_tracks_between_equal = " << count_of_tracks_between_equal << endl;
	}
#endif

    if (zend_hash_get_current_key(ht, &key, &num_index) == HASH_KEY_IS_LONG)
	{
	       /* Получаем данные и копируем их во временную переменную */
		data =	zend_hash_get_current_data(ht);

	        //tmp = *data;
	        ZVAL_DUP(&tmp, data);
		//zval_copy_ctor(&tmp);
	        //INIT_PZVAL(&tmp);

	        if (Z_TYPE(tmp) == IS_ARRAY)
	        {
	        	zval_ht = find_data_in_node(&tmp, (char*)DEFAULT_SINGER TSRMLS_CC);

#ifdef DEBUG
    			if (ofs.is_open())  ofs << endl << "Singer in playlist = " << Z_STRVAL_P(zval_ht) <<  " Num_index = " << num_index  << endl;
#endif

	    		if (zval_ht && Z_TYPE_P(zval_ht) == IS_STRING)
	    		{
	    			if (is_equal(zval_ht, tmp_singer)) /* нашли совпадение по исполнителю */
	    			{

#ifdef DEBUG
	    		    	if (ofs.is_open())  ofs << "Нашли совпадение по исполнителю! " << endl;
#endif
	    				if (count_of_tracks_between_equal >= m_rule_singer)
	    				{
	    					/* правило исполнителя выполнено, проверим правило трека */
#ifdef DEBUG
	    					if (ofs.is_open())  ofs << "правило исполнителя выполнено, проверим правило трека! " << endl;

#endif

	    					zval_ht = find_data_in_node(&tmp, (char*)DEFAULT_TITLE TSRMLS_CC);

	    					/* ищем совпадение по треку */
	    					if (is_equal(zval_ht, tmp_title)) fl_ok = count_of_tracks_between_equal >= m_rule_track ? ADD_TO_PLAYLIST /* правило трека выполнено */ : NEED_TO_SKIP_RECORD;
	    				}
	    				else fl_ok = NEED_TO_SKIP_RECORD;

	    			}
	    		}
	        }

	        zval_dtor(&tmp);
	}

	return fl_ok;
}

void TRotator_Ctrl::add_to_folder(HashTable *ht, zval *tmp_singer, zval *tmp_title, zval *tmp_duration)
{
		return;
		zval *array = NULL;

	        //MAKE_STD_ZVAL(array);
		ZVAL_NEW_ARR(array);

		if (!array) return;

		add_assoc_string(array, (char*)DEFAULT_SINGER,   Z_STRVAL_P(tmp_singer)    );
		add_assoc_string(array, (char*)DEFAULT_TITLE,    Z_STRVAL_P(tmp_title)     );
		add_assoc_string(array, (char*)DEFAULT_DURATION, Z_STRVAL_P(tmp_duration)  );
		add_assoc_string(array, (char*)DEFAULT_BYPASS,   (char*)"1" );

//		zend_hash_next_index_insert(ht, (void**)&array,  sizeof(zval*), NULL);
		zend_hash_next_index_insert_new(ht, array);
#ifdef DEBUG
		if (ofs.is_open()) ofs << " добавить запись в конец текущей папки и перейти к следующей записи:  Tmp_singer " << Z_STRVAL_P(tmp_singer) << " Tmp_title " << Z_STRVAL_P(tmp_title) <<  endl;
#endif
}

bool TRotator_Ctrl::is_valid_type(zval *ex, zval *tr)
{
	return (ex !=NULL && tr != NULL && Z_TYPE_P(ex) == IS_STRING && Z_TYPE_P(tr) == IS_STRING && Z_STRLEN_P(ex) != 0 && Z_STRLEN_P(tr) != 0);
}

int  TRotator_Ctrl::find_record_in_playlist(THFolders  hf  /* element of folders */ , char *m_key_folder)
{
  zval 	 *pData = zend_hash_index_find(hf.ht, hf.index);

  if (pData && (Z_TYPE_P(pData) == IS_ARRAY))	
  {
	  	HashTable *ht = Z_ARRVAL_P(pData); /* запись из папки tracks */

		zval *tmp_singer =  find_data_in_node(ht, (char*)DEFAULT_SINGER); /* executor */
		zval *tmp_title  =  find_data_in_node(ht, (char*)DEFAULT_TITLE);  /* title    */


		if (!is_valid_type(tmp_singer, tmp_title TSRMLS_CC)) return NEED_TO_SKIP_RECORD;

		int     ht_size    = array_size(playlist);
		TRules  r = get_current_rules(m_key_folder);

#ifdef DEBUG
		if (ofs.is_open())
		{	ofs << endl << "Current rules are : key_folder = " << m_key_folder << " - IsArray index " << hf.index << endl;
			ofs << "Rule_singer = " << r.m_singer_limit << ", Rule_track = " << r.m_track_limit << ", Max_rule = "  << r.m_max_rules << " Playlist Size = " << ht_size << endl;
			ofs << "Tmp_singer = "  << Z_STRVAL_P(tmp_singer) << " Tmp_title = " << Z_STRVAL_P(tmp_title) << endl;
			ofs << "Текущий вес папки  = " <<  hf.statistics << endl;
		}
#endif

		int 	what_to_do = NEED_TO_CONTINUE;
		int     counter    = hf.statistics;


		if (hf.service) what_to_do = ADD_TO_PLAYLIST; /* ignoring the rules */
		else what_to_do = find_record_in_history(tmp_singer, tmp_title, r.m_singer_limit, r.m_track_limit, r.m_max_rules, counter TSRMLS_CC);

		if (what_to_do == ADD_TO_PLAYLIST) add_to_playlist(pData, counter);

		return what_to_do;
  }
  else
  {
#ifdef DEBUG
     	    if (ofs.is_open()) ofs << "Index not found " <<  endl;
#endif
  }
  return 0;
}

bool TRotator_Ctrl::global_analize(HFolders::iterator it, bool &to_next /* признак конца папки */)
{
	bool fl_next  = true; /* true = перейти к следующей папке */
	THFolders  hf = (*it).second;

#ifdef DEBUG
	if (ofs.is_open()) ofs << "global_analize  Index = "<< hf.index << endl;
#endif

	if (hf.index >= array_size(hf.ht))
	{
    			to_next 	  = (*it).second.index < max_ht_size;
			hf.index 	  = (*it).second.index = search_next_hash_index((*it).second.ht, START_INDEX TSRMLS_CC);
    			hf.statistics = ++(*it).second.statistics;
#ifdef DEBUG
        		if (ofs.is_open()) ofs << "Достигли конца папки " << (*it).first.c_str() << ". Установленный вес папки = " << (*it).second.statistics <<endl;
#endif
	}


	char *key_folder = strdup((*it).first.c_str());
   	int what_to_do = find_record_in_playlist(hf, key_folder TSRMLS_CC);
   	free(key_folder);

 	(*it).second.index = search_next_hash_index(hf.ht, hf.index+1 TSRMLS_CC);

   	if (what_to_do == NEED_TO_SKIP_RECORD)
   	{
#ifdef DEBUG
   					if (ofs.is_open()) ofs << "NEED_TO_SKIP_RECORD. Continue!  " << (*it).second.index << endl;
#endif
   					fl_next = (*it).second.index >= array_size(hf.ht);
   	}


#ifdef DEBUG
   					if (ofs.is_open()) ofs << "fl_next = " << fl_next << endl;
#endif

   	return fl_next;
}

void TRotator_Ctrl::analise_folders_by_index()
{
#ifdef DEBUG
    	if (ofs.is_open()) ofs << "Analise_folders_by_index" << endl;

#endif

    	if (!sFolders.hfolders.size() )
    	{
#ifdef DEBUG
    		if (ofs.is_open()) ofs << "ROTATOR_G(hfolders.size() == 0" << endl;
#endif
    		return;
    	}

	    THFolders  hf;
		int	   what_to_do 	  = ADD_TO_PLAYLIST;
	    HFolders::iterator it = sFolders.hfolders.begin();
		bool to_next 		  = true;

		while (it != sFolders.hfolders.end())
    	{
    		hf 		= (*it).second;

    		if (hf.index >= zend_hash_num_elements(hf.ht))
    		{
    			to_next 	  = (*it).second.size < max_ht_size;

    			hf.index 	  = (*it).second.index = search_next_hash_index((*it).second.ht, START_INDEX TSRMLS_CC);
    			hf.statistics = ++(*it).second.statistics;
#ifdef DEBUG
        		if (ofs.is_open()) ofs << "Достигли конца папки " << (*it).first.c_str() << ". Установленный вес папки = " << (*it).second.statistics <<endl;
#endif
    		}

			char *key_folder = strdup((*it).first.c_str());
			what_to_do = find_record_in_playlist(hf, key_folder TSRMLS_CC);
   	    		free(key_folder);

   	    	(*it).second.index = search_next_hash_index((*it).second.ht, hf.index+1 TSRMLS_CC);

   	    	if (what_to_do == NEED_TO_SKIP_RECORD)
   	    	{
#ifdef DEBUG
   	    			if (ofs.is_open()) ofs << "NEED_TO_SKIP_RECORD. Continue!" << endl;
#endif
   	    			if ((*it).second.index < array_size(hf.ht)) continue;
   	    	}

			if (playlist_construction_completed()) 	break;

    		if ( ++it == sFolders.hfolders.end()) it = sFolders.hfolders.begin();

    		if (it == sFolders.hfolders.end() || to_next == false)
        	{
        		if (m_special_conditions && this->playlist_size_increase())
        		{
        			prepare();
        			it      = sFolders.hfolders.begin();
        			to_next = true;
        		}
        		else break;
        	}

    	}
}

void TRotator_Ctrl::analise_folders_by_order()
{
	int counter = sOrder.folders_order.size();

#ifdef DEBUG
    	if (ofs.is_open()) ofs << "Analise_folders_by_order" << endl;
#endif

    if (!sFolders.hfolders.size() ) return;

    Order::iterator ito = sOrder.folders_order.begin(),
    		 	 	it_next;

    HFolders::iterator it;
	
	set_start_folder = false;
	while(true)
	{
		if (set_start_folder) 
		{
		    ito = sOrder.folders_order.begin();
		    set_start_folder = false;
		}
#ifdef DEBUG
 		if (ofs.is_open()) ofs << "					sOrder.folders_order = " << (*ito).second << endl;
#endif
   		it = sFolders.hfolders.find((*ito).second/* Название папки */);

   		bool to_next_folder = true;
   		while (!global_analize(it, to_next_folder TSRMLS_CC));


    		it_next = sOrder.folders_order.upper_bound((*ito).first);

   		if (playlist_construction_completed()) break;

#ifdef DEBUG
 		if (ofs.is_open()) ofs << "(*it).second.size VS max_ht_size: " << (*it).second.size << " VS " << max_ht_size << endl;
#endif

    		if ( it_next != sOrder.folders_order.end()) ito = it_next;
    		else 	ito = sOrder.folders_order.begin();

    		if (ito == sOrder.folders_order.end() || to_next_folder == false /*дошла до конца папки с максимальной длиной */)
    		{
#ifdef DEBUG
 			if (ofs.is_open()) ofs << "m_special_conditions =  " << (int) m_special_conditions << endl;
#endif
    			if (m_special_conditions && this->playlist_size_increase())
    			{
    				prepare();
    				ito = sOrder.folders_order.begin();
    				if (ito == sOrder.folders_order.end()) break; /* папка order пуста */
    			}
    			else 	break;

    		}
	}	/* while */
}

zval *TRotator_Ctrl::create_playlist()

{

	if (zend_hash_num_elements(playlist)) zend_hash_clean(playlist);

	sFolders.m_previos_playlist_size = 0;
	sFolders.m_increase_counter		 = 0;

	zval tmp;
	ZVAL_UNDEF(&tmp);
	ZVAL_DUP(&tmp, &Array);

	if (array_size(HASH_OF(&tmp)) == 0 || m_error > EMPTY_ERROR)
	{
#ifdef DEBUG
 		if (ofs.is_open()) ofs << "EMPTY_ERROR" << endl;
#endif
		zval_dtor(&tmp);
		return &play_list;
	}

	if (m_order) analise_folders_by_order(TSRMLS_CC);
//        else analise_folders_by_index(TSRMLS_CC);

	zval_dtor(&tmp);

        m_playlist_construction_complete = true;

#ifdef DEBUG

    ofstream f;
//    f.open("/var/tmp/rotator/Playlist.txt", ofstream::out | ofstream::trunc);
    f.open("Playlist.txt", ofstream::out | ofstream::trunc);
    if (f.is_open())
    {
    	zval *data = NULL, tmp1, *singer, *title, *duration, *person;
    	zend_string *key = NULL;
    	unsigned long  num_index = 0;

    	for (	zend_hash_internal_pointer_reset(playlist);
    			zend_hash_has_more_elements(playlist) == SUCCESS;
    			zend_hash_move_forward(playlist)
    		)
    	{

    		if (zend_hash_get_current_key(playlist, &key, &num_index))
    		{
    			   data = zend_hash_get_current_data(playlist);

    			   if (Z_TYPE_P(data) == IS_ARRAY)
    			   {
    				   singer   =  find_data_in_node(Z_ARRVAL_P(data), (char*) DEFAULT_SINGER);
    				   title    =  find_data_in_node(Z_ARRVAL_P(data), (char*) DEFAULT_TITLE);
    				   duration =  find_data_in_node(Z_ARRVAL_P(data), (char*) DEFAULT_DURATION);

    				   char *s = strdup(duration ? Z_STRVAL_P(duration) : (char*)"No data");

    				   if (singer != NULL && title != NULL)
    					   f << num_index << "  Singer = " << Z_STRVAL_P(singer) << "  Title = " << Z_STRVAL_P(title) << "  Duration = " << s << endl;

    				   free(s); s = NULL;
    			   }
    		}

    	}

    	if (pTimer) f << "Playlist duration = " << pTimer->get_playlist_duration() << endl;

  		f.close();
    }

#endif

	return &play_list;
}

string  TRotator_Ctrl::zend_string_to_std_string(zend_string *str) 
{ 
	string s;
	s.assign(ZSTR_VAL(str), ZSTR_LEN(str));
	
	//char* s = (char*) malloc(ZSTR_LEN(string) + 1);
	//memcpy(s, ZSTR_VAL(string), ZSTR_LEN(string));
	//s[ZSTR_LEN(string)] = '\0';
	
	return s;
}

} /* namespace */
