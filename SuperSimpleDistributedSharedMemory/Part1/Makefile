all: s2dsm_P1b s2dsm_P1c

s2dsm_P1b: s2dsm_P1b.c
	gcc -o $@ $^ -lpthread
s2dsm_P1c: s2dsm_P1c.c
	gcc -o $@ $^ -lpthread

clean:
	rm -f s2dsm_P1b s2dsm_P1c

