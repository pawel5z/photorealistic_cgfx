CC = g++
CFLAGS = -std=c++17 -O3
LFLAGS = -lassimp -lIL -lILU -lpthread -lepoxy -lGL -lglfw
NAME = raytrace

OBJS = $(shell find . -name '*.cpp' | sed 's/\.cpp/\.o/')
DEPS = $(shell find . -name '*.hpp')

all: $(NAME)

$(NAME) : $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $^ $(LFLAGS)

debug : CFLAGS += -Og -g
debug : $(NAME)

%.o : %.cpp $(DEPS)
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	@find . -name '*.o' -delete

distclean : clean
	@rm -f $(NAME)

.PHONY : clean distclean debug
