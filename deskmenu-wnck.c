#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "deskmenu-wnck.h"

void 
refresh_viewportlist_item (GtkWidget *item, gpointer data) {
	DeskmenuVplist *vplist = g_object_get_data(G_OBJECT(item), "vplist");
	deskmenu_vplist_new(vplist);
	g_object_set_data(G_OBJECT(item), "vplist", vplist);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), vplist->menu);
}

void
refresh_windowlist_item (GtkWidget *item, gpointer data) {
	DeskmenuWindowlist *windowlist = g_object_get_data(G_OBJECT(item), "windowlist");
	deskmenu_windowlist_new(windowlist);
	g_object_set_data(G_OBJECT(item), "windowlist", windowlist);
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), windowlist->menu);
}

/* borrowed from libwnck selector.c */
static GdkPixbuf *
wnck_selector_dimm_icon (GdkPixbuf *pixbuf)
{
  int x, y, pixel_stride, row_stride;
  guchar *row, *pixels;
  int w, h;
  GdkPixbuf *dimmed;

  w = gdk_pixbuf_get_width (pixbuf);
  h = gdk_pixbuf_get_height (pixbuf);

  if (gdk_pixbuf_get_has_alpha (pixbuf)) 
    dimmed = gdk_pixbuf_copy (pixbuf);
  else
    dimmed = gdk_pixbuf_add_alpha (pixbuf, FALSE, 0, 0, 0);

  pixel_stride = 4;

  row = gdk_pixbuf_get_pixels (dimmed);
  row_stride = gdk_pixbuf_get_rowstride (dimmed);

  for (y = 0; y < h; y++)
    {
      pixels = row;
      for (x = 0; x < w; x++)
        {
          pixels[3] /= 2;
          pixels += pixel_stride;
        }
      row += row_stride;
    }

  return dimmed;
}
#define SELECTOR_MAX_WIDTH 50   /* maximum width in characters */

static gint
wnck_selector_get_width (GtkWidget  *widget,
                         const char *text)
{
  PangoContext *context;
  PangoFontMetrics *metrics;
  gint char_width;
  PangoLayout *layout;
  PangoRectangle natural;
  gint max_width;
  gint screen_width;
  gint width;

  gtk_widget_ensure_style (widget);

  context = gtk_widget_get_pango_context (widget);
  metrics = pango_context_get_metrics (context, widget->style->font_desc,
                                       pango_context_get_language (context));
  char_width = pango_font_metrics_get_approximate_char_width (metrics);
  pango_font_metrics_unref (metrics);
  max_width = PANGO_PIXELS (SELECTOR_MAX_WIDTH * char_width);

  layout = gtk_widget_create_pango_layout (widget, text);
  pango_layout_get_pixel_extents (layout, NULL, &natural);
  g_object_unref (G_OBJECT (layout));

  screen_width = gdk_screen_get_width (gtk_widget_get_screen (widget));

  width = MIN (natural.width, max_width);
  width = MIN (width, 3 * (screen_width / 4));

  return width;
}

/* end borrowing */

static void
dmwin_set_weight (GtkWidget *label, PangoWeight weight)
{
    PangoFontDescription *font_desc;
    font_desc = pango_font_description_new ();
    pango_font_description_set_weight (font_desc, weight);
    gtk_widget_modify_font (label, font_desc);
    pango_font_description_free (font_desc);
}

static void
dmwin_set_decoration (WnckWindow *window, DeskmenuWindowlist *windowlist, GtkWidget *label, gchar *ante, gchar *post)
{
    GString *name;
    gchar *namecpy, *mnemonic, *decorated_name, *unescaped;
    guint i, n;
    name = g_string_new (wnck_window_get_name (window));
    
    namecpy = g_strdup (name->str);

    n = 0;
    for (i = 0; i < name->len; i++)
    {
        if (namecpy[i] == '_')
        {
            g_string_insert_c (name, i+n, '_');
            n++;
        }
    }
    g_free (namecpy);

    if (name->len)
        mnemonic = "_";
    else
        mnemonic = "";
        //wnck_window_get_workspace (dmwin->window)
        //TODO: get this to calculate right

	decorated_name = g_strconcat (ante, mnemonic, name->str, post, NULL);

    unescaped = g_strconcat (ante, wnck_window_get_name (window),
        post, NULL);
    gtk_label_set_text_with_mnemonic (GTK_LABEL (label), decorated_name);

    gtk_widget_set_size_request (label,
        wnck_selector_get_width (windowlist->menu, unescaped), -1);

    g_string_free (name, TRUE);
    //g_free (vpid);
    g_free (decorated_name);
    g_free (unescaped);
}

