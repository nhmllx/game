//Isaiah Malleaux
//program: background.cpp
//author:  Gordon Griesel
//date:    2017 - 2018
//
//The position of the background QUAD does not change.
//Just the texture coordinates change.
//In this example, only the x coordinates change.
//
#include <iostream>
#include <cstdlib>
#include <ostream>
#include <cstring>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "fonts.h"
#include <ctime>
#include <cmath>

//24-bit color:  8 + 8 + 8 = 24
//               R   G   B
//how many colors?  256*256*256 = 16-million+
//
//32-bit color:  8 + 8 + 8     = 24
//               R   G   B   A
//               R   G   B     = 24
//
//char data[1000][3]  aaabbbcccdddeeefff
//char data[1000][4]  aaa bbb ccc ddd eee fff
//
//
//
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

class Image {
    public:
        int width, height;
        unsigned char *data;
        ~Image() { delete [] data; }
        Image(const char *fname) {
            if (fname[0] == '\0')
                return;
            //printf("fname **%s**\n", fname);
            char name[40];
            strcpy(name, fname);
            int slen = strlen(name);
            name[slen-4] = '\0';
            //printf("name **%s**\n", name);
            char ppmname[80];
            sprintf(ppmname,"%s.ppm", name);
            //printf("ppmname **%s**\n", ppmname);
            char ts[100];
            //system("convert eball.jpg eball.ppm");
            sprintf(ts, "convert %s %s", fname, ppmname);
            system(ts);
            //sprintf(ts, "%s", name);
            FILE *fpi = fopen(ppmname, "r");
            if (fpi) {
                char line[200];
                fgets(line, 200, fpi);
                fgets(line, 200, fpi);
                while (line[0] == '#')
                    fgets(line, 200, fpi);
                sscanf(line, "%i %i", &width, &height);
                fgets(line, 200, fpi);
                //get pixel data
                int n = width * height * 3;			
                data = new unsigned char[n];			
                for (int i=0; i<n; i++)
                    data[i] = fgetc(fpi);
                fclose(fpi);
            } else {
                printf("ERROR opening image: %s\n",ppmname);
                exit(0);
            }
            unlink(ppmname);
        }
};
//Image img[1] = {"seamless_back.jpg"};
//Image img[2] = {"seamless_back.jpg", "elpis.jpg"};
Image img[3] = {"imgs/ce.png", "imgs/unit1.png", "imgs/squid.png"};
//ce is background
//squid is Tatics game and start
//elpis is Placeholder
class Texture {
    public:
        Image *backImage;
        GLuint backTexture;
        float xc[2];
        float yc[2];
};

class Global {
    public:
        int xres, yres;
        Texture tex;
        Texture unit1;
        Texture squid;
        int flag;
        int vsync; // :o
        char keys[65536];
        int fps;

        Global() {
            xres=1000, yres=800;
            flag = 1;
            memset(keys, 0, 65536);
            vsync = 1;
        }
} g;

