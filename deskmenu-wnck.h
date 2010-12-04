
#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>

typedef struct DeskmenuWindowlist
{
    WnckScreen *screen;
    GtkWidget *menu;
    gboolean images; //toggles use of icons
} DeskmenuWindowlist;

typedef struct DeskmenuWindow
{
    WnckWindow *window;
    DeskmenuWindowlist *windowlist;
    GtkWidget *item;
    GtkWidget *label;
    GtkWidget *image;
} DeskmenuWindow;

typedef struct DeskmenuVplist
{
    WnckScreen *screen;
    WnckWorkspace *workspace;
    GtkWidget *menu;
    GtkWidget *go_left;
    GtkWidget *go_right;
    GtkWidget *go_up;
    GtkWidget *go_down;
    gboolean wrap;
    gboolean images; //toggles use of icons
    gboolean file; // whether the icon of choice is from theme or not
    /* store some calculations */
    guint hsize; /* 1-indexed horizontal viewport count */
    guint vsize; 
    guint x; /* current viewport x position (in pixels) */
    guint y;
    guint xmax; /* leftmost coordinate of rightmost viewport */
    guint ymax;
    guint screen_width; /* store screen_get_width (screen) */
    guint screen_height;
    guint workspace_width; /* store workspace_get_width (workspace) */
    guint workspace_height;
    gchar *icon; /* stores viewport icon of choice */
} DeskmenuVplist;

DeskmenuWindowlist* deskmenu_windowlist_new (gboolean images);

DeskmenuVplist* deskmenu_vplist_new (gboolean toggle_wrap, gboolean toggle_images, gboolean toggle_file, gchar *viewport_icon);

