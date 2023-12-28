#include <curl/multi.h>
#include <stdlib.h>

#include "recv_buf.h"
#include "url_vector.h"

#ifndef HTTP_SEARCH_H
#define HTTP_SEARCH_H

int process_data(CURL* curl_handle, RECV_BUF* p_recv_buf,
        URL_VECTOR* p_png_vec,
        URL_VECTOR* p_frontier_vec,
        unsigned* p_png_counter,
        unsigned max_png_count
);

#endif // HTTP_SEARCH_H


