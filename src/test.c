#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <pthread.h>
#include "db_operation.h"
#include <sys/time.h>
#include "log.h"
typedef struct _dt_record dt_record;
#define WAIT_MINUTE 1
int thread1_state = -1;// -1 exit 0 running or block
int thread2_state = -1;
int thread3_state = -1;

data_base_struct* pdb_struct = NULL;
int ditryTablecount = 0;
int tableCount = 0;

pthread_mutex_t unachieve_queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t achieve_queue_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t th2_ready = PTHREAD_COND_INITIALIZER;
pthread_cond_t th3_ready = PTHREAD_COND_INITIALIZER;

comlogbufqueue *unachievelogbufqueue;
comlogbufqueue *achievelogbufqueue;

int printLogrecord(logrecord* plogrec)
{
		int i = 0;
		if(plogrec == NULL)
		{
				return FAILURE;
		}
		for(i = 0; i < plogrec->newrecordlength; i++)
		{
				printf("%d",plogrec->newrecord[i]);
		}
		printf("\n");
		return SUCCESS;
}

int printLogcache(logcache* logbuf)
{
		void* ptemp = NULL;
		list* prec = NULL;
		if(logbuf == NULL)
		{
				return FAILURE;
		}
		prec = (list*)(logbuf->loghead);
		while(prec != NULL)
		{
				printf("transaction ID is %d\n", logbuf->transactionID);
				printLogrecord((logrecord*)prec);
				prec = prec->next;
		}
		return SUCCESS;
}
int printQueue(comlogbufqueue* pQueue)
{
		list* ptemp = NULL;
		if(pQueue == NULL)
		{
				return FAILURE;
		}
		ptemp = (list*)(pQueue->logbufhead);
		while(ptemp != NULL)
		{
				printLogcache((logcache*)ptemp);
				ptemp = ptemp->next;
		}
		return SUCCESS;
}

int restore(char* logPath, char* dbPath, comlogbufqueue* unachievelogbufqueue)
{
		int fd;
		int compIncreLog = 0;	
		unsigned int startCPOffset;
		if(logPath == NULL || dbPath == NULL)
		{
				printf("error: path is NULL\n");
				return FAILURE;
		}
		
		//when test, set startCPOffset equal to filesize
		//startCPOffset = get_file_size(logPath);	
		//int searchCheckpoint(int* startCPOffset)
		compIncreLog = searchCheckpoint(&startCPOffset);
		printf("the finally startCPOffset is %d",(get_file_size(logPath) - startCPOffset));
		fd = open(logPath, O_RDONLY);
		if(fd < 0)
		{
				printf("open file %s failure\n",logPath);
				return FAILURE;
		}

		if(compIncreLog == 1)	
		{
				//日志文件的增量部分完整
				printf("complete log file\n");
				system("cp -f ./dbbackupinc/* ./dbbackup/");
				system("rm ./dbbackupinc/*");
				//readlogbuffromdisk(fd, startCPOffset, unachievelogbufqueue);
				//return SUCCESS;
		}
		else if(compIncreLog == 0)
		{
				//日志文件的增量部分不完整，备份时发生系统崩溃
				printf("uncomplete log file\n");
				system("rm ./dbbackupinc/*");
				//readlogbuffromdisk(fd, startCPOffset, unachievelogbufqueue);
				//return SUCCESS;
		}
		else
		{
				printf("restore failure\n");
				return FAILURE;
		}
		//调用rebuild函数将磁盘上的镜像文件载入到系统内存
		/*now do not consider multi database situation and 
		 * */
		pdb_struct = create_db_default();
		if(rebuilddb(pdb_struct, "dbbackup/") != SUCCESS)
		{
				printf("rebuild db failure\n");
				return FAILURE;
		}
		readlogbuffromdisk(fd, startCPOffset, unachievelogbufqueue);
		return SUCCESS;
		//int readlogbuffromdisk(int fd, int startCPOffset, comlogbufqueue* unachievelogbufqueue)
		//out put unachievelogbufqueue
}

