#include "http.h"
#include "request.h"


int http_parse_method(const slice* m, http_method* method) {
	if(slice_eq_str(m, "GET", true))
		*method = METHOD_GET;
	else if(slice_eq_str(m, "HEAD", true))
		*method = METHOD_HEAD;
	else if(slice_eq_str(m, "POST", true))
		*method = METHOD_POST;
	else if(slice_eq_str(m, "PUT", true))
		*method = METHOD_PUT;
	else if(slice_eq_str(m, "DELETE", true))
		*method = METHOD_DELETE;
	else if(slice_eq_str(m, "CONNECT", true))
		*method = METHOD_CONNECT;
	else if(slice_eq_str(m, "OPTIONS", true))
		*method = METHOD_OPTIONS;
	else if(slice_eq_str(m, "TRACE", true))
		*method = METHOD_TRACE;
	else if(slice_eq_str(m, "PATCH", true))
		*method = METHOD_PATCH;
	else
		return HTTP_INV_METHOD;

	return 0;
}

int http_parse_version(const slice* v, http_version* version) {
	if(slice_eq_str(v, "HTTP/1.0", false))
		*version = HTTP1_0;
	else if(slice_eq_str(v, "HTTP/1.1", false))
		*version = HTTP1_1;
	else
		return HTTP_INV_VERSION;

	return 0;
}

int http_parse_req_line(slice* data, http_method* method, f_path* path, http_version* version) {
	int err = 0;
	slice line = slice_line(data);
	slice s_method = slice_until_ch(&line, ' ');

	if(line.length == SIZE_MAX || s_method.length == SIZE_MAX)
		return HTTP_INV_REQ_LINE;

	if((err = http_parse_method(&s_method, method)))
		return err;

	// Advance to the request path
	slice_advance(&line, s_method.length + 1);
	slice s_path = slice_until_ch(&line, ' ');

	if(s_path.length == SIZE_MAX)
		return HTTP_INV_REQ_LINE;

	*path = fp_from_slice(&s_path);

	// Parse the HTTP version
	slice_advance(&line, s_path.length + 1);

	if((err = http_parse_version(&line, version)))
		return err;

	return 0;
}


http_req http_parse_request(const char* str, size_t length) {
	http_req request;
	request.error = 0;
	slice data = { length, str };

	if((request.error = http_parse_req_line(&data, &request.method, &request.path, &request.version)))
		return request;

	// if((request.error = http_parse_headers(&data,

	return request;
}


void http_free_request(http_req* request) {
	if(request->path.path != NULL)
		fp_free(&request->path);
}
