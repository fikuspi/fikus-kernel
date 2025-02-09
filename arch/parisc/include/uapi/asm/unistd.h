#ifndef _UAPI_ASM_PARISC_UNISTD_H_
#define _UAPI_ASM_PARISC_UNISTD_H_

/*
 * This file contains the system call numbers.
 */

/*
 *   HP-UX system calls get their native numbers for binary compatibility.
 */

#define __NR_HPUX_exit                    1
#define __NR_HPUX_fork                    2
#define __NR_HPUX_read                    3
#define __NR_HPUX_write                   4
#define __NR_HPUX_open                    5
#define __NR_HPUX_close                   6
#define __NR_HPUX_wait                    7
#define __NR_HPUX_creat                   8
#define __NR_HPUX_link                    9
#define __NR_HPUX_unlink                 10
#define __NR_HPUX_execv                  11
#define __NR_HPUX_chdir                  12
#define __NR_HPUX_time                   13
#define __NR_HPUX_mknod                  14
#define __NR_HPUX_chmod                  15
#define __NR_HPUX_chown                  16
#define __NR_HPUX_break                  17
#define __NR_HPUX_lchmod                 18
#define __NR_HPUX_lseek                  19
#define __NR_HPUX_getpid                 20
#define __NR_HPUX_mount                  21
#define __NR_HPUX_umount                 22
#define __NR_HPUX_setuid                 23
#define __NR_HPUX_getuid                 24
#define __NR_HPUX_stime                  25
#define __NR_HPUX_ptrace                 26
#define __NR_HPUX_alarm                  27
#define __NR_HPUX_oldfstat               28
#define __NR_HPUX_pause                  29
#define __NR_HPUX_utime                  30
#define __NR_HPUX_stty                   31
#define __NR_HPUX_gtty                   32
#define __NR_HPUX_access                 33
#define __NR_HPUX_nice                   34
#define __NR_HPUX_ftime                  35
#define __NR_HPUX_sync                   36
#define __NR_HPUX_kill                   37
#define __NR_HPUX_stat                   38
#define __NR_HPUX_setpgrp3               39
#define __NR_HPUX_lstat                  40
#define __NR_HPUX_dup                    41
#define __NR_HPUX_pipe                   42
#define __NR_HPUX_times                  43
#define __NR_HPUX_profil                 44
#define __NR_HPUX_ki_call                45
#define __NR_HPUX_setgid                 46
#define __NR_HPUX_getgid                 47
#define __NR_HPUX_sigsys                 48
#define __NR_HPUX_reserved1              49
#define __NR_HPUX_reserved2              50
#define __NR_HPUX_acct                   51
#define __NR_HPUX_set_userthreadid       52
#define __NR_HPUX_oldlock                53
#define __NR_HPUX_ioctl                  54
#define __NR_HPUX_reboot                 55
#define __NR_HPUX_symlink                56
#define __NR_HPUX_utssys                 57
#define __NR_HPUX_readlink               58
#define __NR_HPUX_execve                 59
#define __NR_HPUX_umask                  60
#define __NR_HPUX_chroot                 61
#define __NR_HPUX_fcntl                  62
#define __NR_HPUX_ulimit                 63
#define __NR_HPUX_getpagesize            64
#define __NR_HPUX_mremap                 65
#define __NR_HPUX_vfork                  66
#define __NR_HPUX_vread                  67
#define __NR_HPUX_vwrite                 68
#define __NR_HPUX_sbrk                   69
#define __NR_HPUX_sstk                   70
#define __NR_HPUX_mmap                   71
#define __NR_HPUX_vadvise                72
#define __NR_HPUX_munmap                 73
#define __NR_HPUX_mprotect               74
#define __NR_HPUX_madvise                75
#define __NR_HPUX_vhangup                76
#define __NR_HPUX_swapoff                77
#define __NR_HPUX_mincore                78
#define __NR_HPUX_getgroups              79
#define __NR_HPUX_setgroups              80
#define __NR_HPUX_getpgrp2               81
#define __NR_HPUX_setpgrp2               82
#define __NR_HPUX_setitimer              83
#define __NR_HPUX_wait3                  84
#define __NR_HPUX_swapon                 85
#define __NR_HPUX_getitimer              86
#define __NR_HPUX_gethostname42          87
#define __NR_HPUX_sethostname42          88
#define __NR_HPUX_getdtablesize          89
#define __NR_HPUX_dup2                   90
#define __NR_HPUX_getdopt                91
#define __NR_HPUX_fstat                  92
#define __NR_HPUX_select                 93
#define __NR_HPUX_setdopt                94
#define __NR_HPUX_fsync                  95
#define __NR_HPUX_setpriority            96
#define __NR_HPUX_socket_old             97
#define __NR_HPUX_connect_old            98
#define __NR_HPUX_accept_old             99
#define __NR_HPUX_getpriority           100
#define __NR_HPUX_send_old              101
#define __NR_HPUX_recv_old              102
#define __NR_HPUX_socketaddr_old        103
#define __NR_HPUX_bind_old              104
#define __NR_HPUX_setsockopt_old        105
#define __NR_HPUX_listen_old            106
#define __NR_HPUX_vtimes_old            107
#define __NR_HPUX_sigvector             108
#define __NR_HPUX_sigblock              109
#define __NR_HPUX_siggetmask            110
#define __NR_HPUX_sigpause              111
#define __NR_HPUX_sigstack              112
#define __NR_HPUX_recvmsg_old           113
#define __NR_HPUX_sendmsg_old           114
#define __NR_HPUX_vtrace_old            115
#define __NR_HPUX_gettimeofday          116
#define __NR_HPUX_getrusage             117
#define __NR_HPUX_getsockopt_old        118
#define __NR_HPUX_resuba_old            119
#define __NR_HPUX_readv                 120
#define __NR_HPUX_writev                121
#define __NR_HPUX_settimeofday          122
#define __NR_HPUX_fchown                123
#define __NR_HPUX_fchmod                124
#define __NR_HPUX_recvfrom_old          125
#define __NR_HPUX_setresuid             126
#define __NR_HPUX_setresgid             127
#define __NR_HPUX_rename                128
#define __NR_HPUX_truncate              129
#define __NR_HPUX_ftruncate             130
#define __NR_HPUX_flock_old             131
#define __NR_HPUX_sysconf               132
#define __NR_HPUX_sendto_old            133
#define __NR_HPUX_shutdown_old          134
#define __NR_HPUX_socketpair_old        135
#define __NR_HPUX_mkdir                 136
#define __NR_HPUX_rmdir                 137
#define __NR_HPUX_utimes_old            138
#define __NR_HPUX_sigcleanup_old        139
#define __NR_HPUX_setcore               140
#define __NR_HPUX_getpeername_old       141
#define __NR_HPUX_gethostid             142
#define __NR_HPUX_sethostid             143
#define __NR_HPUX_getrlimit             144
#define __NR_HPUX_setrlimit             145
#define __NR_HPUX_killpg_old            146
#define __NR_HPUX_cachectl              147
#define __NR_HPUX_quotactl              148
#define __NR_HPUX_get_sysinfo           149
#define __NR_HPUX_getsockname_old       150
#define __NR_HPUX_privgrp               151
#define __NR_HPUX_rtprio                152
#define __NR_HPUX_plock                 153
#define __NR_HPUX_reserved3             154
#define __NR_HPUX_lockf                 155
#define __NR_HPUX_semget                156
#define __NR_HPUX_osemctl               157
#define __NR_HPUX_semop                 158
#define __NR_HPUX_msgget                159
#define __NR_HPUX_omsgctl               160
#define __NR_HPUX_msgsnd                161
#define __NR_HPUX_msgrecv               162
#define __NR_HPUX_shmget                163
#define __NR_HPUX_oshmctl               164
#define __NR_HPUX_shmat                 165
#define __NR_HPUX_shmdt                 166
#define __NR_HPUX_m68020_advise         167
/* [168,189] are for Discless/DUX */
#define __NR_HPUX_csp                   168
#define __NR_HPUX_cluster               169
#define __NR_HPUX_mkrnod                170
#define __NR_HPUX_test                  171
#define __NR_HPUX_unsp_open             172
#define __NR_HPUX_reserved4             173
#define __NR_HPUX_getcontext_old        174
#define __NR_HPUX_osetcontext           175
#define __NR_HPUX_bigio                 176
#define __NR_HPUX_pipenode              177
#define __NR_HPUX_lsync                 178
#define __NR_HPUX_getmachineid          179
#define __NR_HPUX_cnodeid               180
#define __NR_HPUX_cnodes                181
#define __NR_HPUX_swapclients           182
#define __NR_HPUX_rmt_process           183
#define __NR_HPUX_dskless_stats         184
#define __NR_HPUX_sigprocmask           185
#define __NR_HPUX_sigpending            186
#define __NR_HPUX_sigsuspend            187
#define __NR_HPUX_sigaction             188
#define __NR_HPUX_reserved5             189
#define __NR_HPUX_nfssvc                190
#define __NR_HPUX_getfh                 191
#define __NR_HPUX_getdomainname         192
#define __NR_HPUX_setdomainname         193
#define __NR_HPUX_async_daemon          194
#define __NR_HPUX_getdirentries         195
#define __NR_HPUX_statfs                196
#define __NR_HPUX_fstatfs               197
#define __NR_HPUX_vfsmount              198
#define __NR_HPUX_reserved6             199
#define __NR_HPUX_waitpid               200
/* 201 - 223 missing */
#define __NR_HPUX_sigsetreturn          224
#define __NR_HPUX_sigsetstatemask       225
/* 226 missing */
#define __NR_HPUX_cs                    227
#define __NR_HPUX_cds                   228
#define __NR_HPUX_set_no_trunc          229
#define __NR_HPUX_pathconf              230
#define __NR_HPUX_fpathconf             231
/* 232, 233 missing */
#define __NR_HPUX_nfs_fcntl             234
#define __NR_HPUX_ogetacl               235
#define __NR_HPUX_ofgetacl              236
#define __NR_HPUX_osetacl               237
#define __NR_HPUX_ofsetacl              238
#define __NR_HPUX_pstat                 239
#define __NR_HPUX_getaudid              240
#define __NR_HPUX_setaudid              241
#define __NR_HPUX_getaudproc            242
#define __NR_HPUX_setaudproc            243
#define __NR_HPUX_getevent              244
#define __NR_HPUX_setevent              245
#define __NR_HPUX_audwrite              246
#define __NR_HPUX_audswitch             247
#define __NR_HPUX_audctl                248
#define __NR_HPUX_ogetaccess            249
#define __NR_HPUX_fsctl                 250
/* 251 - 258 missing */
#define __NR_HPUX_swapfs                259
#define __NR_HPUX_fss                   260
/* 261 - 266 missing */
#define __NR_HPUX_tsync                 267
#define __NR_HPUX_getnumfds             268
#define __NR_HPUX_poll                  269
#define __NR_HPUX_getmsg                270
#define __NR_HPUX_putmsg                271
#define __NR_HPUX_fchdir                272
#define __NR_HPUX_getmount_cnt          273
#define __NR_HPUX_getmount_entry        274
#define __NR_HPUX_accept                275
#define __NR_HPUX_bind                  276
#define __NR_HPUX_connect               277
#define __NR_HPUX_getpeername           278
#define __NR_HPUX_getsockname           279
#define __NR_HPUX_getsockopt            280
#define __NR_HPUX_listen                281
#define __NR_HPUX_recv                  282
#define __NR_HPUX_recvfrom              283
#define __NR_HPUX_recvmsg               284
#define __NR_HPUX_send                  285
#define __NR_HPUX_sendmsg               286
#define __NR_HPUX_sendto                287
#define __NR_HPUX_setsockopt            288
#define __NR_HPUX_shutdown              289
#define __NR_HPUX_socket                290
#define __NR_HPUX_socketpair            291
#define __NR_HPUX_proc_open             292
#define __NR_HPUX_proc_close            293
#define __NR_HPUX_proc_send             294
#define __NR_HPUX_proc_recv             295
#define __NR_HPUX_proc_sendrecv         296
#define __NR_HPUX_proc_syscall          297
/* 298 - 311 missing */
#define __NR_HPUX_semctl                312
#define __NR_HPUX_msgctl                313
#define __NR_HPUX_shmctl                314
#define __NR_HPUX_mpctl                 315
#define __NR_HPUX_exportfs              316
#define __NR_HPUX_getpmsg               317
#define __NR_HPUX_putpmsg               318
/* 319 missing */
#define __NR_HPUX_msync                 320
#define __NR_HPUX_msleep                321
#define __NR_HPUX_mwakeup               322
#define __NR_HPUX_msem_init             323
#define __NR_HPUX_msem_remove           324
#define __NR_HPUX_adjtime               325
#define __NR_HPUX_kload                 326
#define __NR_HPUX_fattach               327
#define __NR_HPUX_fdetach               328
#define __NR_HPUX_serialize             329
#define __NR_HPUX_statvfs               330
#define __NR_HPUX_fstatvfs              331
#define __NR_HPUX_lchown                332
#define __NR_HPUX_getsid                333
#define __NR_HPUX_sysfs                 334
/* 335, 336 missing */
#define __NR_HPUX_sched_setparam        337
#define __NR_HPUX_sched_getparam        338
#define __NR_HPUX_sched_setscheduler    339
#define __NR_HPUX_sched_getscheduler    340
#define __NR_HPUX_sched_yield           341
#define __NR_HPUX_sched_get_priority_max 342
#define __NR_HPUX_sched_get_priority_min 343
#define __NR_HPUX_sched_rr_get_interval 344
#define __NR_HPUX_clock_settime         345
#define __NR_HPUX_clock_gettime         346
#define __NR_HPUX_clock_getres          347
#define __NR_HPUX_timer_create          348
#define __NR_HPUX_timer_delete          349
#define __NR_HPUX_timer_settime         350
#define __NR_HPUX_timer_gettime         351
#define __NR_HPUX_timer_getoverrun      352
#define __NR_HPUX_nanosleep             353
#define __NR_HPUX_toolbox               354
/* 355 missing */
#define __NR_HPUX_getdents              356
#define __NR_HPUX_getcontext            357
#define __NR_HPUX_sysinfo               358
#define __NR_HPUX_fcntl64               359
#define __NR_HPUX_ftruncate64           360
#define __NR_HPUX_fstat64               361
#define __NR_HPUX_getdirentries64       362
#define __NR_HPUX_getrlimit64           363
#define __NR_HPUX_lockf64               364
#define __NR_HPUX_lseek64               365
#define __NR_HPUX_lstat64               366
#define __NR_HPUX_mmap64                367
#define __NR_HPUX_setrlimit64           368
#define __NR_HPUX_stat64                369
#define __NR_HPUX_truncate64            370
#define __NR_HPUX_ulimit64              371
#define __NR_HPUX_pread                 372
#define __NR_HPUX_preadv                373
#define __NR_HPUX_pwrite                374
#define __NR_HPUX_pwritev               375
#define __NR_HPUX_pread64               376
#define __NR_HPUX_preadv64              377
#define __NR_HPUX_pwrite64              378
#define __NR_HPUX_pwritev64             379
#define __NR_HPUX_setcontext            380
#define __NR_HPUX_sigaltstack           381
#define __NR_HPUX_waitid                382
#define __NR_HPUX_setpgrp               383
#define __NR_HPUX_recvmsg2              384
#define __NR_HPUX_sendmsg2              385
#define __NR_HPUX_socket2               386
#define __NR_HPUX_socketpair2           387
#define __NR_HPUX_setregid              388
#define __NR_HPUX_lwp_create            389
#define __NR_HPUX_lwp_terminate         390
#define __NR_HPUX_lwp_wait              391
#define __NR_HPUX_lwp_suspend           392
#define __NR_HPUX_lwp_resume            393
/* 394 missing */
#define __NR_HPUX_lwp_abort_syscall     395
#define __NR_HPUX_lwp_info              396
#define __NR_HPUX_lwp_kill              397
#define __NR_HPUX_ksleep                398
#define __NR_HPUX_kwakeup               399
/* 400 missing */
#define __NR_HPUX_pstat_getlwp          401
#define __NR_HPUX_lwp_exit              402
#define __NR_HPUX_lwp_continue          403
#define __NR_HPUX_getacl                404
#define __NR_HPUX_fgetacl               405
#define __NR_HPUX_setacl                406
#define __NR_HPUX_fsetacl               407
#define __NR_HPUX_getaccess             408
#define __NR_HPUX_lwp_mutex_init        409
#define __NR_HPUX_lwp_mutex_lock_sys    410
#define __NR_HPUX_lwp_mutex_unlock      411
#define __NR_HPUX_lwp_cond_init         412
#define __NR_HPUX_lwp_cond_signal       413
#define __NR_HPUX_lwp_cond_broadcast    414
#define __NR_HPUX_lwp_cond_wait_sys     415
#define __NR_HPUX_lwp_getscheduler      416
#define __NR_HPUX_lwp_setscheduler      417
#define __NR_HPUX_lwp_getstate          418
#define __NR_HPUX_lwp_setstate          419
#define __NR_HPUX_lwp_detach            420
#define __NR_HPUX_mlock                 421
#define __NR_HPUX_munlock               422
#define __NR_HPUX_mlockall              423
#define __NR_HPUX_munlockall            424
#define __NR_HPUX_shm_open              425
#define __NR_HPUX_shm_unlink            426
#define __NR_HPUX_sigqueue              427
#define __NR_HPUX_sigwaitinfo           428
#define __NR_HPUX_sigtimedwait          429
#define __NR_HPUX_sigwait               430
#define __NR_HPUX_aio_read              431
#define __NR_HPUX_aio_write             432
#define __NR_HPUX_lio_listio            433
#define __NR_HPUX_aio_error             434
#define __NR_HPUX_aio_return            435
#define __NR_HPUX_aio_cancel            436
#define __NR_HPUX_aio_suspend           437
#define __NR_HPUX_aio_fsync             438
#define __NR_HPUX_mq_open               439
#define __NR_HPUX_mq_close              440
#define __NR_HPUX_mq_unlink             441
#define __NR_HPUX_mq_send               442
#define __NR_HPUX_mq_receive            443
#define __NR_HPUX_mq_notify             444
#define __NR_HPUX_mq_setattr            445
#define __NR_HPUX_mq_getattr            446
#define __NR_HPUX_ksem_open             447
#define __NR_HPUX_ksem_unlink           448
#define __NR_HPUX_ksem_close            449
#define __NR_HPUX_ksem_post             450
#define __NR_HPUX_ksem_wait             451
#define __NR_HPUX_ksem_read             452
#define __NR_HPUX_ksem_trywait          453
#define __NR_HPUX_lwp_rwlock_init       454
#define __NR_HPUX_lwp_rwlock_destroy    455
#define __NR_HPUX_lwp_rwlock_rdlock_sys 456
#define __NR_HPUX_lwp_rwlock_wrlock_sys 457
#define __NR_HPUX_lwp_rwlock_tryrdlock  458
#define __NR_HPUX_lwp_rwlock_trywrlock  459
#define __NR_HPUX_lwp_rwlock_unlock     460
#define __NR_HPUX_ttrace                461
#define __NR_HPUX_ttrace_wait           462
#define __NR_HPUX_lf_wire_mem           463
#define __NR_HPUX_lf_unwire_mem         464
#define __NR_HPUX_lf_send_pin_map       465
#define __NR_HPUX_lf_free_buf           466
#define __NR_HPUX_lf_wait_nq            467
#define __NR_HPUX_lf_wakeup_conn_q      468
#define __NR_HPUX_lf_unused             469
#define __NR_HPUX_lwp_sema_init         470
#define __NR_HPUX_lwp_sema_post         471
#define __NR_HPUX_lwp_sema_wait         472
#define __NR_HPUX_lwp_sema_trywait      473
#define __NR_HPUX_lwp_sema_destroy      474
#define __NR_HPUX_statvfs64             475
#define __NR_HPUX_fstatvfs64            476
#define __NR_HPUX_msh_register          477
#define __NR_HPUX_ptrace64              478
#define __NR_HPUX_sendfile              479
#define __NR_HPUX_sendpath              480
#define __NR_HPUX_sendfile64            481
#define __NR_HPUX_sendpath64            482
#define __NR_HPUX_modload               483
#define __NR_HPUX_moduload              484
#define __NR_HPUX_modpath               485
#define __NR_HPUX_getksym               486
#define __NR_HPUX_modadm                487
#define __NR_HPUX_modstat               488
#define __NR_HPUX_lwp_detached_exit     489
#define __NR_HPUX_crashconf             490
#define __NR_HPUX_siginhibit            491
#define __NR_HPUX_sigenable             492
#define __NR_HPUX_spuctl                493
#define __NR_HPUX_zerokernelsum         494
#define __NR_HPUX_nfs_kstat             495
#define __NR_HPUX_aio_read64            496
#define __NR_HPUX_aio_write64           497
#define __NR_HPUX_aio_error64           498
#define __NR_HPUX_aio_return64          499
#define __NR_HPUX_aio_cancel64          500
#define __NR_HPUX_aio_suspend64         501
#define __NR_HPUX_aio_fsync64           502
#define __NR_HPUX_lio_listio64          503
#define __NR_HPUX_recv2                 504
#define __NR_HPUX_recvfrom2             505
#define __NR_HPUX_send2                 506
#define __NR_HPUX_sendto2               507
#define __NR_HPUX_acl                   508
#define __NR_HPUX___cnx_p2p_ctl         509
#define __NR_HPUX___cnx_gsched_ctl      510
#define __NR_HPUX___cnx_pmon_ctl        511