logrecord* initrecord(int index)
{
		logrecord* logrecbuf = NULL;
		logrecbuf = (logrecord*)malloc(sizeof(logrecord));
		(logrecbuf->link).pre = NULL;
		(logrecbuf->link).next = NULL;
		logrecbuf->IDdb = index;
		logrecbuf->IDtable = index;
		logrecbuf->recordKey = index;
		logrecbuf->oldrecordlength = 10;
		logrecbuf->newrecordlength = 10;
		logrecbuf->oldrecord = (unsigned char*)malloc(10);
		memset((void*)(logrecbuf->oldrecord),index,10);
		logrecbuf->newrecord = (unsigned char*)malloc(10);
		memset((void*)(logrecbuf->newrecord),index,10);
		
		return logrecbuf;
}
logcache* initlogrecordbuf(int index)
{
		logcache* logbuf = NULL;
		logbuf = (logcache*)malloc(sizeof(logcache));
		(logbuf->link).pre = NULL;
		(logbuf->link).next = NULL;
		(logbuf->achievelink).pre = NULL;
		(logbuf->achievelink).next = NULL;
		logbuf->transactionID = index;
		logbuf->logrecCount = 0;
		logbuf->loghead = NULL;
		logbuf->logtail = NULL;
		return logbuf;
}
void maketimeout(struct timespec *tsp, long minutes)
{
		struct timeval now;

		gettimeofday(&now,NULL);
		tsp->tv_sec = now.tv_sec;
		tsp->tv_nsec = now.tv_usec * 1000;

		tsp->tv_sec += minutes * 60;
}

void output(void* logbufhead,void* logbuftail)
{
		logcache* logbuf = NULL;
		logrecord* logrecbuf = NULL;
		void* logbuftemp = NULL;
		
		while((logbuftemp = dequeue_new(&logbufhead,&logbuftail)) != NULL)
		{
				logbuf = (logcache*)getstructpointer(logbuftemp);
				printf("logcache info\n");
				printf("transactionID: %d\n",logbuf->transactionID);
				printf("logrecCount: %d\n",logbuf->logrecCount);

				while((logrecbuf = (logrecord*)dequeue_new(GET_POINTER_LOG(logbuf))) != NULL)
				{
						printf("logrecord info\n");
						printf("%d\n",logrecbuf->IDdb);
				}
		}
}
int startCheckpoint(comlogbufqueue* pAchieveQueue)
{
		//将已经提交的日志全部写回到磁盘
		writelogbuftodisk(pAchieveQueue->logbufhead, pAchieveQueue->logbuftail,1);
		//写入startcp
		if(SUCCESS == backupdb(pdb_struct))
		{
				printf("backup success\n");
				return SUCCESS;
		} 
		return FAILURE;
}

/*
typedef struct _record
{
		list link;
		unsigned int db_operation_tp;
		unsigned int IDdb;
		unsigned int IDtable;
		unsigned int recordKey;
		unsigned char *oldrecord;
		unsigned char *newrecord;
		unsigned int oldrecordlength;
		unsigned int newrecordlength;
}logrecord;
*/
int changeMemDb(logcache* plogcache, data_base_struct* pdb_struct)
{
		list* ptemp = NULL;
		int findTable = 0;
		list_head* phead = NULL;
		void* oldbuffer = NULL;
		dt_record* record = NULL;
		if(plogcache == NULL)
		{
				return FAILURE;
		}
		ptemp = (list*)(plogcache->loghead);
		while(ptemp != NULL)
		{
				findTable = 0;
				phead = (list_head*)((pdb_struct->ptable_head)->next);
				while(phead != ((list_head*)(pdb_struct->ptable_head)))
				{
						if(((logrecord*)ptemp)->IDtable == ((data_table_struct*)phead)->tid)
						{
								findTable = 1;
								break;
						}
						phead = phead->next;
				}
				if(findTable == 0)
				{
						printf("changeMemDb error: don not find table %d\n",((logrecord*)ptemp)->IDtable);
						return FAILURE;
				}
				if(((data_table_struct*)phead)->dirty != 1)
				{
						ditryTablecount++;
				}
				switch(((logrecord*)ptemp)->db_operation_tp)
				{
						case DB_UPDATE_OP:
								
								oldbuffer = (int*)dt_update_func((data_table_struct*)phead, ((logrecord*)ptemp)->recordKey, ((logrecord*)ptemp)->newrecord);
								if(oldbuffer != NULL)
								{
										free(oldbuffer);
										oldbuffer = NULL;
								}
								break;
						case DB_DELETE_OP:
								record = dt_search_rec((data_table_struct*)phead, ((logrecord*)ptemp)->recordKey);
								if(FAILURE == dt_delete_func((data_table_struct*)phead, record))
								{
										printf("delete FAILURE\n");
								}
								break;
						case DB_INSERT_OP:
								/*create dt_record dada struct
								 * */
								if((record = create_record_def((void*)(((logrecord*)ptemp)->newrecord, ((logrecord*)ptemp)->recordKey))) == NULL)
								{
										printf("false occur when create record\n");
										return FAILURE;
								}
								if(dt_insert_func((data_table_struct*)phead, record) == FAILURE)
								{
										printf("add record FAILURE\n");
										return FAILURE;
								}
						default:
								break;
				}

				ptemp = ptemp->next;
		}
		return SUCCESS;
}
/*the entry of thread1,put new comming transaction logbuf to logbufqueue
 * */
