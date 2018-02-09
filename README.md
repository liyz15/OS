# OS

This is my project for OS course, which convert AVL tree in WRK to rbtree based on Linux Kernel.

### AVL 树 $\rightarrow$ 红黑树问题

在 Windows 的虚拟内存管理中，将 VAD 组织成AVL树。VAD 树是一种平衡二叉树。
红黑树也是一种自平衡二叉查找树，在 Linux 2.6 及其以后版本的内核中，采用红黑树来维护内存块。
请尝试参考 Linux 源代码将 WRK 源代码中的 VAD 树由 AVL 树替换成红黑树。

### 原理背景

#### VAD

在以页为单位的虚拟内存分配方式中，对于指定地址空间是否已经被提交了物理内存，可以通过页目录和页表来判断，不过这样做很麻烦。而对于指定地址空间是否已经被保留，通过页目录页页表没有办法判断。
在 Windows 系统中，使用 VAD（Virtual Address Descriptor） 来解决地址空间是否被分配的问题。对于每一个进程，Windows 的内存管理器维护一组虚拟地址描述符 VAD 来描述一段被分配的进程虚拟空间的状态。
虚拟地址描述信息被构造成一棵平衡二叉树（AVL 树）以使查找更有效率。

#### AVL 树

Windows 中的 VAD 被构造成为一棵 AVL 树，AVL 树是最先发明的自平衡二叉查找树。在 AVL 树中任何节点的两个子树的高度最大差别为 1，所以它也被称为高度平衡树。查找、插入和删除在平均和最坏情况下的时间复杂度都是 $O(\log {n})$。增加和删除可能需要通过一次或多次树旋转来重新平衡这个树。
节点的平衡因子是它的左子树的高度减去它的右子树的高度（有时相反）。带有平衡因子 1、0 或 -1的节点被认为是平衡的。带有平衡因子 -2 或 2 的节点被认为是不平衡的，并需要重新平衡这个树。平衡因子可以直接存储在每个节点中，或从可能存储在节点中的子树高度计算出来。

#### 红黑树

红黑树（Red–black tree）同样是一种自平衡二叉查找树。与 AVL 树相同，其查找、插入和删除在最坏情况下的时间复杂度同样是 $O(\log {n})$。但红黑树牺牲了部分平衡性以换取插入/删除时做更少的旋转操作，因此在统计特性上优于 AVL 树。红黑树有以下性质：

1. 节点是红色或黑色。
2. 根是黑色。
3. 所有叶子都是黑色（叶子是NIL节点，即空叶子）。
4. 每个红色节点必须有两个黑色的子节点。
5. 从任一节点到其每个叶子的所有简单路径都包含相同数目的黑色节点。

篇幅所限，不再对二者的查找、插入、删除操作做详细描述。对比二者结构，有以下几点值得注意：

1. AVL 树和红黑树均为二叉平衡查找树，但 AVL 树为严格平衡，而红黑树相对不严格。
2. 对于 AVL 树和红黑树，其算法时间复杂度上相同，但统计性能上来说，红黑树更好。
3. 在节点结构上，AVL 树往往设有平衡因子，其值限制为 $[-2,2]$ 的区间内，而红黑树则有节点颜色，往往为红或黑（0、1）。

综合以上几点可以发现，二者均为平衡二叉树，结构上差距并不多，因此需要修改的关键部分是结点的插入和删除。

### 代码分析

#### Windows 中的 AVL 树

Windows Research Kernel（以下简称 WRK）中 VAD 树定义在 vadtree.c 和 mi.h 中，从 mi.h 中我们可以找到 VAD 树节点的定义位置，VAD 节点

```c
typedef struct _MMADDRESS_NODE {
    union {
        LONG_PTR Balance : 2;
        struct _MMADDRESS_NODE *Parent;
    } u1;
    struct _MMADDRESS_NODE *LeftChild;
    struct _MMADDRESS_NODE *RightChild;
    ULONG_PTR StartingVpn;
    ULONG_PTR EndingVpn;
} MMADDRESS_NODE, *PMMADDRESS_NODE;
```

