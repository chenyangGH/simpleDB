#ifndef __LIST_H__
#define __LIST_H__
#define pNULL (0x00000000)
typedef struct _list_head list_head;
struct _list_head
{
		list_head* prev;
		list_head* next;
};
#define INIT_LIST_HEAD(p) do{(p)->prev = (p);(p)->next = (p);}while(0)
static inline void __list_add(list_head *newnode,
				list_head *prev,
				list_head *next)
{
		prev->next = newnode;
		newnode->prev = prev;
		next->prev = newnode;
		newnode->next = next;
}
static inline void list_add(list_head* head, list_head* newnode)
{
		__list_add(newnode, head, head->next);
}
static inline void list_add_tail(list_head* head, list_head* newnode)
{
		__list_add(newnode, head->prev, head);
}
static inline void __list_del(list_head* del_prev, list_head* del_next)
{
		del_prev->next = del_next;
		del_next->prev = del_prev;
}
static inline void list_del(list_head* del_node)
{
		__list_del(del_node->prev, del_node->next);
		del_node->prev = pNULL;
		del_node->next = pNULL;
}
static inline int list_empty(const list_head* head)
{
		return head->next == head;
}
#endif
