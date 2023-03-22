CC=gcc
FLAGS=-g -Wall -std=c11
OBJ=obj
SRC=src
RES=res
LNKDIR=dependencies
ADINCL=-I X:/libs/GLFW/glfw-3.3.8/include -I X:/libs/glew/64/glew-2.1.0/include -I headers
LNK=-lglew32 -lglfw3 -lgdi32 -lopengl32
RM=del
CPY=xcopy
CPYFLAGS=/s /y
CPYDEST=bin\$(RES)

SRCS=$(wildcard $(SRC)/*.c)
OBJS=$(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))
BIN=bin/main.exe

all: $(BIN)

gdb: $(BIN)
	gdb ./$(BIN)

run: $(BIN)
	./$(BIN)

cpyres:
	$(CPY) $(CPYFLAGS) $(RES) $(CPYDEST)

release: FLAGS=-Wall -O2 -std=c11
release: clean
release: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(FLAGS) $(OBJS) $(LNK) -o $(BIN) -L $(LNKDIR)

$(OBJ)/%.o: $(SRC)/%.c
	$(CC) $(FLAGS) -c $< -o $@ $(ADINCL)

clean:
	cd obj && $(RM) *.o
	cd bin && $(RM) *.exe