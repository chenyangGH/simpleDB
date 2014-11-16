#include <stdio.h>
#include <assert.h>
#include "rbtree.h"

int tree_insert(rbnode** root, rbnode* inode)
{
		assert(lchild(inode) == NULL && rchild(inode) == NULL);
		rbnode* index = (*root);
		rbnode* pre_index = NULL;
		if(inode == NULL)
		{
				return FAILURE;
		}
		if((*root) == NULL)
		{
				color(inode) = b_color;
				(*root) = inode;
				return SUCCESS;
		}
		while(index != NULL)
		{
				pre_index = index;
				if(key(inode) < key(index))
				{
						//inode should be insert in lchild(index)
						index = lchild(index);
				}
				else
				{
						index = rchild(index);
				}
		}
		//index == NULL ===> pre_index is a leaf node or a inner node holding one child
		parent(inode) = pre_index;
		if(key(inode) < key(pre_index))
		{
				assert(lchild(pre_index) == NULL);
				lchild(pre_index) = inode;
		}
		else
		{
				assert(rchild(pre_index) == NULL);
				rchild(pre_index) = inode;
		}
		color(inode) = r_color;
		rb_insert_fixup(root, inode);
		return SUCCESS;
}

//
rbnode* tree_delete(rbnode** root, rbnode* dnode)
{
		rbnode* dnode_new = NULL;
		rbnode* dnode_new_child = NULL;
		if(dnode == NULL)
		{
				return NULL; 
		}
		if(lchild(dnode) == NULL || rchild(dnode) == NULL)
		{
				dnode_new = dnode;
		}
		else
		{
				dnode_new = tree_successor(dnode);
		}
		assert(lchild(dnode_new) == NULL || rchild(dnode_new) == NULL);
		if(lchild(dnode_new) != NULL)
		{
				dnode_new_child = lchild(dnode_new);
		}
		else
		{
				assert(rchild(dnode_new) == NULL || rchild(dnode_new) != NULL);
				dnode_new_child = rchild(dnode_new);
		}
		if(dnode_new_child != NULL)
		{
				parent(dnode_new_child) = parent(dnode_new);
		}
		if(parent(dnode_new) == NULL)
		{
				(*root) = dnode_new_child;
		}
		else
		{
				if(lchild(parent(dnode_new)) == dnode_new)
				{
						lchild(parent(dnode_new)) = dnode_new_child;
				}
				else
				{
						rchild(parent(dnode_new)) = dnode_new_child;
				}
		}
		if(dnode != dnode_new)
		{
			//copy data from dnode_new to dnode
				key(dnode) = key(dnode_new);
		}
		if(color(dnode_new) == b_color)
		{
				rb_delete_fixup(root, dnode_new_child);
		}
		return dnode_new;
}
rbnode* tree_successor(rbnode* snode)
{
		assert(snode != NULL);
		rbnode* index = NULL;
		rbnode* pre_index = NULL;
		if(rchild(snode) != NULL)
		{
				//search the leftest node of rchild(snode)
				index = rchild(snode);
				while(index != NULL)
				{
						pre_index = index;
						index = lchild(index);
				}
				return pre_index;
		}
		index = parent(snode);
		while(index != NULL && snode == rchild(index))
		{
				snode = index;
				index = parent(index);
		}
		return index;
}
int left_rotate(rbnode** root, rbnode* head)
{
		assert(rchild(head) != NULL);
		rbnode* new_head = rchild(head);
		rchild(head) = lchild(new_head);
		if(lchild(new_head) != NULL)
		{
				parent(lchild(new_head)) = head;
		}
		parent(new_head) = parent(head);
		if(parent(head) != NULL)
		{
				if(lchild(parent(head)) == head)
				{
						lchild(parent(head)) = new_head;
				}
				else
				{
						rchild(parent(head)) = new_head;
				}
		}
		else
		{
				(*root) = new_head;
		}
		lchild(new_head) = head;
		parent(head) = new_head;

		return SUCCESS;
}
int right_rotate(rbnode** root, rbnode* head)
{
		assert(lchild(head) != NULL);
		rbnode* new_head = lchild(head);

		lchild(head) = rchild(new_head);
		if(rchild(new_head) != NULL)
		{
				parent(rchild(new_head)) = head;
		}

		parent(new_head) = parent(head);
		if(parent(head) == NULL)
		{
				assert((*root) == head);
				(*root) = new_head;
		}
		else
		{
				if(lchild(parent(head)) == head)
				{
						lchild(parent(head)) = new_head;
				}
				else
				{
						rchild(parent(head)) = new_head;
				}
		}

		rchild(new_head) = head;
		parent(head) = new_head;
		return SUCCESS;
}
int rb_insert_fixup(rbnode** root, rbnode* inode)
{
		rbnode* inode_parent = parent(inode);
		rbnode* temp = NULL;
		//color(inode_parent) == r_color ===> parent(inode_parent) != NULL
		while(inode_parent != NULL && color(inode_parent) == r_color)
		{
				assert(parent(inode_parent) != NULL);
				if(lchild(parent(inode_parent)) == inode_parent)
				{
						if(rchild(parent(inode_parent)) && color(rchild(parent(inode_parent))) == r_color)
						{
								color(parent(inode_parent)) = r_color;
								color(inode_parent) = b_color;
								color(rchild(parent(inode_parent))) = b_color;
								inode = parent(inode_parent);
								inode_parent = parent(inode);
						}
						else//null pointer is also black
						{
								if(rchild(inode_parent) == inode)
								{
										left_rotate(root, inode_parent);
										temp = inode_parent;
										inode_parent = inode;
										inode = temp;
								}
								color(parent(inode_parent)) = r_color;
								color(inode_parent) = b_color;
								right_rotate(root, parent(inode_parent));
						}
				}
				else
				{

						if(lchild(parent(inode_parent)) && color(lchild(parent(inode_parent))) == r_color)
						{
								color(parent(inode_parent)) = r_color;
								color(inode_parent) = b_color;
								color(lchild(parent(inode_parent))) = b_color;
								inode = parent(inode_parent);
								inode_parent = parent(inode);
						}
						else//null pointer is also a black color node
						{
								if(lchild(inode_parent) == inode)
								{
										right_rotate(root, inode_parent);
										temp = inode_parent;
										inode_parent = inode;
										inode = temp;
								}
								color(parent(inode_parent)) = r_color;
								color(inode_parent) = b_color;
								left_rotate(root, parent(inode_parent));
						}
				}
		}
		if(inode_parent == NULL)
		{
				color(inode) = b_color;
		}	
		return SUCCESS;
}
int rb_delete_fixup(rbnode** root, rbnode* rpnode)
{
		rbnode* rpnode_sib = NULL;
		if(rpnode == NULL)
		{
				return FAILURE;
		}
		//while(parent(rpnode) != NULL && color(rpnode) == b_color)
		while(rpnode != (*root) && color(rpnode) == b_color)
		{
				if(rpnode == lchild(parent(rpnode)))
				{
						rpnode_sib = rchild(parent(rpnode));
						//(rpnode != NULL && color(rpnode) == black) \
						===> assert(rpnode_sib != NULL)
						assert(rpnode_sib != NULL);
						if(color(rpnode_sib) == r_color)
						{
								color(parent(rpnode)) == r_color;
								color(rpnode_sib) = b_color;
								left_rotate(root,parent(rpnode));
								rpnode_sib = rchild(parent(rpnode));
						}
						//(rpnode != NULL && color(rpnode) == black) \
						===> assert(rpnode_sib != NULL)
						assert(rpnode_sib != NULL && color(rpnode_sib) == b_color);
						if((!lchild(rpnode_sib) || color(lchild(rpnode_sib)) == b_color) && (!rchild(rpnode_sib) || color(rchild(rpnode_sib)) == b_color))
						{
								color(rpnode_sib) = r_color;
								rpnode = parent(rpnode);
						}
						else
						{
								if(!rchild(rpnode_sib) || (color(rchild(rpnode_sib)) == b_color))
								{
										color(rpnode_sib) = r_color;
										if(lchild(rpnode_sib))
										{
												color(lchild(rpnode_sib)) = b_color;
												right_rotate(root, rpnode_sib);
										}
										rpnode_sib = rchild(parent(rpnode));
								}
								//whatever color(parent(rpnode)) is red or black
								color(rpnode_sib) = color(parent(rpnode_sib));
								color(parent(rpnode)) = b_color;
								if(rchild(rpnode_sib))
								{
										color(rchild(rpnode_sib)) = b_color;
								}
								left_rotate(root, parent(rpnode));
								//
								rpnode = *(root);
								break;
						}
				}
				else//rpnode == rchild(parent(rpnode))
				{
						rpnode_sib = lchild(parent(rpnode));
						if(color(rpnode_sib) == r_color)
						{
								color(parent(rpnode)) == r_color;
								color(rpnode_sib) = b_color;
								right_rotate(root,parent(rpnode));
								rpnode_sib = lchild(parent(rpnode));
						}
						if((!lchild(rpnode_sib) || color(lchild(rpnode_sib)) == b_color) && (!rchild(rpnode_sib) || color(rchild(rpnode_sib)) == b_color))
						{
								color(rpnode_sib) = r_color;
								rpnode = parent(rpnode);
						}
						else//color(rpnode_sib) == b_color
						{
								if(!lchild(rpnode_sib) || (color(lchild(rpnode_sib)) == b_color))
								{
										color(rpnode_sib) = r_color;
										if(rchild(rpnode_sib))
										{
												color(rchild(rpnode_sib)) = b_color;
												left_rotate(root, rpnode_sib);
										}
										rpnode_sib = lchild(parent(rpnode));
								}
								//whatever color(parent(rpnode)) is red or black
								//color(lchild(rpnode_sib)) == r_color
								color(rpnode_sib) = color(parent(rpnode_sib));
								color(parent(rpnode)) = b_color;
								if(lchild(rpnode_sib))
								{
										color(lchild(rpnode_sib)) = b_color;
								}
								right_rotate(root, parent(rpnode));
								//
								rpnode = *(root);
								break;
						}
				}
		}
		if(rpnode)
		{
				color(rpnode) = b_color;
		}	
		return SUCCESS;
}
rbnode* tree_search(rbnode* root, key_type key_value)
{
		rbnode* pin = root;
		while(pin != NULL)
		{
				if(key(pin) == key_value)
				{
						return pin;
				}
				else
				{
						if(key(pin) > key_value)
						{
								pin = lchild(pin);
						}
						else
						{
								pin = rchild(pin);
						}
				}
		}
		return pin;
}
