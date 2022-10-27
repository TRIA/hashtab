hashtab: rsrc/rsrc.c hashtab.c hash/main.c rsrc/include/rsrc.h hashtab.h
	cc -o hashtab -I . -I rsrc/include -std=c99 rsrc/rsrc.c hashtab.c -D POSIX=1 hash/main.c

