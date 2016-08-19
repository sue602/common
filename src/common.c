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

static short g_common_inited = 0;

const int MALLOC_NUMBERS = 1000;

const short CHAR_SIZE = sizeof(char);

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
			void * buf = zmalloc(sz+CHAR_SIZE);
			*( (char*)buf) = (short) i;
			listAddNodeTail(g_bufs[i].data, (buf + CHAR_SIZE) );
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
		//release
		listRelease(g_bufs[i].data);

		SPIN_UNLOCK(&g_bufs[i])
	}
}

static enum buf_type _check_sz(unsigned int sz)
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

void * easy_malloc(unsigned int sz)
{
	enum buf_type buft = _check_sz(sz);

	SPIN_LOCK(&g_bufs[buft]);

	void * buf = NULL;
	if(0 == listLength(g_bufs[buft].data))
	{
		unsigned int malloc_sz = (buft+1) * BASE_2N;
		buf = zmalloc(malloc_sz+CHAR_SIZE);
		*( (char*)buf) = (short) buft;
		listAddNodeTail(g_bufs[buft].data,(buf + CHAR_SIZE) );
	}
	listNode * node = listFirst(g_bufs[buft].data);
	buf = node->value;
	if(NULL == buf)
	{
		printf("easy malloc error ,size =%d\n",sz);
		exit(0);
	}
	listDelNode(g_bufs[buft].data,node);
	SPIN_UNLOCK(&g_bufs[buft]);
	//printf("easy malloc %p ,type=%d\n",buf,buft);
	return buf;
}

void easy_free(void * p)
{
	short type = * ( (char*)(p-CHAR_SIZE) );
	if(type < 0 || type >=BUF_TYPE_MAX )
	{
		printf("easy free error,size=%d,p=%p",type,p);
		exit(0);
	}
	enum buf_type buft = type ;

	SPIN_LOCK(&g_bufs[buft]);

	//printf("before free=%d\n",listLength(g_bufs[buft].data));

	listAddNodeTail(g_bufs[buft].data,p);

	//printf("free add=%d\n\n",listLength(g_bufs[buft].data));

	SPIN_UNLOCK(&g_bufs[buft]);
}
