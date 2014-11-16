#ifndef _RB_TREE_H
#define _RB_TREE_H
typedef int key_type;
typedef struct _rbnode rbnode;

struct _rbnode
{
		int color;
		key_type key;
		rbnode* parent;
		rbnode* lchild;
		rbnode* rchild;
};
#define parent(p) ((p)->parent)
#define lchild(p) ((p)->lchild)
#define rchild(p) ((p)->rchild)
#define color(p)  ((p)->color)
#define key(p)    ((p)->key)
#define r_color 1
#define b_color 0
#define SUCCESS 0
#define FAILURE -1

int tree_insert(rbnode** root, rbnode* inode);
//rbnode* tree_insert(rbnode* root, rbnode* inode);
rbnode* tree_delete(rbnode** root, rbnode* dnode);
rbnode* tree_successor(rbnode* snode);
rbnode* tree_search(rbnode* root, key_type key);
int left_rotate(rbnode** root, rbnode* head);
int right_rotate(rbnode** root, rbnode* head);
int rb_insert_fixup(rbnode** root, rbnode* inode);
int rb_delete_fixup(rbnode** root, rbnode* rpnode);


#endif
