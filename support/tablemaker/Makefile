tablemaker: tablemaker.tab.o lex.yy.o tablemaker.pas
	fpc tablemaker.pas -gl
tablemaker.tab.o lex.yy.o: tablemaker.tab.c tablemaker.tab.h lex.yy.c
	gcc -c -g tablemaker.tab.c lex.yy.c
tablemaker.tab.c tablemaker.tab.h: tablemaker.y
	bison -d tablemaker.y
lex.yy.c: tablemaker.l
	flex tablemaker.l
clean:
	rm -vf tablemaker{,.o,.tab.[cho]} lex.yy.[cho]