class X11_wrapper {
    public:
        Display *dpy;
        Window win;
        GLXContext glc;
        X11_wrapper() {
            GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
            //GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
            setup_screen_res(1000, 600);
            dpy = XOpenDisplay(NULL);
            if(dpy == NULL) {
                printf("\n\tcannot connect to X server\n\n");
                exit(EXIT_FAILURE);
            }
            Window root = DefaultRootWindow(dpy);
            XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
            if(vi == NULL) {
                printf("\n\tno appropriate visual found\n\n");
                exit(EXIT_FAILURE);
            } 
            Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
            XSetWindowAttributes swa;
            swa.colormap = cmap;
            swa.event_mask =
                ExposureMask | KeyPressMask | KeyReleaseMask | PointerMotionMask |
                ButtonPressMask | ButtonReleaseMask |
                StructureNotifyMask | SubstructureNotifyMask;
            win = XCreateWindow(dpy, root, 0, 0, g.xres, g.yres, 0,
                    vi->depth, InputOutput, vi->visual,
                    CWColormap | CWEventMask, &swa);
            set_title();
            glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
            glXMakeCurrent(dpy, win, glc);
        }
        void cleanupXWindows() {
            XDestroyWindow(dpy, win);
            XCloseDisplay(dpy);
        }
        void setup_screen_res(const int w, const int h) {
            g.xres = w;
            g.yres = h;
        }
        void reshape_window(int width, int height) {
            //window has been resized.
            setup_screen_res(width, height);
            glViewport(0, 0, (GLint)width, (GLint)height);
            glMatrixMode(GL_PROJECTION); glLoadIdentity();
            glMatrixMode(GL_MODELVIEW); glLoadIdentity();
            glOrtho(0, g.xres, 0, g.yres, -1, 1);
            set_title();
        }
        void set_title() {
            //Set the window title bar.
            XMapWindow(dpy, win);
            XStoreName(dpy, win, "scrolling background (seamless)");
        }
        bool getXPending() {
            return XPending(dpy);
        }
        XEvent getXNextEvent() {
            XEvent e;
            XNextEvent(dpy, &e);
            return e;
        }
        void swapBuffers() {
            glXSwapBuffers(dpy, win);
        }
        void check_resize(XEvent *e) {
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
} x11;

void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void physics(void);
void render(void);
void render2(void);
//void animate(frameno, w, h, string img)'


//===========================================================================
/*
void *spriteThread(void *arg)
{
        //-------------------------------------------------------------------
        //Setup timers
        //const double OOBILLION = 1.0 / 1e9;
        struct timespec pstart, pend;
        struct timespec start, end;
        extern double timeDiff(struct timespec *start, struct timespec *end);
        extern void timeCopy(struct timespec *dest, struct timespec *source);
        //--------------------------------------------------------------------
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
*/
//===========================================================================
int main()
{
    init_opengl();
    srand(time(NULL));
    clock_gettime(CLOCK_REALTIME, &timePause);
    clock_gettime(CLOCK_REALTIME, &timeStart);
   // x11.set_mouse_position(100,100);
    struct timespec fpsStart;
    struct timespec fpsCurr;
    clock_gettime(CLOCK_REALTIME, &fpsStart);
    int fps = 0;

    int done=0;
    while (!done) {
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            x11.check_resize(&e);
            check_mouse(&e);
            done = check_keys(&e);
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


        physics();
        if (g.flag == 1)
        {
            render();
        }



        x11.swapBuffers();

    }
    return 0;
}

unsigned char *buildAlphaData(Image *img, int r, int g, int bb)
{
    //add 4th component to RGB stream...
    int i;
    int a,b,c;
    unsigned char *newdata, *ptr;
    unsigned char *data = (unsigned char *)img->data;
    newdata = (unsigned char *)malloc(img->width * img->height * 4);
    ptr = newdata;
    for (i=0; i<img->width * img->height * 3; i+=3) {
        a = *(data+0);
        b = *(data+1);
        c = *(data+2);
        *(ptr+0) = a;
        *(ptr+1) = b;
        *(ptr+2) = c;
        if (a == r && b == g && c == bb)
            *(ptr+3) = 0;
        else
            *(ptr+3) = 1;
        //-----------------------------------------------
        ptr += 4;
        data += 3;
    }
    return newdata;
}

void init_opengl(void)
{
    //OpenGL initialization
    glViewport(0, 0, g.xres, g.yres);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //This sets 2D mode (no perspective)
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
    //Clear the screen
    glClearColor(1.0, 1.0, 1.0, 1.0);
    //glClear(GL_COLOR_BUFFER_BIT);
    //Do this to allow texture maps
    glEnable(GL_TEXTURE_2D);
    //
    //load the images file into a ppm structure.
    //
    //------------------------------------------------------------------------------
    g.tex.backImage = &img[0];
    //create opengl texture elements
    glGenTextures(1, &g.tex.backTexture);
    int w = g.tex.backImage->width;
    int h = g.tex.backImage->height;
    glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
            GL_RGB, GL_UNSIGNED_BYTE, g.tex.backImage->data);
    g.tex.xc[0] = 0.0;
    g.tex.xc[1] = 0.25;
    g.tex.yc[0] = 0.0;
    g.tex.yc[1] = 1.0;
    //------------------------------------------------------------------------------
    /*g.elpis.backImage = &img[1];
      glGenTextures(1, &g.elpis.backTexture);
      w = g.elpis.backImage->width;
      h = g.elpis.backImage->height;
      glBindTexture(GL_TEXTURE_2D, g.elpis.backTexture);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
      glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
      GL_RGB, GL_UNSIGNED_BYTE, g.elpis.backImage->data);
      g.elpis.xc[0] = 0.0;
      g.elpis.xc[1] = 1.0;
      g.elpis.yc[0] = 0.0;
      g.elpis.yc[1] = 1.0;*/
    //----------------------------------------------------------------------------------------------------
    g.squid.backImage = &img[2];
    glGenTextures(1, &g.squid.backTexture);
    w = g.squid.backImage->width;
    h = g.squid.backImage->height;
    glBindTexture(GL_TEXTURE_2D, g.squid.backTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    //glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
    //						GL_RGB, GL_UNSIGNED_BYTE, g.squid.backImage->data);
    unsigned char *data2 = buildAlphaData(&img[2], 0, 0, 0);/////CLEAR

    glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
    g.squid.xc[0] = 0.0;
    g.squid.xc[1] = 1.0;
    g.squid.yc[0] = 0.0;
    g.squid.yc[1] = 1.0;
    //--------------------------------------------------------------------------------------------------
    g.unit1.backImage = &img[1];
    glGenTextures(1, &g.unit1.backTexture);
    w = g.unit1.backImage->width;
    h = g.unit1.backImage->height;
    glBindTexture(GL_TEXTURE_2D, g.unit1.backTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    //glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
    //						GL_RGB, GL_UNSIGNED_BYTE, g.squid.backImage->data);
    unsigned char *data3 = buildAlphaData(&img[1], 131, 164, 131);/////CLEAR

    glTexImage2D(GL_TEXTURE_2D, 0, 4, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data3);
    g.unit1.xc[0] = 0.0;
    g.unit1.xc[1] = 1.0;
    g.unit1.yc[0] = 0.0;
    g.unit1.yc[1] = 1.0;

}

void check_mouse(XEvent *e)
{
    //Did the mouse move?
    //Was a mouse button clicked?
    static int savex = 0;
    static int savey = 0;
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
                render2();
                printf("hi");
                // XDestroyWindow(x11.dpy, x11.win);
                g.flag = 0;

            }
            //Left button is down
        }
        if (e->xbutton.button==3) {
            //Right button is down
        }
    }
    if (savex != e->xbutton.x || savey != e->xbutton.y) {
        //Mouse moved
        savex = e->xbutton.x;
        savey = e->xbutton.y;
    }
}

