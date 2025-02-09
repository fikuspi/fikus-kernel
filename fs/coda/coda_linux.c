/*
 * Inode operations for Coda filesystem
 * Original version: (C) 1996 P. Braam and M. Callahan
 * Rewritten for Fikus 2.1. (C) 1997 Carnegie Mellon University
 * 
 * Carnegie Mellon encourages users to contribute improvements to
 * the Coda project. Contact Peter Braam (coda@cs.cmu.edu).
 */

#include <fikus/types.h>
#include <fikus/kernel.h>
#include <fikus/time.h>
#include <fikus/fs.h>
#include <fikus/stat.h>
#include <fikus/errno.h>
#include <asm/uaccess.h>
#include <fikus/string.h>

#include <fikus/coda.h>
#include <fikus/coda_psdev.h>
#include "coda_fikus.h"

/* initialize the debugging variables */
int coda_fake_statfs;

/* print a fid */
char * coda_f2s(struct CodaFid *f)
{
	static char s[60];

 	sprintf(s, "(%08x.%08x.%08x.%08x)", f->opaque[0], f->opaque[1], f->opaque[2], f->opaque[3]);

	return s;
}

/* recognize special .CONTROL name */
int coda_iscontrol(const char *name, size_t length)
{
	return ((CODA_CONTROLLEN == length) && 
                (strncmp(name, CODA_CONTROL, CODA_CONTROLLEN) == 0));
}

/* recognize /coda inode */
int coda_isroot(struct inode *i)
{
    return ( i->i_sb->s_root->d_inode == i );
}

unsigned short coda_flags_to_cflags(unsigned short flags)
{
	unsigned short coda_flags = 0;
	
	if ((flags & O_ACCMODE) == O_RDONLY)
		coda_flags |= C_O_READ;

	if ((flags & O_ACCMODE) == O_RDWR)
		coda_flags |= C_O_READ | C_O_WRITE;

	if ((flags & O_ACCMODE) == O_WRONLY)
		coda_flags |= C_O_WRITE;

	if (flags & O_TRUNC)
		coda_flags |= C_O_TRUNC;

	if (flags & O_CREAT)
		coda_flags |= C_O_CREAT;

	if (flags & O_EXCL)
		coda_flags |= C_O_EXCL;

	return coda_flags;
}


/* utility functions below */
void coda_vattr_to_iattr(struct inode *inode, struct coda_vattr *attr)
{
        int inode_type;
        /* inode's i_flags, i_ino are set by iget 
           XXX: is this all we need ??
           */
        switch (attr->va_type) {
        case C_VNON:
                inode_type  = 0;
                break;
        case C_VREG:
                inode_type = S_IFREG;
                break;
        case C_VDIR:
                inode_type = S_IFDIR;
                break;
        case C_VLNK:
                inode_type = S_IFLNK;
                break;
        default:
                inode_type = 0;
        }
	inode->i_mode |= inode_type;

	if (attr->va_mode != (u_short) -1)
	        inode->i_mode = attr->va_mode | inode_type;
        if (attr->va_uid != -1) 
	        inode->i_uid = make_kuid(&init_user_ns, (uid_t) attr->va_uid);
        if (attr->va_gid != -1)
	        inode->i_gid = make_kgid(&init_user_ns, (gid_t) attr->va_gid);
	if (attr->va_nlink != -1)
		set_nlink(inode, attr->va_nlink);
	if (attr->va_size != -1)
	        inode->i_size = attr->va_size;
	if (attr->va_size != -1)
		inode->i_blocks = (attr->va_size + 511) >> 9;
	if (attr->va_atime.tv_sec != -1) 
	        inode->i_atime = attr->va_atime;
	if (attr->va_mtime.tv_sec != -1)
	        inode->i_mtime = attr->va_mtime;
        if (attr->va_ctime.tv_sec != -1)
	        inode->i_ctime = attr->va_ctime;
}


/* 
 * BSD sets attributes that need not be modified to -1. 
 * Fikus uses the valid field to indicate what should be
 * looked at.  The BSD type field needs to be deduced from fikus 
 * mode.
 * So we have to do some translations here.
 */

void coda_iattr_to_vattr(struct iattr *iattr, struct coda_vattr *vattr)
{
        unsigned int valid;

        /* clean out */        
	vattr->va_mode = -1;
        vattr->va_uid = (vuid_t) -1; 
        vattr->va_gid = (vgid_t) -1;
        vattr->va_size = (off_t) -1;
	vattr->va_atime.tv_sec = (time_t) -1;
	vattr->va_atime.tv_nsec =  (time_t) -1;
        vattr->va_mtime.tv_sec = (time_t) -1;
        vattr->va_mtime.tv_nsec = (time_t) -1;
	vattr->va_ctime.tv_sec = (time_t) -1;
	vattr->va_ctime.tv_nsec = (time_t) -1;
        vattr->va_type = C_VNON;
	vattr->va_fileid = -1;
	vattr->va_gen = -1;
	vattr->va_bytes = -1;
	vattr->va_nlink = -1;
	vattr->va_blocksize = -1;
	vattr->va_rdev = -1;
        vattr->va_flags = 0;

        /* determine the type */
#if 0
        mode = iattr->ia_mode;
                if ( S_ISDIR(mode) ) {
                vattr->va_type = C_VDIR; 
        } else if ( S_ISREG(mode) ) {
                vattr->va_type = C_VREG;
        } else if ( S_ISLNK(mode) ) {
                vattr->va_type = C_VLNK;
        } else {
                /* don't do others */
                vattr->va_type = C_VNON;
        }
#endif 

        /* set those vattrs that need change */
        valid = iattr->ia_valid;
        if ( valid & ATTR_MODE ) {
                vattr->va_mode = iattr->ia_mode;
	}
        if ( valid & ATTR_UID ) {
                vattr->va_uid = (vuid_t) from_kuid(&init_user_ns, iattr->ia_uid);
	}
        if ( valid & ATTR_GID ) {
                vattr->va_gid = (vgid_t) from_kgid(&init_user_ns, iattr->ia_gid);
	}
        if ( valid & ATTR_SIZE ) {
                vattr->va_size = iattr->ia_size;
	}
        if ( valid & ATTR_ATIME ) {
                vattr->va_atime = iattr->ia_atime;
	}
        if ( valid & ATTR_MTIME ) {
                vattr->va_mtime = iattr->ia_mtime;
	}
        if ( valid & ATTR_CTIME ) {
                vattr->va_ctime = iattr->ia_ctime;
	}
}

