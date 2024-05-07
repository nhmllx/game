//modified by:
//date:
//
//author: Gordon Griesel
//date: Spring 2022, 2024
//purpose: work with animated sprites
//
#include <iostream>
#include <fstream>
#include <cstring>
#include <ostream>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <chrono>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
//for text on the screen
#include "fonts.h"
#include <pthread.h>
#include <random>
#include <fcntl.h>
#include <sys/stat.h>
//#ifdef USE_OPENAL_SOUND
//#include </usr/include/AL/alut.h>
//#endif //USE_OPENAL_SOUND


using Clock = std::chrono::high_resolution_clock;
#ifdef USE_SOUND
#include </usr/include/AL/alut.h>
class Openal {
    ALuint alBuffer[2];
    ALuint alSource[2];
    public:
    Openal() {
        //Get started right here.
        alutInit(0, NULL);
        if (alGetError() != AL_NO_ERROR) {
            printf("ERROR: alutInit()\n");
        }
        //Clear error state.
        alGetError();
        //
        //Setup the listener.
        //Forward and up vectors are used.
        //The person listening is facing forward toward the sound.
        //The first 3 components of vec are 0,0,1
        //this means that the person is facing x=0, y=0, z=1, forward.
        //The second 3 components means that up is x=0,y=1,z=0, straight up!
        float vec[6] = {0.0f,0.0f,1.0f, 0.0f,1.0f,0.0f};
        alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
        alListenerfv(AL_ORIENTATION, vec);
        alListenerf(AL_GAIN, 1.0f);
        //
        //Buffers hold the sound information.
        alBuffer[0] = alutCreateBufferFromFile("./mach3.wav");
        //let's create a looping sound.
        //alBuffer[1] = alutCreateBufferFromFile("./737engine.wav");
        alBuffer[1] = alutCreateBufferFromFile("./mach3.wav");

        //
        //Source refers to the sound.
        //Generate 2 sources, and store in the matching buffers.
        alGenSources(2, alSource);
        alSourcei(alSource[0], AL_BUFFER, alBuffer[0]);
        alSourcei(alSource[1], AL_BUFFER, alBuffer[1]);
        //
        //FirstSet volume and pitch to normal, no looping of sound.
        alSourcef(alSource[0], AL_GAIN, 1.0f);
        alSourcef(alSource[0], AL_PITCH, 1.0f);
        alSourcei(alSource[0], AL_LOOPING, AL_FALSE);
        //alSourcei(alSource[0], AL_LOOPING, AL_TRUE);
        if (alGetError() != AL_NO_ERROR) {
            printf("ERROR: setting source\n");
        }
        alSourcef(alSource[1], AL_GAIN, 0.5f);
        alSourcef(alSource[1], AL_PITCH, 1.0f);
        alSourcei(alSource[1], AL_LOOPING, AL_TRUE);
        if (alGetError() != AL_NO_ERROR) {
            printf("ERROR: setting source\n");
        }
    }
    void playSound(int i)
    {
        alSourcePlay(alSource[i]);
        //	for (int i=0; i<1000; i++) {
        //      alSourcePlay(alSource[0]);
        //	    usleep(500000);
    }
    }
    ~Openal() {
        //Cleanup.
        //First delete the sources.
        alDeleteSources(1, &alSource[0]);
        alDeleteSources(1, &alSource[1]);
        //Delete the buffers.
        alDeleteBuffers(1, &alBuffer[0]);
        alDeleteBuffers(1, &alBuffer[1]);
        //
        //Close out OpenAL itself.
        //Get active context.
        ALCcontext *Context = alcGetCurrentContext();
        //Get device for active context.
        ALCdevice *Device = alcGetContextsDevice(Context);
        //Disable context.
        alcMakeContextCurrent(NULL);
        //Release context(s).
        alcDestroyContext(Context);
        //Close device.
        alcCloseDevice(Device);
    }
}oal;

#endif //USE_OPENAL_SOUND
void playSound(int s) {
#ifdef USE_SOUND
    oal.playSound(s);
    printf("sound");
#else
    if (s) {}
#endif
}

class Image {
    public:
        int width, height, max;
        char *data;
        Image() { }
        Image(const char *fname) {
            bool isPPM = true;
            char str[1200];
            char newfile[200];
            ifstream fin;
            char *p = strstr((char *)fname, ".ppm");
            if (!p) {
                //not a ppm file
                isPPM = false;
                strcpy(newfile, fname);
                newfile[strlen(newfile)-4] = '\0';
                strcat(newfile, ".ppm");
                sprintf(str, "convert %s %s", fname, newfile);
                system(str);
                fin.open(newfile);
            } else {
                fin.open(fname);
            }
            char p6[10];
            fin >> p6;
            fin >> width >> height;
            fin >> max;
            data = new char [width * height * 3];
            fin.read(data, 1);
            fin.read(data, width * height * 3);
            fin.close();
            if (!isPPM)
                unlink(newfile);
        }
} road("imgs/ground.png"),
    sprite("imgs/runner.png"),
    clouds("imgs/back.png"),
    tv("imgs/tv.png"),
    title("imgs/ce.png"),
    punch("imgs/punch.ppm"),
    // punch("imgs/pawnch.png"),
    idle("imgs/idle.png"),
    idletv("imgs/idletv.png"),
    boost("imgs/speed.png"),
    ptime("imgs/ptime.png"),
    health("imgs/health.png"),
    taunt("imgs/taunt.png"),
    // tarr[5] = {"imgs/t1.png","imgs/t2.png","imgs/t3.png","imgs/t4.png","imgs/t5.png"},
    t1("imgs/t1.png"),
    t2("imgs/t2.png"),
    t3("imgs/t3.png"),
    t4("imgs/t4.png"),
    t5("imgs/t5.png"),
    bomb("imgs/bomb.png");

struct Vector {
    float x,y,z;
};

typedef double Flt;
struct Box {
    Flt h,w, pos[3];
};
//a game object
class Player {
    public:
        Flt pos[3]; //vector
        Flt vel[3]; //vector
        float w, h;
        Box box[5];
        unsigned int color;
        bool alive_or_dead;
        Flt mass;
        Player() {
            w = h = 4.0;
            pos[0] = 1.0;
            pos[1] = 200.0;
            vel[0] = 4.0;
            vel[1] = 0.0;
        }
        Player(int x, int y) {
            w = h = 4.0;
            pos[0] = x;
            pos[1] = y;
            vel[0] = 0.0;
            vel[1] = 0.0;
        }
        void set_dimensions(int x, int y) {
            w = (float)x * 0.05;
            h = w;
        }
} player(200,100);