void* thread1_function(void* arg)
{
		logcache* logbuf = NULL;
		logrecord* logrecbuf = NULL;
		int count = 0;
		int j = 0;
		int i = 0;
		int input = 0;
		/********initialize logbuf********/
		for(;;)
		{
		//		printf("input 0 to stop or input 1 to continue\n");
		//		scanf("%d",&input);
	//			if(input == 0)
				if(i == 10003)
				{
						thread1_state = -1;
						printf("thread1 exit normally\n");
						break;
				}
				logbuf = initlogrecordbuf(i);
				for(j = 0; j < 10; j++)
				{
						logrecbuf = initrecord(j);
						if(FAILURE == addlog(logrecbuf,logbuf))
						{
								perror("thread1 add log error\n");
								pthread_exit(NULL);
						}
				}
				/*****contact with input device to limite the speed****/

				/**start input**/
				printf("start lock and input\n");
				pthread_mutex_lock(&unachieve_queue_lock);
		//		if(FAILURE == enqueue_new(&(unachievelogbufqueue->logbufhead),&(unachievelogbufqueue->logbuftail),(void*)logbuf))
				if(FAILURE == enqueue_new(GET_POINTER(unachievelogbufqueue),(void*)logbuf))
				{
						pthread_mutex_unlock(&unachieve_queue_lock);
						perror("thread_1 enqueue FAILURE\n");
						pthread_exit(NULL);
				}
				pthread_mutex_lock(&unachievelogbufqueue->datalock);
				unachievelogbufqueue->count += 1;
				pthread_mutex_unlock(&unachievelogbufqueue->datalock);

				pthread_mutex_unlock(&unachieve_queue_lock);
				pthread_cond_signal(&th2_ready);
		/***there should judge whether the thread2 is blocked,if it do blocked then wake up it***/
				printf("one turn\n");
				i++;
		}
		pthread_exit(NULL);
}
logcache* th2_get_unachieve()
{
		logcache* logbuf = NULL;
		pthread_mutex_lock(&unachieve_queue_lock);
	//	logbuf = (logcache*)dequeue_new(&(unachievelogbufqueue->logbufhead),&(unachievelogbufqueue->logbuftail));
		logbuf = (logcache*)dequeue_new(GET_POINTER(unachievelogbufqueue));

		pthread_mutex_lock(&unachievelogbufqueue->datalock);
		unachievelogbufqueue->count -= 1;
		pthread_mutex_unlock(&unachievelogbufqueue->datalock);

		pthread_mutex_unlock(&unachieve_queue_lock);
		return logbuf;
}
void th2_inpto_achieve(logcache* logbuf)
{
		pthread_mutex_lock(&achieve_queue_lock);
	//	if(FAILURE == enqueue_new(&(achievelogbufqueue->logbufhead),&(achievelogbufqueue->logbuftail),getlinkpointer((void*)logbuf)))
		if(FAILURE == enqueue_new(GET_POINTER(achievelogbufqueue),getlinkpointer((void*)logbuf)))
		{
				pthread_mutex_unlock(&achieve_queue_lock);
				perror("thread2 input buffer into achievelogbufqueue FAILURE\n");
				pthread_exit(NULL);
		}
		pthread_mutex_lock(&achievelogbufqueue->datalock);
		achievelogbufqueue->count += 1;
		/**????????????***/
		if(achievelogbufqueue->count == 10)
		{
				pthread_cond_signal(&th3_ready);
		}
		/**????????????***/
		pthread_mutex_unlock(&achievelogbufqueue->datalock);
		pthread_mutex_unlock(&achieve_queue_lock);

}
/*the entry of thread2,get transaction logbuf from logbufqueue and change 
 *the content of memory database according the log info stored in the buffer
 * then move the buffer to achievelogbufqueue*/