static void
activate_window (GtkWidget  *widget,
                 WnckWindow *window)
{
    guint32 timestamp;

    timestamp = gtk_get_current_event_time ();

    wnck_window_activate (window, timestamp);
}

static void
window_name_changed (WnckWindow *window, GtkWidget *label, DeskmenuWindowlist *windowlist)
{
    if (wnck_window_is_minimized (window))
    {
        if (wnck_window_is_shaded (window))
            dmwin_set_decoration (window, windowlist, label, "=", "=");
        else
            dmwin_set_decoration (window, windowlist, label, "[", "]");
    }
    else
        dmwin_set_decoration (window, windowlist, label, "", "");

}

static void
window_icon_changed (WnckWindow *window, 
                     GtkWidget *image)
{
    GdkPixbuf *pixbuf;
    gboolean free_pixbuf;
	pixbuf = wnck_window_get_mini_icon (window); 
	free_pixbuf = FALSE;    
	if (wnck_window_is_minimized (window))
	{
		pixbuf = wnck_selector_dimm_icon (pixbuf);
		free_pixbuf = TRUE;
	}

	gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
	
	if (free_pixbuf)
		g_object_unref (pixbuf);
}

static void
deskmenu_windowlist_window_new (WnckWindow *window,
                                DeskmenuWindowlist *windowlist)
{
    GtkWidget *item = gtk_image_menu_item_new ();
    GtkWidget *label = gtk_label_new (NULL);
	
	gtk_container_add (GTK_CONTAINER (item), label);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
	
    g_signal_connect (G_OBJECT (item), "activate", 
        G_CALLBACK (activate_window), window);

    window_name_changed (window, label, windowlist);

    if (wnck_window_or_transient_needs_attention (window))
        dmwin_set_weight (label, PANGO_WEIGHT_BOLD);
    else
        dmwin_set_weight (label, PANGO_WEIGHT_NORMAL);

	if (windowlist->images) {
		GtkWidget *image = gtk_image_new ();
		window_icon_changed (window, image);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
	}
//wnck_window_is_in_viewport (iterator->data, wnck_screen_get_workspace(windowlist->screen,0))
    gtk_widget_show_all (item);

    gtk_menu_shell_append (GTK_MENU_SHELL (windowlist->menu), item);
}

void
deskmenu_windowlist_new (DeskmenuWindowlist *windowlist)
{	
	if (!wnck_screen_get_default ())
	{
		while (gtk_events_pending ())
			gtk_main_iteration (); //wait until we get a screen
	}

    windowlist->screen = wnck_screen_get_default ();
	if (windowlist->menu)
	{
		gtk_widget_destroy(windowlist->menu);
	}
    windowlist->menu = gtk_menu_new ();

	GList* list = NULL, *iterator = NULL;

	if (!wnck_screen_get_windows (windowlist->screen))
	{
		while (gtk_events_pending ())
			gtk_main_iteration (); //wait until we get a screen
	}

	list = wnck_screen_get_windows (windowlist->screen);

	if (list)
	{
		for (iterator = list; iterator; iterator = iterator->next) {
			if (!wnck_window_is_skip_tasklist (iterator->data)) //don't bother making the item if it isn't to be on a tasklist
			{
				if (!wnck_window_is_minimized (iterator->data) && windowlist->iconified_only)
				{
					continue;
				}
				if (!wnck_window_is_in_viewport (iterator->data, 
						wnck_screen_get_workspace (windowlist->screen,0)) &&
					windowlist->this_viewport)
				{
					continue;
				}
				deskmenu_windowlist_window_new(iterator->data, windowlist);
			}
		}
	}

	if (gtk_container_get_children (GTK_CONTAINER(windowlist->menu)) == NULL)
	{
		GtkWidget *empty_item = gtk_menu_item_new_with_label ("None");
		gtk_widget_set_sensitive (empty_item, FALSE);
		gtk_menu_shell_append (GTK_MENU_SHELL (windowlist->menu),
			empty_item);
	}

	gtk_widget_show_all (windowlist->menu);
}

DeskmenuWindowlist*
deskmenu_windowlist_initialize (gboolean images, gboolean this_viewport, gboolean iconified_only) {
	DeskmenuWindowlist *windowlist;
    windowlist = g_slice_new0 (DeskmenuWindowlist);

    windowlist->images = images;
    windowlist->this_viewport = this_viewport;
    windowlist->iconified_only = iconified_only;

    return windowlist;
}

