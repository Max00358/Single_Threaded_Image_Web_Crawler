#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <curl/curl.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/uri.h>
#include <pthread.h>

#include "recv_buf.h"
#include "url_vector.h"

#define SEED_URL "http://ece252-1.uwaterloo.ca/lab4/"
#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576  /* 1024*1024 = 1M */
#define BUF_INC  524288   /* 1024*512  = 0.5M */

#define CT_PNG  "image/png"
#define CT_HTML "text/html"
#define CT_PNG_LEN  9
#define CT_HTML_LEN 9

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

//typedef struct recv_buf2 {
//    char *buf;       /* memory to hold a copy of received data */
//    size_t size;     /* size of valid data in buf in bytes*/
//    size_t max_size; /* max capacity of buf in bytes*/
//    int seq;         /* >=0 sequence number extracted from http header */
//                     /* <0 indicates an invalid seq number */
//} RECV_BUF;


htmlDocPtr mem_getdoc(char *buf, int size, const char *url);
xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath);
int find_http(
		char *fname,
		int size,
		int follow_relative_links,
		const char *base_url,
		URL_VECTOR* p_frontier_vec
);
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);
void cleanup(CURL *curl, RECV_BUF *ptr);
int write_file(const char *path, const void *in, size_t len);
CURL *easy_handle_init(RECV_BUF *ptr, const char *url);
bool check_PNG_header(char* buf, long int len);

int process_html(
		CURL *curl_handle,
		RECV_BUF *p_recv_buf,
		URL_VECTOR* p_frontier_vec
);

int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf,
        URL_VECTOR* p_png_vec,
        unsigned* p_png_counter,
        unsigned max_png_count
);

int process_data(CURL* curl_handle, RECV_BUF* p_recv_buf,
        URL_VECTOR* p_png_vec,
        URL_VECTOR* p_frontier_vec,
        unsigned* p_png_counter,
        unsigned max_png_count
);

void push_frontier(
        CURL* p_handle,
        char* p_ptr,
        size_t size,
        URL_VECTOR* p_png_vec,
        URL_VECTOR* p_frontier_vec,
        unsigned* p_png_count,
        unsigned max_png_count
);

htmlDocPtr mem_getdoc(char *buf, int size, const char *url) {
    int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | \
               HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
    htmlDocPtr doc = htmlReadMemory(buf, size, url, NULL, opts);

    if ( doc == NULL ) {
        fprintf(stderr, "Document not parsed successfully.\n");
        return NULL;
    }
    return doc;
}

xmlXPathObjectPtr getnodeset (xmlDocPtr doc, xmlChar *xpath)
{
	
    xmlXPathContextPtr context;
    xmlXPathObjectPtr result;

    context = xmlXPathNewContext(doc);
    if (context == NULL) {
        // printf("Error in xmlXPathNewContext\n");
        return NULL;
    }
    result = xmlXPathEvalExpression(xpath, context);
    xmlXPathFreeContext(context);
    if (result == NULL) {
        // printf("Error in xmlXPathEvalExpression\n");
        return NULL;
    }
    if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
        xmlXPathFreeObject(result);
        // printf("No result\n");
        return NULL;
    }
    return result;
}

