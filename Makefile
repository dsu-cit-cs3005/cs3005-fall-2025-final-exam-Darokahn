# Compiler
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic -g

# Targets

all: arena

config: config.struct makeConfig.c config.h
	$(CXX) makeConfig.c -o makeConfig
	./makeConfig

RobotBase.o: RobotBase.cpp RobotBase.h
	$(CXX) $(CXXFLAGS) -c RobotBase.cpp -fPIC

test_robot: test_robot.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) test_robot.cpp RobotBase.o -ldl -o test_robot

arena: config arena.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) arena.cpp -Wl,--export-dynamic RobotBase.o -fsanitize=address

clean:
	rm -f *.o test_robot *.so