#define __NR_HPUX_syscalls		512

/*
 * Fikus system call numbers.
 *
 * Cary Coutant says that we should just use another syscall gateway
 * page to avoid clashing with the HPUX space, and I think he's right:
 * it will would keep a branch out of our syscall entry path, at the
 * very least.  If we decide to change it later, we can ``just'' tweak
 * the FIKUS_GATEWAY_ADDR define at the bottom and make __NR_Fikus be
 * 1024 or something.  Oh, and recompile libc. =)
 *
 * 64-bit HPUX binaries get the syscall gateway address passed in a register
 * from the kernel at startup, which seems a sane strategy.
 */

#define __NR_Fikus                0
#define __NR_restart_syscall      (__NR_Fikus + 0)
#define __NR_exit                 (__NR_Fikus + 1)
#define __NR_fork                 (__NR_Fikus + 2)
#define __NR_read                 (__NR_Fikus + 3)
#define __NR_write                (__NR_Fikus + 4)
#define __NR_open                 (__NR_Fikus + 5)
#define __NR_close                (__NR_Fikus + 6)
#define __NR_waitpid              (__NR_Fikus + 7)
#define __NR_creat                (__NR_Fikus + 8)
#define __NR_link                 (__NR_Fikus + 9)
#define __NR_unlink              (__NR_Fikus + 10)
#define __NR_execve              (__NR_Fikus + 11)
#define __NR_chdir               (__NR_Fikus + 12)
#define __NR_time                (__NR_Fikus + 13)
#define __NR_mknod               (__NR_Fikus + 14)
#define __NR_chmod               (__NR_Fikus + 15)
#define __NR_lchown              (__NR_Fikus + 16)
#define __NR_socket              (__NR_Fikus + 17)
#define __NR_stat                (__NR_Fikus + 18)
#define __NR_lseek               (__NR_Fikus + 19)
#define __NR_getpid              (__NR_Fikus + 20)
#define __NR_mount               (__NR_Fikus + 21)
#define __NR_bind                (__NR_Fikus + 22)
#define __NR_setuid              (__NR_Fikus + 23)
#define __NR_getuid              (__NR_Fikus + 24)
#define __NR_stime               (__NR_Fikus + 25)
#define __NR_ptrace              (__NR_Fikus + 26)
#define __NR_alarm               (__NR_Fikus + 27)
#define __NR_fstat               (__NR_Fikus + 28)
#define __NR_pause               (__NR_Fikus + 29)
#define __NR_utime               (__NR_Fikus + 30)
#define __NR_connect             (__NR_Fikus + 31)
#define __NR_listen              (__NR_Fikus + 32)
#define __NR_access              (__NR_Fikus + 33)
#define __NR_nice                (__NR_Fikus + 34)
#define __NR_accept              (__NR_Fikus + 35)
#define __NR_sync                (__NR_Fikus + 36)
#define __NR_kill                (__NR_Fikus + 37)
#define __NR_rename              (__NR_Fikus + 38)
#define __NR_mkdir               (__NR_Fikus + 39)
#define __NR_rmdir               (__NR_Fikus + 40)
#define __NR_dup                 (__NR_Fikus + 41)
#define __NR_pipe                (__NR_Fikus + 42)
#define __NR_times               (__NR_Fikus + 43)
#define __NR_getsockname         (__NR_Fikus + 44)
#define __NR_brk                 (__NR_Fikus + 45)
#define __NR_setgid              (__NR_Fikus + 46)
#define __NR_getgid              (__NR_Fikus + 47)
#define __NR_signal              (__NR_Fikus + 48)
#define __NR_geteuid             (__NR_Fikus + 49)
#define __NR_getegid             (__NR_Fikus + 50)
#define __NR_acct                (__NR_Fikus + 51)
#define __NR_umount2             (__NR_Fikus + 52)
#define __NR_getpeername         (__NR_Fikus + 53)
#define __NR_ioctl               (__NR_Fikus + 54)
#define __NR_fcntl               (__NR_Fikus + 55)
#define __NR_socketpair          (__NR_Fikus + 56)
#define __NR_setpgid             (__NR_Fikus + 57)
#define __NR_send                (__NR_Fikus + 58)
#define __NR_uname               (__NR_Fikus + 59)
#define __NR_umask               (__NR_Fikus + 60)
#define __NR_chroot              (__NR_Fikus + 61)
#define __NR_ustat               (__NR_Fikus + 62)
#define __NR_dup2                (__NR_Fikus + 63)
#define __NR_getppid             (__NR_Fikus + 64)
#define __NR_getpgrp             (__NR_Fikus + 65)
#define __NR_setsid              (__NR_Fikus + 66)
#define __NR_pivot_root          (__NR_Fikus + 67)
#define __NR_sgetmask            (__NR_Fikus + 68)
#define __NR_ssetmask            (__NR_Fikus + 69)
#define __NR_setreuid            (__NR_Fikus + 70)
#define __NR_setregid            (__NR_Fikus + 71)
#define __NR_mincore             (__NR_Fikus + 72)
#define __NR_sigpending          (__NR_Fikus + 73)
#define __NR_sethostname         (__NR_Fikus + 74)
#define __NR_setrlimit           (__NR_Fikus + 75)
#define __NR_getrlimit           (__NR_Fikus + 76)
#define __NR_getrusage           (__NR_Fikus + 77)
#define __NR_gettimeofday        (__NR_Fikus + 78)
#define __NR_settimeofday        (__NR_Fikus + 79)
#define __NR_getgroups           (__NR_Fikus + 80)
#define __NR_setgroups           (__NR_Fikus + 81)
#define __NR_sendto              (__NR_Fikus + 82)
#define __NR_symlink             (__NR_Fikus + 83)
#define __NR_lstat               (__NR_Fikus + 84)
#define __NR_readlink            (__NR_Fikus + 85)
#define __NR_uselib              (__NR_Fikus + 86)
#define __NR_swapon              (__NR_Fikus + 87)
#define __NR_reboot              (__NR_Fikus + 88)
#define __NR_mmap2             (__NR_Fikus + 89)
#define __NR_mmap                (__NR_Fikus + 90)
#define __NR_munmap              (__NR_Fikus + 91)
#define __NR_truncate            (__NR_Fikus + 92)
#define __NR_ftruncate           (__NR_Fikus + 93)
#define __NR_fchmod              (__NR_Fikus + 94)
#define __NR_fchown              (__NR_Fikus + 95)
#define __NR_getpriority         (__NR_Fikus + 96)
#define __NR_setpriority         (__NR_Fikus + 97)
#define __NR_recv                (__NR_Fikus + 98)
#define __NR_statfs              (__NR_Fikus + 99)
#define __NR_fstatfs            (__NR_Fikus + 100)
#define __NR_stat64           (__NR_Fikus + 101)
/* #define __NR_socketcall         (__NR_Fikus + 102) */
#define __NR_syslog             (__NR_Fikus + 103)
#define __NR_setitimer          (__NR_Fikus + 104)
#define __NR_getitimer          (__NR_Fikus + 105)
#define __NR_capget             (__NR_Fikus + 106)
#define __NR_capset             (__NR_Fikus + 107)
#define __NR_pread64            (__NR_Fikus + 108)
#define __NR_pwrite64           (__NR_Fikus + 109)
#define __NR_getcwd             (__NR_Fikus + 110)
#define __NR_vhangup            (__NR_Fikus + 111)
#define __NR_fstat64            (__NR_Fikus + 112)
#define __NR_vfork              (__NR_Fikus + 113)
#define __NR_wait4              (__NR_Fikus + 114)
#define __NR_swapoff            (__NR_Fikus + 115)
#define __NR_sysinfo            (__NR_Fikus + 116)
#define __NR_shutdown           (__NR_Fikus + 117)
#define __NR_fsync              (__NR_Fikus + 118)
#define __NR_madvise            (__NR_Fikus + 119)
#define __NR_clone              (__NR_Fikus + 120)
#define __NR_setdomainname      (__NR_Fikus + 121)
#define __NR_sendfile           (__NR_Fikus + 122)
#define __NR_recvfrom           (__NR_Fikus + 123)
#define __NR_adjtimex           (__NR_Fikus + 124)
#define __NR_mprotect           (__NR_Fikus + 125)
#define __NR_sigprocmask        (__NR_Fikus + 126)
#define __NR_create_module      (__NR_Fikus + 127)
#define __NR_init_module        (__NR_Fikus + 128)
#define __NR_delete_module      (__NR_Fikus + 129)
#define __NR_get_kernel_syms    (__NR_Fikus + 130)
#define __NR_quotactl           (__NR_Fikus + 131)
#define __NR_getpgid            (__NR_Fikus + 132)
#define __NR_fchdir             (__NR_Fikus + 133)
#define __NR_bdflush            (__NR_Fikus + 134)
#define __NR_sysfs              (__NR_Fikus + 135)
#define __NR_personality        (__NR_Fikus + 136)
#define __NR_afs_syscall        (__NR_Fikus + 137) /* Syscall for Andrew File System */
#define __NR_setfsuid           (__NR_Fikus + 138)
#define __NR_setfsgid           (__NR_Fikus + 139)
#define __NR__llseek            (__NR_Fikus + 140)
#define __NR_getdents           (__NR_Fikus + 141)
#define __NR__newselect         (__NR_Fikus + 142)
#define __NR_flock              (__NR_Fikus + 143)
#define __NR_msync              (__NR_Fikus + 144)
#define __NR_readv              (__NR_Fikus + 145)
#define __NR_writev             (__NR_Fikus + 146)
#define __NR_getsid             (__NR_Fikus + 147)
#define __NR_fdatasync          (__NR_Fikus + 148)
#define __NR__sysctl            (__NR_Fikus + 149)
#define __NR_mlock              (__NR_Fikus + 150)
#define __NR_munlock            (__NR_Fikus + 151)
#define __NR_mlockall           (__NR_Fikus + 152)
#define __NR_munlockall         (__NR_Fikus + 153)
#define __NR_sched_setparam             (__NR_Fikus + 154)
#define __NR_sched_getparam             (__NR_Fikus + 155)
#define __NR_sched_setscheduler         (__NR_Fikus + 156)
#define __NR_sched_getscheduler         (__NR_Fikus + 157)
#define __NR_sched_yield                (__NR_Fikus + 158)
#define __NR_sched_get_priority_max     (__NR_Fikus + 159)
#define __NR_sched_get_priority_min     (__NR_Fikus + 160)
#define __NR_sched_rr_get_interval      (__NR_Fikus + 161)
#define __NR_nanosleep          (__NR_Fikus + 162)
#define __NR_mremap             (__NR_Fikus + 163)
#define __NR_setresuid          (__NR_Fikus + 164)
#define __NR_getresuid          (__NR_Fikus + 165)
#define __NR_sigaltstack        (__NR_Fikus + 166)
#define __NR_query_module       (__NR_Fikus + 167)
#define __NR_poll               (__NR_Fikus + 168)
#define __NR_nfsservctl         (__NR_Fikus + 169)
#define __NR_setresgid          (__NR_Fikus + 170)
#define __NR_getresgid          (__NR_Fikus + 171)
#define __NR_prctl              (__NR_Fikus + 172)
#define __NR_rt_sigreturn       (__NR_Fikus + 173)
#define __NR_rt_sigaction       (__NR_Fikus + 174)
#define __NR_rt_sigprocmask     (__NR_Fikus + 175)
#define __NR_rt_sigpending      (__NR_Fikus + 176)
#define __NR_rt_sigtimedwait    (__NR_Fikus + 177)
#define __NR_rt_sigqueueinfo    (__NR_Fikus + 178)
#define __NR_rt_sigsuspend      (__NR_Fikus + 179)
#define __NR_chown              (__NR_Fikus + 180)
#define __NR_setsockopt         (__NR_Fikus + 181)
#define __NR_getsockopt         (__NR_Fikus + 182)
#define __NR_sendmsg            (__NR_Fikus + 183)
#define __NR_recvmsg            (__NR_Fikus + 184)
#define __NR_semop              (__NR_Fikus + 185)
#define __NR_semget             (__NR_Fikus + 186)
#define __NR_semctl             (__NR_Fikus + 187)
#define __NR_msgsnd             (__NR_Fikus + 188)
#define __NR_msgrcv             (__NR_Fikus + 189)
#define __NR_msgget             (__NR_Fikus + 190)
#define __NR_msgctl             (__NR_Fikus + 191)
#define __NR_shmat              (__NR_Fikus + 192)
#define __NR_shmdt              (__NR_Fikus + 193)
#define __NR_shmget             (__NR_Fikus + 194)
#define __NR_shmctl             (__NR_Fikus + 195)