可以发现，尽管 vadtree 主要定义在 vadtree.c 中，其节点结构体 \_MMVAD 定义在 mi.h 文件中，但实际上，VAD 树的根节点 \_MM\_AVL\_TABLE 定义在 ps.h 文件中，其根节点的类型为 MMADDRESS\_NODE 定义如下

```C
typedef struct _MM_AVL_TABLE {
    MMADDRESS_NODE  BalancedRoot;
    ULONG_PTR DepthOfTree: 5;
    ULONG_PTR Unused: 3;
#if defined (_WIN64)
    ULONG_PTR NumberGenericTableElements: 56;
#else
    ULONG_PTR NumberGenericTableElements: 24;
#endif
    PVOID NodeHint;
    PVOID NodeFreeHint;
} MM_AVL_TABLE, *PMM_AVL_TABLE;

typedef struct _MMADDRESS_NODE {
union {
    LONG_PTR Balance : 2;
    struct _MMADDRESS_NODE *Parent;
} u1;
    struct _MMADDRESS_NODE *LeftChild;
    struct _MMADDRESS_NODE *RightChild;
    ULONG_PTR StartingVpn;
    ULONG_PTR EndingVpn;
} MMADDRESS_NODE, *PMMADDRESS_NODE;
```

因此 VAD 树实际上的节点是由 \_MMADDRESS\_NODE 定义的。该节点便是 WRK 中对 AVL 树的定义。因此，VAD 树实际上使用的是 WRK 对 AVL 树的封装，经查阅发现在源码的地方也使用了 AVL 树的结构。因为我们希望修改的是树的类型，所以重点实际上并不在 VAD 的管理上，而在于 AVL 树操作的修改。而对 AVL 树的基本操作则定义在文件 addrsup.c 中，该文件才是修改的重点。

addrsup.c 文件中主要定义的操作列表如下：

| 函数名                                    | 功能                                                         |
| :---------------------------------------- | :----------------------------------------------------------- |
| MiFindNodeOrParent                        | 对于给定地址，返回节点或者节点的父节点                       |
| MiCheckForConflictingNode                 | 判断一个给定的地址范围是否已经被包含                         |
| MiGetFirstNode                            | 找到地址空间中逻辑位置最靠前的 vad                           |
| MiPromoteNode                             | 增删中用到的基本操作，提升某个节点的 Level                   |
| MiRebalanceNode                           | 对于平衡因子为 2 或 -2 的节点进行 rebalance                  |
| MiRemoveNode                              | 删除特定节点，并做平衡                                       |
| MiRealSuccessor                           | 返回给定节点的后继节点                                       |
| MiRealPredecessor                         | 返回给定节点的前驱节点                                       |
| MiInitializeVadTableAvl                   | 初始化 vad 表                                                |
| MiInsertNode                              | 插入节点                                                     |
| MiEnumerateGenericTableWithoutSplayingAvl | 逐个返回表中元素                                             |
| MiGetNextNode                             | 找到地址空间中相连的下一个节点                               |
| MiGetPreviousNode                         | 找到地址空间中相连的上一个节点                               |
| MiLocateAddressInTree                     | 对于给定地址，返回 vad                                       |
| MiFindEmptyAddressRangeInTree             | 对于给定大小的地址空间，返回一段可用空间的首地址             |
| MiFindEmptyAddressRangeDownTree           | 对于给定大小的空间和最高位地址，返回最高位地址之上的一段可用空间的首地址 |
| MiFindEmptyAddressRangeDownBasedTree      | 对于给定大小的空间和最高位地址，返回最高位地址之下的一段可用空间的首地址 |
| MiLocateAddress                           | 对于给定虚拟地址，返回 vad                                   |
| MiNodeTreeWalk                            | 遍历树中结点                                                 |

可以看到 WRK 中对 AVL 树的封装并不清晰，尽管列表中有大量用于对其操作的函数，但从前述原理我们知道，对于一棵树来说，基本操作实际上只有查找、插入和删除而已，对于 AVL 树和红黑树来说，查找操作无须进行改动，因此需要我们修改的只有对外接口 MiInsertNode 和 MiRemoveNode 而已。弄清楚了这一点，有利于之后移植的顺利进行。

