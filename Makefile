CC = g++
LIB = -Llib
INC = -Iinclude/libHh
CFLAGS = -c -std=c++11
LDFLAGS = -lSDL2 -lglut -lGL -lGLU -lglfw -lGLEW -lavformat -lavcodec -lavutil -lswscale -lSDL2_image -lHh -lpthread `pkg-config --cflags --libs protobuf`
SOURCES = glWindow.cpp PMeshRenderer.cpp codec.cpp communicator.cpp CommonProtocol.pb.cc
OBJECTS1 = server.o $(SOURCES:.cpp=.o)
OBJECTS2 = client.o $(SOURCES:.cpp=.o)

EXECUTABLE1 = server
EXECUTABLE2 = client

all: $(SOURCES) $(EXECUTABLE1) $(EXECUTABLE2)

$(EXECUTABLE1): $(OBJECTS1)
	$(CC) $(LIB) $(OBJECTS1) -o $@ $(LDFLAGS)

$(EXECUTABLE2): $(OBJECTS2)
	$(CC) $(LIB) $(OBJECTS2) -o $@ $(LDFLAGS)

CommonProtocol.pb.cc: CommonProtocol.proto
	protoc --cpp_out=./ ./CommonProtocol.proto

.cpp.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

.cc.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE1) $(EXECUTABLE2) *.pb.cc *.pb.h
