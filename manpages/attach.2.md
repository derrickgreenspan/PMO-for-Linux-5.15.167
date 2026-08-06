.\" Copyright 2023 Derrick Greenspan (derrick.greenspan@ucf.edu)
.\"
.\" %%%LICENSE_START(VERBATIM)
.\" Permission is granted to make and distribute verbatim copies of this
.\" manual provided the copyright notice and this permission notice are
.\" preserved on all copies.
.\"
.\" Permission is granted to copy and distribute modified versions of this
.\" manual under the conditions for verbatim copying, provided that the
.\" entire resulting derived work is distributed under the terms of a
.\" permission notice identical to this one.
.\"
.\" Since the Linux kernel and libraries are constantly changing, this
.\" manual page may be incorrect or out-of-date.  The author(s) assume no
.\" responsibility for errors or omissions, or for damages resulting from
.\" the use of the information contained herein.  The author(s) may not
.\" have taken the same level of care in the production of this manual,
.\" which is licensed free of charge, as they might when working
.\" professionally.
.\"
.\" Formatted or processed versions of this manual, if unaccompanied by
.\" the source, must acknowledge the copyright and authors of this work.
.\" %%%LICENSE_END
.TH ATTACH 2 2023-03-07 
.SH NAME
.Nm ATTACH 
.Nd ATTACH
attach()
.SH SYNOPSIS
.BI "void * attach(const char *" name ", char " permission ", const char *" key ");
.TP
Render accessible to the calling process a persistent memory object in an NVMM.
.SH DESCRIPTION
The attach() system call shall render accessible to the calling process's address space, the PMO specified by 
.IR name .
On success, the return value of attach() is a pointer to the first address within the mapped PMO. On error, the value 
.I MAP_FAILED
(i.e., (void * )-1) is returned, and 
.I errno
is set to indicate the cause of the error.

If 
.I name
refers to anything other than the name of an existing PMO (i.e., the name PMO does not exist), then errno is set to -EMEDIUMTYPE, and MAP_FAILED is returned.

.I Permission
describes the desired operation. It can be either 'r' for read, or 'w' for write.
.SH SEE ALSO
.BR pmo (3),
.BR detach (2),
.BR psync (2),
.BR pmo_create (2),
.BR pmo_destroy (2),
.SH COLOPHON
This page was written by Derrick Greenspan.

You can find him on the Web at
\%https://derrickgreenspan.github.io/.