值得注意的是，AVL 树的根节点并非 \_MM\_AVL\_TABLE 所包含的节点，BalancedRoot 是一个内嵌结构体，其真正的根节点是其右子节点，其左子节点置空。

#### Linux 中的红黑树

相比起来，Linux 中对封装就抽象许多，甚至完全没有涉及到内存的分配，复制粘贴出来就可以拿来做算法题，这也为我们的移植带来了方便。Linux 的红黑树定义在 rbtree.h 和 rbtree.c 文件中。rbtree.h 中主要定义了节点的结构以及一些简单操作，如 rb\_parent、rb\_color、rb\_is\_red、rb\_is\_black、rb\_set\_red、rb\_set\_black、rb\_set\_parent、rb\_set\_color 等，从函数名可以直接看出作用。节点的定义如下：

```C
struct rb_node
{
    unsigned long  rb_parent_color;
#define	RB_RED		0
#define	RB_BLACK	1
    struct rb_node *rb_right;
    struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));

struct rb_root
{
	struct rb_node *rb_node;
};
```

相比起来，该定义几乎就是一个单纯的算法定义，具有很高的抽象度。
rbtree.c 中定义的则是对红黑树的一些操作，列表如下：

| 函数名            | 功能               |
| ----------------- | ------------------ |
| __rb_rotate_left  | 节点左旋           |
| __rb_rotate_right | 节点右旋           |
| rb_insert_color   | 插入结点和颜色     |
| __rb_erase_color  | 删除颜色           |
| rb_erase          | 删除节点           |
| rb_first          | 找出最左端的叶结点 |
| rb_next           | 找到下一个节点     |
| rb_prev           | 找到上一个节点     |
| rb_replace_node   | 替换一个结点       |
| rb_link_node      | 链接节点           |

依然可以拿来做算法题。 

对比 WRK 和 Linux Kernel 中的两种树，主要有以下几点：

1. 二者并无很大不同。我们只需要将 AVL 结点中的平衡因子拿来当作红黑树中的节点颜色即可。
2. 从操作上来看，尽管 addrsup.c 文件中有三千多行代码，但实际上涉及到树结构更改的操作只有 MiInsertNode 和 MiRemoveNode 两个函数，即增删操作，对应在 Linux Kernel 的红黑树中则是 rb\_insert\_color 和 rb\_erase 两个函数。
3. 经过对引用的查找，rb\_insert\_color 和 rb\_erase 两个函数用到的操作函数有 \_\_rb\_rotate\_left、\_\_rb\_rotate\_right、rb\_insert\_color、\_\_rb\_erase\_color 四种，用到的宏定义以及内联函数有 rb\_parent、rb\_is\_red、rb\_is\_black、rb\_set\_red、rb\_set\_black、rb\_set\_parent、rb\_set\_color 几种，我们需要完成的，便是这些操作到 WRK 的移植以及增删操作两个函数的修改移植。

至此，我们便完成了对 WRK 中涉及到的 AVL 树和 Linux Kernel 中涉及到的红黑树结构分析。可以发现这次实验所需要的代码量并不大，最大的难点在于前期的代码分析、目标确定以及后期的差错调试。

分析之后我们便可以将 WRK 中的代码与 Linux Kernel 中的代码对应起来：

| struct rb\_node * | PMMADDRESS\_NODE               |
| ----------------- | ------------------------------ |
| rb\_parent\_color | u1.Balance                     |
| struct rb\_root * | PMM\_AVL\_TABLE                |
| root -> rb_node   | Root -> BalanceRoot.RightChild |
| rb\_right         | RightChild                     |
| rb\_left          | LeftChild                      |

进行代码移植时的大部分任务都在于上表中的简单替换。

#### 宏定义的移植

