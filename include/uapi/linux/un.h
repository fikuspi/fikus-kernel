#ifndef _FIKUS_UN_H
#define _FIKUS_UN_H

#include <fikus/socket.h>

#define UNIX_PATH_MAX	108

struct sockaddr_un {
	__kernel_sa_family_t sun_family; /* AF_UNIX */
	char sun_path[UNIX_PATH_MAX];	/* pathname */
};

#endif /* _FIKUS_UN_H */