void* thread2_function(void* arg)
{
		/***first check if there have logbuf in logbufqueue,if not then block,otherwise proceed***/
		logcache* logbuf = NULL;
		double drityRatio = 0;
		struct timespec tout;
		for(;;)
		{
				maketimeout(&tout, WAIT_MINUTE);
				pthread_mutex_lock(&unachievelogbufqueue->datalock);
				while(unachievelogbufqueue->count == 0)
				{
						if(ETIMEDOUT== pthread_cond_timedwait(&th2_ready,&unachievelogbufqueue->datalock,&tout))
						{
								/*do something ???*/
								/*remove all buffer in unachieve buffer queue to achievebuffer
								 * queue*/	
								/*before input buffer to achieve buffer queue we should change
								 * memory database by buffer log record*/
								/*there is no competition when access unachievebufferqueue,but 
								 * competition still exit when access achievebufferqueue*/
								if(-1 == thread1_state)
								{
										pthread_mutex_unlock(&unachievelogbufqueue->datalock);
										while(unachievelogbufqueue->count > 0)
										{
											//	logbuf = (logcache*)dequeue_new(&(unachievelogbufqueue->logbufhead),&(unachievelogbufqueue->logbuftail));
												logbuf = (logcache*)dequeue_new(GET_POINTER(unachievelogbufqueue));
												/*change memory database*/
//int changeMemDb(logcache* plogcache, data_base_struct* pdb_struct)
												
												changeMemDb(logbuf, pdb_struct);
												unachievelogbufqueue->count -= 1;
												th2_inpto_achieve(logbuf);
										}
										printf("thread2 exit SUCCESS\n");
										thread2_state = -1;
										pthread_exit(NULL);
								}
						}
				}
				pthread_mutex_unlock(&unachievelogbufqueue->datalock);
				/**remove and input**/
				logbuf = th2_get_unachieve();
				/*change memory database*/
				changeMemDb(logbuf, pdb_struct);

				/*checkpoint*/
				drityRatio = (double)ditryTablecount / (double)(pdb_struct->ditryTablecount);
				if(drityRatio >= 0.3)
				{
						//int startCheckpoint(comlogbufqueue* pAchieveQueue)
						startCheckpoint(achievelogbufqueue);
				}

				/*after commit changes to memory database*/
				th2_inpto_achieve((void*)logbuf);
		}
}
/*the entry of thread3,get transaction logbuf from achievelogbufqueue
 * and write it to hard disk then release the buffer memory space*/
/*considering the low speed of disk IO thread3 first pick off the 
 * list from achievelogbufqueue then write the list to disk without
 * lock the list*/
/*here is a question. if the achievelogbufque have more than one 
 * but less than five logbuffer and thread3 has blocked 10 minutes
 * or more than 10 minutes,how can we deal with this situation*/
/*the solution for above problem is to use pthread_cond_timedwait()
 * rather than pthread_cond_wait() function to block the thread3
 *then the thread3 can periodically check whether there are some 
 *logbuffer in achievelogbufqueue and whether the thread1 or thread2
 *is exited*/