int find_http(char *buf, int size, int follow_relative_links, const char *base_url,
			  URL_VECTOR* p_frontier_vec
) {

    int i;
    htmlDocPtr doc;
    xmlChar* xpath = (xmlChar*) "//a/@href";
    xmlNodeSetPtr nodeset;
    xmlXPathObjectPtr result;
    xmlChar* href;
	char* href_copy = NULL;
	size_t href_len = 0;
	bool pushed = false;

    if (buf == NULL) {
        return 1;
    }

    doc = mem_getdoc(buf, size, base_url);
    result = getnodeset (doc, xpath);
    if (result) {
		nodeset = result->nodesetval;
		
		for (i=0; i < nodeset->nodeNr; i++) {
            href = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
            if ( follow_relative_links ) {
                xmlChar *old = href;
                href = xmlBuildURI(href, (xmlChar *) base_url);
                xmlFree(old);
            }
            if ( href != NULL && !strncmp((const char *)href, "http", 4) ) {
				href_len = strlen((char*)href);
				href_copy = (char*)malloc((href_len + 1)*sizeof(char));
				href_copy[href_len] = '\0';
				strcpy(href_copy, (char*)href);

//				printf("        %s\n", href_copy);
					pushed = push(p_frontier_vec, href_copy);
					if (!pushed) {
						free(href_copy);
					}
            }
            xmlFree(href);
        }
        xmlXPathFreeObject (result);
    }
    xmlFreeDoc(doc);
	//xmlCleanupParser();
    return 0;
}
/**
 * @brief  cURL header call back function to extract image sequence number from 
 *         http header data. An example header for image part n (assume n = 2) is:
 *         X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structurea
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line 
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;

#ifdef DEBUG1_
    printf("%s", p_recv);
#endif /* DEBUG1_ */
    if (realsize > strlen(ECE252_HEADER) &&
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
	p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
    return realsize;
}


/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv, 
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */

size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata) {
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);   
        char *q = realloc(p->buf, new_size);
        if (q == NULL) {
			printf("NULL recv_buf\n");
			perror("realloc"); /* out of memory */
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}


int recv_buf_init(RECV_BUF *ptr, size_t max_size) {
    void *p = NULL;
    
    if (ptr == NULL) {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL) {
	return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be positive */
    return 0;
}

int recv_buf_cleanup(RECV_BUF *ptr) {
    if (ptr == NULL) {
	return 1;
    }
    
    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
    return 0;
}

void cleanup(CURL *curl, RECV_BUF *ptr) {
        curl_easy_cleanup(curl);
        //curl_global_cleanup();
        recv_buf_cleanup(ptr);
}
/**
 * @brief output data in memory to a file
 * @param path const char *, output file path
 * @param in  void *, input data to be written to the file
 * @param len size_t, length of the input data in bytes
 */

int write_file(const char *path, const void *in, size_t len) {
  //  FILE *fp = NULL;

    if (path == NULL) {
//        fprintf(stderr, "write_file: file name is null!\n");
        return -1;
    }

    if (in == NULL) {
    //    fprintf(stderr, "write_file: input data is null!\n");
        return -1;
    }

/*    fp = fopen(path, "wb");
    if (fp == NULL) {
        perror("fopen");
        return -2;
    }

    if (fwrite(in, 1, len, fp) != len) {
        fprintf(stderr, "write_file: imcomplete write!\n");
        return -3; 
    }
    return fclose(fp);
*/
	return 0;
}

/**
 * @brief create a curl easy handle and set the options.
 * @param RECV_BUF *ptr points to user data needed by the curl write call back function
 * @param const char *url is the target url to fetch resoruce
 * @return a valid CURL * handle upon sucess; NULL otherwise
 * Note: the caller is responsbile for cleaning the returned curl handle
 */

CURL *easy_handle_init(RECV_BUF *ptr, const char *url)
{
    CURL *curl_handle = NULL;

    if ( ptr == NULL || url == NULL) {
        return NULL;
    }

    /* init user defined call back function buffer */
    if ( recv_buf_init(ptr, BUF_SIZE) != 0 ) {
        return NULL;
    }
    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return NULL;
    }

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)ptr);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)ptr);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "ece252 lab4 crawler");
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_UNRESTRICTED_AUTH, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl_handle, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl_handle, CURLOPT_COOKIEFILE, "");
    curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_ANY);

    return curl_handle;
}

int process_html(CURL *curl_handle, RECV_BUF *p_recv_buf,
        URL_VECTOR* p_frontier_vec
) {
    char fname[256];
    int follow_relative_link = 1;
    char *url = NULL; 
    //pid_t pid = getpid();

    curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &url);
	find_http(p_recv_buf->buf, p_recv_buf->size, follow_relative_link, url, p_frontier_vec);

    // sprintf(fname, "./output_%d.html", pid);
    return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
}

