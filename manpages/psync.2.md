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
.TH PSYNC 2 2023-03-07 
.SH NAME
.Nm PSYNC
.Nd PSYNC 
psync()   
.SH SYNOPSIS
.BI "char psync(void * " addr ");
.TP
Force modifications to all or a specified portion of a persistent memory object to be made durable.
.SH DESCRIPTION
Given a pointer 
.IR addr ,
the psync() system call shall force a persistent memory object to be rendered durable.
.TP
The psync() system call can be considered an atomic and fault-tolerant operation. When psync() is invoked, and until psync has completed, the data that are being rendered durable is considered invalid, and is discarded in the event of an interruption (e.g., power loss, crash, etc.).
.SH SEE ALSO
.BR pmo (3),
.BR pmo_attach (2),
.BR pmo_detach (2),
.BR pmo_create (2),
.BR pmo_destroy (2),
.SH COLOPHON
This page was written by Derrick Greenspan.

You can find him on the Web at
\%https://derrickgreenspan.github.io/.
