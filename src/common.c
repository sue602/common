/*
 * common.c
 *
 *  Created on: Aug 18, 2016
 *      Author: ltzd
 */
#include <stdlib.h>
#include <stdio.h>
#include "zmalloc.h"
#include "spinlock.h"
#include "adlist.h"
#include "common.h"

//easy_malloc,easy_realloc,size can't be grater than 2^24 - 1
typedef uint32_t common_uint32;
#define MAKE_PREFIX_SIZE(t,sz) ( (t<<24) | sz )
#define PRE_TYPE(makesize) ( makesize>>24 )
#define PRE_SIZE(makesize) (makesize & 0x00ffffff )

static short g_common_inited = 0;

const int MALLOC_NUMBERS = 1000;

const short CHAR_SIZE = sizeof(char);
const short COMMON_PRE_SIZE = sizeof(common_uint32);

const short BASE_2N	= 16;

enum buf_type{
	BUF16			= 0,
	BUF32			= 1,
	BUF64			= 2,
	BUF128			= 3,
	BUF256			= 4,
	BUF512			= 5,
	BUF1K 			= 6,
	BUF2K 			= 7,
	BUF4K 			= 8,
	BUF8K 			= 9,
	BUF16K 			= 10,
	BUF32K			= 11,
	BUF64K			= 12,
	BUF128K			= 13,
	BUF_TYPE_MAX	= 14
};

struct abuf{
	struct spinlock lock;
	list * data;
};

static struct abuf g_bufs[BUF_TYPE_MAX-1];

void common_init()
{
	if(1 == g_common_inited)
	{
		printf("common init already \n");
		return;
	}
	printf("size_t =%d \n",COMMON_PRE_SIZE);
	g_common_inited = 1;

	short i=0;
	int sz = BASE_2N;
	for(;i<BUF_TYPE_MAX;i++)
	{
		g_bufs[i].data = listCreate();
		short j = 0;
		short weight = MALLOC_NUMBERS / (i+1) ;
		for(;j<weight;j++)
		{
			void * allocbuf = zmalloc(sz);//2^N
			*( (common_uint32*)allocbuf) = MAKE_PREFIX_SIZE(i,sz);
			listAddNodeTail(g_bufs[i].data, allocbuf );
		}
		sz = sz * 2;
	}
}

void common_fini()
{
	int i=0;
	for(;i<BUF_TYPE_MAX;i++)
	{
		SPIN_LOCK(&g_bufs[i]);
		//设置释放函数
		g_bufs[i].data->free = zfree;
		//release
		listRelease(g_bufs[i].data);

		SPIN_UNLOCK(&g_bufs[i])
	}
	g_common_inited = 0;
}

static enum buf_type _check_sz(uint32_t sz)
{
	enum buf_type buft = BUF16;
	unsigned int test_sz = BASE_2N;
	do
	{
		if(test_sz >= sz)
			break;
		test_sz = test_sz * 2;
		buft = buft + 1;
	}while(1);
	return buft;
}

void * easy_malloc(uint32_t sz)
{
	enum buf_type buft = _check_sz(sz+COMMON_PRE_SIZE);//预先找最合适大小

	SPIN_LOCK(&g_bufs[buft]);

	void * buf = NULL;
	if(0 == listLength(g_bufs[buft].data))
	{
		unsigned int malloc_sz = (buft+1) * BASE_2N;
		void * allocbuf = zmalloc(malloc_sz);//2^N
		*( (common_uint32*) allocbuf ) = MAKE_PREFIX_SIZE(buft,sz);
		listAddNodeTail(g_bufs[buft].data,allocbuf);
	}
	listNode * node = listFirst(g_bufs[buft].data);
	buf = node->value;
	*( (common_uint32*) (buf) ) = MAKE_PREFIX_SIZE(buft,sz);
	if(NULL == buf)
	{
		printf("easy malloc error ,size =%d\n",sz);
		exit(0);
	}
	listDelNode(g_bufs[buft].data,node);
	SPIN_UNLOCK(&g_bufs[buft]);
//	printf("easy malloc %p ,type=%d ,sz=%d make int=%d \n",buf,buft,sz,MAKE_PREFIX_SIZE(buft,sz));
	return buf+COMMON_PRE_SIZE;
}

void easy_free(void * p)
{
	common_uint32 pre_data = * ( (common_uint32*)(p-COMMON_PRE_SIZE) );
	short type = PRE_TYPE(pre_data);
	size_t oldsize = PRE_SIZE(pre_data);
	if(type < 0 || type >=BUF_TYPE_MAX )
	{
		printf("easy free error,size=%d,p=%p",type,p);
		exit(0);
	}
	enum buf_type buft = type ;

	SPIN_LOCK(&g_bufs[buft]);

	printf("easy free %p ,size=%d,type=%d pre_data=%d\n",p,oldsize,type,pre_data);
	//printf("before free=%d\n",listLength(g_bufs[buft].data));

	//listAddNodeTail(g_bufs[buft].data,p-COMMON_PRE_SIZE);
	listAddNodeHead(g_bufs[buft].data,p-COMMON_PRE_SIZE);
	//printf("free add=%d\n\n",listLength(g_bufs[buft].data));

	SPIN_UNLOCK(&g_bufs[buft]);
}

void * easy_realloc(void * p,uint32_t sz)
{
	if (NULL == p)
		return easy_malloc(sz);

	if(0 == sz)
	{
		easy_free(p);
		return NULL;
	}

	common_uint32 pre_data = * ( (common_uint32*)(p-COMMON_PRE_SIZE) );
	short type = PRE_TYPE(pre_data);
	size_t oldsize = PRE_SIZE(pre_data);
	if(type < 0 || type >=BUF_TYPE_MAX )
	{
		printf("easy free error,size=%d,p=%p",type,p);
		exit(0);
	}
	enum buf_type buft = type ;

	unsigned int malloc_sz = (buft+1) * BASE_2N;
	if(sz > oldsize && sz > malloc_sz )
	{
		void * oldp = p;
		void * allocbuf = easy_malloc(sz);
		memcpy(allocbuf,p,oldsize);//copy data
		easy_free(oldp);//free p
		p = allocbuf;
	}
	return p;
}
