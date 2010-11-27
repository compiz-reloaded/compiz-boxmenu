
#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>

typedef struct DeskmenuWindowlist
{
    WnckScreen *screen;
    GtkWidget *menu;
    GtkWidget *empty_item;
    GList *windows;
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
    GPtrArray *goto_items;
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
    guint old_count; /* store old hsize * vsize */
    guint old_vpid; /* store old viewport number */
    gchar *icon; /* stores viewport icon of choice */
} DeskmenuVplist;

DeskmenuWindowlist* deskmenu_windowlist_new (void);

DeskmenuVplist* deskmenu_vplist_new (void);