int process_png(CURL *curl_handle, RECV_BUF *p_recv_buf,
		URL_VECTOR* p_png_vec,
		unsigned* p_png_counter,
		unsigned max_png_count
) {
    //pid_t pid = getpid();
    char fname[256];
    char* eurl = NULL;          /* effective URL */
	char* eurl_copy = NULL;
	size_t eurl_len = 0;
	bool is_PNG = false;
	bool pushed = false;

	curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &eurl);
    if ( eurl != NULL) {
		is_PNG = check_PNG_header(p_recv_buf->buf, p_recv_buf->size);
		
		if (is_PNG) {
			if (*p_png_counter >= max_png_count) {
				goto __process_png_exit;
			}
			++*p_png_counter;
			eurl_len = strlen(eurl);
			eurl_copy = (char*)malloc((eurl_len + 1)*sizeof(char));
			eurl_copy[eurl_len] = '\0';
			strcpy(eurl_copy, eurl);

				pushed = push(p_png_vec, eurl_copy);
			if (!pushed){
				free(eurl_copy);
			} else {
//				printf("    PNG Pushed: %s\n", eurl_copy);
			}
		}
  	}

//    sprintf(fname, "./output_%d_%d.png", p_recv_buf->seq, pid);
__process_png_exit:	
  	return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
}
/**
 * @brief process teh download data by curl
 * @param CURL *curl_handle is the curl handler
 * @param RECV_BUF p_recv_buf contains the received data. 
 * @return 0 on success; non-zero otherwise
 */

int process_data(CURL* curl_handle, RECV_BUF* p_recv_buf,
		URL_VECTOR* p_png_vec,
        URL_VECTOR* p_frontier_vec,
		unsigned* p_png_counter,
		unsigned max_png_count
) {
	//printf("	Process Data()\n");
    CURLcode res;
    char fname[256];
    pid_t pid =getpid();
    long response_code;

    res = curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    if ( res == CURLE_OK ) {
	    // printf("Response code: %ld\n", response_code);
    }

    if ( response_code >= 400 ) { 
		fprintf(stderr, "Error.\n");
        return 1;
    }

    char *ct = NULL;
    res = curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &ct);
    if ( res == CURLE_OK && ct != NULL ) {
    	//printf("Content-Type: %s, len=%ld\n", ct, strlen(ct));
    } else {
        //fprintf(stderr, "Failed obtain Content-Type\n");
        return 2;
    }

    if ( strstr(ct, CT_HTML) ) {
        return process_html(curl_handle, p_recv_buf, p_frontier_vec);
	} else if ( strstr(ct, CT_PNG) ) {
		return process_png(curl_handle, p_recv_buf, p_png_vec, p_png_counter, max_png_count);
    } else {
        sprintf(fname, "./output_%d", pid);
    }

    return write_file(fname, p_recv_buf->buf, p_recv_buf->size);
}

bool check_PNG_header(char* buf, long int len) {
    if (len < 8) { return false; }
    const char png_check[] = "PNG";
    for (int i = 0; i < 3; i++) {
        if (buf[i+1] != png_check[i])
            return false;
    }
    return true;
}

void push_frontier(
		CURL* p_handle,
		char* p_ptr,
		size_t size,
		URL_VECTOR* p_png_vec,
		URL_VECTOR* p_frontier_vec,
		unsigned* p_png_count,
		unsigned max_png_count
) {
    RECV_BUF recv_buf;
    
	recv_buf_init(&recv_buf, size + 1);
	recv_buf.size = size;
	recv_buf.buf[size] = '\0';
	for (unsigned i = 0; i < size; i++) {
		recv_buf.buf[i] = p_ptr[i];
	}
    /* process the download data */
//    printf("%s\n", recv_buf.buf);
	
	process_data(p_handle, &recv_buf, p_png_vec, p_frontier_vec,
				 p_png_count, max_png_count);

	recv_buf_cleanup(&recv_buf);
}


