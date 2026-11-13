template <class Type>
struct TreeNode
{
    Type data;

    TreeNode* leftChild;
    TreeNode*rightChild;
    TreeNode*current;

    int leftThread;
    int rightThread;/*标志位	值	对应指针的含义
                  leftThread	0	leftChild 指向左孩子
                  leftThread	1	leftChild 指向中序前驱
                 rightThread	0	rightChild 指向右孩子
                 rightThread	1	rightChild 指向中序后继*/
}
Node*next()
{
    Node*p=current->rightChild;
    if(current->rightThread==0)
    {
        while(p->leftThread==0)
        {
            p=p->leftChild;
        }
    }
    current=p;
    return (current==root)?nullptr:current;
}
void InThread(TreeNode*current,TreeNode*&pre)
{
    if(current!=nullptr) return;
    InThread(current->leftChild,pre);
    if(current->leftChild==nullptr)
    {
            current->leftChild=pre;//建立前驱联系
            current->leftThread=1;
    }
    if(pre->rightChild==nullptr)
    {
            pre->rightChild=current;//建立后继联系
            pre->rightThread=1;
    }
    pre=current;
    InThread(current->rightChild,pre);
}
void CreatInThread()
{
    TreeNode*pre;
    Troot=new TreeNode<Type>;
    Troot->leftChild=this->root;
    pre=Troot;
}
