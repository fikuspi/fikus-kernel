#ifndef _FIKUS_MMU_CONTEXT_H
#define _FIKUS_MMU_CONTEXT_H

struct mm_struct;

void use_mm(struct mm_struct *mm);
void unuse_mm(struct mm_struct *mm);

#endif
