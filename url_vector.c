#include "url_vector.h"

bool push(URL_VECTOR* p_vec, char* p_url) {
	if (p_vec->size >= MAX_SIZE) { 
		printf("	MAX_SIZE Exceeded ***\n***\n***\n");
		for (int i = 0; i < 1000; i++) {
			printf("**\n");
		}
		return false;
	}

	p_vec->urls[p_vec->size] = p_url;
	++p_vec->size;

	return true;
}

char* pop(URL_VECTOR* p_vec) {
	if (p_vec->size == 0) return NULL;

	char* url = p_vec->urls[p_vec->size-1];
	p_vec->urls[p_vec->size] = NULL;
	--p_vec->size;
	
	return url;
}

