CC = g++
CFLAGS = -std=c++17 -O3
LFLAGS = -lassimp -lpthread -lepoxy -lGL -lglfw -lboost_program_options -lIlmImf -lImath -lHalf -lIex -lIexMath -lIlmThread
NAME = raytrace

OBJS = $(shell find . -name '*.cpp' | sed 's/\.cpp/\.o/')
DEPS = $(shell find . -name '*.hpp')

all: $(NAME)

$(NAME) : $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $^ $(LFLAGS)

debug : CFLAGS = -std=c++17 -g -DDEBUG
debug : $(NAME)

%.o : %.cpp $(DEPS)
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	@find . -name '*.o' -delete

distclean : clean
	@rm -f $(NAME)

.PHONY : clean distclean debug
