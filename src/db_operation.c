#include <stdio.h>
#include <assert.h>
#include "db_operation.h"
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <dirent.h>
int dt_insert_func(data_table_struct* data_table, dt_record* add_record)
{
		assert(data_table != NULL);
		if(FAILURE == tree_insert(&(data_table->rbroot), (rbnode*)add_record))
		{
				return FAILURE;
		}
		data_table->rec_count += 1;
}
int dt_addtable_func(list_head* phead, data_table_struct* dt_struct)
{
		if(phead == NULL || dt_struct == NULL)
		{
				return FAILURE;
		}
		list_add(phead, (list_head*)dt_struct);
		return SUCCESS;
}


int dt_delete_func(data_table_struct* data_table, dt_record* del_record)
{
		dt_record* d_record = NULL;
		if(NULL==(d_record = (dt_record*)tree_delete(&(data_table->rbroot),(rbnode*)del_record)))
		{
				return FAILURE;
		}
		else
		{
				data_table->rec_count -= 1;
				return SUCCESS;
		}
}

dt_record* dt_search_rec(data_table_struct* data_table, key_type key_value)
{
		dt_record* record = NULL;
		assert(data_table->rbroot);
		assert(lchild(data_table->rbroot) && rchild(data_table->rbroot));
		record = (dt_record*)tree_search(data_table->rbroot, key_value);
		return record;
}
int init_record_def(dt_record* new_record, void* data_buffer, int keyValue)
{
		if(new_record == NULL || data_buffer == NULL)
		{
				return FAILURE;
		}
		new_record->record_data = data_buffer;
		key((rbnode*)(new_record)) =keyValue;
		color((rbnode*)new_record) = r_color;
		parent((rbnode*)new_record) = NULL;
		lchild((rbnode*)new_record) = NULL;
		rchild((rbnode*)new_record) = NULL;
		return SUCCESS;
}

inline void init_attinfo(rec_attribute* rec_attr_info, int count)
{
		assert(count > 1 && rec_attr_info != NULL);
		int i = 0;
		rec_attribute* pattr = NULL;
		pattr = rec_attr_info;
		for(;i < count; i++)
		{
				pattr->attr_type = DB_INTEGER_TP;
				pattr->attr_size = DB_INTEGER_SIZE;
				pattr->attr_name = "attr";
				pattr->key_attr  = 0;
				pattr++;
		}
}
unsigned char* formatpattern(unsigned char* buffer, dt_pattern* pdt_pattern)
{
		unsigned char* buftemp = buffer;
		rec_attribute* attinfo = pdt_pattern->attr_info;
		int i = 0;
		int attcount = pdt_pattern->attr_count;
		*((int*)buftemp) = pdt_pattern->attr_count;//attr_count
		buftemp += 4;
		*((int*)buftemp) = pdt_pattern->key_num;//key_num
		buftemp += 4;
		for(i = 0; i < attcount; i++)
		{
				*((int*)buftemp) = attinfo[i].attr_type;
				buftemp += 4;
				*((int*)buftemp) = attinfo[i].attr_size;
				buftemp += 4;
				*((int*)buftemp) = strlen(attinfo[i].attr_name);
				buftemp += 4;
				memcpy(buftemp,attinfo[i].attr_name,strlen(attinfo[i].attr_name));
				buftemp += strlen(attinfo[i].attr_name);
		}
		return buftemp;
}
void preorder(unsigned char** buffer, rbnode* rbroot, int recsize)
{
		dt_record* precord = NULL;
		if(rbroot == NULL)
				return;
		precord = (dt_record*)rbroot;
		memcpy((*buffer), precord->record_data, recsize);
		(*buffer) += recsize;	
		preorder(buffer, lchild(rbroot), recsize);
		preorder(buffer, rchild(rbroot), recsize);
}
int formattable(unsigned char* buffer, data_table_struct* pdt_struct, int recsize)
{
		//preorder traversal rbtree
		preorder(&buffer, pdt_struct->rbroot, recsize);
}

