CC	 = $(GCC_INSTALL_DIR)/bin/g++

# OpenCV config
IPATHS	+= `pkg-config --cflags opencv`
LIBS	+= `pkg-config --libs opencv`

# general config
CFLAGS	+= -Wall -fopenmp #-pedantic
CFLAGS	+= -fcilkplus -O3
CFLAGS	+= -std=c++11

TARGET	 = calibrate

OBJECTS	+= 

$(TARGET): $(foreach OBJ, $(OBJECTS), $(OBJ).o) main.cpp
	@$(CC) -o $@ $^ $(IPATHS) $(LPATHS) $(CFLAGS) $(LIBS)

assembler: $(foreach OBJ, $(OBJECTS), $(OBJ).s) main.cpp

%.o: %.cpp
	@$(CC) $(IPATHS) $(CFLAGS) -c $<

%.s: %.cpp
	@$(CC) $(IPATHS) $(CFLAGS) -S $<

clean:
	@rm -rf *.o *.s $@ $(TARGET)