class Global {
    public:
        int xres, yres;
        double sxres, syres;
        //char keys[65536];
        char keys[ 0xffff ];
        //the box components
        float pos[2];
        float w;
        float dir;
        int inside;
        //[----------IMAGE ID's------------]
        unsigned int texid;
        unsigned int roadid;
        unsigned int spriteid;
        unsigned int tvid;
        unsigned int ttid;
        unsigned int idleid;
        unsigned int idletvid;
        unsigned int punchid;
        unsigned int bombid;
        unsigned int boostid;
        unsigned int ptimeid;
        unsigned int healthid;
        unsigned int tauntid;
        unsigned int t1id; //not animated
        unsigned int t2id; //not animated
        unsigned int t3id; //not animated
        unsigned int t4id; //not animated
        unsigned int t5id; //not animated
                           // unsigned int tt[5] = {t1id, t2id, t3id, t4id, t5id};
                           //unsigned int nameid;

        Flt gravity;
        int frameno;
        int jump;
        int pause;
        int show_boxes;
        int vsync; // :o
        int fps;
        int flag;

        Global() {
            memset(keys, 0, 0xffff);
            xres = 800;
            yres = 400;
            sxres = (double)xres;
            syres = (double)yres;
            //box
            w = 20.0f;
            pos[0] = 0.0f + w;
            pos[1] = yres/2.0f;
            dir = 5.0f;
            inside = 0;
            gravity = 20.0;
            frameno = 0;
            jump = 0;
            pause = 0;
            show_boxes = 0;
            vsync = 1;
            flag = 1;
        }
} g;

class X11_wrapper {
    private:
        Display *dpy;
        Window win;
        GLXContext glc;
    public:
        ~X11_wrapper();
        X11_wrapper();
        void set_title();
        bool getXPending();
        XEvent getXNextEvent();
        void swapBuffers();
        void reshape_window(int width, int height);
        void check_resize(XEvent *e);
        void check_mouse(XEvent *e);
        int check_keys(XEvent *e);
} x11;

//Function prototypes
void init_opengl(void);
void physics(void);
void render(void);
void renderTitle(void);


void *spriteThread(void *arg)
{
    //-------------------------------------------------------------------------
    //Setup timers
    //    const double physicsRate = 1.0 / 60.0;
    //	const double OOBILLION = 1.0 / 1e9;
    struct timespec pstart, pend;// for sprites
    struct timespec start, end;
    extern double timeDiff(struct timespec *start, struct timespec *end);
    extern void timeCopy(struct timespec *dest, struct timespec *source);
    //  extern double physicsCountdown;
    //  extern double timeSpan;
    //-------------------------------------------------------------------------
    clock_gettime(CLOCK_REALTIME, &pstart);
    clock_gettime(CLOCK_REALTIME, &start);
    double diff;
    while (1) {
        //if some amount of time has passed, change the frame number.
        clock_gettime(CLOCK_REALTIME, &pend);
        diff = timeDiff(&pstart, &pend);
        if (diff >= (1.0 / 60.0)) {
            physics();  //move things
            timeCopy(&pstart, &pend);
        }
        clock_gettime(CLOCK_REALTIME, &end);
        diff = timeDiff(&start, &end);
        if (diff >= 0.043) {
            //enough time has passed
            if (!g.pause) {
                ++g.frameno;
                //  g.frameno = g.frameno + 0.5;
            }
            if (g.frameno > 21)//total frame number
                g.frameno = 0;
            timeCopy(&start, &end);
        }
        usleep(1000);        //pause to let X11 work better
    }
    return (void *)0;
}
//TIMER STUFF-----------------------------------------------------------------------------
const double physicsRate = 1.0 / 60.0;
const double oobillion = 1.0 / 1e9;
extern struct timespec timeStart, timeCurrent;
extern struct timespec timePause;
extern double physicsCountdown;
extern double timeSpan;
extern double timeDiff(struct timespec *start, struct timespec *end);
extern void timeCopy(struct timespec *dest, struct timespec *source);
//-----------------------------------------------------------------------------

int main()
{
    cout << "Program starting up!" << endl;
    char str[100];
    int n = 0;
    for (int i=0; i<5; i++) {
        str[n] = 'd' + i;
        n++;
        str[n] = '\0';
    }
    cout << str << endl;

    //start the thread...
    pthread_t th;
    pthread_create(&th, NULL, spriteThread, NULL);

    init_opengl();
    //main game loop
    srand(time(NULL));
    clock_gettime(CLOCK_REALTIME, &timePause);
    clock_gettime(CLOCK_REALTIME, &timeStart);
    struct timespec fpsStart;
    struct timespec fpsCurr;
    clock_gettime(CLOCK_REALTIME, &fpsStart);
    int fps = 0;

    int done = 0;
    while (!done) {
        //process events...
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.check_resize(&e);
            x11.check_mouse(&e);
            done = x11.check_keys(&e);
        }
        clock_gettime(CLOCK_REALTIME, &timeCurrent);
        timeSpan = timeDiff(&timeStart, &timeCurrent);
        timeCopy(&timeStart, &timeCurrent);
        physicsCountdown += timeSpan;
        //         while (physicsCountdown >= physicsRate) {
        //                   physics();
        //                     physicsCountdown -= physicsRate;
        //                     //                }
        ++fps;
        clock_gettime(CLOCK_REALTIME, &fpsCurr);
        double diff = timeDiff(&fpsStart, &fpsCurr);
        if (diff >= 1.0) {
            g.fps = fps;
            fps = 0;
            timeCopy(&fpsStart, &fpsCurr);
        }


        //physics();         //moved to a thread
        if (g.flag == 1)
        {
            renderTitle();            //draw things
        }
        if (g.flag == 0)
        {
            render();
        }
        x11.swapBuffers();   //make video memory visible
        usleep(1000);        //pause to let X11 work better
    }
    cleanup_fonts();
    return 0;
}

X11_wrapper::~X11_wrapper()
{
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    int w = g.xres, h = g.yres;
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        cout << "\n\tcannot connect to X server\n" << endl;
        exit(EXIT_FAILURE);
    }
    Window root = DefaultRootWindow(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    if (vi == NULL) {
        cout << "\n\tno appropriate visual found\n" << endl;
        exit(EXIT_FAILURE);
    } 
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask | SubstructureNotifyMask;
    win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
            InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    set_title();
    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
    //Set the window title bar.
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "4490 runner");
}

bool X11_wrapper::getXPending()
{
    //See if there are pending events.
    return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
    //Get a pending event.
    XEvent e;
    XNextEvent(dpy, &e);
    return e;
}