int calculatebuffersize(data_table_struct* pdt_struct, int* precsize)
{
		int countofbytes = 0;
		int recsize      = 0;
		int attcount = pdt_struct->attr_count;
		int i = 0;
		rec_attribute* attinfo = pdt_struct->dt_pt->attr_info;
		countofbytes += 4;//attcount
		countofbytes += 4;//keynum
		for(i = 0; i < attcount; i++)
		{
				recsize += attinfo[i].attr_size;
				countofbytes += strlen(attinfo[i].attr_name);
				countofbytes += 4;//strlen(attinfo[i].attr_name)
				countofbytes += 4;//attr_type
				countofbytes += 4;//attr_size
				//countofbytes += 4;//key_attr
		}
		countofbytes += recsize * pdt_struct->rec_count;
		*precsize = recsize;
		return countofbytes;
}
unsigned char* backuptable(data_table_struct* pdt_struct, int* pbufsize)
{
		unsigned char* recbuf = NULL;
		int recsize = 0;
		int buffersize = calculatebuffersize(pdt_struct, &recsize);
		printf("buffer size is %d\n",buffersize);
		unsigned char* buffer = (unsigned char*)malloc(buffersize + 4);
		*((int*)buffer) = buffersize + 4;
		recbuf = formatpattern(buffer + 4, pdt_struct->dt_pt);
		formattable(recbuf, pdt_struct, recsize);
		*pbufsize = buffersize + 4;
		return buffer;
}
//此处简化处理，假设一个tableID最多有10位
void myuitoa(char* str, int length, unsigned int num)
{
		int temp;
		int i = length - 1;
		str[i--] = '\0';
		while(num > 0)
		{
				temp = num % 10;
				num  = num / 10;
				str[i--] = '0' + temp;
		}
		while(i >= 0)
		{
				str[i--] = '0';
		}
}
int do_backup_io(unsigned char* buffer, int bufsize,unsigned int tableID)
{
		char* dir = "dbbackupinc/";
		int dirlength = strlen(dir);
		char* name = (char*)malloc(dirlength + sizeof(char) * 11);
		char* namenew = (char*)malloc(dirlength + sizeof(char) * 13);
		unsigned char* cpBuffer = (unsigned char*)malloc(16);
		memcpy(name,dir,dirlength);
		myuitoa(name + dirlength, 11, tableID);

		int fd1;
		//尝试着打开，主要为了检查是否已经存在该table的备份文件
		fd1 = open(name,O_WRONLY|O_APPEND);
		if(fd1 < 0)
		{
				fd1 = open(name,O_CREAT|O_WRONLY|O_APPEND);
				if(fd1 < 0)
				{
						printf("open file %s failure\n",name);
						return FAILURE;
				}
		}
		else
		{
				close(fd1);
				strcpy(namenew,name);
				namenew[dirlength + 11] = '-';
				namenew[dirlength + 12] = '1';
				namenew[dirlength + 13] = '\0';
				fd1 = open(namenew,O_CREAT|O_WRONLY|O_APPEND);
				if(fd1 < 0)
				{
						printf("open file %s failure\n", namenew);
						return FAILURE;
				}
		}
		write(fd1,buffer,bufsize);
		close(fd1);
		free(cpBuffer);
		free(name);
		free(namenew);
}
inline void initfilelist(filelist* pfilelist)
{
		if(pfilelist == NULL)
				perror("filelist is NULL\n");
		pfilelist->size = 100;
		pfilelist->count = 0;
		pfilelist->next = NULL;
}
filelist* getfilelist(char* directory, int* filecount)
{
		//先分配指针数组的长度为100，不够后再添加
		filelist* pfilelisthead = (filelist*)malloc(sizeof(filelist));
		filelist* pfilelisttemp = NULL;
		initfilelist(pfilelisthead);
		int index = 0;
		struct dirent* entry;
		DIR *dp;
		dp = opendir(directory);
		if(dp == NULL)
		{
				perror("opendir\n");
				return NULL;
		}
		while((entry = readdir(dp)))
		{
				if(pfilelisthead->count == 100)
				{
						pfilelisttemp = (filelist*)malloc(sizeof(filelist));
						initfilelist(pfilelisttemp);
						pfilelisttemp->next = pfilelisthead;
						pfilelisthead = pfilelisttemp;
						index = 0;
				}
				pfilelisthead->pfilename[index] = (char*)malloc(strlen(entry->d_name) + 1);
				strcpy(pfilelisthead->pfilename[index], entry->d_name);
				pfilelisthead->count ++;
				index++;
		}
}
int rebuilddt(data_table_struct* pdt_struct, char* path)
{
		assert(pdt_struct != NULL);
		int i = 0;
		int recsize = 0;
		int fd = open(path, O_RDONLY);
		unsigned char* buffer = NULL;
		unsigned char* bufferend = NULL;
		int buffersize = 0;
		int bytes_read = 0;
		if(fd < 0)
		{
				printf("open file %s to read failure\n",path);
				//perror("open file %s to read failure\n",path);
				return FAILURE;
		}
		//此处未检查是否出错
		bytes_read = read(fd, &buffersize, 4);
		//buffersize = tablesize + sizeof(buffersize)
		buffer = (unsigned char*)malloc(buffersize);
		//正常情况下会读到文件末尾
		bytes_read = read(fd, buffer, buffersize);
		if(bytes_read == -1)
		{
				printf("read file %s failure\n",path);
				return FAILURE;
		}

		bufferend = buffer + bytes_read;

		//do analyse and create data_table
		pdt_struct->dt_pt = (dt_pattern*)malloc(sizeof(dt_pattern));	
		pdt_struct->dt_pt->attr_count = *((int*)buffer);
		pdt_struct->dt_pt->key_num = *((int*)(buffer + 4));
		pdt_struct->dt_pt->attr_info = (rec_attribute*)malloc(sizeof(rec_attribute) * pdt_struct->dt_pt->attr_count);
		//此处做了一个假设，即该读入的备份文件是完整的，若是不完整的，此处有错
		buffer += 8;
		for(i = 0; i < pdt_struct->dt_pt->attr_count; i++)
		{
				(pdt_struct->dt_pt->attr_info[i]).attr_type = *((int*)buffer);
				buffer += 4;
				(pdt_struct->dt_pt->attr_info[i]).attr_size = *((int*)buffer);
				recsize += *((int*)buffer);
				buffer += 4;
				(pdt_struct->dt_pt->attr_info[i]).attr_name = (char*)malloc((*((int*)buffer)) + 1);
				memcpy((pdt_struct->dt_pt->attr_info[i]).attr_name, buffer + 4, *((int*)buffer));
				(pdt_struct->dt_pt->attr_info[i]).attr_name[*((int*)buffer)] = '\0';
				buffer = buffer + 4 + *((int*)buffer);
		}
		//create record
		while(buffer < bufferend)
		{
				dt_record* pdt_record = (dt_record*)malloc(sizeof(dt_record));
				pdt_record->record_data = (unsigned char*)malloc(recsize);
				memcpy(pdt_record->record_data, buffer, recsize);

				key((rbnode*)(pdt_record)) =*((int*)(buffer + 4 * pdt_struct->dt_pt->key_num));
				color((rbnode*)pdt_record) = r_color;
				parent((rbnode*)pdt_record) = NULL;
				lchild((rbnode*)pdt_record) = NULL;
				rchild((rbnode*)pdt_record) = NULL;
				if(dt_insert_func(pdt_struct, pdt_record) == FAILURE)	
				{
						return FAILURE;
				}
				buffer += recsize;
		}
		return SUCCESS;
}

