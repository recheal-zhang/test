SOURCEDIR := .
INCLUDEDIR := .
BUILD_DIR := ./bin
CURRENTDIR := .
EXE := $(BUILD_DIR)/server
CC := g++
LIBS := pthread
SOURCES := $(wildcard $(SOURCEDIR)/*.cpp $(SOURCEDIR)/*.c)
OBJS := $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SOURCES))) 
RM := rm -rf
CXXFLAGS := -Wall -g -O -std=c++11 -DDEBUG -DTCP_SOCKET

start: prepare $(EXE)

prepare: 
	mkdir -p $(BUILD_DIR)

$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(addprefix -l, $(LIBS));

#$(CURRENTDIR)/%.o: $(CURRENTDIR)/%.cpp
#	$(CC) $(addprefix -I, $(INCLUDEDIR)) $(addprefix -I, $(SOURCEDIR)) $(addprefix -I, $(CURRENTDIR)) -c $(CXXFLAGS) %< -o %@

$(SOURCEDIR)/%.o: $(SOURCEDIR)/%.cpp $(INCLUDEDIR)/%.h
	$(CC) -c $(CXXFLAGS) %< -o %@

.PHONY: clean rebuild
clean:
	$(RM) $(BUILD_DIR)
	$(RM) $(SOURCEDIR)/*.o
	$(RM) $(CURRENTDIR)/*.o
rebuild: clean start


