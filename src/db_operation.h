#ifndef __DB_OPERATION_H__
#define __DB_OPERATION_H__
#include <pthread.h>
#include "db_operation.h"
#include "common.h"
#include "list.h"
#include "rbtree.h"
typedef struct _create_table_info create_table_info;
typedef struct _rec_attribute rec_attribute;
typedef struct _dt_pattern dt_pattern;
typedef struct _dt_record dt_record;
typedef struct _data_table_struct data_table_struct;
typedef struct _data_base_struct data_base_struct;
typedef struct _transactioncp transactioncp;
typedef struct _filelist filelist;
struct _filelist
{
		int count;
		int size;
		char* pfilename[100];
		filelist* next;
};
struct _create_table_info
{
		unsigned int tid;
		unsigned char* name;
		unsigned int attr_count;
		unsigned int key_num;
		rec_attribute* attr_info;
};
struct _rec_attribute
{
	unsigned int attr_type;
	unsigned int attr_size;//bytes
	char* attr_name;
	int key_attr;
};

struct _dt_pattern
{
	unsigned int attr_count;
	rec_attribute*    attr_info;
	//we also can store key attribute value at first.
	unsigned int key_num;//the number of key attribute within the record;
};
struct _dt_record
{
	rbnode rb_node;
	void* record_data;//buffer format based on attr_info and attr_count
};
struct _data_table_struct
{
	list_head list_node;
	dt_pattern* dt_pt;//the partten of a data table define the structure of record
	pthread_mutex_t lock_obj;
	//unsigned int dirtyflag;
	unsigned int tid;//table ID
	rbnode* rbroot;//
	unsigned int rec_count;
	unsigned int attr_count;
	int dirty;
};
struct _data_base_struct
{
		list_head* ptable_head;
		unsigned int tablecount;
//	data_table_struct* data_table_head;//make data_tables form a circular queue
//	data_table_struct** hash_data_table;//search a table pointer by ID
};

struct _transactioncp
{
	unsigned int operation;
	unsigned char* newrecord;
	unsigned char* oldrecord;
	unsigned int oldrecordlength;
	unsigned int newrecordlength;
	unsigned int IDdb;
	unsigned int IDtable;
};
int rebuilddb(data_base_struct* pdb_struct, char* directory);
void* dt_update_func(data_table_struct* data_table, key_type key_value, void* new_data_buffer);
int dt_insert_func(data_table_struct* data_table, dt_record* add_record);
dt_record* dt_search_rec(data_table_struct* data_table, key_type key_value);
data_table_struct* db_search_table(list_head* ptable_head, unsigned int IDdt);
int dt_delete_func(data_table_struct* data_table, dt_record* del_record);
int dt_addtable_func(list_head* phead, data_table_struct* dt_struct);
int init_dt_default(data_table_struct* ptable, create_table_info* ptableinfo);
data_base_struct* create_db_default();
int init_record_def(dt_record* new_record, void* data_buffer, int keyValue);
#endif
