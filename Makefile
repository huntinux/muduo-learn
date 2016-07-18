
TARGET = mini-muduo

SRCS = $(wildcard ./*.cc)
OBJS = $(patsubst ./%.cc, obj/%.o, $(SRCS))
INCLUDES = -I.

COMFLAG = -Wall -O3 -Wno-deprecated $(INCLUDES)
LDFLAGS = -Wall -O3 
LIBS = -lpthread

obj/%.o:./%.cc
	g++ $(COMFLAG) -c $<  -o $@

all: obj $(OBJS)
	g++ $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)
	@echo "Build Successfully."

obj:
	test -d obj || mkdir obj

.PHONEY:clean
clean:
	rm -rf obj/*.o $(TARGET)