static void
deskmenu_vplist_goto (GtkWidget      *widget,
                      DeskmenuVplist *vplist)
{
    guint viewport;
    viewport = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget), 
        "viewport"));
    guint column, row, i;

    column = viewport % vplist->hsize;
    if (!column)
        column = vplist->hsize - 1; /* needs to be zero-indexed */
    else
        column--;

    row = 0;
    if (viewport > vplist->hsize)
    {
        i = 0;
        while (i < (viewport - vplist->hsize))
        {
            i += vplist->hsize;
            row++;
        }
    }

    wnck_screen_move_viewport (vplist->screen,
        column * vplist->screen_width,
        row * vplist->screen_height);
}

static void
deskmenu_vplist_go_direction (GtkWidget      *widget,
                              DeskmenuVplist *vplist)
{
    WnckMotionDirection direction;
    direction = (WnckMotionDirection) GPOINTER_TO_UINT (g_object_get_data 
        (G_OBJECT (widget), "direction"));

    guint x = vplist->x, y = vplist->y;

    switch (direction)
    {
        case WNCK_MOTION_LEFT:
            if (vplist->screen_width > x)
                x = vplist->xmax;
            else
                x -= vplist->screen_width;
            break;
        case WNCK_MOTION_RIGHT:
            if (x + vplist->screen_width > vplist->xmax)
                x = 0;
            else
                x += vplist->screen_width;
            break;
        case WNCK_MOTION_UP:
            if (vplist->screen_height > y)
                y = vplist->ymax;
            else
                y -= vplist->screen_height;
            break;
        case WNCK_MOTION_DOWN:
            if (y + vplist->screen_height > vplist->ymax)
                y = 0;
            else
                y += vplist->screen_height;
            break;
        default:
            g_assert_not_reached ();
            break;
    }
    wnck_screen_move_viewport (vplist->screen, x, y);
}

static gboolean
deskmenu_vplist_can_move (DeskmenuVplist      *vplist,
                          WnckMotionDirection  direction)
{
    if (!vplist->wrap)
    {
        switch (direction)
        {
            case WNCK_MOTION_LEFT:
                return vplist->x;
            case WNCK_MOTION_RIGHT:
                return (vplist->x != vplist->xmax);
            case WNCK_MOTION_UP:
                return vplist->y;
            case WNCK_MOTION_DOWN:
                return (vplist->y != vplist->ymax);
            default:
                break;
        }
    }
    else
    {
        switch (direction)
        {
            case WNCK_MOTION_LEFT:
            case WNCK_MOTION_RIGHT:
                return (vplist->hsize > 1);
            case WNCK_MOTION_UP:
            case WNCK_MOTION_DOWN:
                return (vplist->vsize > 1);
            default:
                break;
        }
    }
    g_assert_not_reached ();
}

static GtkWidget*
deskmenu_vplist_make_go_item (DeskmenuVplist      *vplist,
                              WnckMotionDirection  direction,
                              gchar               *name,
                              gchar               *stock_id)
{
    GtkWidget *item;
    item = gtk_image_menu_item_new_with_mnemonic (name);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
		gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU));
    g_object_set_data (G_OBJECT (item), "direction",
        GINT_TO_POINTER (direction));
    g_signal_connect (G_OBJECT (item), "activate", 
        G_CALLBACK (deskmenu_vplist_go_direction), vplist);
    gtk_menu_shell_append (GTK_MENU_SHELL (vplist->menu), item);
    
    return item;
}

guint
deskmenu_vplist_get_vpid (DeskmenuVplist *vplist)
{
    guint column, row;
    column = vplist->x / vplist->screen_width;
    row = vplist->y / vplist->screen_height;
    
    return (row * vplist->hsize + column + 1);
}

static void
deskmenu_vplist_update (WnckScreen *screen, DeskmenuVplist *vplist)
{
	if (!wnck_screen_get_workspace
        (vplist->screen, 0))
	{
		while (gtk_events_pending ())
			gtk_main_iteration (); //wait until we get a workspace
	}
	
    /* get dimensions */
    vplist->workspace = wnck_screen_get_workspace
        (vplist->screen, 0);

    vplist->screen_width = wnck_screen_get_width (screen);
    vplist->workspace_width = wnck_workspace_get_width
        (vplist->workspace);
    vplist->screen_height = wnck_screen_get_height (screen);
    vplist->workspace_height = wnck_workspace_get_height
        (vplist->workspace);

    vplist->x = wnck_workspace_get_viewport_x (vplist->workspace);
    vplist->y = wnck_workspace_get_viewport_y (vplist->workspace);
    vplist->xmax = vplist->workspace_width - vplist->screen_width;
    vplist->ymax = vplist->workspace_height - vplist->screen_height;
    vplist->hsize = vplist->workspace_width / vplist->screen_width;

    vplist->vsize = vplist->workspace_height / vplist->screen_height;
}

