CPP=g++ -std=c++17 -Wall -Wextra -Werror -g
CPPFLAGS=-I./include
LDFLAGS=-L./lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRCS=$(wildcard src/*.cpp)
OBJS=$(patsubst src/%.cpp,obj/%.o,$(SRCS))

build: $(OBJS)
	@mkdir -p bin
	$(CPP) $(OBJS) $(CPPFLAGS) $(LDFLAGS) -o bin/cgfs

obj/%.o: src/%.cpp
	@mkdir -p obj
	$(CPP) -c $< $(CPPFLAGS) -o $@

clean:
	@rm -rf obj bin