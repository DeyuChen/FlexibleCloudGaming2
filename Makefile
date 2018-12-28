CC = g++
LIB = -Llib
INC = -Iinclude/libHh
CFLAGS = -c -std=c++11 
LDFLAGS = -lSDL2 -lglut -lGL -lGLU -lglfw -lGLEW -lavformat -lavcodec -lavutil -lswscale -lSDL2_image -lHh -lpthread 
SOURCES = glWindow.cpp PMeshRenderer.cpp codec.cpp
OBJECTS1 = server.o $(SOURCES:.cpp=.o)
OBJECTS2 = client.o $(SOURCES:.cpp=.o)

EXECUTABLE1 = server
EXECUTABLE2 = client

all: $(SOURCES) $(EXECUTABLE1) $(EXECUTABLE2)

$(EXECUTABLE1): $(OBJECTS1)
	$(CC) $(LIB) $(OBJECTS1) -o $@ $(LDFLAGS)

$(EXECUTABLE2): $(OBJECTS2)
	$(CC) $(LIB) $(OBJECTS2) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

clean:
	rm -rf *o $(EXECUTABLE)
