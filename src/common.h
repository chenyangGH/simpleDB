#ifndef __COMMON_H__
#define __COMMON_H__
#define FAILURE -1
#define SUCCESS 0
#define OFFSET(type,member) ((unsigned int)&(((type*)0)->member))
typedef struct _list
{
		struct _list* pre;
		struct _list* next;
}list;
#define TRANS_START 0xfefefefe
#define TRANS_END   0xefefefef

#define DB_INTEGER_TP 0x00000001
#define DB_BYTE_TP    0x00000002
#define DB_FLOAT_TP   0x00000003
#define DB_CHAR_TP    0x00000004

#define DB_INTEGER_SIZE  0x00000004
#define DB_BYTE_SIZE     0x00000001
#define DB_FLOAT_SIZE    0x00000004
#define DB_CHAR_SIZE     0x00000001


#define DB_CREATE_OP  0x00000001 
#define DB_DELETE_OP  0x00000002
#define DB_INSERT_OP  0x00000003
#define DB_UPDATE_OP  0x00000004
#define DB_READ_OP    0x00000005
int enqueue(void** head,void* link);
void* dequeue(void** head);


#endif