void X11_wrapper::swapBuffers()
{
    glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
    //window has been resized.
    g.xres = width;
    g.yres = height;
    //
    glViewport(0, 0, (GLint)width, (GLint)height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
}

void X11_wrapper::check_resize(XEvent *e)
{
    //The ConfigureNotify is sent by the
    //server if the window is resized.
    if (e->type != ConfigureNotify)
        return;
    XConfigureEvent xce = e->xconfigure;
    if (xce.width != g.xres || xce.height != g.yres) {
        //Window size did change.
        reshape_window(xce.width, xce.height);
    }
}
//-----------------------------------------------------------------------------

void X11_wrapper::check_mouse(XEvent *e)
{
    static int savex = 0;
    static int savey = 0;

    //Weed out non-mouse events
    if (e->type != ButtonRelease &&
            e->type != ButtonPress &&
            e->type != MotionNotify) {
        //This is not a mouse event that we care about.
        return;
    }
    //
    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        if (e->xbutton.button==1) {

            //printf("look: %i\n", savex);
            if (savex > g.xres*0.445 && savex < g.xres*0.605){
                printf("look: %i\n", savex);
                // x11.cleanupXWindows();
                //     render();
                printf("hi");
                // XDestroyWindow(x11.dpy, x11.win);
                g.flag = 0;

            }
            //Left button is down
        }

        if (e->xbutton.button==3) {
            //Right button was pressed.
            return;
        }
    }
    if (e->type == MotionNotify) {
        //The mouse moved!
        if (savex != e->xbutton.x || savey != e->xbutton.y) {
            savex = e->xbutton.x;
            savey = e->xbutton.y;
            //Code placed here will execute whenever the mouse moves.

        }
    }
}