int check_keys(XEvent *e)
{
    //Was there input from the keyboard?
    // if (e->type == KeyPress) {
    //   int key = XLookupKeysym(&e->xkey, 0);
    // if (key == XK_Escape) {
    //      return 1;
    //  }
    // }
    // (void)shift;i
    //
    //keyboard input?
    static int shift=0;
    if (e->type != KeyPress && e->type != KeyRelease)
        return 0;
    int key = (XLookupKeysym(&e->xkey, 0) & 0x0000ffff);
    //Log("key: %i\n", key);
    if (e->type == KeyRelease) {
        g.keys[key]=0;
        if (key == XK_Shift_L || key == XK_Shift_R)
            shift=0;
        return 0;
    }
    if (e->type == KeyPress) {
        //std::cout << "press" << std::endl;
        g.keys[key]=1;
        if (key == XK_Shift_L || key == XK_Shift_R) {
            shift=1;
            return 0;
        }
    }
    (void)shift;

    switch (key) {
        case XK_Escape:
            return 1;
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
        case XK_f:
                   break;
        case XK_s:
                   break;
        case XK_Down:
                   break;
        case XK_equal:
                   break;
        case XK_minus:
                   break;
    }

    return 0;
}

void physics()
{
    //move the background
    g.tex.xc[0] += 0.00001;
    g.tex.xc[1] += 0.00001;
}

