#include "queue.h"
#include "memory.h"

  
/*1. ��ʼ��������*/  
// ��  ��ʼ���� �������ǳ�ʼ�����еĶ�ͷ�Ͷ�β��������־λ��
// ���Ծ���ɾ�����ǲ����ʱ�򣬻��ж���û�� ����Ϊ�յ�ʱ��  
void initQueue(queue_t * queue_eg)  
{  
	InitializeCriticalSection(&queue_lock);
    queue_eg->head = NULL; //��ͷ��־λ  
    queue_eg->tail = NULL; //��β��־λ  
}  
  
/*2.�����ӵ�<span style="background-color: rgb(255, 102, 102);">��β����</span>һ��Ԫ��x*/  
int enQueue(queue_t *hq, elemType *x)  
{  
	EnterCriticalSection(&queue_lock);
	if(hq == NULL)
		return 0;

    node_t * new_p = new node_t();//(node_t *)malloc(sizeof(queue_t));  
	
    if (new_p == NULL )  
    {  
		LeaveCriticalSection(&queue_lock);
        return 0;
    }  
    new_p->data = x;  
    new_p->next = NULL;  
    if (hq->head == NULL)  
    {  
        hq->head = new_p;  
        hq->tail = new_p;  
    } else {  
        hq->tail->next = new_p;  
        hq->tail = new_p;  
    }  
	
	memory_add();
	LeaveCriticalSection(&queue_lock);
    return 1;  
}  
  
/*3. ���ж���<span style="background-color: rgb(255, 153, 102);">����ɾ��</span>һ��Ԫ��*/  
void outQueue(queue_t * hq,elemType *temp)  
{  EnterCriticalSection(&queue_lock);
    node_t * p;  
   // elemType temp;//=new elemType();  queue_times++;
    if (hq->head == NULL)  
    {  
       // printf("����Ϊ�գ�����ɾ����");  
       // exit(1);  
		//return;// NULL;
		temp=NULL;LeaveCriticalSection(&queue_lock);
		return;//0;
    }  

    temp = hq->head->data;  
    p = hq->head;  
    hq->head = hq->head->next;  
    if(hq->head == NULL)  
    {  
        hq->tail = NULL;  
    }  
    free(p);  
	memory_sub();
	LeaveCriticalSection(&queue_lock);
}  
  
/*4. ��ȡ����Ԫ�� */  
elemType *peekQueue(queue_t * hq)  
{  
    if (hq->head == NULL)  
    {  
        printf("����Ϊ�ա�");  
        exit(1);  
    }   
    return hq->head->data;  
}  
  
/*5. �������Ƿ�Ϊ�գ����ǿշ���1������Ϊ�շ���0 ��*/  
int is_emptyQueue(queue_t * hq)  
{  
    if (hq->head == NULL)  
    {  
        return 1;  
    } else {  
        return 0;  
    }  
}  
  
/*6. ��������е�����Ԫ��*/  
  
void clearQueue(queue_t * hq)  
{  
    node_t * p = hq->head;  
    while(p != NULL)  
    {  
        hq->head = hq->head->next;  
        free(p);  memory_sub("sub memory clearQueue 93\r\n");
        p = hq->head;  
    }  
    hq->tail = NULL;  
    return;  
}  
  
/*main()����*/  
/*int main(int argc, char* argv[])  
{  
    queue_t q;  
    int a[8] = {1, 2, 3, 4, 5, 6, 7, 8};  
    int i;  
    initQueue(&q);  
    for(i=0; i<8; i++)  
    {  
        enQueue(&q, a[i]);  
    }  
    //printf("%d",outQueue(&q));  
  
    enQueue(&q, 68);  
    //printf("%d", peekQueue(&q));  
      
    while (!is_emptyQueue(&q))  
    {  
        printf("%d.\n", outQueue(&q));  
    }  
  
    printf(" \n");  
    clearQueue(&q);  
    system("pause");  
    system("pause");  
    system("pause");  
}  */