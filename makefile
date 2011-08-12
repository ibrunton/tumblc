all :
	gcc -g -o tumblc -lcurl -Wl,--hash-style=gnu -Wl,--as-needed tumbl.c
