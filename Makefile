CXXFLAGS_EXTRA =
CXXFLAGS = -std=c++20 -Wall -Wextra $(CXXFLAGS_EXTRA) -I src -I for -I dep

%.vert.spv: %.vert
	glslangValidator $< -V -o $@
%.frag.spv: %.frag
	glslangValidator $< -V -o $@

TARGET = sbuild.exe
SRC = $(wildcard src/*.cpp)
SHA = sha/fwd_v2.vert sha/base.frag

OBJ = $(SRC:.cpp=.o)
SHA_VERT = $(SHA:.vert=.vert.spv)
SHA_FRAG = $(SHA:.frag=.frag.spv)
SHAS = $(filter-out $(SHA), $(SHA_VERT) $(SHA_FRAG))

FOR = for/fr.cpp for/vma.cpp
FOR_OBJ = $(FOR:.cpp=.o)

all: $(TARGET)

for/vma.o: CXXFLAGS_EXTRA = -Wno-nullability-completeness -Wno-missing-field-initializers -Wno-unused-variable -Wno-unused-parameter
src/main.o: $(wildcard src/*.hpp)

$(TARGET): $(SHAS) $(OBJ) $(FOR_OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(FOR_OBJ) -o $(TARGET) -L$(VULKAN_SDK)/Lib/ -lvulkan-1 -lglfw3

clean:
	rm -f $(SHAS) $(OBJ) $(TARGET)

clean_all: clean
	rm -f $(FOR_OBJ)