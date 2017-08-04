#ifndef HTTP_CONSTANTS
#define HTTP_CONSTANTS

const char* INFO[] = {
/*100*/ "Continue",
/*101*/ "Switching Protocols"
};

const char *SUCCESS[] = {
/*200*/ "OK",
/*201*/ "Created",
/*202*/ "Accepted",
/*203*/ "Non-Authoritative Information",
/*204*/ "No Content",
/*205*/ "Reset Content",
/*206*/ "Partial Content"
};

const char *REDIRECTION[] = {
/*300*/ "Multiple Choices",
/*301*/ "Moved Permanently",
/*302*/ "Found",
/*303*/ "See Other",
/*304*/ "Not Modified",
/*305*/ "Use Proxy",
		"Switch Proxy",	//No longer used
/*307*/ "Temporary Redirect"
};

const char *CLIENT_ERROR[] = {
/*400*/ "Bad Request",
/*401*/ "Unauthorized",
/*402*/ "Payment Required",
/*403*/ "Forbidden",
/*404*/ "Not Found",
/*405*/ "Method Not Allowed",
/*406*/ "Not Acceptable",
/*407*/ "Proxy Authentication Required",
/*408*/ "Request Time-out",
/*409*/ "Conflict",
/*410*/ "Gone",
/*411*/ "Length Required",
/*412*/ "Precondition Failed",
/*413*/ "Request Entity Too Large",
/*414*/ "Request-URI Too Large",
/*415*/ "Unsupported Media Type",
/*416*/ "Requested range not satisfiable",
/*417*/ "Expectation Failed",
/*418*/ "I'm a teapot"
};

const char *SERVER_ERROR[] = {
/*500*/ "Internal Server Error",
/*501*/ "Not Implemented",
/*502*/ "Bad Gateway",
/*503*/ "Service Unavailable",
/*504*/ "Gateway Time-out",
/*505*/ "HTTP Version not supported"
};

#endif
