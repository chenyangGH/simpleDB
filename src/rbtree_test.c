#include <stdio.h>
#include <malloc.h>
#include <time.h>
#include <assert.h>
#include "rbtree.h"
#define create_rb_node(p,key_value) 				 \
do{  												 \
		p = (rbnode*)malloc(sizeof(rbnode)); 		 \
		color(p) = r_color;                          \
		lchild(p) = NULL;                            \
		rchild(p) = NULL;                            \
		key(p)    = key_value;                       \
}while(0)
int main()
{
		key_type key_value = 0;
		rbnode* tree_root = NULL;
		rbnode* new_node = NULL;
		int i,count = 100;
		srand(time(NULL));	
		for(i = 0; i < count; i++)
		{
				key_value = rand() % count;
				create_rb_node(new_node, key_value);
				if(tree_insert(&tree_root, new_node) == SUCCESS)
				{
						printf("insert key value %d node success\n",key_value);
				}
				else
				{
						printf("insert key value %d node failure\n",key_value);
						exit(FAILURE);
				}
				if(new_node = tree_search(tree_root,key_value))
				{
						printf("search key value %d node success\n",key(new_node));
				}
				else
				{
						printf("search key value %d node failure\n", key_value);
						exit(FAILURE);
				}
				if(!(i % 10))
				{
						printf("begin delete %d \n",i);
						new_node = tree_search(tree_root,i);
						printf("begin delete %d \n",i);
						if(new_node = tree_delete(&tree_root, new_node))
						{
								printf("delete key value %d node success\n",i);
						}
						else
						{
								printf("delete key value %d node failure\n",i);
						}
				}
		}
		return 0;
}
