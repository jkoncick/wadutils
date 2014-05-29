CPP=g++
CPPFLAGS=-Wall

HEADERS=wad_file.h wad_structs.h
OBJFILES=wad_file.o

all: listlumps texturefinder lumpfinder mapstats mapoptimizer

listlumps: listlumps.o $(OBJFILES)
	$(CPP) $(CPPFLAGS) -o $@ $^

listlumps.o: listlumps.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ listlumps.cpp

texturefinder: texturefinder.o $(OBJFILES)
	$(CPP) $(CPPFLAGS) -o $@ $^

texturefinder.o: texturefinder.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ texturefinder.cpp

lumpfinder: lumpfinder.o $(OBJFILES)
	$(CPP) $(CPPFLAGS) -o $@ $^

lumpfinder.o: lumpfinder.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ lumpfinder.cpp

mapstats: mapstats.o $(OBJFILES)
	$(CPP) $(CPPFLAGS) -o $@ $^

mapstats.o: mapstats.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ mapstats.cpp

mapoptimizer: mapoptimizer.o $(OBJFILES)
	$(CPP) $(CPPFLAGS) -o $@ $^

mapoptimizer.o: mapoptimizer.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ mapoptimizer.cpp

wad_file.o: wad_file.cpp $(HEADERS)
	$(CPP) $(CPPFLAGS) -c -o $@ wad_file.cpp
