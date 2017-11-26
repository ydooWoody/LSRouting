CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG) -std=c++11 -pthread
LFLAGS = -pthread

all : manager

manager : manager.o router.o
	$(CC) $(LFLAGS) -o manager manager.o router.o

manager.o : manager.cpp project3.h
	$(CC) $(CFLAGS) manager.cpp

router.o : router.cpp
	$(CC) $(CFLAGS) router.cpp



