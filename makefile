CXX?=c++
CXXFLAGS?= -std=c++11

.PHONY: all msg clean fullclean

all: msg ev3proto

msg:
	@echo --- C++11 ---

ev3proto: main.cpp
	${CXX} ${CXXFLAGS} -O3 -o $@ $<

small: main.cpp
	${CXX} ${CXXFLAGS} -Os -o main $<
	-strip main
	-sstrip main

debug: main.cpp
	${CXX} ${CXXFLAGS} -O0 -g -o main $<

asm: ev3proto.asm

ev3proto.asm: main.cpp
	${CC} ${CFLAGS} -S -o main.asm $<

run: msg main
	time ./main

clean:
	rm -f ev3proto *.o ev3proto.asm

fullclean: clean