```C
#define	RB_RED		1
#define	RB_BLACK	0
PMMADDRESS_NODE rb_parent(PMMADDRESS_NODE node)
{
    node=SANITIZE_PARENT_NODE(node->u1.Parent);
    return node;
}
int rb_color(PMMADDRESS_NODE node)
{
    return node->u1.Balance;
}
int rb_is_red(PMMADDRESS_NODE node)
{
    return (node->u1.Balance)&1;
}
int rb_is_black(PMMADDRESS_NODE node)
{
    return !((node->u1.Balance)&1);
}
void rb_set_black(PMMADDRESS_NODE node)
{
    node->u1.Balance=RB_BLACK;
}
void rb_set_red(PMMADDRESS_NODE node)
{
    node->u1.Balance=RB_RED;
}
void rb_set_parent(PMMADDRESS_NODE rb,PMMADDRESS_NODE p)
{
    rb->u1.Parent =(PMMADDRESS_NODE) (rb_color(rb) +(unsigned long) p);
}

void rb_set_color(PMMADDRESS_NODE rb, int color)
{
    rb->u1.Balance = color;
}
```

二者相比较，除了上述替换之外，有几点需要注意的地方：

1. 方便起见将所有的宏定义都封装成了函数。
2. RB\_RED 和 RB\_BLACK 的定义并不本质。
3. 之所以 Linux Kernel 中用到了大量的逻辑运算，是因为 long 类型的指向 rb\_node 的指针其最后两位必定为 00，因此最后一位被用来保存节点颜色。这种做法虽然搞笑，但在移植时却为我们带来了麻烦，因此将所有的逻辑运算都转化为简单的赋值或者加减操作。
4. SANITIZE\_PARENT\_NODE 函数用在 AVL 树中来确保赋值父节点指针的顺利进行。这一细节必不可少，不能像 Linux Kernel 中那样直接对节点赋值。在早期版本中（如 2.6.0）没有对 rb\_set\_parent 进行封装，给移植造成了许多麻烦，所幸在后期版本（如 2.6.39）中封装更为全面。

#### 红黑树操作的移植

除增删操作以外，红黑树需要移植的操作有左旋、右旋以及删除颜色操作。这三个函数的移植都只是简单的替换。

```C
static void __rb_rotate_left(PMMADDRESS_NODE node, PMM_AVL_TABLE root)
{
    PMMADDRESS_NODE right = node->RightChild;
    PMMADDRESS_NODE parent = SANITIZE_PARENT_NODE(node->u1.Parent);

    if ((node->RightChild = right->LeftChild))
        right->LeftChild->u1.Parent = (PMMADDRESS_NODE) (rb_color(right->LeftChild) +(unsigned long) node);
    right->LeftChild = node;

    rb_set_parent(right, parent);

    if (parent)
    {
        if (node == parent->LeftChild)
            parent->LeftChild = right;
        else
            parent->RightChild = right;
    }
    else
        root->BalancedRoot.RightChild = right;
    rb_set_parent(node, right);
}

static void __rb_rotate_right(PMMADDRESS_NODE node, PMM_AVL_TABLE root)
{
    PMMADDRESS_NODE left = node->LeftChild;
    PMMADDRESS_NODE parent = rb_parent(node);

    if ((node->LeftChild = left->RightChild))
        rb_set_parent(left->RightChild, node);
    left->RightChild = node;

    rb_set_parent(left, parent);

    if (parent)
    {
        if (node == parent->RightChild)
            parent->RightChild = left;
        else
            parent->LeftChild = left;
    }
    else
        root->BalancedRoot.RightChild = left;
    rb_set_parent(node, left);
}

static void __rb_erase_color(PMMADDRESS_NODE node, PMMADDRESS_NODE parent,
                    PMM_AVL_TABLE root)
{
    PMMADDRESS_NODE other;

    while ((!node || rb_is_black(node)) && node != root->BalancedRoot.RightChild)
    {
        if (parent->LeftChild == node)
        {
            other = parent->RightChild;
            if (rb_is_red(other))
            {
                rb_set_black(other);
                rb_set_red(parent);
                __rb_rotate_left(parent, root);
                other = parent->RightChild;
            }
            if ((!other->LeftChild || rb_is_black(other->LeftChild)) &&
                (!other->RightChild || rb_is_black(other->RightChild)))
            {
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else
            {
                if (!other->RightChild || rb_is_black(other->RightChild))
                {
                    rb_set_black(other->LeftChild);
                    rb_set_red(other);
                    __rb_rotate_right(other, root);
                    other = parent->RightChild;
                }
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->RightChild);
                __rb_rotate_left(parent, root);
                node = root->BalancedRoot.RightChild;
                break;
            }
        }
        else
        {
            other = parent->LeftChild;
            if (rb_is_red(other))
            {
                rb_set_black(other);
                rb_set_red(parent);
                __rb_rotate_right(parent, root);
                other = parent->LeftChild;
            }
            if ((!other->LeftChild || rb_is_black(other->LeftChild)) &&
                (!other->RightChild || rb_is_black(other->RightChild)))
            {
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else
            {
                if (!other->LeftChild || rb_is_black(other->LeftChild))
                {
                    rb_set_black(other->RightChild);
                    rb_set_red(other);
                    __rb_rotate_left(other, root);
                    other = parent->LeftChild;
                }
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                rb_set_black(other->LeftChild);
                __rb_rotate_right(parent, root);
                node = root->BalancedRoot.RightChild;
                break;
            }
        }
    }
    if (node)
        rb_set_black(node);
}
```