int rebuilddb(data_base_struct* pdb_struct, char* directory)
{
		char *path = (char*)malloc(strlen(directory) + 1 + 13);
		int tableId = 0;
		char *pathtemp = path;
		data_table_struct* pdt_struct = NULL;
		if(pdb_struct == NULL)
				return FAILURE;
		struct dirent *entry;
		DIR *dp;

		dp = opendir(directory);
		if (dp == NULL) {
				perror("opendir");
				return FAILURE;
		}

		strcpy(pathtemp, directory);
		pathtemp += strlen(directory);
		while((entry = readdir(dp)) != NULL)
		{
			//	puts(entry->d_name);
				//memcpy(pathtemp,directory,strlen(directory));	
				//*pathtemp = '/';
				//pathtemp += 1;
				//memcpy((void*)pathtemp,(void*)(entry->d_name),strlen(entry->d_name) + 1);
				if(*((char*)(entry->d_name)) != '0')
				{
						continue;
				}
				strcpy(pathtemp, entry->d_name);
				pdt_struct = (data_table_struct*)malloc(sizeof(data_table_struct));
				tableId = atoi(entry->d_name);
				pdt_struct->tid = tableId;
				if(FAILURE == rebuilddt(pdt_struct, path))
				{
						perror("rebuild table error\n");
						return FAILURE;
				}
				if(SUCCESS == dt_addtable_func(pdb_struct->ptable_head, pdt_struct))
				{
						pdb_struct->tablecount += 1;
				}
		}

		closedir(dp);
												
		return SUCCESS;
}
int backupdb(data_base_struct* pdb_struct)
{
		//first write log to file
		unsigned char* ptablebuf = NULL;
		int bufsize = 0;
		data_table_struct* pdt_struct = NULL;
//		pdt_struct = (data_base_struct*)((pdb_struct->ptable_head)->next);
		list_head* phead = (list_head*)((pdb_struct->ptable_head)->next);
		while(phead != ((list_head*)(pdb_struct->ptable_head)))
		{
				assert(phead != NULL);
				pdt_struct = (data_table_struct*)phead;
				//lock
				if(pdt_struct->dirty)
				{
						ptablebuf = backuptable(pdt_struct, &bufsize);
				}
				//unlock
				do_backup_io(ptablebuf, bufsize, pdt_struct->tid);
				phead = phead->next;
		}
		return SUCCESS;
}

