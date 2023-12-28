#include <stdbool.h>
#include <curl/curl.h>
//#include <curl/multi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>

#include <time.h>
#include <unistd.h>

#include "http_search.h"
#include "recv_buf.h"
#include "url_vector.h"

#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */
#define ECE252_HEADER "X-Ece252-Fragment: "

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })



#define URL_HEAD "http://ece252-1.uwaterloo.ca/lab4"
#define URL_MAIN "http://ece252-1.uwaterloo.ca/~yqhuang/lab4/"
#define URL_CURL_ERR "http://ece252-2.uwaterloo.ca:2542/image?q=gAAAAABdHlHZmbxmnenlxhndlipqfntjijjammhualsfjfkbgruhpprimngcjssinigxalforazyxxmsbuhxcsqwstfveoonvi=="
#define URL_HTTP_ERR "http://ece252-2.uwaterloo.ca:2540/image?q=gAAAAABdHlHZdbvevhbqhxzduixyawuowrfdsewzujoijxfjdroglulfpmdurqvwpvppgdntcpabflmrhhiepsjlefolalxwur=="
#define URL_PNG "http://ece252-3.uwaterloo.ca:2540/image?q=gAAAAABdHkoqOKR-cFRCkiCUBEMAAAWfDvBFlRisL9ysLWHYHbcQbn1b28PV_uHBZ0gJf5bvzrnf1HNXxB6KRlAVETwTIqBH2Q=="

#define URL_INIT URL_HEAD


static void init(CURLM *p_mh, CURL* p_easy_handle, RECV_BUF* p_handle_data);
static size_t data_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
static size_t header_callback(char* ptr, size_t size, size_t nmemb, void* userdata);

URL_VECTOR log_vec = { .size = 0 };
URL_VECTOR frontier_vec = { .size = 0 };
URL_VECTOR png_vec = { .size = 0 };


RECV_BUF* recv_buf_arr;

unsigned png_count = 0;
unsigned max_png_count = 50;

int current_running_handles = 0;
int previous_running_handles = 0;

int main(int argc, char** argv) {
	double times[2];
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
	times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);


	int c;
	int t = 1; // ** 1 **
	int m = 50;
	char* p_v = NULL;
	char *str = "option requires an argument";

	bool log_v = false;
	int args = 1;
   
	while ((c = getopt (argc, argv, "t:m:v:")) != -1) {
		switch (c) {
			case 't':
		    t = strtoul(optarg, NULL, 10);
			args += 2;
		    printf("option -t specifies a value of %d.\n", t);
			if (t <= 0) {
			    fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
				return -1;
	        }
		        break;
			case 'm':
				m = strtoul(optarg, NULL, 10);
				args += 2;
				printf("option -m specifies a value of %d.\n", m);
		        if (m <= 0) {
			        fprintf(stderr, "%s: %s 1, 2, or 3 -- 'm'\n", argv[0], str);
				    return -1;
	            }
		        break;
			case 'v':
				if (optind - 1 < argc) {
					p_v = argv[optind - 1];
					log_v = true;
					args += 2;
					printf("option -v specifies a value of %s.\n", p_v);
				}
			default:
				continue;
        }
    }
	max_png_count = m;
	printf("max_png_count: %d\n", max_png_count);
	char* p_head = NULL;

	if (args < argc) {
		printf("URL: %s\n", argv[args]);
		p_head = (char*)malloc((strlen(argv[args]) + 1)*sizeof(char));
		p_head[strlen(argv[args])] = '\n';
		strcpy(p_head, argv[args]);
		printf("%s\n", p_head);
	} else {
		printf("DEFAULT\n");
		p_head = (char*)malloc((strlen(URL_HEAD) + 1)*sizeof(char));
		p_head[strlen(URL_HEAD)] = '\n';
		strcpy(p_head, URL_HEAD);
		printf("%s\n", p_head);
	}

	recv_buf_arr = (RECV_BUF*)malloc(t * sizeof(RECV_BUF));

	CURL* easy_handle_arr[t];
	CURLM* p_multi_handle;

	curl_global_init(CURL_GLOBAL_ALL);
	p_multi_handle = curl_multi_init();
	xmlInitParser();

	// create handles
	for (int i = 0; i < t; i++) {
		easy_handle_arr[i] = curl_easy_init();
		if (!easy_handle_arr[i]) { printf("CURL INIT ERR\n"); return -1; }
	}

	// push head
	//p_head = (char*)malloc((strlen(URL_INIT) + 1)*sizeof(char));
	//p_head[strlen(URL_INIT)] = '\n';
	//strcpy(p_head, URL_INIT);
	push(&frontier_vec, p_head);

	for (int i = 0; i < t; i++) {
		init(p_multi_handle, easy_handle_arr[i], &recv_buf_arr[i]);
		recv_buf_arr[i].buf = (char*)malloc(BUF_SIZE * sizeof(char));
		recv_buf_arr[i].size = 0;
		recv_buf_arr[i].max_size = BUF_SIZE;
	}
	
	while (frontier_vec.size > 0 || current_running_handles > 0 || previous_running_handles > 0) {
		if (png_count >= max_png_count) { break; }
		curl_multi_wait(p_multi_handle, NULL, 0, 1000, NULL);
		CURLMsg* p_msg;
		int msgs_left;
		CURL* p_handle;
		RECV_BUF* p_recv_buf;
		char* p_URL = NULL;

		while ((p_msg = curl_multi_info_read(p_multi_handle, &msgs_left)) && png_count <= max_png_count) {
			if (p_msg->msg != CURLMSG_DONE) { continue; }
			p_handle = p_msg->easy_handle;
			p_recv_buf = NULL;
			curl_easy_getinfo(p_handle, CURLINFO_PRIVATE, &p_recv_buf);
			
			if (p_recv_buf) {
				process_data(p_handle, p_recv_buf, &png_vec,
							 &frontier_vec, &png_count, max_png_count);
				//free(p_recv_buf->buf);
				//p_recv_buf->buf = NULL;
				p_recv_buf->size = 0;
			}

__get_URL:
			p_URL = pop(&frontier_vec);
			if (p_URL) {
				for (int j = 0; j < log_vec.size; j++) {
					if (!strcmp(log_vec.urls[j], p_URL)) {
						free(p_URL);
						goto __get_URL;
					}
				}
			}
			
			if (p_URL) {
				push(&log_vec, p_URL);
				curl_multi_remove_handle(p_multi_handle, p_handle);
				curl_easy_setopt(p_handle, CURLOPT_URL, p_URL);
				curl_multi_add_handle(p_multi_handle, p_handle);
			} else {
				curl_multi_remove_handle(p_multi_handle, p_handle);
				curl_easy_setopt(p_handle, CURLOPT_URL, "");
				curl_multi_add_handle(p_multi_handle, p_handle);
			}
		}
		
		previous_running_handles = current_running_handles;
		curl_multi_perform(p_multi_handle, &current_running_handles);	
	}

	printf("Log Count %u\n", log_vec.size);
	printf("PNG Count %u\n", png_count);

	// Create Files
	FILE* p_png_urls = fopen("png_urls.txt", "w");
	if (!p_png_urls) { exit(1); }

	for (unsigned i = 0; i < png_vec.size; i++) {
		if (i == max_png_count) { break; }
		fprintf(p_png_urls, "%s\n", png_vec.urls[i]);
	}
	fclose(p_png_urls);

	if (log_v){
		FILE* p_log_file = fopen(p_v, "w");
		if (!p_log_file) { exit(1); }
		
		for (unsigned i = 0; i < log_vec.size; i++)
			fprintf(p_log_file, "%s\n", log_vec.urls[i]);
		fclose(p_log_file);
		printf("Logged to: %s\n", p_v);
	} else {
		printf("Did Not Log\n");
	}




	// Cleanup
	for (int i = 0; i < log_vec.size; i++) {
		free(log_vec.urls[i]);
		log_vec.urls[i] = NULL;
	}
	for (int i = 0; i < png_vec.size; i++) {
		free(png_vec.urls[i]);
		png_vec.urls[i] = NULL;
	}
	for (int i = 0; i < frontier_vec.size; i++) {
		free(frontier_vec.urls[i]);
		frontier_vec.urls[i] = NULL;
	}

	for (int i = 0; i < t; i++) {
		curl_easy_cleanup(easy_handle_arr[i]);
		free(recv_buf_arr[i].buf);
		recv_buf_arr[i].buf = NULL;
	}
	free(recv_buf_arr);
	
	xmlCleanupParser();
    curl_multi_cleanup(p_multi_handle);
	curl_global_cleanup();



	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("paster2 execution time: %.6lf seconds\n", elapsed_time);

	if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;


	return 0;
}