#### 增删操作的移植

对于增加结点操作，MiInsertNode 实际上除了将插入新结点之外，还需要先找到插入结点所在的位置，而 rb\_insert\_color 则只包含了插入操作，因此需要保留原本的查找部分的代码。

其中 while 循环之内的代码（70 行后）由 rb\_insert\_color 中移植而来，除了替换之外没有做其他的更改，而之前的部分除了保留原本的查找结点步骤之外，还将原本的父节点赋值改用 rb\_set\_parent，从而确保其不出错。所有的检查 ASSERT 均保留，在调试时发现没有影响。

```C
VOID
FASTCALL
MiInsertNode (
    IN PMMADDRESS_NODE node,
    IN PMM_AVL_TABLE Table
    )

{
    //
    // Holds a pointer to the node in the table or what would be the
    // parent of the node.
    //

	PMMADDRESS_NODE parent, gparent;
    PMMADDRESS_NODE NodeOrParent;
    TABLE_SEARCH_RESULT SearchResult;

    ASSERT((Table->NumberGenericTableElements >= MiWorstCaseFill[Table->DepthOfTree]) &&
           (Table->NumberGenericTableElements <= MiBestCaseFill[Table->DepthOfTree]));

    SearchResult = MiFindNodeOrParent (Table,
                                       node->StartingVpn,
                                       &NodeOrParent);

    ASSERT (SearchResult != TableFoundNode);

    //
    // The node wasn't in the (possibly empty) tree.
    //
    // We just check that the table isn't getting too big.
    //

    ASSERT (Table->NumberGenericTableElements != (MAXULONG-1));

    node->LeftChild = NULL;
    node->RightChild = NULL;

    Table->NumberGenericTableElements += 1;

    //
    // Insert the new node in the tree.
    //

    if (SearchResult == TableEmptyTree) {

        Table->BalancedRoot.RightChild = node;
		rb_set_parent(node,&Table->BalancedRoot);
        // node->u1.Parent = &Table->BalancedRoot;
        ASSERT (node->u1.Balance == 0);
        ASSERT(Table->DepthOfTree == 0);
        Table->DepthOfTree = 1;

    ASSERT((Table->NumberGenericTableElements >= MiWorstCaseFill[Table->DepthOfTree]) &&
           (Table->NumberGenericTableElements <= MiBestCaseFill[Table->DepthOfTree]));

    }
    else {

        if (SearchResult == TableInsertAsLeft) {
            NodeOrParent->LeftChild = node;
        }
        else {
            NodeOrParent->RightChild = node;
        }

        rb_set_parent(node,NodeOrParent);

        node->u1.Balance = RB_RED;

		while ((parent = rb_parent(node)) && rb_is_red(parent))
		{
			gparent = rb_parent(parent);

			if (parent == gparent->LeftChild)
			{
				{
					register PMMADDRESS_NODE uncle = gparent->RightChild;
					if (uncle && rb_is_red(uncle))
					{
						rb_set_black(uncle);
						rb_set_black(parent);
						rb_set_red(gparent);
						node = gparent;
						continue;
					}
				}

				if (parent->RightChild == node)
				{
					register PMMADDRESS_NODE tmp;
					__rb_rotate_left(parent, Table);
					tmp = parent;
					parent = node;
					node = tmp;
				}

				rb_set_black(parent);
				rb_set_red(gparent);
				__rb_rotate_right(gparent, Table);
			} else {
				{
					register PMMADDRESS_NODE uncle = gparent->LeftChild;
					if (uncle && rb_is_red(uncle))
					{
						rb_set_black(uncle);
						rb_set_black(parent);
						rb_set_red(gparent);
						node = gparent;
						continue;
					}
				}

				if (parent->LeftChild == node)
				{
					register PMMADDRESS_NODE tmp;
					__rb_rotate_right(parent, Table);
					tmp = parent;
					parent = node;
					node = tmp;
				}

				rb_set_black(parent);
				rb_set_red(gparent);
				__rb_rotate_left(gparent, Table);
			}
		}

		rb_set_black(Table->BalancedRoot.RightChild);
    }

    //
    // Sanity check tree size and depth.
    //

    ASSERT((Table->NumberGenericTableElements >= MiWorstCaseFill[Table->DepthOfTree]) &&
           (Table->NumberGenericTableElements <= MiBestCaseFill[Table->DepthOfTree]));

    return;
}
```

