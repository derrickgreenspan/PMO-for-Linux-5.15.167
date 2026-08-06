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
.TH DETACH 2 2023-03-07 
.SH NAME
.Nm DETACH
.Nd DETACH
detach()   
.SH SYNOPSIS
.BI "char detach(void * " addr ");
.TP
Render inaccessible from the calling process the portion of the persistent memory object associated with the corresponding address in an NVMM.
.SH DESCRIPTION
The detach() system call shall render inaccessible from the calling process's
address space the portion of the PMO associated with 
.IR addr ,
and return 0 if successful.

If addr is invalid or is not associated with a PMO attached to the calling process's 
address space, 
.I errno
is set to -EINVAL, and the 
.I errnor 
is returned.
.TP
If the PMO associated with addr is already detached, then detach() does nothing and return 0.
.SH SEE ALSO
.BR pmo (3),
.BR pmo_attach (2),
.BR pmo_create (2),
.BR psync (2),
.BR pmo_destroy (2),
.SH COLOPHON
This page was written by Derrick Greenspan.

You can find him on the Web at
\%https://derrickgreenspan.github.io/.
