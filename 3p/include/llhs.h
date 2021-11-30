
#define MAX_HEADERS 	16
#define MAX_URI  		4096
#define MAX_BODY 		(1024 * 1024)
#define MAX_HNAME 		128
#define MAX_HBODY		1024

#define BUFFER_INIT_SIZE	256
#define BUFFER_GROW_FACTOR 	2
#define BUFFER_GROW_ADDEND 	0

#define BACKLOG 		128
#define POLL_TIMEOUT 	5000
#define MAX_POLL_EVENTS 64
#define IMPLICIT_LENGTH 1

#define MAX_CLIENTS		256
#define MAX_ALIVE		64

typedef void llhs_response;

typedef enum {
	llhs_HTTP_0_9,
	llhs_HTTP_1_0,
	llhs_HTTP_1_1,
} llhs_version;

typedef enum {
	llhs_GET,
	llhs_PUT,
	llhs_POST,
	llhs_HEAD,
	llhs_TRACE,
	llhs_PATCH,
	llhs_DELETE,
	llhs_OPTIONS,
	llhs_CONNECT,
} llhs_method;

typedef struct {
	const char *name;
	const char *body;
	int 		namel;
	int 		bodyl;
} llhs_header;

typedef struct {
	llhs_version version;
	llhs_method  method;
	const char  *URI;
	int 		 URIl;
	llhs_header headers[MAX_HEADERS];
	int 		headerc;
	const void *body;
	int 		bodyl;
} llhs_request;

typedef struct {
	_Bool occurred;
	char message[256];
	int  length;
	const char *file,
			   *func;
	int 		line;
} llhs_error;

typedef void (*llhs_callback)(void *userp, const llhs_request *req, llhs_response *res);
typedef void (*llhs_logger  )(void *userp, const char *level, const char *msg);

_Bool llhs_serve(unsigned short port, void *userp, llhs_callback callback, llhs_logger logger, llhs_error *error);

_Bool 		llhs_match (const char *x, const char *y);
_Bool 		llhs_match2(const char *x, const char *y, int xl, int yl);
const char *llhs_getheader(llhs_request *res, const char *name);

void  llhs_setcode   (llhs_response *res, int code);
_Bool llhs_setbody   (llhs_response *res, const void *body, int size);
void  llhs_setbody2  (llhs_response *res,       void *body, int bodyl, void (*free)(void *addr));
_Bool llhs_setheader (llhs_response *res, const char *name, const char *body);
_Bool llhs_setheader2(llhs_response *res, const char *name, int body);
