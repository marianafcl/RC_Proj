# RC_Proj

**Descrição**
Este projeto consiste de três entidades diferentes (Central Server, Working Server, user) que vão comunicar entre si para o processamento e resolução de várias tarefas bem definidas. Os parâmetros de conexão entre estas três entidades podem ser definidos ou default.
Cada instância do user pode ligar-se a um Central Server (que, em qualquer execução, deve ser entidade única), e requisitar o processamento de uma tarefa que este disponibilize sobre um dado ficheiro. O Central Server, por sua vez, distribui esta tarefa por Working Servers que sejam capazes de a realizar. As respostas dos WS's são aglomeradas pelo CS e devolvidas ao user, que as apresenta como output no seu terminal.
O Working Server implementado neste projeto tem disponíveis quatro tarefas:
---
FLW - find longest word
UPP - convert to upper case
LOW - convert to lower case
WCT - word count
---
Quaisqueres erros são explícitos no respetivo terminal (da entidade de onde provém o erro).



**Makefile**
"make": compila instâncias executáveis do Central Server (de nome 'CS'), do Working Server (de nome 'WS'), e do user (de nome 'user'), na mesma diretoria;
"make clear": elimina executáveis, na mesma diretoria, de nome 'user', 'CS', e 'WS';
"make compile_and_execute": compila e executa uma instância do Central Server (de nome 'CS'), uma instância do Working Server (de nome 'WS'), e uma instância do user (de nome 'user'), todas no mesmo host com definições default;
"make compile": compila instâncias executáveis do Central Server (de nome 'CS'), do Working Server (de nome 'WS'), e do user (de nome 'user'), na mesma diretoria;
"make execute": executa uma instância do Central Server (de nome 'CS'), uma instância do Working Server (de nome 'WS'), e uma instância do user (de nome 'user'), todas no mesmo host com definições default;
"make CS": compila e executa uma instância do Central Server (de nome 'CS') com definições default, na mesma diretoria;
"make WS": compila e executa uma instância do Working Server (de nome 'WS') com definições default, na mesma diretoria;
"make user": compila e executa uma instância do user (de nome 'user') com definições default, na mesma diretoria;
"make compile_CS": compila instância executável do Central Server (de nome 'CS'), na mesma diretoria;
"make compile_WS": compila uma instância executável do Working Server (de nome 'WS'), na mesma diretoria;
"make compile_user": compila uma instância executável do user (de nome 'user'), na mesma diretoria.



**Execução**
Para executar instâncias compiladas do Central Server, Working Server e user, com argumentos definidos, a seguinte notação deve ser usada:
Central Server: ./CS [-p CSport],
	onde: CSport is the well-known port where the CS server accepts requests, in TCP. This is an optional argument. If omitted, it assumes the value 58012.
Working Server: ./WS PTC1 … PTCn [-p WSport] [-n CSname] [-e CSport], 
	onde:	PTC1 … PTCn is the list of available file processing task codes implemented by this WS, each with a different 3 letter code. There is a maximum of 99 task codes that it is possible to implement.
		WSport is the well-known port where the WS server accepts requests from the CS. This is an optional argument. If omitted, it assumes the value 59000.
		CSname is the name of the machine where the central server (CS) runs. This is an optional argument. If this argument is omitted, the CS should be running on the same machine. CSport  is the well-known port where the CS server accepts requests. This is an optional argument. If omitted, it assumes the value 58012;
user: ./user [-n CSname] [-p CSport],
	onde:	CSname is the name of the machine where the central server (CS) runs. This is an optional argument. If this argument is omitted, the CS should be running on the same machine.
		CSport  is the well-known port where the CS server accepts user requests, in TCP. This is an optional argument. If omitted, it assumes the value 58012.



**Teste**
Executando o comando "make" descrito na makefile, três terminais ('CS', 'WS', 'user') são lançados. No user, é possível a seguinte interação:
---
>list
1 - FLW - find longest word
2 - UPP - convert to upper case
3 - LOW - convert to lower case
4 - WCT - word count
>request WCT helloworld.txt
2
>exit
---
Nos terminais que se referem ao CS e ao WS, é possível verficar a progressão da execução das tarefas.



**Grupo 12**
Alexandra Figueiredo, 83420
Denis Voicu, 83443
Mariana Loureiro, 83520