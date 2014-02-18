all: mycc_main mycc interpreter
mycc_main: mycc_main.c
	gcc -Wall mycc_main.c -o mycc_main
mycc: lex.yy.c y.tab.h y.tab.c ast.c ast.h def.h utility.c utility.h semantics.c semantics.h translate.c translate.h
	gcc -Wall -g y.tab.c ast.c utility.c semantics.c translate.c -ll -ly -o mycc
y.tab.c: myc.y
	yacc -dv myc.y
y.tab.h: myc.y
	yacc -dv myc.y
lex.yy.c: myc.l
	lex myc.l
clean:
	rm mycc y.output y.tab.h y.tab.c lex.yy.c interpreter mycc_main
interpreter: interpreter.c
	gcc -Wall -g interpreter.c -o interpreter
test3: tmp35.c mycc interpreter
	./mycc tmp35.c > tmp35.c.trans
	./interpreter < tmp35.c.trans > tmp35.c.trans.mips
	/usr/bin/spim -stat -file tmp35.c.trans.mips
