#
# To disable OpenAL sound, place comments on the #define
# and library statements.
#
# Like this...
#
#   #-D USE_OPENAL_SOUND \
#   #/usr/lib/x86_64-linux-gnu/libopenal.so \
#   #/usr/lib/x86_64-linux-gnu/libalut.so




all: background runner oalTest2

background: background.cpp timers.cpp 
	g++ background.cpp timers.cpp libggfonts.a -Wall -lX11 -lGL -lGLU -lm


runner: runner.cpp timers.cpp
	g++ runner.cpp timers.cpp libggfonts.a -Wall \
		-lGL -lGLU -lX11 -lpthread -o runner \
	-D USE_OPENAL_SOUND \
        /usr/lib/x86_64-linux-gnu/libopenal.so \
        /usr/lib/x86_64-linux-gnu/libalut.so

oalTest2: oalTest2.cpp
	g++ oalTest2.cpp -ootest2 \
	-D USE_OPENAL_SOUND \
	/usr/lib/x86_64-linux-gnu/libopenal.so \
	/usr/lib/x86_64-linux-gnu/libalut.so

clean:
	rm -f background a.out runner otest2


