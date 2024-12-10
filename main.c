#include <stdio.h>

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>

#include <X11/Xft/Xft.h>

// Config
#define HEADER_FONT "Roboto:size=15"
#define FOOTER_FONT "Roboto:size=11"

#define HEADER_TEXT "Activate Linux"
#define FOOTER_TEXT "Go to Settings to activate Linux"

#define FOREGROUND_COLOR 0xFF928374

#define XPAD 25
#define YPAD 49

// Helpers
#define render_color(c)                                                                            \
    ((XRenderColor){                                                                               \
        .red = (((c) >> (2 * 8)) & 0xFF) << 8,                                                     \
        .green = (((c) >> (1 * 8)) & 0xFF) << 8,                                                   \
        .blue = (((c) >> (0 * 8)) & 0xFF) << 8,                                                    \
        .alpha = (((c) >> (3 * 8)) & 0xFF) << 8,                                                   \
    })

#define return_defer(value)                                                                        \
    do {                                                                                           \
        result = (value);                                                                          \
        goto defer;                                                                                \
    } while (0)

int main() {
    int result = 0;

    // For the defer macro to work :sigh:
    XftDraw *draw = NULL;
    Display *display = NULL;
    XftFont *header_font = NULL;
    XftFont *footer_font = NULL;

    display = XOpenDisplay(NULL);
    if (!display) {
        fprintf(stderr, "error: could not open display\n");
        return_defer(1);
    }

    // Fonts
    header_font = XftFontOpenName(display, 0, HEADER_FONT);
    footer_font = XftFontOpenName(display, 0, FOOTER_FONT);
    if (!header_font || !footer_font) {
        fprintf(stderr, "error: could not open font\n");
        return_defer(1);
    }

    XGlyphInfo extents = {0};
    XftTextExtentsUtf8(
        display, header_font, (XftChar8 *)HEADER_TEXT, sizeof(HEADER_TEXT) - 1, &extents);
    const size_t header_width = extents.xOff;

    XftTextExtentsUtf8(
        display, footer_font, (XftChar8 *)FOOTER_TEXT, sizeof(FOOTER_TEXT) - 1, &extents);
    const size_t footer_width = extents.xOff;

    const size_t overlay_width = header_width > footer_width ? header_width : footer_width;
    const size_t overlay_height =
        header_font->ascent + header_font->descent + footer_font->ascent + footer_font->descent;

    // Window
    const Window root = DefaultRootWindow(display);

    const int screen = DefaultScreen(display);
    const size_t width = DisplayWidth(display, screen);
    const size_t height = DisplayHeight(display, screen);

    XVisualInfo vi;
    if (!XMatchVisualInfo(display, screen, 32, TrueColor, &vi)) {
        fprintf(stderr, "error: could not find ARGB visual\n");
        return_defer(1);
    }

    XSetWindowAttributes wa = {0};
    wa.colormap = XCreateColormap(display, root, vi.visual, AllocNone);
    wa.override_redirect = True;

    const Window overlay = XCreateWindow(
        display,
        root,
        width - overlay_width - XPAD,
        height - overlay_height - YPAD,
        overlay_width,
        overlay_height,
        0,
        vi.depth,
        InputOutput,
        vi.visual,
        CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect,
        &wa);

    // Make the window click through
    const XserverRegion region = XFixesCreateRegion(display, NULL, 0);
    XFixesSetWindowShapeRegion(display, overlay, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(display, region);

    // Graphics
    draw = XftDrawCreate(display, overlay, vi.visual, wa.colormap);
    if (!draw) {
        fprintf(stderr, "error: could not create xft draw context\n");
        return_defer(1);
    }

    XftColor foreground;
    const XRenderColor color = render_color(FOREGROUND_COLOR);
    XftColorAllocValue(display, vi.visual, wa.colormap, &color, &foreground);

    // Show the window
    XClassHint class_hint = {
        .res_name = "overlay",
        .res_class = "Overlay",
    };
    XSetClassHint(display, overlay, &class_hint);

    XSelectInput(display, root, SubstructureNotifyMask);
    XSelectInput(display, overlay, ExposureMask | VisibilityChangeMask);
    XMapWindow(display, overlay);

    while (1) {
        XEvent e;
        XNextEvent(display, &e);

        if (e.type == VisibilityNotify) {
            XVisibilityEvent *ve = (XVisibilityEvent *)&e;
            if (ve->state != VisibilityUnobscured) {
                XRaiseWindow(display, overlay);
            }
        }

        if (e.type == ConfigureNotify) {
            XConfigureEvent *ce = (XConfigureEvent *)&e;
            if (ce->window != overlay) {
                XRaiseWindow(display, overlay);
            }
        }

        if (e.type == Expose) {
            XftDrawStringUtf8(
                draw,
                &foreground,
                header_font,
                0,
                header_font->ascent,
                (XftChar8 *)HEADER_TEXT,
                sizeof(HEADER_TEXT) - 1);

            XftDrawStringUtf8(
                draw,
                &foreground,
                footer_font,
                0,
                header_font->ascent + header_font->descent + footer_font->ascent,
                (XftChar8 *)FOOTER_TEXT,
                sizeof(FOOTER_TEXT) - 1);
        }
    }

defer:
    if (header_font) XftFontClose(display, header_font);
    if (footer_font) XftFontClose(display, footer_font);

    if (draw) {
        XftColorFree(display, vi.visual, wa.colormap, &foreground);
        XftDrawDestroy(draw);
    }

    if (display) {
        XFreeColormap(display, wa.colormap);
        XDestroyWindow(display, overlay);
        XCloseDisplay(display);
    }
    return result;
}
