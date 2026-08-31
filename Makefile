CXX = g++
CXXFLAGS = -O3 -Wall -std=c++11 -pthread

TARGET = varredor
SRC = varredor.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET) parcial_*.txt