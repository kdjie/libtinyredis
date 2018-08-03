INCLUDE = -I. -I..
LIBRARY =

THREADFLAGS = -pthread -DTHREAD

CXXFLAGS    = -Wall -ggdb -DXML_NULL -DTIXML_USE_STL 
CXXFLAGS_MT  = $(CXXFLAGS) $(THREADFLAGS)
CXXFLAGS_R    = -Wall -O2 -O3 -DXML_NULL -DTIXML_USE_STL 
CXXFLAGS_MT_R  = $(CXXFLAGS_R) $(THREADFLAGS)

LINKFLAGS   = -Wl,-rpath,./:../bin
LINKFLAGS_MT = $(LINKFLAGS)

CXXFLAGS_EXTERN =
LINKFLAGS_EXTERN =

TARGET   = libtinyredis.a
TARGET_MT = libtinyredis_mt.a
TARGET_R   = libtinyredis.ra
TARGET_MT_R = libtinyredis_mt.ra

TARGET_SAMPLE = sample

#SRC_FILES   = $(wildcard *.cpp)
SRC_FILES   = RedisFactory.cpp RedisClient.cpp

SRC_SAMPLE_FILES   = sample.cpp

OBJ_FILES   = $(SRC_FILES:.cpp=.o)
OBJ_FILES_MT = $(SRC_FILES:.cpp=.mo)
OBJ_FILES_R   = $(SRC_FILES:.cpp=.ro)
OBJ_FILES_MT_R = $(SRC_FILES:.cpp=.mro)

OBJ_SAMPLE_FILES = $(SRC_SAMPLE_FILES:.cpp=.o)

$(TARGET) : $(OBJ_FILES)
	ar -ru -o $@ $(OBJ_FILES)

$(TARGET_MT) : $(OBJ_FILES_MT)
	ar -ru -o $@ $(OBJ_FILES_MT)
	
$(TARGET_R) : $(OBJ_FILES_R)
	ar -ru -o $@ $(OBJ_FILES_R)

$(TARGET_MT_R) : $(OBJ_FILES_MT_R)
	ar -ru -o $@ $(OBJ_FILES_MT_R)

$(TARGET_SAMPLE) : $(OBJ_SAMPLE_FILES)
	g++ -o $@ $(OBJ_SAMPLE_FILES) $(CXXFLAGS) $(CXXFLAGS_EXTERN) $(TARGET) -lhiredis

%.o : %.cpp
	g++ -c -o $@ $< $(CXXFLAGS) $(CXXFLAGS_EXTERN) $(INCLUDE)
%.mo : %.cpp
	g++ -c -o $@ $< $(CXXFLAGS_MT) $(CXXFLAGS_EXTERN) $(INCLUDE)
%.ro : %.cpp
	g++ -c -o $@ $< $(CXXFLAGS_R) $(CXXFLAGS_EXTERN) $(INCLUDE)
%.mro : %.cpp
	g++ -c -o $@ $< $(CXXFLAGS_MT_R) $(CXXFLAGS_EXTERN) $(INCLUDE)

.PHONY : all clean

all : $(TARGET) $(TARGET_MT) $(TARGET_R) $(TARGET_MT_R) $(TARGET_SAMPLE)

clean :
	-rm $(OBJ_FILES) $(OBJ_FILES_MT) $(OBJ_FILES_R) $(OBJ_FILES_MT_R) $(OBJ_SAMPLE_FILES)
	-rm $(TARGET) $(TARGET_MT) $(TARGET_R) $(TARGET_MT_R) $(TARGET_SAMPLE)
