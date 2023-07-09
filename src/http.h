// Handle HTTP requests & responses

#include "util.h"


typedef enum {
	GET,
	POST
} http_method;

typedef enum {
	HTTP1_0,
	HTTP1_1
} http_version;

typedef struct {
	http_method method;
	slice path;
	http_version version;
	slice headers;
	slice body;
} http_req;

typedef struct {
	http_version version;
	int status_code;
	slice status_msg;
	slice headers;
	slice body;
} http_res;


const char* mime_from_path(const char* path);

http_req http_parse_req(const char* data);

http_res http_create_res(int status_code, const char* status_msg);
int http_send_res(int fd, http_res* res);

int http_handle_request(int client);
