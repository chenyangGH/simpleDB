#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "db_operation.h"
inline rec_attribute* create_attinfo(int count)
{
		assert(count > 1);
		rec_attribute* rec_attr_info = (rec_attribute*)malloc(sizeof(rec_attribute) * count);
		init_attinfo(rec_attr_info, count);
		return rec_attr_info;
}
inline dt_pattern* create_dt_pattern(create_table_info* ptableinfo)
{
		dt_pattern* ppattern = (dt_pattern*)malloc(sizeof(dt_pattern));
		ppattern->attr_count = ptableinfo->attr_count;
		ppattern->attr_info = ptableinfo->attr_info;
		ppattern->key_num = ptableinfo->key_num;
		return ppattern;
}
data_table_struct* create_dt_default(create_table_info* ptableinfo)
{
		data_table_struct* ptable = (data_table_struct*)malloc(sizeof(data_table_struct));
		if(FAILURE == init_dt_default(ptable, ptableinfo))
		{
				return NULL;
		}
		return ptable;
}
dt_record* create_record_def(void* data_buffer, int keyValue)
{
		srand(time(NULL));
		dt_record* new_record = (dt_record*)malloc(sizeof(dt_record));
#if 0
		data_buffer = (void*)malloc(sizeof(int) * 4);
		*((int*)data_buffer) = rand() % 100;
		*((int*)(data_buffer + 4)) = 111;
		*((int*)(data_buffer + 8)) = 111;
		*((int*)(data_buffer + 12)) = 111;
#endif
		if(FAILURE == init_record_def(new_record, data_buffer, keyValue))	
		{
				free(new_record);
				return NULL;
		}
		return new_record;
}
data_base_struct* create_db_default()
{
		data_base_struct* db_node = (data_base_struct*)malloc(sizeof(data_base_struct));
		db_node->ptable_head = (list_head*)malloc(sizeof(list_head));
		INIT_LIST_HEAD(db_node->ptable_head);
		db_node->tablecount = 0;
		return db_node;
}

data_base_struct* init_data_base(int count)
{
		int i = 0;
		data_base_struct* pdb_struct = create_db_default();
		create_table_info* pinfo = NULL;
		data_table_struct* ptable = NULL;
		for(;i < count; i++)
		{
				pinfo = (create_table_info*)malloc(sizeof(create_table_info));
				pinfo->tid = i;
				pinfo->name = "datatable";
				pinfo->attr_count = 4;
				pinfo->key_num = 0;
				pinfo->attr_info = create_attinfo(4);

				ptable = create_dt_default(pinfo);
				if(SUCCESS == dt_addtable_func(pdb_struct->ptable_head, ptable))
				{
						pdb_struct->tablecount += 1;
				}
		}
		return pdb_struct;
}
//为了简便，用默认的int输出
int printrecord(dt_record* precord, dt_pattern* ppattern)
{
		if(precord == NULL || ppattern == NULL)
		{
				return FAILURE;
		}
		rec_attribute* pattr_info = ppattern->attr_info;
		if(pattr_info == NULL)
		{
				printf("");
		}
		unsigned char* prec_data  = precord->record_data;
		int i = 0;
		for(i = 0; i < ppattern->attr_count; i++)
		{
				printf("%d ",*((int*)prec_data));
				prec_data += 4;
		}
		printf("\n");
}
#if 0
int main()
{
		int i = 0;
		int count = 0;
		dt_record* record = NULL;
		int* buffer = NULL;
		void* oldbuffer = NULL;
		data_base_struct* pdb_struct = NULL;
		list_head* phead = NULL;
/*		pdb_struct = init_data_base(100);
		phead = (list_head*)((pdb_struct->ptable_head)->next);
		
		while(phead != ((list_head*)(pdb_struct->ptable_head)))
		{
				assert(phead != NULL);
				for(i = 0; i < 100; i ++)
				{
						buffer = (int*)malloc(sizeof(int) * 4);
						buffer[0] = i;
						buffer[1] = i;
						buffer[2] = i;
						buffer[3] = i;
						if((record = create_record_def((void*)buffer), *(int*)buffer) == NULL)
						{
								printf("false occur when create record\n");
						}
						if(dt_insert_func((data_table_struct*)phead, record) == FAILURE)
						{
								printf("add record FAILURE\n");
						}
				}
	 			for(i = 0; i < 100; i++)
				{
						if(i % 5 == 0)
						{
								record = dt_search_rec((data_table_struct*)phead, i);
								if(FAILURE == dt_delete_func((data_table_struct*)phead, record))
								{
										printf("delete FAILURE\n");
								}
								else
								{
										free(record->record_data);
										free(record);
										record = NULL;
								}
						}
				}
				phead = phead->next;
		}
		*/
		pdb_struct = create_db_default();
		if(rebuilddb(pdb_struct, "dbbackup/") != SUCCESS)
		{
				printf("rebuild db failure\n");
				return 0;
		}
		record = NULL;
		phead = (list_head*)((pdb_struct->ptable_head)->next);
		while(phead != ((list_head*)(pdb_struct->ptable_head)))
		{
				((data_table_struct*)phead)->dirty = 1;
				for(i = 0; i < 100; i++)
				{
						if(i % 6 == 0)
						{
								buffer = (int*)malloc(sizeof(int) * 4);
								buffer[0] = i;
								buffer[1] = i * 100;
								buffer[2] = i * 100;
								buffer[3] = i * 100;

								oldbuffer = (int*)dt_update_func((data_table_struct*)phead, i, buffer);
								if(oldbuffer != NULL)
								{
										free(oldbuffer);
										oldbuffer = NULL;
								}
						}
				}
				

				for(i = 0; i < 100; i++)
				{
					//	if(i % 6 == 0)
					//	{
								record = dt_search_rec((data_table_struct*)phead, i);
								if(record != NULL)
								{
										printrecord(record, ((data_table_struct*)phead)->dt_pt);
								}
					//	}
				}



				phead = phead->next;
		}
	/*	
		if(SUCCESS == backupdb(pdb_struct))
		{
				printf("backup success\n");
				int fd;
				unsigned char* cpBuffer = (unsigned char*)malloc(16);
				fd = open("log.log",O_CREAT|O_WRONLY|O_APPEND);
				if(fd < 0)
				{
						printf("open file FAILURE\n");
						return FAILURE;
				};
				*((int*)cpBuffer) = strlen("EndCP") + 2 + 4;
				strcpy((char*)(cpBuffer + 4), "EndCP");
				*((char*)(cpBuffer + 10)) = '\n';
				close(fd);
				free(cpBuffer);
		}
		else
		{
				return 0;
		}   */
		return 0;
}
#endif
