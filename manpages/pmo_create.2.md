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
.TH PMO_CREATE 2 2023-03-07 
.SH NAME
.Nm PMO_CREATE
.Nd PMO_CREATE
pmo_create()   
.SH SYNOPSIS
.BI "char * pmo_create(const char *" name ", size_t " size ", const char *" key ");
.TP
Create a persistent memory object, given a name, size, and key.
.SH DESCRIPTION
Given a 
.IR name ,
.IR size ,
and a 
.IR key ,
the pmo_create() call shall allocate space for, and generate, a persistent 
memory object (PMO). On success, the return value of pmo_create() shall be
a string representing the key associated with the PMO.

If the size of name is larger than the maximum name length for a PMO, as
defined in the PMO header, then errno is set to EINVAL, and a null pointer is
returned. If name refers to a PMO which already exists, then errno is set to 
-EEXIST, and a null pointer is returned. If size is larger than the remaining
available space for the PMO system, then errno is set to ENOSPC, and a null
pointer is returned.

If 
.I key 
is a null pointer, then the kernel will randomly generate the keys to be
associated with the PMO. If
.I key 
is malformed, then errno is set to EINVAL, and
a null pointer is returned. Otherwise, the PMO's keys will be set to the values
specified by 
.IR key ,
and the string representing these keys will be returned. 
.SH BUGS 
The current implementation does not randomly generate keys. If you fail to
specify a key, the generated key will be "DEFAULT\\0"
.SH SEE ALSO
.BR pmo (3),
.BR pmo_attach (2),
.BR pmo_detach (2),
.BR psync (2),
.BR pmo_destroy (2),
.SH COLOPHON
This page was written by Derrick Greenspan.

You can find him on the Web at
\%https://derrickgreenspan.github.io/.