而删节点操作相比起来就比较简单，由于指定了节点，只需要进行简单的替换即可。其中前面移植过来的删除节点颜色操作 \_\_rb\_erase\_color 就在这里使用。

```C
static void __rb_erase_color(PMMADDRESS_NODE node, PMMADDRESS_NODE parent,
PMM_AVL_TABLE root)
{
PMMADDRESS_NODE other;

while ((!node || rb_is_black(node)) && node != root->BalancedRoot.RightChild)
{
if (parent->LeftChild == node)
{
other = parent->RightChild;
if (rb_is_red(other))
{
   rb_set_black(other);
   rb_set_red(parent);
   __rb_rotate_left(parent, root);
   other = parent->RightChild;
}
if ((!other->LeftChild || rb_is_black(other->LeftChild)) &&
   (!other->RightChild || rb_is_black(other->RightChild)))
{
   rb_set_red(other);
   node = parent;
   parent = rb_parent(node);
}
else
{
   if (!other->RightChild || rb_is_black(other->RightChild))
   {
       rb_set_black(other->LeftChild);
       rb_set_red(other);
       __rb_rotate_right(other, root);
       other = parent->RightChild;
   }
   rb_set_color(other, rb_color(parent));
   rb_set_black(parent);
   rb_set_black(other->RightChild);
   __rb_rotate_left(parent, root);
   node = root->BalancedRoot.RightChild;
   break;
}
}
else
{
other = parent->LeftChild;
if (rb_is_red(other))
{
   rb_set_black(other);
   rb_set_red(parent);
   __rb_rotate_right(parent, root);
   other = parent->LeftChild;
}
if ((!other->LeftChild || rb_is_black(other->LeftChild)) &&
   (!other->RightChild || rb_is_black(other->RightChild)))
{
   rb_set_red(other);
   node = parent;
   parent = rb_parent(node);
}
else
{
   if (!other->LeftChild || rb_is_black(other->LeftChild))
   {
       rb_set_black(other->RightChild);
       rb_set_red(other);
       __rb_rotate_left(other, root);
       other = parent->LeftChild;
   }
   rb_set_color(other, rb_color(parent));
   rb_set_black(parent);
   rb_set_black(other->LeftChild);
   __rb_rotate_right(parent, root);
   node = root->BalancedRoot.RightChild;
   break;
}
}
}
if (node)
rb_set_black(node);
}
```

