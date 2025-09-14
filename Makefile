CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O3 -I.
TARGET = kfetch
SRCS = kfetch.cpp config/config.cpp gpu/gpu.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	mkdir -p ~/.config/
	cp kfetch.conf.example ~/.config/kfetch.conf

.PHONY: all clean install