void* thread3_function(void* arg)
{
		int count = 0;
		void* newhead = NULL;
		void* newtail = NULL;
		void* logbuftemp = NULL;
		struct timespec tout;
		for(;;)
		{
				maketimeout(&tout, WAIT_MINUTE);
				pthread_mutex_lock(&achievelogbufqueue->datalock);
				while(achievelogbufqueue->count < 10)
				{
						if(ETIMEDOUT == pthread_cond_timedwait(&th3_ready,&achievelogbufqueue->datalock,&tout))
						{
								/*do something*/
								if(thread2_state == -1)
								{
										printf("thread3 find thread2 is exit\n");
										writelogbuftodisk(achievelogbufqueue->logbufhead,achievelogbufqueue->logbuftail,0);
										achievelogbufqueue->count = 0;
										achievelogbufqueue->logbufhead = NULL;
										achievelogbufqueue->logbuftail = NULL;
										thread3_state = -1;
										pthread_exit(NULL);
								}
						}
				}
				pthread_mutex_unlock(&achievelogbufqueue->datalock);

				newhead = achievelogbufqueue->logbufhead;
				newtail = achievelogbufqueue->logbuftail;
				pthread_mutex_lock(&achieve_queue_lock);
				achievelogbufqueue->logbufhead = NULL;	
				pthread_mutex_lock(&achievelogbufqueue->datalock);
				achievelogbufqueue->count = 0;
				pthread_mutex_unlock(&achievelogbufqueue->datalock);
				pthread_mutex_unlock(&achieve_queue_lock);

				/***here write to disk without lock****/
				printf("begin write to disk,thread2_state is %d\n",thread2_state);
				writelogbuftodisk(newhead,newtail,0);
		}
}
comlogbufqueue* initcomlogbufqueue()
{
		comlogbufqueue* bufqueue = (comlogbufqueue*)malloc(sizeof(comlogbufqueue));
		bufqueue = (comlogbufqueue*)malloc(sizeof(comlogbufqueue));
		bufqueue->count = 0;
		bufqueue->logbufhead = NULL;
		bufqueue->logbuftail = NULL;
		if(0 != pthread_mutex_init(&bufqueue->datalock, NULL))
		{
				free(bufqueue);
				printf("init datalock FAILURE\n");
				return NULL;
		}
		return bufqueue;
}

int main()
{
		int res;
		void* thread_result;
		pthread_t thread_1;
		pthread_t thread_2;
		pthread_t thread_3;
		if(NULL == (unachievelogbufqueue = initcomlogbufqueue()) || NULL == (achievelogbufqueue = initcomlogbufqueue()))
		{
				return FAILURE;
		}
#if 1
		if(FAILURE == restore("log.log", "ll", unachievelogbufqueue))
		{
				printf("restore error\n");
				return 0;
		}
		printQueue(unachievelogbufqueue);
#endif
#if 1
		res = pthread_create(&thread_1, NULL, thread1_function, NULL);
		if(res != 0)
		{
				perror("Thread_1 creation failed\n");
				exit(EXIT_FAILURE);
		}
		thread1_state = 0;

		res = pthread_create(&thread_2, NULL, thread2_function, NULL);
		if(res != 0)
		{
				perror("Thread_2 creation failed\n");
				exit(EXIT_FAILURE);
		}
		thread2_state = 0;

		res = pthread_create(&thread_3, NULL, thread3_function, NULL);
		if(res != 0)
		{
				perror("Thread_3 creation failed\n");
				exit(EXIT_FAILURE);
		}
		thread3_state = 0;

		res = pthread_join(thread_3, &thread_result);
		if(res != 0)
		{
				perror("Thread join failed\n");
				exit(EXIT_FAILURE);
		}

		printf("unachieve log buffer output...\n");
		output(unachievelogbufqueue->logbufhead, unachievelogbufqueue->logbuftail);	
		printf("achieve log buffer output...\n");
		output(achievelogbufqueue->logbufhead, achievelogbufqueue->logbuftail);
#endif
		return 0;
}
