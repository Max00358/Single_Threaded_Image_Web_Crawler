#include <stdio.h>

#ifndef RECV_BUF_H
#define RECV_BUF_H

typedef struct recv_buf2 {
	char *buf;       /* memory to hold a copy of received data */
	size_t size;     /* size of valid data in buf in bytes*/
	size_t max_size; /* max capacity of buf in bytes*/
	int seq;         /* >=0 sequence number extracted from http header */
} RECV_BUF;


#endif // RECV_BUF_H

