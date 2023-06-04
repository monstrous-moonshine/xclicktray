#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#ifdef DEBUG
#include "util.h"
#endif

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define XEMBED_VERSION (0)
#define XEMBED_MAPPED  (1 << 0)

void send_message(
     Display* dpy, /* display */
     Window w,     /* sender (tray icon window) */
     long message, /* message opcode */
     long data1,   /* message data 1 */
     long data2,   /* message data 2 */
     long data3    /* message data 3 */
){
    XEvent ev;
  
    memset(&ev, 0, sizeof(ev));
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = XInternAtom (dpy, "_NET_SYSTEM_TRAY_OPCODE", False );
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = CurrentTime;
    ev.xclient.data.l[1] = message;
    ev.xclient.data.l[2] = data1;
    ev.xclient.data.l[3] = data2;
    ev.xclient.data.l[4] = data3;

    XSendEvent(dpy, w, False, NoEventMask, &ev);
    XSync(dpy, False);
}

void spawn(char *const *argv) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
    } else if (pid == 0) {
        if (setsid() < 0)
            perror("setsid");
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
        } else if (pid == 0) {
            execvp(argv[0], argv);
            perror("exec");
        } else {
            _exit(0);
        }
    } else {
        //
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s program [args...]\n", argv[0]);
        return 0;
    }
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy)
    {
        fprintf(stderr, "Could not open X display\n");
        return 1;
    }

    Atom _NET_SYSTEM_TRAY_S0 = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
    Window tray = XGetSelectionOwner(dpy, _NET_SYSTEM_TRAY_S0);
    if (!tray) {
        fprintf(stderr, "System tray not found\n");
        return 1;
    }

    int white_color = WhitePixel(dpy, DefaultScreen(dpy));
    Window w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                                   200, 100, 0, white_color, white_color);
    GC gc = XCreateGC(dpy, w, 0, NULL);
    XSetForeground(dpy, gc, white_color);
    XSelectInput(dpy, w, ButtonPressMask | ExposureMask);
    Atom _XEMBED_INFO = XInternAtom(dpy, "_XEMBED_INFO", False);
    const unsigned long xembed_info[2] = { XEMBED_VERSION, XEMBED_MAPPED };
    XChangeProperty(dpy, w, _XEMBED_INFO, _XEMBED_INFO, 32, PropModeReplace, (const unsigned char *)xembed_info, 2);
    send_message(dpy, tray, SYSTEM_TRAY_REQUEST_DOCK, w, 0, 0);

    int is_open = 1;
    while (is_open) {
        XEvent ev;
        XExposeEvent *expose_ev;
        XButtonEvent *button_ev;

        XNextEvent(dpy, &ev);
#ifdef DEBUG
        printf("Event received: %s\n", event_names[ev.type]);
#endif

        switch (ev.type) {
        case Expose:
            expose_ev = &ev.xexpose;
            XFillRectangle(dpy, w, gc, expose_ev->x, expose_ev->y,
                           expose_ev->width, expose_ev->height);
            XSync(dpy, False);
            break;
        case ButtonPress:
            button_ev = &ev.xbutton;
            if (button_ev->button == Button1 || button_ev->button == Button3) {
                if (button_ev->state & ShiftMask)
                    is_open = 0;
                else
                    spawn(&argv[1]);
            }
            break;
        default:
            break;
        }
    }

    return 0;
}
