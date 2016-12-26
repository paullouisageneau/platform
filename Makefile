CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-std=c++11 -g -O2
LDFLAGS=-g
LDLIBS=-lpthread -lGL -lGLEW -lSDL2 -lnettle -lhogweed -lgmp -lgnutls

SRCS=$(shell printf "%s " pla/*.cpp p3d/*.cpp demo/*.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))

OUTPUT=platformdemo

all: $(OUTPUT)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -I. -MMD -MP -o $@ -c $<
	
-include $(subst .o,.d,$(OBJS))
	
$(OUTPUT): $(OBJS)
	$(CXX) $(LDFLAGS) -o $(OUTPUT) $(OBJS) $(LDLIBS) 
	
clean:
	$(RM) pla/*.o pla/*.d p3d/*.o p3d/*.d demo/*.o demo/*.d

dist-clean: clean
	$(RM) $(OUTPUT)
	$(RM) pla/*~ p3d/*~ demo/*~
