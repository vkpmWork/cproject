extern "C" 
{
#include "php.h"
}
#include "php_ini.h"
#include "ext/standard/info.h"
#include "Zend/zend_hash.h"
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include "obj_timer.h"

//#define DEBUG
using namespace std;

namespace php_rotator
{

#define EMPTY_ERROR				0x00
#define EXECUTOR_ERROR   			0x01
#define TRACK_ERROR   				0x02
#define SHEDULE_ERROR   			0x04
#define INPUT_ARRAY_ERROR   			0x08
#define FOLDERS_ERROR   			0x10
#define RULES_ERROR				0x20

struct TRules
{
	int m_singer_limit;
	int m_track_limit;
	int m_return_interval;
	int m_max_rules;

};

typedef struct tHistory
{
	char *singer;
	char *title;
	int  statistics;
}THistory;


typedef map<int, THistory*> MHistory;
//====================================================================================

class TRotator_Ctrl
{
private:
	zval 		 play_list,
			 Array;

	HashTable 	 *playlist;
	bool 		  m_order,
				  m_schedule,
				  m_special_conditions;
//				  m_soft,
//				  m_soft_owner,
//				  m_random_selection;

	int			  max_tracks,
				  m_error;
	char		 *general_duration;
	char	         *track_duration;
	bool		  m_playlist_construction_complete;
	TTimer 		 *pTimer;
	int 		  max_ht_size;

#ifdef DEBUG
	ofstream ofs;
#endif

	struct THFolders
	{
		HashTable    *ht;
		unsigned long index;      /* индекс */
		bool		  bypass;     /* признак повторного обхода массива */
		int			  size;
		bool		  service;    /* признак сервисной папки, когда правила игнорируются */
		int			  statistics; /* текущий вес треков для папки */
	};

	typedef std::map<string, THFolders>     HFolders;
	typedef std::map<int, string>		Order;
	typedef std::map<int, Order>		Schedule;
	typedef std::map<string, bool>          Existing_folders;

	struct TParam
	{
		unsigned long index;
		int 	      length;
	};

	TParam        	Tracks,
			Service,
			Statistics; /* поле Вес */

	struct TSOrder
	{
		TParam		  param;
		Order  		  folders_order;
	} sOrder;

	struct	TFolders
	{
		TParam		  param;
		Existing_folders folders; /* контейнер непустых папок */
		HFolders	  hfolders;
		int 		  m_increase_counter;
		int 		  m_previos_playlist_size;
	} sFolders;

	TRules sRules; /* общие правила */

	typedef std::map<string, TRules> MRules; /* правила папок */

	struct TSHistory
	{
		TParam 		  param;
		long 		  last_init_index;
		HashTable        *hash_history;
	} sHistory;

	struct TSSchedule
	{
		TParam 		  param;
		Schedule	  schedule;
		Schedule::iterator  it;
	} sSchedule;


	MHistory	mHistory;
	MRules		mRules;
	bool	set_start_folder;
	inline 	void 	on_init(zval*);
	inline  bool 	is_equal(zval *str1, zval *str2);
	inline 	bool 	is_valid_type(zval *ex, zval *tr);
	inline  bool    playlist_construction_completed();	/* 258 */
	inline 	unsigned long search_next_hash_index(HashTable *ht, unsigned long index);
	void 	increase_max_ht_size(int value) {         
							if (value > max_ht_size)  
							{
#ifdef DEBUG
								if (ofs.is_open()) ofs <<  "value VS max_ht_size = " << value << " VS " << max_ht_size << endl;
#endif
								max_ht_size = value; 
#ifdef DEBUG
								if (ofs.is_open()) ofs <<  "max_ht_size = " <<  max_ht_size << endl;
#endif
							}	
						}

	void	get_node_hash_index();
	int	array_size(HashTable*);
	void	find_data(zval **data, char * argument);
	void	add_to_playlist(zval *singer, zval *track, zval *duration);
	void	add_to_playlist(zval *data, long statistics);
	zval*	get_node_ptr(HashTable* ht,    char *node_name);
	zval*	get_node_ptr(zval     *z_node, char *node_name);
	zval*	find_data_in_node(HashTable *ht, char *str);
	zval*	find_data_in_node(zval *z_node,  char *str);
	TRules 	search_rules(zval*, bool);
	void 	search_tracks (zval* , string);
	bool 	search_folders(zval *arr, bool);
	void  	search_history(zval *arr);
	inline  int calc_statistics_in_history(zval*);
	bool 	search_order(zval * arr);
	bool 	search_schedule(zval * arr);
	TRules 	get_current_rules(char *folder);
//	int  	find_record_in_history(HashTable *ht, zval *tmp_singer, zval *tmp_title, int m_rule_singer, int m_rule_track, int m_max_rule, int &);
	int  	find_record_in_history(zval *tmp_singer, zval *tmp_title, int m_rule_singer, int m_rule_track, int m_max_rule, int &);
	int    title_statistics_in_history(char *m_title, int m_rule_track, int &stat);
	int 	check_current_record(HashTable *ht, int m_ht_size, zval *tmp_singer, zval *tmp_title, int m_rule_singer, int m_rule_track, int m_max_rule, int &delta);
	void 	add_to_folder(HashTable *ht, zval *tmp_singer, zval *tmp_title, zval *tmp_duration);
	int  	find_record_in_playlist(THFolders  hf  /* element of folders */ , char *m_key_folder);
	bool 	global_analize(HFolders::iterator it, bool &);
	void 	analise_folders_by_index();
	void 	analise_folders_by_order();
	bool    create_schedule_order();
	inline  bool  set_special_conditions();
	inline  bool   playlist_size_increase();
//	inline  int  random_selection(int);
	string  zend_string_to_std_string(zend_string *str);
public:

	TRotator_Ctrl(zval *);
	TRotator_Ctrl(zval *, int);
	TRotator_Ctrl(zval *, char*);
	~TRotator_Ctrl();
	void     OnDestroy();
	zval 	*create_playlist();

	bool    ordered()			{ return m_order; } 			/* true - правило обхода папок существует */

	int  	get_max_tracks() 		{ return max_tracks; }	/* вернуть макс кол-во треков в плейслисте */
        void  	set_max_tracks(int m_tracks)	{ max_tracks = m_tracks; }/* установить макс кол-во треков в плейслисте */

	void	set_general_duration(char* str)	{ if (pTimer) pTimer->set_general_duration(str); } 	/* установить длительность плейлиста */
	long int get_general_duration() 	{ return pTimer != NULL ? pTimer->get_general_duration() : 0;}

	char   *default_track_duration()        { return track_duration; }
	void   	default_track_duration(char *str)
						{
							if (track_duration) free(track_duration);
							track_duration = strdup(str);
							if (pTimer) pTimer->set_default_track_duration(track_duration);
						}

	long int get_playlist_duration()        { return pTimer != NULL ? pTimer->get_playlist_duration() : 0;}

	long    size();
	bool    isempty();
	bool	iserror()			{ return (bool) m_error;}
	bool    prepare();
//	void    set_soft(bool v_soft)   		{ if (v_soft != m_soft) m_soft = m_soft_owner = v_soft; }
//	bool	get_soft()			    		{ return m_soft;  }
	int 	get_error()						{ return m_error; }
	int 	get_singer_interval()			{ return sRules.m_singer_limit;}
	int 	get_track_interval()			{ return sRules.m_track_limit;}
//	void 	set_random_selection(bool);		/* 1025 */

protected:

};

}



	


