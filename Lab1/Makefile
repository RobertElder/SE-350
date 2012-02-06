CXX = gcc
CXXFLAGS = -g --cflags
OBJECTS = *.o
DEPENDS = ${OBJECTS:.o=.d}
EXEC = G015

${EXEC} : ${OBJECTS} 
	${CXX} ${CXXFLAGS} ${OBJECTS} -o ${EXEC}

clean :
	rm -rf ${DEPENDS} ${OBJECTS} ${EXEC}

-include ${DEPENDS}
