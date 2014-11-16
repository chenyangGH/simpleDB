#include<string.h>
#include<stdio.h>
#include<fcntl.h>
#include<malloc.h>
#include<assert.h>
#include<sys/stat.h>
#include"log.h"

#define INITLOGRECORD(pLog) do{						\
	(pLog)->db_operation_tp = DB_UPDATE_OP;			\
	(pLog)->IDdb = -1;								\
	(pLog)->IDtable = -1;							\
	(pLog)->recordKey = -1;							\
	(pLog)->oldrecord = NULL;						\
	(pLog)->newrecord = NULL;						\
	(pLog)->oldrecordlength = 0;					\
	(pLog)->newrecordlength = 0;					\
	((pLog)->link).pre = NULL;						\
	((pLog)->link).next = NULL;						\
}while(0)

int addlog(logrecord* logrecbuf, logcache* logbuf)
{
		logrecord *logtemp = NULL;
		if(logrecbuf == NULL || logbuf == NULL)
		{
				return FAILURE;
		}
		if(enqueue_new(GET_POINTER_LOG(logbuf),(void*)logrecbuf) == FAILURE)
		{
				return FAILURE;
		}
		(logbuf->logrecCount) += 1;
		return SUCCESS;

}
/*
int addlogbuf(logcache* logbuf, logcache** logbufqueue)
{
		if(logbuf == NULL)
		{
				return FAILURE;
		}
		if(enqueue((void**)logbufqueue,(void*)logbuf) == FAILURE)
		{
				return FAILURE;
		}
}
*/
//使用时保证传递进来的数据结构的link->pre == NULL 且 link->next == NULL不然程序可能出错
int enqueue(void** head, void* link)
{
		list* temp = NULL;
		list* linktemp = (list*)link;
		if(link == NULL)
		{
				return FAILURE;
		}
		if((*head) == NULL)
		{
				(*head) = link;
				return SUCCESS;
		}
		temp = (list*)(*head);
		while(temp->next != NULL)
		{
				temp = temp->next;
		}
		temp->next = linktemp;
		linktemp->pre = temp;
		linktemp->next = NULL;
		return SUCCESS;
}
//link->pre == NULL && link->next == NULL
int enqueue_new(void** head, void** tail, void* link)
{
		assert(((list*)link)->pre == NULL && ((list*)link)->next == NULL);

		if(link == NULL)
		{
				return FAILURE;
		}
		//use || or && ???
		if((*head) == NULL || (*tail) == NULL)
		{
				(*head) = link;
				(*tail) = link;
		}
		else
		{
				((list*)(*tail))->next = (list*)link;
				((list*)link)->pre = (list*)(*tail);
				(*tail) = link;
		}
}
void* dequeue(void** head)
{
		list* oldHead = NULL;
		if((list*)(*head) == NULL)
				return NULL;
		oldHead = (list*)(*head);
		(*head) = (list*)oldHead->next;
		return (void*)oldHead;
}
void* dequeue_new(void** head, void** tail)
{
		void* temp = NULL;
		if((*head) == NULL)
		{
				return NULL;
		}
		else
		{
				//only one node
				if((*head) == (*tail))
				{
						temp = (*head);
						(*head) = NULL;
						(*tail) = NULL;

						assert(((list*)temp)->pre == NULL && ((list*)temp)->next == NULL);
						return temp;
				}
				else//at least two nodes
				{
						temp = (*head);
						(*head) = (void*)(((list*)temp)->next);
						((list*)(*head))->pre = NULL;	
						((list*)temp)->next = NULL;
						
						assert(((list*)temp)->pre == NULL && ((list*)temp)->next == NULL);
						return temp;
				}
		}
}
//int addlog(logrecord* logrecbuf, logcache* logbuf)
int readlogbuffromdisk(int fd, int startCPOffset, comlogbufqueue* unachievelogbufqueue)
{
		int circle = 0;
		FILE* fp = NULL;
		int logcount = 0;
		logcache* logbuf = NULL;
		logrecord* plogrecord = NULL;
		int logIndex = 0;
		unsigned char** pplogbuffer = NULL;
		int tempsize = 0;
		int startTransID = -1;
		char* line = NULL;
		unsigned char* readBuffer = NULL;
		int logLength = 0;
		int len = 0;
		int i = 0;
		assert(startCPOffset >= 0);
		if(fd < 0)
		{
				printf("readlogbuffromdisk failure, fd < 0 \n");
				return FAILURE;
		}
		if(lseek(fd, 0 - startCPOffset, SEEK_END) == -1)
		{
				printf("set offset failure\n");
				return FAILURE;
		}
		//读入一个事务的日志
		//一个事务的日志是以"startTrans" + transactionID开始，以"endTrans" + transactionID结尾
	/*	fp = fdopen(fd, "r");
		if(fp == NULL)
		{
				printf("fdopen error\n");
				return -1;
		}
		*/

		//关于读到文件末尾该如何判断，还未确定，暂时用返回值-1表示
		/*for startTrans log
		 *logreclength + logrecflag + logrecname + logrectransID + translogcount + '\n'
		 * */
		/*for endTrans log
		 *logreclength + logrecflag + logrecname + logrectransID + '\n'
		 * */
		/*for doTrans log
		 *length + IDdb + IDtable + recordKey + db_operation_tp + newrecord + '\n'
		 * */
		while(read(fd, &tempsize, 4) != -1)
		{
				if(tempsize == 0)
				{
						break;
				}
				printf("circle %d\n",circle);
				if(tempsize > 1)
				{
						printf("tempsize > 1 circle %d\n",circle);
						//每读一次数据，文件指针就向后移动
						readBuffer = (unsigned char*)malloc(tempsize - 4);
						printf("tempsize is %d\n",tempsize);
						if(read(fd, readBuffer, tempsize - 4) == -1)
						{
								printf("read logrecord error\n");
								return FAILURE;
						}
						if(*((int*)readBuffer) == TRANS_START)
						{
								if(strcmp((char*)(readBuffer + 4),"startTrans") == 0)
								{
										//倒数第二个位置
										startTransID = *((int*)(readBuffer + (tempsize - 13)));
										logcount = *((int*)(readBuffer + (tempsize - 9)));
										logIndex = 0;
										//开始缓存

										pplogbuffer = (unsigned char**)malloc(logcount * sizeof(unsigned char*));
										memset(pplogbuffer,0,sizeof(unsigned char*) * logcount);
										printf("transaction start %d\n",startTransID);
								}
						}
						else if(*((int*)readBuffer) == TRANS_END)
						{
								if(strcmp((char*)(readBuffer + 4),"endTrans") == 0)
								{
										if(startTransID == *((int*)(readBuffer + (tempsize - 9))))
										{
												printf("transaction end %d, logcount is %d\n",startTransID, logcount);
												//可以开始建立logcache结构体
												//然后将logcahe加入到unachievequeue中

												if(logIndex != logcount)
												{
														//说明在向硬盘写入log时发生系统崩溃，不恢复
														printf("find uncommit transaction\n");
														goto endTransError;
												}
												logbuf = initlogrecordbuf(startTransID);
												for(i = 0; i < logcount && pplogbuffer[i] != NULL; i++)
												{
														plogrecord = parseLogBuffer(pplogbuffer[i]);
														if(FAILURE == addlog(plogrecord, logbuf))
														{
																fprintf(stderr,"readlogbuffromdisk add log error\n");
																return FAILURE;
														}
												}

												//pthread_mutex_lock(&unachieve_queue_lock);
												if(FAILURE == enqueue_new(GET_POINTER(unachievelogbufqueue),(void*)logbuf))
												{
												//		pthread_mutex_unlock(&unachieve_queue_lock);
														fprintf(stderr, "readlogbuffromdisk unachievelogbufqueue enqueue error\n");
														return FAILURE;
												}
												//pthread_mutex_lock(&unachievelogbufqueue->datalock);
												unachievelogbufqueue->count += 1;
												//pthread_mutex_unlock(&unachievelogbufqueue->datalock);

												//pthread_mutex_unlock(&unachieve_queue_lock);
endTransError:
												for(i = 0; i < logIndex; i++)
												{
														free(pplogbuffer[i]);
												}
												if(pplogbuffer != NULL)
												{
														free(pplogbuffer);
												}
												logIndex = 0;
												logcount = 0;
										}
								}
						}
						else
						{
								//即使是在log备份过程中发生了系统崩溃，logIndex也不会等于logcount，出现这种情况只能说明程序错误，或者是其他未知错误

								if(strcmp((char*)(readBuffer), "StartCP") == 0)
								{
								}
								else if(strcmp((char*)(readBuffer), "EndCP") == 0)
								{
								}
								else
								{
										if(logIndex == logcount)
										{//越界
												printf("未知错误logIndex == logcount %d\n", logcount);
												return FAILURE;
										}
										//logLength = *((int*)readBuffer);
										pplogbuffer[logIndex] = (unsigned char*)malloc(tempsize);
										*((int*)pplogbuffer[logIndex]) = tempsize;
										memcpy(pplogbuffer[logIndex] + 4, readBuffer, tempsize - 4);
										logIndex++;
								}
						}
				}
				circle++;
				tempsize = 0;
		}
		
/*		read = getline(&line, &len, fp);
		if(read == -1)
		{
				printf("getline failure\n");
				return -1;
		}
		*/
}
int writelogbuftodisk(void* newhead, void* newtail, int checkpoint)
{
		static int timeofcall = 1;
		void* logbuftemp = NULL;
		int fd;
		int currentOffset = 0;
		unsigned char* buffer = NULL;
		fd = open("log.log",O_CREAT|O_WRONLY|O_APPEND);
		if(fd < 0)
		{
				printf("open file FAILURE\n");
				return FAILURE;
		}
		
		while((logbuftemp = dequeue_new(&(newhead),&(newtail))) != NULL)
		{
				logbuftemp = getstructpointer(logbuftemp);
	/*			if(timeofcall % 300 == 0)
				{
						buffer = (unsigned char *)malloc(16);
						*((int*)buffer) = strlen("StartCP") + 2 + 4;
						strcpy((char*)(buffer + 4), "StartCP");
						*((char*)(buffer + 12)) = '\n';
						currentOffset = lseek(fd, 0, SEEK_CUR);
						printf("current offset is %d",currentOffset);
						printf("writelogbuftodisk: startcp\n");
						write(fd, buffer, 13);
						free(buffer);
				}
				*/
				if(FAILURE == writelogtodisc(fd, (logcache*)logbuftemp))
				{
						perror("thread3 write to disk FAILURE\n");
						pthread_exit(NULL);
				}
/*				if(timeofcall % 500 == 0)
				{
						buffer = (unsigned char *)malloc(16);
						*((int*)buffer) = strlen("EndCP") + 2 + 4;
						strcpy((char*)(buffer + 4), "EndCP");
						*((char*)(buffer + 10)) = '\n';
						currentOffset = lseek(fd, 0, SEEK_CUR);
						printf("current offset is %d",currentOffset);
						printf("writelogbuftodisk: endcp\n");
						write(fd, buffer, 11);
						free(buffer);
				}
				timeofcall ++;*/
				releaselogcache((logcache*)logbuftemp);
		}
#if 1		
		if(checkpoint == 1)
		{
				buffer = (unsigned char *)malloc(16);
				*((int*)buffer) = strlen("StartCP") + 2 + 4;
				strcpy((char*)(buffer + 4), "StartCP");
				*((char*)(buffer + 12)) = '\n';
				write(fd, buffer, 13);
				free(buffer);
		}
#endif		
		if(close(fd) == FAILURE)
		{
				printf("close file FAILURE\n");
				return FAILURE;
		}
}
unsigned int get_file_size(const char* path)
{
		unsigned int filesize = -1;
		struct stat statbuff;
		if(stat(path, &statbuff) < 0)
		{
				return filesize;
		}
		else
		{
				filesize = statbuff.st_size;
		}
		return filesize;
}
//readlogfile(fd, buffer, threshold, readSize, (0 - nextOffset))
int readlogfile(int fd, unsigned char* buffer, int threshold, int readSize, int offset)
{
		if(buffer == NULL)
		{
				return FAILURE;
		}
		if(readSize > threshold)
		{
				return FAILURE;
		}
		assert(offset <= 0);
		if(lseek(fd, offset, SEEK_END) == -1)
		{
				printf("readlogfile :set offset failure\n");
				return FAILURE;
		}
		if(read(fd, buffer, readSize) == -1)
		{
				printf("readlogfile :reach file end\n");
				//return FAILURE;
		}
		return SUCCESS;
}
//为了实现简便，不需要返回读取的一行数据
//fd未打开，返回-1；读取第一个字符就出现问题(到文件末尾或者其他情况)，返回0；其他情况返回读取到的字节数包括'\n'。
int myGetline(int fd)
{
		char ch;
		int length = 0;
		if(fd < 0)
		{
				return -1;
		}
		while(read(fd, &ch, 1) != -1)
		{
				length ++;
				if(ch == '\n')
				{
						break;
				}
		}
		return length;
}
int getNextOffset(int fd, int preOffset, int threshold, int filesize, int* readSize)
{
		int nextOffset = 0;
		int read = 0;
		*readSize = 0;
		if(fd < 0)
		{
				printf("the file haven't open SUCCESS\n");
				return -1;
		}
		if(preOffset + threshold >=filesize)
		{
				nextOffset = filesize;
				*readSize = filesize - preOffset;
				return nextOffset;
		}
		
		assert((preOffset + threshold) >= 0);
		if(lseek(fd, (0 - (preOffset + threshold)), SEEK_END) == -1)
		{
				printf("set offset failure\n");
				return -1;
		}
		//做这个动作是为了保证读取到buffer里的第一个字节刚好是一个logrecord的第一个字节
		read = myGetline(fd);
		if(read == -1)
		{
				printf("getline failure\n");
				return -1;
		}
		//可能是读到了文件末尾
		if(read == 0)
		{
				printf("myGetline return 0, may generate error\n");
				return -1;
		}
		//边界可能有问题，需要检查
		nextOffset = preOffset + threshold - read;
		*readSize = threshold - read;
		if(*readSize < 0)
		{
				printf("logrecord size exceed threshold \n");
				return -1;
		}
		return nextOffset;
}
//unsigned int get_file_size(const char* path)
//int readlogfile(int fd, unsigned char* buffer, int threshold, int readSize, int offset)
//int getNextOffset(int fd, int preOffset, int threshold, int filesize, int* readSize)
//int findCheckpoint(unsigned char* buffer, int length, int* locNewline, int* startCPnum)
//该函数返回第一对完整的startcheckpoint与endcheckpoint的startcheckpoint的位置,并返回增量log是否完整的信息
int searchCheckpoint(int* startCPOffset)
{
		int fd;
		int startCPnum = 0;
		int endCPnum = 0;
		int checkResult = 0;
		//表示增量log文件是否完整
		int compIncreLog = 0;
		unsigned int filesize = 0;
		int preOffset = 0;
		const int threshold = 4096;
		int nextOffset = 0;
		int readSize = 0;
		int status = 0;
		unsigned char buffer[4096];
		fd = open("log.log",O_RDONLY);
		if(fd < 0)
		{
				printf("open file FAILURE\n");
				return FAILURE;
		}
		filesize = get_file_size("log.log");

		do{
				nextOffset = getNextOffset(fd, preOffset, threshold, filesize, &readSize);
				if(nextOffset == -1)
				{
						printf("getNextOffset FAILURE\n");
						close(fd);
						return FAILURE;
				}
				//int readlogfile(int fd, unsigned char* buffer, int threshold, int readSize, int offset)
				printf("nextOffset is %d\n",nextOffset);
				if(readlogfile(fd, buffer, threshold, readSize, (0 - nextOffset)) == FAILURE)
				{
						printf("searchCheckpoint :readlogfile FAILURE\n");
						close(fd);
						return FAILURE;
				}
				//int findCheckpoint(unsigned char* buffer, int length, int* startCPnum, int* endCPnum)		
				checkResult = findCheckpoint(buffer, readSize, &startCPnum, &endCPnum);
				switch(status)
				{
						//status0：都没找到
						case 0:
								switch(checkResult)
								{
										case 0:
												break;
										case 1:
												compIncreLog = 1;
												*startCPOffset = nextOffset - startCPnum;
												close(fd);
												return compIncreLog;
												break;
										case 2:
												status = 2;
												compIncreLog = 0;
												break;
										case 3:
												status = 3;
												compIncreLog = 1;
												break;
										case 4:
												status = 4;
												compIncreLog = 0;
												break;
										default:
												break;
								}
								break;
						//status1：都找到并且是endcheckpoint在后
						case 1:
								switch(checkResult)
								{
										case 0:
												break;
										case 1:
												break;
										case 2:
												break;
										case 3:
												break;
										case 4:
												break;
										default:
												break;
								}
								break;
						//status2：都找到并且是startcheckpoint在后
						case 2:
								switch(checkResult)
								{
										case 0:
												break;
										case 1://按理说不可能，因为这表示两个endcheckpoint相邻
												printf("status 2 case 1 error \n");
												close(fd);
												return -1;
												break;
										case 2:
												*startCPOffset = nextOffset - startCPnum;
												close(fd);
												return compIncreLog;
												break;
										case 3://同上case1，出错
												printf("status 2 case 3 error \n");
												close(fd);
												return -1;
												break;
										case 4:
												*startCPOffset = nextOffset - startCPnum;
												close(fd);
												return compIncreLog;
												break;
										default:
												break;
								}
								break;
						//status3：只找到endcheckpoint，当然也是在后
						case 3:
								switch(checkResult)
								{
										case 0:
												break;
										case 1://两个endcheckpoint相邻，出错
												printf("status 3 case 1 error \n");
												close(fd);
												return -1;
												break;
										case 2:
												*startCPOffset = nextOffset - startCPnum;
												close(fd);
												return compIncreLog;
												break;
										case 3:
												printf("status 3 case 3 error \n");
												close(fd);
												return -1;
												break;
										case 4:
												*startCPOffset = nextOffset - startCPnum;
												close(fd);
												return compIncreLog;
												break;
										default:
												break;
								}
								break;
						//status4：只找到startcheckpoint，当然也是在后
						case 4:
								switch(checkResult)
								{
										case 0:
												break;
										case 1:
												*startCPOffset = nextOffset - startCPnum;
												close(fd);
												return compIncreLog;
												break;
										case 2:
												status = 2;
												break;
										case 3:
												status = 3;
												break;
										case 4:
												break;
										default:
												break;
								}
								break;
						default:
								break;
				}
				preOffset = nextOffset;
		}while(preOffset != filesize);
		
		close(fd);
		return -1;
}
int findCheckpoint(unsigned char* buffer, int length, int* startCPnum, int* endCPnum)
{
		int i = 0;
		*startCPnum = 0;
		*endCPnum = 0;
		//find StartCheckpoint
		while(i < length)
		{
				//"StartCP"  "EndCP"
				if(*((int*)(buffer + i)) == strlen("StartCP") + 4 + 2)
				{
						if(strcmp((char*)(buffer + i + 4), "StartCP") == 0)
						{
								*startCPnum = i;
						}
				}
				if(*((int*)(buffer + i)) == strlen("EndCP") + 4 + 2)
				{
						if(strcmp((char*)(buffer + i + 4), "EndCP") == 0)
						{
								*endCPnum = i;
						}
				}
				i += *((int*)(buffer + i));
		}
		if(*startCPnum == 0 && *endCPnum == 0)
		{
				//status0：都没找到
				return 0;
		}
		else if(*startCPnum != 0 && *endCPnum != 0)
		{
				if(*startCPnum < *endCPnum)
				{
						//status1：都找到并且是endcheckpoint在后
						return 1;
				}
				else//*startCPnum > *endCPnum
				{
						//status2：都找到并且是startcheckpoint在后
						return 2;
				}
		}
		else
		{
				if(*endCPnum != 0)
				{
						//status3：只找到endcheckpoint，当然也是在后
						return 3;
				}
				else//*startCPnum != 0
				{
						//status4：只找到startcheckpoint，当然也是在后
						return 4;
				}
		}
}
int writelogtodisc(int fd, logcache* logbuf)
{
		logrecord* rec = NULL;
		unsigned char* buffer = NULL;
		unsigned int bufferlength = 0;
		char* begin = "startTrans";
		char* end = "endTrans";
		unsigned char bufTrans[32];
		if(fd < 0)
		{
				printf("the file is not open success \n");
				return FAILURE;
		}

		//logreclength + logrecflag + logrecname + logrectransID + translogcount + '\n'
		//*((int*)bufTrans) = strlen(begin);
		*((int*)bufTrans) = 28;
		*((int*)(bufTrans + 4)) = TRANS_START;
		strcpy((char*)(bufTrans + 8), begin);
		*((int*)(bufTrans + 19)) = logbuf->transactionID; 
		*((int*)(bufTrans + 23)) = logbuf->logrecCount;
		bufTrans[27] = '\n';
		write(fd, bufTrans, 28);

		while((rec = (logrecord*)dequeue_new(GET_POINTER_LOG(logbuf))) != NULL)
		{
				//将rec追加到log文件中
				bufferlength = formatlogrecord(&buffer,rec);
				write(fd,buffer,bufferlength);
		}
		//logreclength + logrecflag + logrecname + logrectransID + '\n'
		*((int*)bufTrans) = 22;
		*((int*)(bufTrans + 4)) = TRANS_END;
		strcpy((char*)(bufTrans + 8), end);
		*((int*)(bufTrans + 17)) = logbuf->transactionID; 
		bufTrans[21] = '\n';
		write(fd, bufTrans, 22);

		return SUCCESS;
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
logrecord* parseLogBuffer(unsigned char* buffer)
{
		logrecord* pLogrecord = (logrecord*)malloc(sizeof(logrecord));
		int length = 0;
		if(buffer == NULL)
		{
				return NULL;
		}
		//length + IDdb + IDtable + recordKey + db_operation_tp + newrecord + '\n'
		INITLOGRECORD(pLogrecord);
		length = *((int*)buffer);
		buffer += 4;
		pLogrecord->IDdb = *((int*)buffer);
		buffer += 4;
		pLogrecord->IDtable = *((int*)buffer);
		buffer += 4;
		pLogrecord->recordKey = *((int*)buffer);
		buffer += 4;
		pLogrecord->db_operation_tp = *((int*)buffer);
		buffer += 4;
		length -= 20;
		pLogrecord->newrecord = (unsigned char*)malloc(length -1);
		memcpy(pLogrecord->newrecord, buffer, length -1);
		pLogrecord->newrecordlength = length - 1;

		return pLogrecord;
}
int formatlogrecord(unsigned char** buffer,logrecord* logrecbuf)
{
		//加入回车符是为了方便从后向前搜索出startCheckpoint和endCheckpoint
		//length + IDdb + IDtable + recordKey + db_operation_tp + newrecord + '\n'
		unsigned int length = 21 + logrecbuf->newrecordlength;
		unsigned char* temp = (unsigned char*)malloc(length * sizeof(unsigned char));
		*buffer = temp;
		*((unsigned int*)temp) = length;
		temp += 4;

		*((unsigned int*)temp) = logrecbuf->IDdb;
		temp += 4;

		*((unsigned int*)temp) = logrecbuf->IDtable;
		temp += 4;

		*((unsigned int*)temp) = logrecbuf->recordKey;
		temp += 4;

		*((unsigned int*)temp) = logrecbuf->db_operation_tp;
		temp += 4;
/*
		memcpy((void*)temp,(void*)(logrecbuf->oldrecord),logrecbuf->oldrecordlength);
		temp += logrecbuf->oldrecordlength;
*/
		memcpy((void*)temp,(void*)(logrecbuf->newrecord),logrecbuf->newrecordlength);
		temp += logrecbuf->newrecordlength;
		
		temp += logrecbuf->newrecordlength;
		*((char*)temp) = '\n';

		return length;
}



int releaselogcache(logcache* logbuf)
{
		logrecord* rec = NULL;
		if(logbuf == NULL)
		{
				return FAILURE;
		}
		while((rec = dequeue_new(GET_POINTER_LOG(logbuf))) != NULL)
		{
				if(rec->oldrecord != NULL)
				{
						free(rec->oldrecord);
				}
				if(rec->newrecord != NULL)
				{
						free(rec->newrecord);
				}
				free(rec);
		}
		free(logbuf);
		return SUCCESS;
}

void* getstructpointer(void* link)
{
		return link - OFFSET(logcache,achievelink);
}

void* getlinkpointer(void* logbuf)
{
		return logbuf + OFFSET(logcache,achievelink);
}

int removefromqueue(void* logbuf, void** logbufqueue)
{
		list* templink = (list*)logbuf;
		if(logbufqueue == NULL || logbuf == NULL)
		{
				return FAILURE;
		}
		if(logbufqueue == logbuf)
		{
				if(templink->next != NULL)
				{
						templink->next->pre = NULL;
				}
				(*logbufqueue) = templink->next;
		}
		else
		{
				templink->pre->next = templink->next;
				if(templink->next != NULL)
				{
						templink->next->pre = templink->pre;
				}
		}
		templink->pre = NULL;
		templink->next = NULL;
		return SUCCESS;
}
/*
int removetoachieve(void* logbuf,void** logbufqueue,void** achievelogbufqueue)
{
		if(FAILURE == removefromqueue(logbuf,logbufqueue))
		{
				printf("remove FAILURE\n");
				return FAILURE;
		}
		
		logbuf = getlinkpointer(logbuf);

		if(FAILURE == enqueue(achievelogbufqueue,logbuf))
		{
				printf("enqueue FAILURE\n");
				return FAILURE;
		}
}
*/
