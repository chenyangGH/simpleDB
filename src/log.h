#ifndef _LOG_H_
#define _LOG_H_

#define GET_HEAD_POINTER(p) (p->logbufhead)
#define GET_TAIL_POINTER(P) (p->logbuftail)
#define GET_POINTER(p) &(p->logbufhead),&(p->logbuftail)
#define GET_POINTER_LOG(p) &(p->loghead),&(p->logtail)
#include "common.h"

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

typedef struct _logcache
{
		list link;
		list achievelink;
		unsigned int transactionID;
		//提交时间的表示待定
		unsigned int logrecCount;
		void* loghead;
		void* logtail;
}logcache;

typedef struct _comlogbufqueue
{
		pthread_mutex_t datalock;
		unsigned int count;
		void* logbufhead;
		void* logbuftail;
}comlogbufqueue;
//将一个修改记录添加到该事务对应的cache中
int addlog(logrecord* logrecbuf, logcache* logbuf);
int addlogbuf(logcache* logbuf,logcache** logbufqueue);
//将一个日志cache中的log写入到logfile
int writelogtodisc(int fd,logcache* logbuf);
int writelogbuftodisk(void* newhead,void* newtail,int checkpoint);
//释放事务日志缓冲区
int releaselogcache(logcache* logbuf);
int formatlogrecord(unsigned char** buffer,logrecord* logrecbuf);
//注意传递给achievebufqueue的指针为achievelink
int removetoachieve(void* logbuf,void** logbufqueue,void** achievelogbufqueue);
int removefromqueue(void*logbuf, void** logbufqueue);
void* getstructpointer(void* link);
void* getlinkpointer(void* logbuf);
logrecord* parseLogBuffer(unsigned char* buffer);
int myGetline(int fd);
#endif
