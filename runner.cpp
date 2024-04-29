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
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
//for text on the screen
#include "fonts.h"
#include <pthread.h>






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
    title("imgs/ce.png");

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
            xres = 400;
            yres = 200;
            sxres = (double)xres;
            syres = (double)yres;
            //box
            w = 20.0f;
            pos[0] = 0.0f + w;
            pos[1] = yres/2.0f;
            dir = 5.0f;
            inside = 0;
            gravity = 20.0;
            frameno = 1;
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
            }
            if (g.frameno > 3)
                g.frameno = 1;
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

void init_opengl(void)
{
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
            int offset2 = i*sprite.width*4 + j*4;
            data4[offset2+0] = tv.data[offset+0];
            data4[offset2+1] = tv.data[offset+1];
            data4[offset2+2] = tv.data[offset+2];
            data4[offset2+3] =
                ((unsigned char)tv.data[offset+0] != 255 ||
                 (unsigned char)tv.data[offset+1] != 0 ||
                 (unsigned char)tv.data[offset+2] != 0) ? 255 : 0;
        }
    }
    //------------------------------------------------------------------------------------------------------------------------------------
    glGenTextures(1, &g.tvid);
    glBindTexture(GL_TEXTURE_2D, g.tvid);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tv.width, tv.height,
            0, GL_RGBA, GL_UNSIGNED_BYTE, data4);
    delete [] data4;
    //------------------------------------------------------------------------------------------------------------------------------------
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

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    //clouds
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
    camerax += 0.00275;

    //road
    glColor3ub(200,200,200);
    glBindTexture(GL_TEXTURE_2D, g.roadid);
    static float xr = 0.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(xr+0, 1); glVertex2i(0,      0);
    glTexCoord2f(xr+0, 0); glVertex2i(0,      g.yres/4.6);
    glTexCoord2f(xr+1, 0); glVertex2i(g.xres, g.yres/4.6);
    glTexCoord2f(xr+1, 1); glVertex2i(g.xres, 0);
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
    xr += 0.055;

    Rect r;
    r.bot = g.yres - 20;
    r.left = 10;
    r.center = 0;
    int score = 25;
    ggprint8b(&r, 16, 0x00ffffff, "speed : %i\n", score*2);
    ggprint8b(&r, 16, 0x00ffff00, "vsync: %s", ((g.vsync)?"ON":"OFF"));
    ggprint8b(&r, 16, 0x00ffff00, "fps: %i", g.fps);
    printf("FRAMES: %i\n", g.fps );
    printf("VSYNC: %s\n", ((g.vsync) ? "ON":"OFF"));
    //
    //Draw man.
    glPushMatrix();
    glColor3ub(255, 255, 255);
    glTranslatef(g.xres/3.2, g.yres/3.3, 0.0f);
    //           x         y         z
    //
    //set alpha test
    //https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/
    //xhtml/glAlphaFunc.xml
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
    glTexCoord2f(tx1, ty2); glVertex2f(-w, -h);
    glTexCoord2f(tx1, ty1); glVertex2f(-w,  h);
    glTexCoord2f(tx2, ty1); glVertex2f( w,  h);
    glTexCoord2f(tx2, ty2); glVertex2f( w, -h);
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
    //Draw mtv--------------------------
    glPushMatrix();
    glColor3ub(255, 255, 255);
    glTranslatef(g.xres*0.85, g.yres*0.85, 0.0f);
    //           x         y         z
    //
    //set alpha test
    glEnable(GL_ALPHA_TEST);
    //transparent if alpha value is greater than 0.0
    glAlphaFunc(GL_GREATER, 0.0f);
    //Set 4-channels of color intensity
    glColor4ub(255,255,255,255);
    //
    glBindTexture(GL_TEXTURE_2D, g.tvid);
    //make texture coordinates based on frame number.
    float tx11 = 0.0f + (float)((g.frameno-1) % 3) * ((300.0f/3.0f)/300.0f);
    float tx22 = tx11 + ((300.0f/3.0f)/300.0f);
    //	float tx11 = 0.0f + (float)((g.frameno-1) % 3) * (1.0f/3.0f);
    //	float tx22 = tx11 + (1.0f/3.0f);

    //float ty1 = 0.0f + (float)((g.frameno-1) / 1) * 1;
    //	float ty11 = 0.0f ;
    //	float ty22 =  1.0f;
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
    //i}
    }





void renderTitle()
{
    glClear(GL_COLOR_BUFFER_BIT);
    //clouds
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
   // camerax += 0.00275;

}