void render()
{
   /* Rect r;
   // glClear(GL_COLOR_BUFFER_BIT);
    r.bot = g.yres - 20;
    r.left = 10;
    r.center = 0;
    ggprint8b(&r, 16, 0x00ffff00, "vsync: %s", ((g.vsync)?"ON":"OFF"));
    ggprint8b(&r, 16, 0x00ffff00, "fps: %i", g.fps);
    printf("FRAMES: %i\n", g.fps );
    printf("VSYNC: %s\n", ((g.vsync) ? "ON":"OFF") );
*/

    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0, 1.0, 1.0);
    //draw background
    glBindTexture(GL_TEXTURE_2D, g.tex.backTexture);
    glBegin(GL_QUADS);//background
    glTexCoord2f(g.tex.xc[0], g.tex.yc[1]); glVertex2i(0, 0);
    glTexCoord2f(g.tex.xc[0], g.tex.yc[0]); glVertex2i(0, g.yres);
    glTexCoord2f(g.tex.xc[1], g.tex.yc[0]); glVertex2i(g.xres, g.yres);
    glTexCoord2f(g.tex.xc[1], g.tex.yc[1]); glVertex2i(g.xres, 0);
    glEnd();
    //draw background
    //

    glBindTexture(GL_TEXTURE_2D, g.unit1.backTexture);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glColor4ub(255,255,255,255);
    glBegin(GL_QUADS);
    glTexCoord2f(g.unit1.xc[0], g.unit1.yc[1]); glVertex2i(10, 10);
    glTexCoord2f(g.unit1.xc[0], g.unit1.yc[0]); glVertex2i(10, 10+480);
    glTexCoord2f(g.unit1.xc[1], g.unit1.yc[0]); glVertex2i(10+32, 10+480);
    glTexCoord2f(g.unit1.xc[1], g.unit1.yc[1]); glVertex2i(10+32, 10);
    //glTexCoord2f(g.elpis.xc[0], g.elpis.yc[1]); glVertex2i(0, 0);
    //glTexCoord2f(g.elpis.xc[0], g.elpis.yc[0]); glVertex2i(0, 100);
    //glTexCoord2f(g.elpis.xc[1], g.elpis.yc[0]); glVertex2i(100, 100);
    //glTexCoord2f(g.elpis.xc[1], g.elpis.yc[1]); glVertex2i(100, 0);
    glEnd();

    glDisable(GL_ALPHA_TEST);
    //  glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    glBindTexture(GL_TEXTURE_2D, g.squid.backTexture);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    glColor4ub(255,255,255,255);
    //glColor4ub(255, 255, 255, (unsigned char)alpha);
    glBegin(GL_QUADS); //start/placeholder



    glTexCoord2f(g.squid.xc[0], g.squid.yc[1]); glVertex2i(g.xres/3, 30);
    glTexCoord2f(g.squid.xc[0], g.squid.yc[0]); glVertex2i(g.xres/3, g.yres*0.50);
    glTexCoord2f(g.squid.xc[1], g.squid.yc[0]); glVertex2i(g.xres*0.70, g.yres*0.50);
    glTexCoord2f(g.squid.xc[1], g.squid.yc[1]); glVertex2i(g.xres * 0.70, 30);
    //glTexCoord2f(g.squid.xc[0], g.squid.yc[1]); glVertex2i(100, 0);
    //glTexCoord2f(g.squid.xc[0], g.squid.yc[0]); glVertex2i(100, 100);
    //glTexCoord2f(g.squid.xc[1], g.squid.yc[0]); glVertex2i(200, 100);
    //glTexCoord2f(g.squid.xc[1], g.squid.yc[1]); glVertex2i(200, 0);
    glEnd();



/*    Rect r;
    glClear(GL_COLOR_BUFFER_BIT);
    r.bot = g.yres - 20;
    r.left = 10;
    r.center = 0;
    ggprint8b(&r, 16, 0x00ffff00, "vsync: %s", ((g.vsync)?"ON":"OFF"));
    ggprint8b(&r, 16, 0x00ffff00, "fps: %i", g.fps);*/




    glDisable(GL_ALPHA_TEST);

    Rect r;
   // glClear(GL_COLOR_BUFFER_BIT);
    r.bot = g.yres - 20;
    r.left = 10;
    r.center = 0;
    ggprint8b(&r, 16, 0x00ffff00, "vsync: %s", ((g.vsync)?"ON":"OFF"));
    ggprint8b(&r, 16, 0x00ffff00, "fps: %i", g.fps);
    printf("FRAMES: %i\n", g.fps );
    printf("VSYNC: %s\n", ((g.vsync) ? "ON":"OFF") );


}



void render2()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.5f, 0.5f, 0.5f); // Set grid color

    // Draw vertical lines
    glBegin(GL_LINES);
    for (int x = 0; x <= g.xres; x += 25) { // Adjust 20 for spacing
        glVertex2i(x, 0);
        glVertex2i(x, g.yres);
    }
    glEnd();

    // Draw horizontal lines
    glBegin(GL_LINES);
    for (int y = 0; y <= g.yres; y += 25) { // Adjust 20 for spacing
        glVertex2i(0, y);
        glVertex2i(g.xres, y);
    }
    glEnd();

    // Swap buffers after drawing
    x11.swapBuffers();
}



















