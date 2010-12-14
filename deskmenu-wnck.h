
#define WNCK_I_KNOW_THIS_IS_UNSTABLE 1
#include <libwnck/libwnck.h>

typedef struct DeskmenuWindowlist
{
    WnckScreen *screen;
    GtkWidget *menu;
    gboolean images; //toggles use of icons
    gboolean this_viewport;
    gboolean iconified_only;
} DeskmenuWindowlist;

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

void refresh_viewportlist_item (GtkWidget *item, gpointer data);
void refresh_windowlist_item (GtkWidget *item, gpointer data);
void deskmenu_windowlist_new (DeskmenuWindowlist *windowlist);
void deskmenu_vplist_new (DeskmenuVplist *vplist);
DeskmenuVplist* deskmenu_vplist_initialize (gboolean toggle_wrap, gboolean toggle_images, gboolean toggle_file, gchar *viewport_icon);
DeskmenuWindowlist* deskmenu_windowlist_initialize (gboolean images, gboolean this_viewport, gboolean iconified_only);