static void init(CURLM* p_mh, CURL* p_easy_handle, RECV_BUF* p_recv_buf) {
	curl_easy_setopt(p_easy_handle, CURLOPT_HEADERFUNCTION, header_callback);
	curl_easy_setopt(p_easy_handle, CURLOPT_HEADERDATA, p_recv_buf);
	curl_easy_setopt(p_easy_handle, CURLOPT_WRITEFUNCTION, data_callback);
	curl_easy_setopt(p_easy_handle, CURLOPT_WRITEDATA, p_recv_buf);
	curl_easy_setopt(p_easy_handle, CURLOPT_PRIVATE, p_recv_buf);
	curl_easy_setopt(p_easy_handle, CURLOPT_HEADER, 0L);
//	curl_multi_perform(p_mh, &running_handles);
	curl_easy_setopt(p_easy_handle, CURLOPT_VERBOSE, 0L);

    curl_easy_setopt(p_easy_handle, CURLOPT_USERAGENT, "ece252 lab4 crawler");
    curl_easy_setopt(p_easy_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(p_easy_handle, CURLOPT_UNRESTRICTED_AUTH, 1L);
    curl_easy_setopt(p_easy_handle, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(p_easy_handle, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(p_easy_handle, CURLOPT_COOKIEFILE, "");
    curl_easy_setopt(p_easy_handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    curl_easy_setopt(p_easy_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

	curl_multi_add_handle(p_mh, p_easy_handle);


}

static size_t data_callback(char* p_recv, size_t size, size_t nmemb, void* userdata) {
	size_t total_size = size * nmemb;
	RECV_BUF* p = (RECV_BUF*)userdata;

	if (p->size + total_size + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, (int)total_size + 1);   
        char *q = (char*)realloc(p->buf, new_size);
        if (q == NULL) {
            perror("realloc"); /* out of memory */
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, total_size); /*copy data from libcurl*/
    p->size += total_size;
    p->buf[p->size] = 0;

	return total_size;
}

static size_t header_callback(char* p_recv, size_t size, size_t nmemb, void* userdata) {
	size_t total_size = size * nmemb;
	RECV_BUF* p = (RECV_BUF*)userdata;
	if (total_size > strlen(ECE252_HEADER) &&
		strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0)
	{
		p->seq = atoi(p_recv + strlen(ECE252_HEADER));
    }
    return total_size;
}



