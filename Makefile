CXX = c++
CXXFLAGS = -std=c++23 -Wall -Wextra -O3 -I.
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
	mkdir -p ~/.config
	if [ ! -f ~/.config/kfetch.conf ]; then \
	   cp kfetch.conf.example ~/.config/kfetch.conf; \
	fi

uninstall:
	rm -f /usr/local/bin/$(TARGET)
	rm -f ~/.config/kfetch.conf

.PHONY: all clean install uninstall
