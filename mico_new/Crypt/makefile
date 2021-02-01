

libcrypt.a : MD5.cpp MD5.h tea.cpp tea.h
	gcc -c -O2 MD5.cpp tea.cpp
	ar crs libcrypt.a MD5.o tea.o

clean:
	rm tea.o MD5.o libcrypt.a
