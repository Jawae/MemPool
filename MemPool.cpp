#include <iostream>
#include "MemPool.h"
#include <assert.h>



MemPool::MemPool(int pool_size, int chunk_size)
{
	void* p = malloc(pool_size);
    
	assert(p != NULL);

	pMemPool_Start = (chunk_hdr_t*)p;

	ChunkSize = chunk_size;
	PoolSize = pool_size;
	TotalChunkNum = PoolSize / ChunkSize;
	FreeMemSize = PoolSize;
	UsedMemSize = 0;
	Alloced_Chunk_Count = 0;
	pBusyLinkListHead = NULL;
	pFreeLinkListHead = pMemPool_Start;
	pBusyLinkListTail = NULL;
	data_offset = ChunkSize - sizeof(chunk_hdr_t);

	chunk_hdr_t* cur_pos = pMemPool_Start;
	int i = 1;
	//空闲链表初始化
	while (i < TotalChunkNum)
	{
		chunk_hdr_t* pHdr = cur_pos;
		chunk_hdr_t* NextChunk = (chunk_hdr_t*)((char*)(pHdr + 1) + data_offset);  //指向下一块hdr的地址
		pHdr->chunk_id = i++;
		pHdr->used_flag = false;
		pHdr->pBusyNext = NULL;
		pHdr->pFreeNext = NextChunk;
		cur_pos = NextChunk;
		FreeLinkListLen++;
	}

	if (i == TotalChunkNum)  //处理最后一块chunk
	{
		pFreeLinkListTail = cur_pos;
		chunk_hdr_t* pHdr = cur_pos;
		pHdr->chunk_id = i;
		pHdr->used_flag = false;
		pHdr->pFreeNext = NULL;
		pHdr->pBusyNext = NULL;
		FreeLinkListLen++;

	}

}

MemPool::~MemPool()
{
	free(pMemPool_Start);
}

void* MemPool::AllocMem(int Memsize)
{
	if (Memsize > DataSize || Memsize > FreeMemSize)
	{
		LOG("Use 'malloc' to alloc memory!\n ");
		void* m = malloc(Memsize);
		return m;
	}


	chunk_hdr_t* start = pFreeLinkListHead;
	chunk_hdr_t* next = start->pFreeNext;
	//对该数据块区域的使用标志置1,更新空闲链表
	start->used_flag = 1;
	pFreeLinkListHead = next;


	/*更新busy链表,将已分配的chunk插入到链表尾*/
	//如果busy链表为空的
	if (pBusyLinkListHead == NULL)
	{
		pBusyLinkListHead = start;
		pBusyLinkListTail = start;
	}
	else 
	{
		chunk_hdr_t* hdr = pBusyLinkListTail;
		hdr->pBusyNext = start;
		pBusyLinkListTail = start;
	}

	BusyLinkListLen++;
	FreeLinkListLen--;
	return start;
}


int MemPool::DestroyMemPool()
{
	free(pMemPool_Start);
	return 0;
}

int MemPool::ReleaseMem(void* p)
{
	if (p < (void*)pMemPool_Start && p < (void*)((char*)pMemPool_Start + PoolSize))
	{
		int offset = (char*)p - (char*)pMemPool_Start;  //地址偏移量
		int chunk_id = offset / ChunkSize;

		chunk_hdr_t* p = pBusyLinkListHead;
		chunk_hdr_t* pre;
		while (p)
		{
			if (p->chunk_id == chunk_id)
			{
				if (!p->used_flag)

				{
					LOG("This is a bad chunk! chunk id: %d\n");
					return -1;
				}

				LOG("Release chunk %d\n");
				pre->pBusyNext = p->pBusyNext;
				p->pBusyNext = NULL;
				pFreeLinkListTail->pFreeNext = p;
				p->pFreeNext = NULL;
				p->used_flag = 0;
			}
			pre = p;
			p = p->pBusyNext;

		}
	}
	else
	{
		free(p);
	}

	return 0;
}


int MemPool::GetBusyLinkListLength()
{
	return BusyLinkListLen;
}


int MemPool::GetFreeLinkListLength()
{
	return FreeLinkListLen;
}


int MemPool::EnlargeMemPoolSize(int add_size)
{
	if (add_size % 512 != 0)
	{
		LOG("增加的内存大小必须是512B的整数倍!\n");
		return -1;
	}

	chunk_hdr_t* p = (chunk_hdr_t*)malloc(add_size);
	if (p == NULL)
	{
		LOG("fail to new molloc memory！\n");
		return -1;
	}

	PoolSize += add_size;
	int chunk_num = add_size / 512;

	//初始化新的chunk
	int count = TotalChunkNum+1;
	pFreeLinkListTail->pFreeNext = p;
	chunk_hdr_t* cur_pos = p;
	//空闲链表加入新的chunk
	while (count < TotalChunkNum + chunk_num)
	{
		chunk_hdr_t* pHdr = cur_pos;
		chunk_hdr_t* NextChunk = (chunk_hdr_t*)((char*)(pHdr + 1) + data_offset);  //指向下一块hdr的地址
		pHdr->chunk_id = count++;
		pHdr->used_flag = false;
		pHdr->pBusyNext = NULL;
		pHdr->pFreeNext = NextChunk;
		cur_pos = NextChunk;
		FreeLinkListLen++;
	}

	if (count == TotalChunkNum + chunk_num)  //处理最后一块chunk
	{
		pFreeLinkListTail = cur_pos;
		chunk_hdr_t* pHdr = cur_pos;
		pHdr->chunk_id = count;
		pHdr->used_flag = false;
		pHdr->pFreeNext = NULL;
		pHdr->pBusyNext = NULL;
		FreeLinkListLen++;

	}

	TotalChunkNum += chunk_num;
	FreeMemSize += add_size;


	return 0;
}



void MemPool::MemPool_info_dump()
{
	LOG("PoolSize: %d\n", PoolSize);
	LOG("ChunkSize: %d\n", ChunkSize);
	LOG("DataSize: %d\n", DataSize);
	LOG("TotalChunkNum: %d\n", TotalChunkNum);
	LOG("FreeMemSize: %d\n", FreeMemSize);
	LOG("UsedMemSize: %d\n", UsedMemSize);
	LOG("Alloced_Chunk_Count: %d\n", Alloced_Chunk_Count);
	LOG("data_offset: %d\n", data_offset);
	LOG("BusyLinkListLen: %d\n", BusyLinkListLen);
	LOG("FreeLinkListLen: %d\n", FreeLinkListLen);
}