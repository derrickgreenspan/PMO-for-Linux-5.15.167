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
.TH PMO_LIBRARY 3 2023-03-07 
.SH NAME
.Nm PMO
.Nd PMO Library Functions
PMO \ - PMO Library Functions
.SH SYNOPSIS
.B #include <pmo.h>
.TP
.BI "void * attach(const char *" name ", char " perm ", const char *" key ");
Render accessible the PMO 
.IR name ,
given a valid
.I key
with permissions 
.IR perm ,
and return a pointer to the start of the PMO.
.TP
.BI "char detach(void *" addr ");
Render the PMO associated with 
.I addr
inaccessible to the calling process. Return 0 on success, return a
specified ERRNO on failure.
.TP
.BI "void psync(void *" addr ");
Force modifications to the PMO associated with
.I addr
to be made durable.
.TP 
.BI "void * pmo_create(const char *" name ", size_t " size ", const char *" key ");
Create a PMO 
.I name
with
.I size
and
.IR key .
.SH DESCRIPTION
The PMO library function calls invoke kernel system calls that are used to
manipulate Persistent Memory Objects (PMOs). 
See the individual man pages for descriptions of each function.
.SH SEE ALSO
.BR attach (2),
.BR detach (2),
.BR psync (2),
.BR pmo_create (2),
.BR pmo_destroy (2),
.SH COLOPHON
This page was written by Derrick Greenspan.

You can find him on the Web at
\%https://derrickgreenspan.github.io/.
