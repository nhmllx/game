all: background runner

background: background.cpp timers.cpp 
	g++ background.cpp timers.cpp libggfonts.a -Wall -lX11 -lGL -lGLU -lm


runner: runner.cpp timers.cpp
	g++ runner.cpp timers.cpp libggfonts.a -Wall \
		-lGL -lGLU -lX11 -lpthread -o runner

clean:
	rm -f background a.out


