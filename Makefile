CC = g++
LIB = -Llib
INC = -Iinclude/libHh
CFLAGS = -c -std=c++17
LDFLAGS = -lSDL2 -lglut -lGL -lGLU -lglfw -lGLEW -lavformat -lavcodec -lavutil -lswscale -lHh -lpthread `pkg-config --cflags --libs protobuf`
PROTOS = CommProto.proto PMeshInfo.proto
SOURCES = glWindow.cpp PMeshController.cpp PMeshRenderer.cpp codec.cpp communicator.cpp tokenGenerator.cpp $(PROTOS:.proto=.pb.cc)
OBJECTS = $(SOURCES:.cpp=.o)

EXECUTABLE = server client serverMT clientMT

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): % : %.o $(OBJECTS)
	$(CC) $(LIB) $(OBJECTS) $< -o $@ $(LDFLAGS)

%.pb.cc : %.proto
	protoc --cpp_out=./ $<

.cpp.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

.cc.o:
	$(CC) $(CFLAGS) $(INC) $< -o $@

clean:
	rm -rf *.o $(EXECUTABLE) *.pb.cc *.pb.h