int init_dt_default(data_table_struct* ptable, create_table_info* ptableinfo)
{
		if(ptable == NULL || ptableinfo == NULL)
		{
				return FAILURE;
		}
		INIT_LIST_HEAD((list_head*)ptable);
		ptable->attr_count = ptableinfo->attr_count;
		ptable->dirty = 0;
		pthread_mutex_init(&(ptable->lock_obj), NULL);
		//ptable->lock_obj = PTHREAD_MUTEX_INITIALIZER;
		ptable->dt_pt = create_dt_pattern(ptableinfo);
		ptable->tid = ptableinfo->tid;
		ptable->rbroot = NULL;
		ptable->rec_count = 0;
		return SUCCESS;
}
//return old_buffer,do not forgot to release the buffer
void* dt_update_func(data_table_struct* data_table, key_type key_value, void* new_data_buffer)
{
		dt_record* targ_record = dt_search_rec(data_table, key_value);
		unsigned char* oldbuffer = NULL;
		if(targ_record == NULL)
		{
				printf("updata FAILURE, can not find target record\n");
				return NULL;
		}
		oldbuffer = (unsigned char*)(targ_record->record_data);
		targ_record->record_data = new_data_buffer;
		return oldbuffer;
}
data_table_struct* db_search_table(list_head* ptable_head, unsigned int IDdt)
{
		list_head* head = (list_head*)(ptable_head->next);
		while(head != (list_head*)ptable_head)
		{
				if(((data_table_struct*)head)->tid = IDdt)
				{
						return (data_table_struct*)head;
				}
				head = head->next;
		}
}