static void
deskmenu_vplist_make_goto_viewport (DeskmenuVplist *vplist)
{
	GtkWidget *item;
    guint i, new_count, current;
    gint w, h;
    current = deskmenu_vplist_get_vpid (vplist);
    new_count = vplist->hsize * vplist->vsize;
    gchar *text;

    gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);

    for (i = 0; i < new_count; i++)
    {
        text = g_strdup_printf ("Viewport _%i", i + 1);
        item = gtk_image_menu_item_new_with_mnemonic (text);

        if (vplist->images) 
        {
        	if (vplist->icon){
	        	if (vplist->file) {
	            	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
				  	 (item), gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_file_at_size (vplist->icon, w, h, NULL)));
	        	}
	        	else {
	            	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
						gtk_image_new_from_icon_name (vplist->icon, GTK_ICON_SIZE_MENU));
				}
			}
	        else {
        	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
					gtk_image_new_from_icon_name ("user-desktop", GTK_ICON_SIZE_MENU));
			}
		}
		if ((i + 1) == (current))
		{
			gtk_widget_set_sensitive (item, FALSE);
		}
        g_object_set_data (G_OBJECT (item), "viewport",
            GUINT_TO_POINTER (i + 1));
        g_signal_connect (G_OBJECT (item), "activate",
            G_CALLBACK (deskmenu_vplist_goto), vplist);
        gtk_menu_shell_append (GTK_MENU_SHELL (vplist->menu), item);
        g_free (text);
    }   
}

void
deskmenu_vplist_new (DeskmenuVplist *vplist)
{	
    vplist->screen = wnck_screen_get_default ();

	if (vplist->menu)
	{
		gtk_widget_destroy(vplist->menu);
	}

    vplist->menu = gtk_menu_new ();

	deskmenu_vplist_update(vplist->screen, vplist);
	
	if (deskmenu_vplist_can_move (vplist, WNCK_MOTION_LEFT))
	{
		if (vplist->images)
		{
			vplist->go_left = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_LEFT,
			"Viewport _Left", GTK_STOCK_GO_BACK);
		}
		else
		{
			vplist->go_left = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_LEFT,
			"Viewport _Left", "");
		}
	}
	if (deskmenu_vplist_can_move (vplist, WNCK_MOTION_RIGHT))
	{
		if(vplist->images) //this rips off those arrows if you don't want images AT ALL
		{
			vplist->go_right = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_RIGHT,
				"Viewport _Right", GTK_STOCK_GO_FORWARD);
		}
		else
		{
			vplist->go_right = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_RIGHT,
				"Viewport _Right", "");
		}
		
	}
	if (deskmenu_vplist_can_move (vplist, WNCK_MOTION_UP))
	{
		if(vplist->images) //this rips off those arrows if you don't want images AT ALL
		{
			vplist->go_up = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_UP,
				"Viewport _Up", GTK_STOCK_GO_UP);
		}
		else
		{
			vplist->go_up = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_UP,
				"Viewport _Up", "");
		}
		
	}
	if (deskmenu_vplist_can_move (vplist, WNCK_MOTION_DOWN))
	{
		if(vplist->images) //this rips off those arrows if you don't want images AT ALL
		{
			vplist->go_down = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_DOWN,
				"Viewport _Down", GTK_STOCK_GO_DOWN);
		}
		else
		{
			vplist->go_down = deskmenu_vplist_make_go_item (vplist, WNCK_MOTION_DOWN,
				"Viewport _Down", "");
		}
	}

	if (vplist->go_right || vplist->go_left || vplist->go_up || vplist->go_down)
	{
		gtk_menu_shell_append (GTK_MENU_SHELL (vplist->menu), 
			gtk_separator_menu_item_new ());
	}

	deskmenu_vplist_make_goto_viewport(vplist);

	gtk_widget_show_all (vplist->menu);
}

DeskmenuVplist*
deskmenu_vplist_initialize(gboolean toggle_wrap, gboolean toggle_images, gboolean toggle_file, gchar *viewport_icon) {
	DeskmenuVplist *vplist;
    vplist = g_slice_new0 (DeskmenuVplist);

	vplist->icon = viewport_icon;
	vplist->wrap = toggle_wrap;
	vplist->images = toggle_images;
	vplist->file = toggle_file;

	return vplist;
}
