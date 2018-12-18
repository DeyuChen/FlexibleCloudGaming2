CC = g++
LIB = -Llib
INC = -Iinclude/libHh
CFLAGS = -c -std=c++11 
LDFLAGS = -lSDL2 -lglut -lGL -lGLU -lglfw -lGLEW -lavformat -lavcodec -lavutil -lswscale -lSDL2_image -lHh -lpthread 
SOURCES = glWindow.cpp PMeshRenderer.cpp
OBJECTS = main.o $(SOURCES:.cpp=.o)

EXECUTABLE = a.out

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LIB) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

clean:
	rm -rf *o $(EXECUTABLE)
