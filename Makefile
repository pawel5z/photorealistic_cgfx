CC = g++
CFLAGS = -std=c++17 -O3
LFLAGS = -lassimp -lpthread -lepoxy -lGL -lglfw -lboost_program_options -lIlmImf -lImath -lHalf -lIex -lIexMath -lIlmThread
NAME = raytrace

OBJS = $(shell find . -name '*.cpp' -a -not \( -name main.cpp -or -name test.cpp \) | sed 's/\.cpp/\.o/')
DEPS = $(shell find . -name '*.hpp')

$(NAME) : source/main.o $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $^ $(LFLAGS)

all: $(NAME) test

test : source/test.o $(OBJS)
	$(CC) $(CFLAGS) -o test $^ $(LFLAGS)

debug : CFLAGS = -std=c++17 -g -DDEBUG
debug : all

%.o : %.cpp $(DEPS)
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	@find . -name '*.o' -delete

distclean : clean
	@rm -f $(NAME) test

.PHONY : clean distclean debug
