#ifndef _FIKUS_KCMP_H
#define _FIKUS_KCMP_H

/* Comparison type */
enum kcmp_type {
	KCMP_FILE,
	KCMP_VM,
	KCMP_FILES,
	KCMP_FS,
	KCMP_SIGHAND,
	KCMP_IO,
	KCMP_SYSVSEM,

	KCMP_TYPES,
};

#endif /* _FIKUS_KCMP_H */