#define __NR_getpmsg		(__NR_Fikus + 196) /* Somebody *wants* streams? */
#define __NR_putpmsg		(__NR_Fikus + 197)

#define __NR_lstat64            (__NR_Fikus + 198)
#define __NR_truncate64         (__NR_Fikus + 199)
#define __NR_ftruncate64        (__NR_Fikus + 200)
#define __NR_getdents64         (__NR_Fikus + 201)
#define __NR_fcntl64            (__NR_Fikus + 202)
#define __NR_attrctl            (__NR_Fikus + 203)
#define __NR_acl_get            (__NR_Fikus + 204)
#define __NR_acl_set            (__NR_Fikus + 205)
#define __NR_gettid             (__NR_Fikus + 206)
#define __NR_readahead          (__NR_Fikus + 207)
#define __NR_tkill              (__NR_Fikus + 208)
#define __NR_sendfile64         (__NR_Fikus + 209)
#define __NR_futex              (__NR_Fikus + 210)
#define __NR_sched_setaffinity  (__NR_Fikus + 211)
#define __NR_sched_getaffinity  (__NR_Fikus + 212)
#define __NR_set_thread_area    (__NR_Fikus + 213)
#define __NR_get_thread_area    (__NR_Fikus + 214)
#define __NR_io_setup           (__NR_Fikus + 215)
#define __NR_io_destroy         (__NR_Fikus + 216)
#define __NR_io_getevents       (__NR_Fikus + 217)
#define __NR_io_submit          (__NR_Fikus + 218)
#define __NR_io_cancel          (__NR_Fikus + 219)
#define __NR_alloc_hugepages    (__NR_Fikus + 220)
#define __NR_free_hugepages     (__NR_Fikus + 221)
#define __NR_exit_group         (__NR_Fikus + 222)
#define __NR_lookup_dcookie     (__NR_Fikus + 223)
#define __NR_epoll_create       (__NR_Fikus + 224)
#define __NR_epoll_ctl          (__NR_Fikus + 225)
#define __NR_epoll_wait         (__NR_Fikus + 226)
#define __NR_remap_file_pages   (__NR_Fikus + 227)
#define __NR_semtimedop         (__NR_Fikus + 228)
#define __NR_mq_open            (__NR_Fikus + 229)
#define __NR_mq_unlink          (__NR_Fikus + 230)
#define __NR_mq_timedsend       (__NR_Fikus + 231)
#define __NR_mq_timedreceive    (__NR_Fikus + 232)
#define __NR_mq_notify          (__NR_Fikus + 233)
#define __NR_mq_getsetattr      (__NR_Fikus + 234)
#define __NR_waitid		(__NR_Fikus + 235)
#define __NR_fadvise64_64	(__NR_Fikus + 236)
#define __NR_set_tid_address	(__NR_Fikus + 237)
#define __NR_setxattr		(__NR_Fikus + 238)
#define __NR_lsetxattr		(__NR_Fikus + 239)
#define __NR_fsetxattr		(__NR_Fikus + 240)
#define __NR_getxattr		(__NR_Fikus + 241)
#define __NR_lgetxattr		(__NR_Fikus + 242)
#define __NR_fgetxattr		(__NR_Fikus + 243)
#define __NR_listxattr		(__NR_Fikus + 244)
#define __NR_llistxattr		(__NR_Fikus + 245)
#define __NR_flistxattr		(__NR_Fikus + 246)
#define __NR_removexattr	(__NR_Fikus + 247)
#define __NR_lremovexattr	(__NR_Fikus + 248)
#define __NR_fremovexattr	(__NR_Fikus + 249)
#define __NR_timer_create	(__NR_Fikus + 250)
#define __NR_timer_settime	(__NR_Fikus + 251)
#define __NR_timer_gettime	(__NR_Fikus + 252)
#define __NR_timer_getoverrun	(__NR_Fikus + 253)
#define __NR_timer_delete	(__NR_Fikus + 254)
#define __NR_clock_settime	(__NR_Fikus + 255)
#define __NR_clock_gettime	(__NR_Fikus + 256)
#define __NR_clock_getres	(__NR_Fikus + 257)
#define __NR_clock_nanosleep	(__NR_Fikus + 258)
#define __NR_tgkill		(__NR_Fikus + 259)
#define __NR_mbind		(__NR_Fikus + 260)
#define __NR_get_mempolicy	(__NR_Fikus + 261)
#define __NR_set_mempolicy	(__NR_Fikus + 262)
#define __NR_vserver		(__NR_Fikus + 263)
#define __NR_add_key		(__NR_Fikus + 264)
#define __NR_request_key	(__NR_Fikus + 265)
#define __NR_keyctl		(__NR_Fikus + 266)
#define __NR_ioprio_set		(__NR_Fikus + 267)
#define __NR_ioprio_get		(__NR_Fikus + 268)
#define __NR_inotify_init	(__NR_Fikus + 269)
#define __NR_inotify_add_watch	(__NR_Fikus + 270)
#define __NR_inotify_rm_watch	(__NR_Fikus + 271)
#define __NR_migrate_pages	(__NR_Fikus + 272)
#define __NR_pselect6		(__NR_Fikus + 273)
#define __NR_ppoll		(__NR_Fikus + 274)
#define __NR_openat		(__NR_Fikus + 275)
#define __NR_mkdirat		(__NR_Fikus + 276)
#define __NR_mknodat		(__NR_Fikus + 277)
#define __NR_fchownat		(__NR_Fikus + 278)
#define __NR_futimesat		(__NR_Fikus + 279)
#define __NR_fstatat64		(__NR_Fikus + 280)
#define __NR_unlinkat		(__NR_Fikus + 281)
#define __NR_renameat		(__NR_Fikus + 282)
#define __NR_linkat		(__NR_Fikus + 283)
#define __NR_symlinkat		(__NR_Fikus + 284)
#define __NR_readlinkat		(__NR_Fikus + 285)
#define __NR_fchmodat		(__NR_Fikus + 286)
#define __NR_faccessat		(__NR_Fikus + 287)
#define __NR_unshare		(__NR_Fikus + 288)
#define __NR_set_robust_list	(__NR_Fikus + 289)
#define __NR_get_robust_list	(__NR_Fikus + 290)
#define __NR_splice		(__NR_Fikus + 291)
#define __NR_sync_file_range	(__NR_Fikus + 292)
#define __NR_tee		(__NR_Fikus + 293)
#define __NR_vmsplice		(__NR_Fikus + 294)
#define __NR_move_pages		(__NR_Fikus + 295)
#define __NR_getcpu		(__NR_Fikus + 296)
#define __NR_epoll_pwait	(__NR_Fikus + 297)
#define __NR_statfs64		(__NR_Fikus + 298)
#define __NR_fstatfs64		(__NR_Fikus + 299)
#define __NR_kexec_load		(__NR_Fikus + 300)
#define __NR_utimensat		(__NR_Fikus + 301)
#define __NR_signalfd		(__NR_Fikus + 302)
#define __NR_timerfd		(__NR_Fikus + 303)
#define __NR_eventfd		(__NR_Fikus + 304)
#define __NR_fallocate		(__NR_Fikus + 305)
#define __NR_timerfd_create	(__NR_Fikus + 306)
#define __NR_timerfd_settime	(__NR_Fikus + 307)
#define __NR_timerfd_gettime	(__NR_Fikus + 308)
#define __NR_signalfd4		(__NR_Fikus + 309)
#define __NR_eventfd2		(__NR_Fikus + 310)
#define __NR_epoll_create1	(__NR_Fikus + 311)
#define __NR_dup3		(__NR_Fikus + 312)
#define __NR_pipe2		(__NR_Fikus + 313)
#define __NR_inotify_init1	(__NR_Fikus + 314)
#define __NR_preadv		(__NR_Fikus + 315)
#define __NR_pwritev		(__NR_Fikus + 316)
#define __NR_rt_tgsigqueueinfo	(__NR_Fikus + 317)
#define __NR_perf_event_open	(__NR_Fikus + 318)
#define __NR_recvmmsg		(__NR_Fikus + 319)
#define __NR_accept4		(__NR_Fikus + 320)
#define __NR_prlimit64		(__NR_Fikus + 321)
#define __NR_fanotify_init	(__NR_Fikus + 322)
#define __NR_fanotify_mark	(__NR_Fikus + 323)
#define __NR_clock_adjtime	(__NR_Fikus + 324)
#define __NR_name_to_handle_at	(__NR_Fikus + 325)
#define __NR_open_by_handle_at	(__NR_Fikus + 326)
#define __NR_syncfs		(__NR_Fikus + 327)
#define __NR_setns		(__NR_Fikus + 328)
#define __NR_sendmmsg		(__NR_Fikus + 329)
#define __NR_process_vm_readv	(__NR_Fikus + 330)
#define __NR_process_vm_writev	(__NR_Fikus + 331)
#define __NR_kcmp		(__NR_Fikus + 332)
#define __NR_finit_module	(__NR_Fikus + 333)

#define __NR_Fikus_syscalls	(__NR_finit_module + 1)


#define __IGNORE_select		/* newselect */
#define __IGNORE_fadvise64	/* fadvise64_64 */
#define __IGNORE_utimes		/* utime */


#define HPUX_GATEWAY_ADDR       0xC0000004
#define FIKUS_GATEWAY_ADDR      0x100

#endif /* _UAPI_ASM_PARISC_UNISTD_H_ */
