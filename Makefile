C = gcc
CXX = g++

INCLUDES = -I$(glfw_inc) -I$(glad_inc) -I$(glm)
LIBRARIES = -L$(glfw_lib) 

glfw = c:/Users/Dominic/dev/opengl/glfw-3.3.8.bin.WIN64
	glfw_inc = $(glfw)/include
	glfw_lib = $(glfw)/lib-mingw-w64

glad = c:/users/Dominic/dev/opengl/glad
	glad_inc = $(glad)/include

glm = c:/users/Dominic/dev/opengl/glm

CFLAGS = -Wall -ggdb -O3 $(INCLUDES)
CXXFLAGS = -Wall -ggdb -O3 $(INCLUDES)
LDFLAGS = $(LIBRARIES) $(INCLUDES) -lglfw3 -lopengl32 -lglu32 -lgdi32

TARGET = main.exe
cpp_files = main.cc scenemanager.cc
objects = $(cpp_files:.cpp=.o) glad.o
headers =

all: $(TARGET)

$(TARGET): $(objects) 
	$(CXX) -o $@ $^ $(LDFLAGS)

# I use this for building on windows command prompt.
clean:
	del *.o 
