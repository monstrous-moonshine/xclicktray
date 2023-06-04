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
        fprintf(stderr, "Could not find system tray\n");
        return 1;
    }
#ifdef DEBUG
    printf("System tray window: 0x%lX\n", tray);
#endif

    int white_color = WhitePixel(dpy, DefaultScreen(dpy));
    Window w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                                   100, 100, 0, white_color, white_color);
    XSelectInput(dpy, w, StructureNotifyMask | ButtonPressMask | ExposureMask);
#ifdef DEBUG
    printf("Created window: 0x%lX\n", w);
#endif

    // Embed into systray
    Atom _XEMBED_INFO = XInternAtom(dpy, "_XEMBED_INFO", False);
    const unsigned long xembed_info[2] = { XEMBED_VERSION, XEMBED_MAPPED };
    XChangeProperty(dpy, w, _XEMBED_INFO, _XEMBED_INFO, 32, PropModeReplace, (const unsigned char *)xembed_info, 2);
    send_message(dpy, tray, SYSTEM_TRAY_REQUEST_DOCK, w, 0, 0);

    GC gc = XCreateGC(dpy, w, 0, NULL);
    XSetForeground(dpy, gc, white_color);

    int is_open = 1;
    while (is_open) {
        XEvent ev;
        XNextEvent(dpy, &ev);
#ifdef DEBUG
        printf("Event received: %s\n", event_names[ev.type]);
#endif

        switch (ev.type) {
#ifdef DEBUG
        case ClientMessage:
            printf("Message type: %s\n", XGetAtomName(dpy, ev.xclient.message_type));
            break;
        case ConfigureNotify:
            printf("%dx%d+%d+%d\n", ev.xconfigure.width, ev.xconfigure.height,
                   ev.xconfigure.x, ev.xconfigure.y);
            break;
        case MapNotify:
            break;
        case ReparentNotify:
            printf("New parent: 0x%lX\n", ev.xreparent.parent);
            break;
#endif
        case Expose:
            XFillRectangle(dpy, w, gc, ev.xexpose.x, ev.xexpose.y,
                           ev.xexpose.width, ev.xexpose.height);
            XSync(dpy, False);
            break;
        case ButtonPress:
            if (ev.xbutton.button == Button1 || ev.xbutton.button == Button3) {
                if (ev.xbutton.state & ShiftMask)
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