int X11_wrapper::check_keys(XEvent *e)
{
    if (e->type != KeyPress && e->type != KeyRelease)
        return 0;
    int key = XLookupKeysym(&e->xkey, 0);
    if (e->type == KeyPress)   g.keys[key] = 1;
    if (e->type == KeyRelease) g.keys[key] = 0;
    if (e->type == KeyPress) {
        switch (key) {
            case XK_1:
                playSound(0);
                //Key 1 was pressed
                break;
            case XK_p:
                g.pause ^= 1;
                break;
            case XK_b:
                g.show_boxes = !g.show_boxes;
                //g.show_boxes = ~g.show_boxes;
                break;
            case XK_v: {
                           g.vsync ^= 1;
                           //vertical synchronization
                           //https://github.com/godotengine/godot/blob/master/platform/
                           //x11/context_gl_x11.cpp
                           static PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = NULL;
                           glXSwapIntervalEXT =
                               (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddressARB(
                                       (const GLubyte *)"glXSwapIntervalEXT");
                           GLXDrawable drawable = glXGetCurrentDrawable();
                           if (g.vsync) {
                               glXSwapIntervalEXT(x11.dpy, drawable, 1);
                           } else {
                               glXSwapIntervalEXT(x11.dpy, drawable, 0);
                           }
                           std::cout << "VVVVVVV" <<std::endl;
                           break;
                       }

                       //case XK_j:
                       //	//jump
                       //	g.jump = 1;
                       //	//g.player.vel[1] += 10;
                       //	break;
            case XK_Return:
                       //jump
                       g.jump = 1;
                       break;
            case XK_Escape:
                       //Escape key was pressed
                       return 1;
        }
    }
    return 0;
}
//yyunsigned int tt[5] = {g.t1id, g.t2id, g.t3id, g.t4id, g.t5id};

void init_opengl(void)
{
    //    unsigned int tt[5] = {g.t1id, g.t2id, g.t3id, g.t4id, g.t5id};
    //OpenGL initialization
    glViewport(0, 0, g.xres, g.yres);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    //Set 2D mode (no perspective)
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
    //
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //Set the screen background color
    glClearColor(0.1, 0.1, 0.1, 1.0);
    //
    //allow 2D texture maps
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();


    //--------------------------background pizza---------------------------------------------------------------------------------------------------
    glGenTextures(1, &g.texid);
    glBindTexture(GL_TEXTURE_2D, g.texid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, clouds.width, clouds.height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, clouds.data);

    //--------------------------purple floor---------------------------------------------------------------------------------------------------
    glGenTextures(1, &g.roadid);
    glBindTexture(GL_TEXTURE_2D, g.roadid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, road.width, road.height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, road.data);
    //-----title scween------------------------------------------------------------------------------------------------------------------------
    glGenTextures(1, &g.ttid);
    glBindTexture(GL_TEXTURE_2D, g.ttid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, title.width, title.height, 0,
            GL_RGB, GL_UNSIGNED_BYTE, title.data);
    //-----------------------------------------------------------------------------------------------------------------------------
    //-----title name------------------------------------------------------------------------------------------------------------------------
    /*glGenTextures(1, &g.nameid);
      glBindTexture(GL_TEXTURE_2D, g.nameid);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
      glTexImage2D(GL_TEXTURE_2D, 0, 3, name.width, name.height, 0,
      GL_RGB, GL_UNSIGNED_BYTE, name.data);*/
    //-----------------------------------------------------------------------------------------------------------------------------
    //runner sprite
    //make a new data stream, with 4 color components
    //add an alpha channel
    //
    //  0 aaabbbccc
    //  1 dddeeefff
    //  0 aaaabbbbcccc
    //  1 ddddeeeeffff
    //--------------------------sprite-------------------------------------------------------------------------------------------------------
    unsigned char *data13 = new unsigned char [t1.width * t1.height * 4];
    for (int i=0; i<t1.height; i++) {
        for (int j=0; j<t1.width; j++) {
            int offset  = i*t1.width*3 + j*3;
            int offset2 = i*t1.width*4 + j*4;
            data13[offset2+0] = t1.data[offset+0];
            data13[offset2+1] = t1.data[offset+1];
            data13[offset2+2] = t1.data[offset+2];
            data13[offset2+3] =
                ((unsigned char)t1.data[offset+0] != 255 ||
                 (unsigned char)t1.data[offset+1] != 0 ||
                 (unsigned char)t1.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.t1id);
    glBindTexture(GL_TEXTURE_2D, g.t1id);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t1.width, t1.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data13);
    delete [] data13;
    //--------------------------sprite-------------------------------------------------------------------------------------------------------
    /* for (int i = 0; i < 5; i++){
       unsigned char *data12 = new unsigned char [tarr[i].width * tarr[i].height * 4];
       for (int i=0; i<tarr[i].height; i++) {
       for (int j=0; j<tarr[i].width; j++) {
       int offset  = i*tarr[i].width*3 + j*3;
       int offset2 = i*tarr[i].width*4 + j*4;
       data12[offset2+0] = tarr[i].data[offset+0];
       data12[offset2+1] = tarr[i].data[offset+1];
       data12[offset2+2] = tarr[i].data[offset+2];
       data12[offset2+3] =
       ((unsigned char)tarr[i].data[offset+0] != 255 ||
       (unsigned char)tarr[i].data[offset+1] != 0 ||
       (unsigned char)tarr[i].data[offset+2] != 0) ? 255 : 0;
       }
       }
       glGenTextures(1, &g.tt[i]);
       glBindTexture(GL_TEXTURE_2D, g.tt[i]);
       glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
       glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
       glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tarr[i].width, tarr[i].height,
       0, GL_RGBA, GL_UNSIGNED_BYTE, data12);
       delete [] data12;
       }
       */
    //--------------------------sprite-------------------------------------------------------------------------------------------------------
    unsigned char *data2 = new unsigned char [sprite.width * sprite.height * 4];
    for (int i=0; i<sprite.height; i++) {
        for (int j=0; j<sprite.width; j++) {
            int offset  = i*sprite.width*3 + j*3;
            int offset2 = i*sprite.width*4 + j*4;
            data2[offset2+0] = sprite.data[offset+0];
            data2[offset2+1] = sprite.data[offset+1];
            data2[offset2+2] = sprite.data[offset+2];
            data2[offset2+3] =
                ((unsigned char)sprite.data[offset+0] != 255 ||
                 (unsigned char)sprite.data[offset+1] != 0 ||
                 (unsigned char)sprite.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.spriteid);
    glBindTexture(GL_TEXTURE_2D, g.spriteid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite.width, sprite.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
    delete [] data2;
    //--------------------------------TV-------------------------------------------------------------------------------------------------
    unsigned char *data4 = new unsigned char [tv.width * tv.height * 4];
    for (int i=0; i<tv.height; i++) {
        for (int j=0; j<tv.width; j++) {
            int offset  = i*tv.width*3 + j*3;
            int offset2 = i*tv.width*4 + j*4;
            data4[offset2+0] = tv.data[offset+0];
            data4[offset2+1] = tv.data[offset+1];
            data4[offset2+2] = tv.data[offset+2];
            data4[offset2+3] =
                ((unsigned char)tv.data[offset+0] != 255 ||
                 (unsigned char)tv.data[offset+1] != 0 ||
                 (unsigned char)tv.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.tvid);
    glBindTexture(GL_TEXTURE_2D, g.tvid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tv.width, tv.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data4);
    delete [] data4;
    //--------------------------------------------TV BOMB----------------------------------------------[
    unsigned char *data8 = new unsigned char [bomb.width * bomb.height * 4];
    for (int i=0; i<bomb.height; i++) {
        for (int j=0; j<bomb.width; j++) {
            int offset  = i*bomb.width*3 + j*3;
            int offset2 = i*bomb.width*4 + j*4;
            data8[offset2+0] = bomb.data[offset+0];
            data8[offset2+1] = bomb.data[offset+1];
            data8[offset2+2] = bomb.data[offset+2];
            data8[offset2+3] =
                ((unsigned char)bomb.data[offset+0] != 255 ||
                 (unsigned char)bomb.data[offset+1] != 0 ||
                 (unsigned char)bomb.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.bombid);
    glBindTexture(GL_TEXTURE_2D, g.bombid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bomb.width, bomb.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data8);
    delete [] data8;
    //----------------------------------------------------------IDLETV------------------------------------------------------------------------

    unsigned char *data11 = new unsigned char [idletv.width * idletv.height * 4];
    for (int i=0; i<idletv.height; i++) {
        for (int j=0; j<idletv.width; j++) {
            int offset  = i*idletv.width*3 + j*3;
            int offset2 = i*idletv.width*4 + j*4;
            data11[offset2+0] = idletv.data[offset+0];
            data11[offset2+1] = idletv.data[offset+1];
            data11[offset2+2] = idletv.data[offset+2];
            data11[offset2+3] =
                ((unsigned char)idletv.data[offset+0] != 255 ||
                 (unsigned char)idletv.data[offset+1] != 0 ||
                 (unsigned char)idletv.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.idletvid);
    glBindTexture(GL_TEXTURE_2D, g.idletvid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, idletv.width, idletv.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data11);
    delete [] data11;

    //--------------------------------------------------------IDLE---------------------------------------------------------------------------
    unsigned char *data5 = new unsigned char [idle.width * idle.height * 4];
    for (int i=0; i<idle.height; i++) {
        for (int j=0; j<idle.width; j++) {
            int offset  = i*idle.width*3 + j*3;
            int offset2 = i*idle.width*4 + j*4;
            data5[offset2+0] = idle.data[offset+0];
            data5[offset2+1] = idle.data[offset+1];
            data5[offset2+2] = idle.data[offset+2];
            data5[offset2+3] =
                ((unsigned char)idle.data[offset+0] != 255 ||
                 (unsigned char)idle.data[offset+1] != 0 ||
                 (unsigned char)idle.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.idleid);
    glBindTexture(GL_TEXTURE_2D, g.idleid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, idle.width, idle.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data5);
    delete [] data5;
    //------------------------------------------------------------------------------------------------------------------------------------
    unsigned char *data6 = new unsigned char [punch.width * punch.height * 4];
    for (int i=0; i<punch.height; i++) {
        for (int j=0; j<punch.width; j++) {
            int offset  = i*punch.width*3 + j*3;
            int offset2 = i*punch.width*4 + j*4;
            data6[offset2+0] = punch.data[offset+0];
            data6[offset2+1] = punch.data[offset+1];
            data6[offset2+2] = punch.data[offset+2];
            data6[offset2+3] =
                ((unsigned char)punch.data[offset+0] != 205 ||
                 (unsigned char)punch.data[offset+1] != 0 ||
                 (unsigned char)punch.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.punchid);
    glBindTexture(GL_TEXTURE_2D, g.punchid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, punch.width, punch.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data6);
    delete [] data6;
    //-----------------------------------------------------------------------------------------------------------------------------------
    unsigned char *data9 = new unsigned char [boost.width * boost.height * 4];
    for (int i=0; i<boost.height; i++) {
        for (int j=0; j<boost.width; j++) {
            int offset  = i*boost.width*3 + j*3;
            int offset2 = i*boost.width*4 + j*4;
            data9[offset2+0] = boost.data[offset+0];
            data9[offset2+1] = boost.data[offset+1];
            data9[offset2+2] = boost.data[offset+2];
            data9[offset2+3] =
                ((unsigned char)boost.data[offset+0] != 255 ||
                 (unsigned char)boost.data[offset+1] != 0 ||
                 (unsigned char)boost.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.boostid);
    glBindTexture(GL_TEXTURE_2D, g.boostid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, boost.width, boost.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data9);
    delete [] data9;
    //-----------------------------------------------------------------------------------------------------------------------------------
    unsigned char *data10 = new unsigned char [ptime.width * ptime.height * 4];
    for (int i=0; i<ptime.height; i++) {
        for (int j=0; j<ptime.width; j++) {
            int offset  = i*ptime.width*3 + j*3;
            int offset2 = i*ptime.width*4 + j*4;
            data10[offset2+0] = ptime.data[offset+0];
            data10[offset2+1] = ptime.data[offset+1];
            data10[offset2+2] = ptime.data[offset+2];
            data10[offset2+3] =
                ((unsigned char)ptime.data[offset+0] != 255 ||
                 (unsigned char)ptime.data[offset+1] != 0 ||
                 (unsigned char)ptime.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.ptimeid);
    glBindTexture(GL_TEXTURE_2D, g.ptimeid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ptime.width, ptime.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data10);
    delete [] data10;
    //-----------------------------------------------------------------------------------------------------------------------------------
    unsigned char *data14 = new unsigned char [taunt.width * taunt.height * 4];
    for (int i=0; i<taunt.height; i++) {
        for (int j=0; j<taunt.width; j++) {
            int offset  = i*taunt.width*3 + j*3;
            int offset2 = i*taunt.width*4 + j*4;
            data14[offset2+0] = taunt.data[offset+0];
            data14[offset2+1] = taunt.data[offset+1];
            data14[offset2+2] = taunt.data[offset+2];
            data14[offset2+3] =
                ((unsigned char)taunt.data[offset+0] != 255 ||
                 (unsigned char)taunt.data[offset+1] != 0 ||
                 (unsigned char)taunt.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.tauntid);
    glBindTexture(GL_TEXTURE_2D, g.tauntid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, taunt.width, taunt.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data14);
    delete [] data14;
    //-----------------------------------------------------------------------------------------------------------------------------------
    unsigned char *data15 = new unsigned char [health.width * health.height * 4];
    for (int i=0; i<health.height; i++) {
        for (int j=0; j<health.width; j++) {
            int offset  = i*health.width*3 + j*3;
            int offset2 = i*health.width*4 + j*4;
            data15[offset2+0] = health.data[offset+0];
            data15[offset2+1] = health.data[offset+1];
            data15[offset2+2] = health.data[offset+2];
            data15[offset2+3] =
                ((unsigned char)health.data[offset+0] != 255 ||
                 (unsigned char)health.data[offset+1] != 0 ||
                 (unsigned char)health.data[offset+2] != 0) ? 255 : 0;
        }
    }
    glGenTextures(1, &g.healthid);
    glBindTexture(GL_TEXTURE_2D, g.healthid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, health.width, health.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data15);
    delete [] data15;

}

#define GRAVITY 0.6

void physics()
{
    if (g.keys[XK_j] == 1) {
        player.vel[1] += 0.5;
        player.vel[0] += 0.2;
    }
    player.vel[1] -= GRAVITY;
    player.pos[0] += player.vel[0];
    player.pos[1] += player.vel[1];
    if (player.pos[1] < 60) {
        player.pos[1] = 60;
        player.vel[1] = 0.0;
    }
    if (player.pos[0] > g.xres) {
        player.vel[0] -= 1.0;
    }
}
//variables for manipulating and controlling sprites
double mover = 3.2; //influence x pos
int moving = 0;//moving flag
double shake = 3.3; //influence y pos
double rshake = 1.0;//shakes the road
int shakecount = 22;//shake duration
int shaker = 0;//shake flag
double jump = 0;
double jumpflag = 0;
int jumping = 0;
double posy = g.yres/3.2;
int upp = -100;
unsigned int tt[5] = {g.t1id, g.t2id, g.t3id, g.t4id, g.t5id};
// tarr[5] = {"imgs/t1.png","imgs/t2.png","imgs/t3.png","imgs/t4.png","imgs/t5.png"},
bool keypress = false;
bool iskeypress = false;

//double jumper(double x){
//  x=(x+9)/1.07;
//return x;
//}
    //[SHAKE]-----
   void shakef()
{
    if (g.keys[XK_t] == 1)
    {
        shaker = 1;
    }
    if (shaker == 1)
    {
        if (shakecount >= 0){
            int rando = rand() % 2;
            if (rando == 0){
                shake = 3.45;
                rshake = 0.975;
            }
            if (rando == 1){
                shake = 3.15;
                rshake = 1.025;
            }
            shakecount--;
        }
        if (shakecount == 0){shaker = 0;}
    }
    else {
        shake = 3.3;
        rshake = 1.0;
        shakecount = 22;
    }
    //[END OF SHAKE]-----
   }
    //------------health---------------------------------------------------------------------------
void hats(float offsetx, float offsety)
{
    glPushMatrix();
glTranslatef(g.xres * offsetx, g.yres * offsety, 0.0f);
glEnable(GL_ALPHA_TEST);
glAlphaFunc(GL_GREATER, 0.0f);
glColor4ub(255, 255, 255, 255);
glBindTexture(GL_TEXTURE_2D, g.healthid);
float sw = 1216.0f;
int numSprites = 19;
float spriteWidth = sw / numSprites;
int spriteIndex = (g.frameno - 1) % numSprites;
float tx111 = spriteIndex * spriteWidth / sw;
float tx222 = (spriteIndex + 1) * spriteWidth / sw;
float ty111 = 0.0f;
float ty222 = 1.0f;
float www = g.xres / 23;
float hhh = g.yres / 10;
glBegin(GL_QUADS);
glTexCoord2f(tx111, ty222); glVertex2f(-www, -hhh);
glTexCoord2f(tx111, ty111); glVertex2f(-www, hhh);
glTexCoord2f(tx222, ty111); glVertex2f(www, hhh);
glTexCoord2f(tx222, ty222); glVertex2f(www, -hhh);
glEnd();
glBindTexture(GL_TEXTURE_2D, 0);
glDisable(GL_ALPHA_TEST);
if (g.show_boxes) {
    glColor3ub(255, 255, 0);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-www, -hhh);
    glVertex2f(-www, hhh);
    glVertex2f(www, hhh);
    glVertex2f(www, -hhh);
    glEnd();
}
glPopMatrix();
}
void render()
{
    shakef();
    //[THE BACKGROUND]--------
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3ub(255, 255, 255);
    glBindTexture(GL_TEXTURE_2D, g.texid);
    static float camerax = 0.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(camerax+0, 1); glVertex2i(0,      0);
    glTexCoord2f(camerax+0, 0); glVertex2i(0,      g.yres);
    glTexCoord2f(camerax+1, 0); glVertex2i(g.xres, g.yres);
    glTexCoord2f(camerax+1, 1); glVertex2i(g.xres, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    if (g.keys[XK_d] == 1 && g.keys[XK_g] != 1){
        camerax += 0.00275;
    }
    if (g.keys[XK_a] == 1 && g.keys[XK_g] != 1){
        camerax -= 0.00275;
    }



    //road
    glColor3ub(200,200,200);
    glBindTexture(GL_TEXTURE_2D, g.roadid);
    static float xr = 0.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(xr+0, 1); glVertex2i(0,      0*rshake);
    glTexCoord2f(xr+0, 0); glVertex2i(0,      (g.yres/4.6) * rshake);
    glTexCoord2f(xr+1, 0); glVertex2i(g.xres, (g.yres/4.6) * rshake);
    glTexCoord2f(xr+1, 1); glVertex2i(g.xres, 0*rshake);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    // move the road, giving the illusion of th player sprite controling movement
    if (g.keys[XK_d] == 1 && g.keys[XK_g] != 1){//move road right
        xr += 0.055;
        playSound(0);
    }
    if (g.keys[XK_a] == 1 && g.keys[XK_g] != 1){//road left
        xr -= 0.055;
    }

    Rect r;
    r.bot = g.yres*0.5;
    r.left = g.xres * 0.85;
    r.center = 0;
    int score = 25;
    ggprint8b(&r, 16, 0x00ffffff, "speed : %i\n", score*2);
    ggprint8b(&r, 16, 0x00ffff00, "vsync: %s", ((g.vsync)?"ON":"OFF"));
    ggprint8b(&r, 16, 0x00ffff00, "fps: %i", g.fps);
    //printf("FRAMES: %i\n", g.fps );
    //printf("VSYNC: %s\n", ((g.vsync) ? "ON":"OFF"));
    //


    //if (g.keys[XK_space] == 1){
    if (g.keys[XK_w] == 1){
        jumping = 1;
        if( jumping == 1){
            if (jumpflag < 110){
                jump = ((jump + 9)/1.075);
                jumpflag = ((jumpflag + 9)/1.075);
                // jumper(jump);
            }
            if(jumpflag >= 110 && ((g.yres/shake)+jump <= 373)){
                //jump = 0;
                jump = ((jump - 5));
            }
            if (jump <= 0){
                jump = 0;
            }
        }
    }
    jumping = 0;
    //if (jump >= 110.99){
    //jump = jump - 3;
    //}
    //printf("%f\n", jump);
    //Draw man.
    //----------------------------------------------------------------------------------------------------
    //if(moving == 1){
    if ((g.keys[XK_d] == 1 || g.keys[XK_a] == 1) && g.keys[XK_g] != 1){
    // (g.keys[XK_d] == 0 && g.keys[XK_a] == 0 && g.keys[XK_g] != 1){
        glPushMatrix();
        glColor3ub(255, 255, 255);

        //if (g.keys[XK_space] == 1){
        //if (g.keys[XK_w] == 1){
        //   jump += 0.5;
        //}
        // printf("%f\n", jump);
        glTranslatef((g.xres/3.2), (g.yres/shake)+jump, 0.0f);
        //           x         y         z
        //}
        //set alpha test
        glEnable(GL_ALPHA_TEST);
        //transparent if alpha value is greater than 0.0
        glAlphaFunc(GL_GREATER, 0.0f);
        //Set 4-channels of color intensity
        glColor4ub(255,255,255,255);
        //
        glBindTexture(GL_TEXTURE_2D, g.spriteid);
        //make texture coordinates based on frame number.
        float tx1 = 0.0f + (float)((g.frameno-1) % 3) * ((300.0f/3.0f)/300.0f);
        float tx2 = tx1 + ((300.0f/3.0f)/300.0f);
        //float ty1 = 0.0f + (float)((g.frameno-1) / 1) * 1;
        float ty1 = 1.0f ;
        float ty2 = ty1 + 1;
        float w = g.xres/10;//size w
        float h = g.yres/8;//size h
        glBegin(GL_QUADS);

        if (g.keys[XK_a] == 0){//face sprite right
            glTexCoord2f(tx1, ty2); glVertex2f(-w, -h);
            glTexCoord2f(tx1, ty1); glVertex2f(-w,  h);
            glTexCoord2f(tx2, ty1); glVertex2f( w,  h);
            glTexCoord2f(tx2, ty2); glVertex2f( w, -h);
        }
        if (g.keys[XK_a] == 1){//face sprite left
            glTexCoord2f(tx1, ty2); glVertex2f(w, -h);
            glTexCoord2f(tx1, ty1); glVertex2f(w,  h);
            glTexCoord2f(tx2, ty1); glVertex2f( -w,  h);
            glTexCoord2f(tx2, ty2); glVertex2f( -w, -h);
        }

        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
        //
        if (g.show_boxes) {
            //Show the sprite's bounding box
            glColor3ub(255, 255, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(-w, -h);
            glVertex2f(-w,  h);
            glVertex2f( w,  h);
            glVertex2f( w, -h);
            glEnd();
        }
        glPopMatrix();

        glPushMatrix();
        glColor3ub(255, 255, 255);


        glTranslatef((g.xres/3.2), (g.yres/shake)+jump, 0.0f);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glColor4ub(255,255,255,95);
        glBindTexture(GL_TEXTURE_2D, g.boostid);
        float btx1 = 0.0f + (float)((g.frameno-1) % 9) * ((900.0f/9.0f)/900.0f);
        float btx2 = btx1 + ((900.0f/9.0f)/900.0f);
        float bty1 = 1.0f ;
        float bty2 = bty1 + 1;
        float bw = g.xres/10;//size w
        float bh = g.yres/8;//size h
        glBegin(GL_QUADS);

        if (g.keys[XK_a] == 0){//face sprite right
            glTexCoord2f(btx1, bty2); glVertex2f(-bw, -bh);
            glTexCoord2f(btx1, bty1); glVertex2f(-bw,  bh);
            glTexCoord2f(btx2, bty1); glVertex2f( bw,  bh);
            glTexCoord2f(btx2, bty2); glVertex2f( bw, -bh);
        }
        if (g.keys[XK_a] == 1){//face sprite left
            glTexCoord2f(btx1, bty2); glVertex2f(bw, -bh);
            glTexCoord2f(btx1, bty1); glVertex2f(bw,  bh);
            glTexCoord2f(btx2, bty1); glVertex2f( -bw,  bh);
            glTexCoord2f(btx2, bty2); glVertex2f( -bw, -bh);
        }

        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
        //
        if (g.show_boxes) {
            glColor3ub(255, 255, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(-bw, -bh);
            glVertex2f(-bw,  bh);
            glVertex2f( bw,  bh);
            glVertex2f( bw, -bh);
            glEnd();
        }
        glPopMatrix();
    }
    //----------------------------------------------------------------------------------------------------
    if (g.keys[XK_d] == 0 && g.keys[XK_a] == 0 && g.keys[XK_g] != 1){
        glPushMatrix();
        glColor3ub(255, 255, 255);
        glTranslatef((g.xres/3.2), (g.yres/shake) + jump, 0.0f);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glColor4ub(255,255,255,255);
        glBindTexture(GL_TEXTURE_2D, g.idleid);
        float spriteSheetWidth = 1100.0f;
        float spriteWidth = spriteSheetWidth / 11.0f;
        float itx1 = 0.0f + (float)((g.frameno - 1) % 11) * (spriteWidth / spriteSheetWidth);
        float itx2 = itx1 + (spriteWidth / spriteSheetWidth);
        float ity1 = 1.0f ;
        float ity2 = ity1 + 1;
        float iw = g.xres/10;//size w
        float ih = g.yres/8;//size h
        glBegin(GL_QUADS);

        if (g.keys[XK_a] == 0){//face sprite right
            glTexCoord2f(itx1, ity2); glVertex2f(-iw, -ih);
            glTexCoord2f(itx1, ity1); glVertex2f(-iw,  ih);
            glTexCoord2f(itx2, ity1); glVertex2f( iw,  ih);
            glTexCoord2f(itx2, ity2); glVertex2f( iw, -ih);
        }
        if (g.keys[XK_a] == 1){//face sprite left
            glTexCoord2f(itx1, ity2); glVertex2f(iw, -ih);
            glTexCoord2f(itx1, ity1); glVertex2f(iw,  ih);
            glTexCoord2f(itx2, ity1); glVertex2f( -iw,  ih);
            glTexCoord2f(itx2, ity2); glVertex2f( -iw, -ih);
        }

        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
        //
        if (g.show_boxes) {
            //Show the sprite's bounding box
            glColor3ub(255, 255, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(-iw, -ih);
            glVertex2f(-iw,  ih);
            glVertex2f( iw,  ih);
            glVertex2f( iw, -ih);
            glEnd();
        }
        glPopMatrix();
    }
    //-------------------------------------------------taunt---------------------------------------
   // if (g.keys[XK_g] == 1){
     //   keypress= true;
   // } else {
    //    keypress = false;
  //  }
  //  if (keypress && !iskeypress){
    if (g.keys[XK_g] == 1){

        glPushMatrix();
        glColor3ub(255, 255, 255);
        glTranslatef((g.xres/3.2), (g.yres/shake) + jump, 0.0f);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glColor4ub(255,255,255,255);
        glBindTexture(GL_TEXTURE_2D, g.t1id);
        float gw = g.xres/10;//size 
        float gh = g.yres/8;//size h
        glBegin(GL_QUADS);

        glTexCoord2f(0, 1); glVertex2f(-gw, -gh);
        glTexCoord2f(0, 0); glVertex2f(-gw,  gh);
        glTexCoord2f(1, 0); glVertex2f( gw,  gh);
        glTexCoord2f(1, 1); glVertex2f( gw, -gh);

        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
        if (g.show_boxes) {
            glColor3ub(255, 255, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(-gw, -gh);
            glVertex2f(-gw,  gh);
            glVertex2f( gw,  gh);
            glVertex2f( gw, -gh);
            glEnd();

            glPopMatrix();
        }
glPushMatrix();
glColor3ub(255, 255, 255);
     glTranslatef((g.xres/3.2), (g.yres/shake) + jump, 0.0f);

glEnable(GL_ALPHA_TEST);
glAlphaFunc(GL_GREATER, 0.0f);
glColor4ub(255, 255, 255, 255);
glBindTexture(GL_TEXTURE_2D, g.tauntid);

float spriteSheetWidth = 546.0f;
float spriteWidth = spriteSheetWidth / 4.0f; // Assuming 4 sprites in one row
float tx1 = 0.0f + (float)((g.frameno - 1) % 4) * (spriteWidth / spriteSheetWidth);
float tx2 = tx1 + (spriteWidth / spriteSheetWidth);
float ty1 = 0.0f; // Assuming sprites start from the top
float ty2 = 1.0f; // Assuming sprites end at the bottom

glBegin(GL_QUADS);
    glTexCoord2f(tx1, ty2); glVertex2f(-gw, -gh);
    glTexCoord2f(tx1, ty1); glVertex2f(-gw,  gh);
    glTexCoord2f(tx2, ty1); glVertex2f( gw,  gh);
    glTexCoord2f(tx2, ty2); glVertex2f( gw, -gh);
glEnd();
glBindTexture(GL_TEXTURE_2D, 0);
glDisable(GL_ALPHA_TEST);
if (g.show_boxes) {
    glColor3ub(255, 255, 0);
    glBegin(GL_LINE_LOOP);
            glVertex2f(-gw, -gh);
            glVertex2f(-gw,  gh);
            glVertex2f( gw,  gh);
            glVertex2f( gw, -gh);
}

glPopMatrix();
}
    //-------------idletv---------------------------------------------------------------------------
    glPopMatrix();
    glPushMatrix();
    glColor3ub(255, 255, 255);
    glTranslatef(g.xres*0.85, g.yres*0.85, 0.0f);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glColor4ub(255,255,255,255);
    glBindTexture(GL_TEXTURE_2D, g.idletvid);
    float tx11 = 0.0f + (float)((g.frameno-1) % 3) * ((300.0f/3.0f)/300.0f);
    float tx22 = tx11 + ((300.0f/3.0f)/300.0f);
    float ty11 = 1.0f ;
    float ty22 = ty11 + 1;
    float ww = g.xres/6;
    float hh = g.yres/5;
    glBegin(GL_QUADS);
    glTexCoord2f(tx11, ty22); glVertex2f(-ww, -hh);
    glTexCoord2f(tx11, ty11); glVertex2f(-ww,  hh);
    glTexCoord2f(tx22, ty11); glVertex2f( ww,  hh);
    glTexCoord2f(tx22, ty22); glVertex2f( ww, -hh);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_ALPHA_TEST);
    if (g.show_boxes) {
        glColor3ub(255, 255, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-ww, -hh);
        glVertex2f(-ww,  hh);
        glVertex2f( ww,  hh);
        glVertex2f( ww, -hh);
        glEnd();
    }
    glPopMatrix();
    //------------health---------------------------------------------------------------------------
/*    glPushMatrix();
glTranslatef(g.xres * 0.05, g.yres * 0.95, 0.0f);
glEnable(GL_ALPHA_TEST);
glAlphaFunc(GL_GREATER, 0.0f);
glColor4ub(255, 255, 255, 255);
glBindTexture(GL_TEXTURE_2D, g.healthid);
float sw = 1216.0f;
int numSprites = 19;
float spriteWidth = sw / numSprites;
int spriteIndex = (g.frameno - 1) % numSprites;
float tx111 = spriteIndex * spriteWidth / sw;
float tx222 = (spriteIndex + 1) * spriteWidth / sw;
float ty111 = 0.0f;
float ty222 = 1.0f;
float www = g.xres / 12;
float hhh = g.yres / 8;
glBegin(GL_QUADS);
glTexCoord2f(tx111, ty222); glVertex2f(-www, -hhh);
glTexCoord2f(tx111, ty111); glVertex2f(-www, hhh);
glTexCoord2f(tx222, ty111); glVertex2f(www, hhh);
glTexCoord2f(tx222, ty222); glVertex2f(www, -hhh);
glEnd();
glBindTexture(GL_TEXTURE_2D, 0);
glDisable(GL_ALPHA_TEST);
if (g.show_boxes) {
    glColor3ub(255, 255, 0);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-www, -hhh);
    glVertex2f(-www, hhh);
    glVertex2f(www, hhh);
    glVertex2f(www, -hhh);
    glEnd();
}
glPopMatrix();*/

    //use if statement to  chekc if man got hit
    hats(0.05, 0.92);//health 6
    hats(0.12, 0.92);//health 5
    hats(0.19, 0.92);//health 4
    hats(0.05, 0.82);//health 3
    hats(0.12, 0.82);//health 2
    hats(0.19, 0.82);//health 1
    //----------------------------------------------------------------------------------------------------
    //for the tv
    if (g.keys[XK_d] == 1 || g.keys[XK_a] == 1){
        glPushMatrix();
        glColor3ub(255, 255, 255);
        glTranslatef(g.xres*0.85, g.yres*0.85, 0.0f);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glColor4ub(255,255,255,255);
        glBindTexture(GL_TEXTURE_2D, g.tvid);
        float tx11 = 0.0f + (float)((g.frameno-1) % 3) * ((300.0f/3.0f)/300.0f);
        float tx22 = tx11 + ((300.0f/3.0f)/300.0f);
        float ty11 = 1.0f ;
        float ty22 = ty11 + 1;
        float ww = g.xres/6;
        float hh = g.yres/5;
        glBegin(GL_QUADS);
        glTexCoord2f(tx11, ty22); glVertex2f(-ww, -hh);
        glTexCoord2f(tx11, ty11); glVertex2f(-ww,  hh);
        glTexCoord2f(tx22, ty11); glVertex2f( ww,  hh);
        glTexCoord2f(tx22, ty22); glVertex2f( ww, -hh);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
        if (g.show_boxes) {
            glColor3ub(255, 255, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(-ww, -hh);
            glVertex2f(-ww,  hh);
            glVertex2f( ww,  hh);
            glVertex2f( ww, -hh);
            glEnd();
        }
        glPopMatrix();
    }
    //PIZZA TIME-------------------------------------------------------------
    if (upp <= g.yres + 600){
        glPushMatrix();
        glColor3ub(255, 255, 255);
        glTranslatef(g.xres/2, upp, 0.0f);
        upp = upp + 12;
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        glColor4ub(255,255,255,255);
        glBindTexture(GL_TEXTURE_2D, g.ptimeid);
        float ptx11 = 0.0f + (float)((g.frameno-1) % 2) * ((500.0f/2.0f)/500.0f);
        float ptx22 = ptx11 + ((500.0f/2.0f)/500.0f);
        float pty11 = 1.0f ;
        float pty22 = pty11 + 1;
        float pww = g.xres/9;
        float phh = g.yres/7;
        glBegin(GL_QUADS);
        glTexCoord2f(ptx11, pty22); glVertex2f(-pww, -phh);
        glTexCoord2f(ptx11, pty11); glVertex2f(-pww,  phh);
        glTexCoord2f(ptx22, pty11); glVertex2f( pww,  phh);
        glTexCoord2f(ptx22, pty22); glVertex2f( pww, -phh);
        glEnd();
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_ALPHA_TEST);
        if (g.show_boxes) {
            glColor3ub(255, 255, 0);
            glBegin(GL_LINE_LOOP);
            glVertex2f(-pww, -phh);
            glVertex2f(-pww,  phh);
            glVertex2f( pww,  phh);
            glVertex2f( pww, -phh);
            glEnd();
        }
        glPopMatrix();
    }
}

void renderTitle()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3ub(255, 255, 255);
    glBindTexture(GL_TEXTURE_2D, g.ttid);
    static float camerax = 0.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(camerax+0, 1); glVertex2i(0,      0);
    glTexCoord2f(camerax+0, 0); glVertex2i(0,      g.yres);
    glTexCoord2f(camerax+1, 0); glVertex2i(g.xres, g.yres);
    glTexCoord2f(camerax+1, 1); glVertex2i(g.xres, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);

    usleep(20000);
    glPushMatrix();
    glColor3ub(255, 255, 255);
    glTranslatef(g.xres*0.85, g.yres*0.85, 0.0f);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glColor4ub(255,255,255,255);
    glBindTexture(GL_TEXTURE_2D, g.bombid);
    float tx11 = 0.0f + (float)((g.frameno-1) % 2) * ((200.0f/2.0f)/200.0f);
    float tx22 = tx11 + ((200.0f/2.0f)/200.0f);
    float ty11 = 1.0f ;
    float ty22 = ty11 + 1;
    float ww = g.xres/6;
    float hh = g.yres/5;
    glBegin(GL_QUADS);
    glTexCoord2f(tx11, ty22); glVertex2f(-ww, -hh);
    glTexCoord2f(tx11, ty11); glVertex2f(-ww,  hh);
    glTexCoord2f(tx22, ty11); glVertex2f( ww,  hh);
    glTexCoord2f(tx22, ty22); glVertex2f( ww, -hh);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_ALPHA_TEST);
    //
    if (g.show_boxes) {
        //Show the sprite's bounding box
        glColor3ub(255, 255, 0);
        glBegin(GL_LINE_LOOP);
        glVertex2f(-ww, -hh);
        glVertex2f(-ww,  hh);
        glVertex2f( ww,  hh);
        glVertex2f( ww, -hh);
        glEnd();
    }
    glPopMatrix();
}

