/*
 * compiz-deskmenu is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * compiz-deskmenu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2008 Christopher Williams <christopherw@verizon.net>
 */

 /*
Roadmap:
Necessary:
	TODO: Add a viewport # indicator for the window list for accesiblity reasons difficulty: hard
	TODO: Add configuration for menu icon size difficulty: easy
	TODO: Add toggle of tearables difficulty: easy
	TODO: Add a sane icon dialog difficulty: medium-hard
For fun, might not implement:
TODO: Add ability to call up menus from the menu.xml file by name, if this is really, really needed or requested
 */

#include <stdlib.h>
#include <string.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>

#define HAVE_WNCK 1

#if HAVE_WNCK
#include "deskmenu-wnck.h"
#endif

#include "deskmenu-utils.h"

#include "deskmenu-menu.h"
#include "deskmenu-glue.h"

G_DEFINE_TYPE(Deskmenu, deskmenu, G_TYPE_OBJECT) //this is calling deskmenu_class_init

static void
start_element (GMarkupParseContext *context,
               const gchar         *element_name,
               const gchar        **attr_names,
               const gchar        **attr_values,
               gpointer             user_data,
               GError             **error);

static void
text (GMarkupParseContext *context,
      const gchar         *text,
      gsize                text_len,
      gpointer             user_data,
      GError             **error);

static void
end_element (GMarkupParseContext *context,
             const gchar         *element_name,
             gpointer             user_data,
             GError             **error);

static GMarkupParser parser = {
    start_element,
    end_element,
    text,
    NULL,
    NULL
};

static GHashTable *item_hash;
static GHashTable *element_hash;

static void
deskmenu_free_item (DeskmenuObject *dm_object);

GQuark
deskmenu_error_quark (void)
{
    static GQuark quark = 0;
    if (!quark)
        quark = g_quark_from_static_string ("deskmenu_error");
    return quark;
}

static void
quit (GtkWidget *widget,
      gpointer   data)
{
    gtk_main_quit ();
}

//This is how menu command is launched
static void
launcher_activated (GtkWidget *widget,
                    gchar     *command)
{
    GError *error = NULL;

	if (!gdk_spawn_command_line_on_screen (gdk_screen_get_default (), parse_expand_tilde(command), &error))
    {
        deskmenu_widget_error(error);
    }
}

//This is how a recent document is opened
static void
recent_activated (GtkRecentChooser *chooser,
                  gchar     *command)
{
    GError *error = NULL;
	gchar *full_command, *file;
	file = gtk_recent_chooser_get_current_uri (chooser);
	full_command = get_full_command(command, file);

	if (!gdk_spawn_command_line_on_screen (gdk_screen_get_default (), parse_expand_tilde(full_command), &error))
    {
        deskmenu_widget_error(error);
    }
}

/* prototype code for pipemenu */

static void
pipe_menu_recreate (GtkWidget *item, gchar *command)
{
	gchar *stdout_for_cmd;
	gchar *cache_entries = g_object_get_data (G_OBJECT(item), "cached");
	GtkWidget *submenu;
	submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM(item));

	if (submenu)
	{
		if (strcmp (cache_entries, "yes") != 0)
		{
			gtk_widget_destroy (submenu);
			submenu = NULL;
		}
	}
	if (!submenu)
	{
		submenu = gtk_menu_new();
		if (g_spawn_command_line_sync (parse_expand_tilde(command), &stdout_for_cmd, NULL, NULL, NULL))
		{
			GError *error = NULL;
			DeskmenuObject *dm_object = g_object_get_data (G_OBJECT(item), "menu");
			dm_object->current_menu = submenu;
			dm_object->make_from_pipe = TRUE;
			if (gtk_menu_get_tearoff_state (GTK_MENU(dm_object->menu)))
			{
				gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu),
					gtk_tearoff_menu_item_new());
			}
			GMarkupParseContext *context = g_markup_parse_context_new (&parser,
				0, dm_object, NULL);
			g_markup_parse_context_parse (context, (const gchar *)stdout_for_cmd, strlen((const char *)stdout_for_cmd), &error);
			g_markup_parse_context_free (context);
			if (error)
			{
				g_print("%s", error->message); //spit out the message manually
				if (dm_object->current_item)
				{
					//force reset
					deskmenu_free_item(dm_object);
				}
				g_error_free (error);
			}
			g_free(stdout_for_cmd);
			dm_object->make_from_pipe = FALSE;
		}
		else
		{
			GtkWidget *empty_item = gtk_menu_item_new_with_label ("Unable to get output");
			gtk_widget_set_sensitive (empty_item, FALSE);
			gtk_menu_shell_append (GTK_MENU_SHELL (submenu), empty_item);
		}
		gtk_widget_show_all(submenu);
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
	}
}

static void
launcher_name_exec_update (GtkWidget *label)
{
    gchar *exec, *stdout_for_cmd;
    exec = g_object_get_data (G_OBJECT (label), "exec");
    if (g_spawn_command_line_sync (exec, &stdout_for_cmd, NULL, NULL, NULL))
        gtk_label_set_text (GTK_LABEL (label), g_strstrip((gchar *)stdout_for_cmd));
    else
        gtk_label_set_text (GTK_LABEL (label), "execution error");
    g_free (stdout_for_cmd);
}

static
GtkWidget *make_recent_documents_list (gboolean images, gchar *command, int limit, int age, gchar *sort_type)
{
	GtkWidget *widget = gtk_recent_chooser_menu_new ();

    if (images)
    {
		gtk_recent_chooser_set_show_icons (GTK_RECENT_CHOOSER(widget), TRUE);
    }
	else
	{
		gtk_recent_chooser_set_show_icons (GTK_RECENT_CHOOSER(widget), FALSE);
	}

	g_signal_connect (G_OBJECT (widget), "item-activated", G_CALLBACK (recent_activated), g_strdup (command));

    if (age)
    {
		GtkRecentFilter *filter = gtk_recent_filter_new ();
		gtk_recent_filter_add_pattern (filter, "*");
        gtk_recent_filter_add_age (filter, age);
        gtk_recent_chooser_add_filter (GTK_RECENT_CHOOSER(widget), filter);
    }

	if (sort_type) {
			if (strcmp (sort_type, "most used") == 0)
			{
				gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER(widget), GTK_RECENT_SORT_MRU);
			}
			else
			{
				gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER(widget), GTK_RECENT_SORT_LRU);
			}
	}
	gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER(widget), limit);

	return widget;
}

static void
deskmenu_construct_item (DeskmenuObject *dm_object)
{
    DeskmenuItem *item = dm_object->current_item;
    GtkWidget *menu_item, *submenu;
    gchar *name, *icon, *command, *vpicon;
    gboolean images;
    gint w, h;
//constructs the items in menu
    switch (item->type)
    {
        case DESKMENU_ITEM_LAUNCHER:
            if (item->name_exec)
            {
                GtkWidget *label;
                GHook *hook;

                name = g_strstrip (item->name->str);

                menu_item = gtk_image_menu_item_new ();
                label = gtk_label_new_with_mnemonic (NULL);
                gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

                g_object_set_data (G_OBJECT (label), "exec", g_strdup (name));
                gtk_container_add (GTK_CONTAINER (menu_item), label);
                hook = g_hook_alloc (dm_object->show_hooks);

                hook->data = (gpointer) label;
                hook->func = (GHookFunc *) launcher_name_exec_update;
                g_hook_append (dm_object->show_hooks, hook);
            }
            else
            {
                if (item->name)
                    name = g_strstrip (item->name->str);
                else
                    name = "";

                menu_item = gtk_image_menu_item_new_with_mnemonic (name);

            }
            if (item->icon)
            {
                icon = g_strstrip (item->icon->str);
                if (item->icon_file) {
					gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
                	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
					   (menu_item), gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_file_at_size (parse_expand_tilde(icon), w, h, NULL)));
				   }
				else {
					gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
						gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU));
					}
            }
			if (item->command_pipe)
			{
				command = g_strstrip (item->command->str);
				if (item->cache_output)
				{
					g_object_set_data(G_OBJECT(menu_item), "cached", g_strdup("yes"));
				}
				else
				{
					g_object_set_data(G_OBJECT(menu_item), "cached", g_strdup("no"));
				}
				g_object_set_data(G_OBJECT(menu_item), "menu", dm_object);
				g_signal_connect (G_OBJECT (menu_item), "activate",
					G_CALLBACK (pipe_menu_recreate), g_strdup(command));
				submenu = gtk_menu_new();
				gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);
			}
			else
			{
				if (item->command)
				{
					command = g_strstrip (item->command->str);
					g_signal_connect (G_OBJECT (menu_item), "activate",
						G_CALLBACK (launcher_activated), g_strdup (command));
				}
			}
            gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu),
                menu_item);
            break;
#if HAVE_WNCK
        case DESKMENU_ITEM_WINDOWLIST:
            menu_item = gtk_image_menu_item_new_with_mnemonic ("_Windows");
			images = FALSE;
			gboolean this_vp = FALSE;
			gboolean mini_only = FALSE;
            if (item->icon)
            {
				images = TRUE;
                icon = g_strstrip (item->icon->str);
                if (item->icon_file) {
					gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
                	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
					   (menu_item), gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_file_at_size (parse_expand_tilde(icon), w, h, NULL)));
				}
				else {
					gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
						gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU));
				}
            }
            if (item->thisvp
                && strcmp (g_strstrip (item->thisvp->str), "true") == 0)
                this_vp = TRUE;
            if (item->mini_only
                && strcmp (g_strstrip (item->mini_only->str), "true") == 0)
                mini_only = TRUE;
            g_object_set_data(G_OBJECT(menu_item), "windowlist", deskmenu_windowlist_initialize (images, this_vp, mini_only));
            g_signal_connect (G_OBJECT (menu_item), "activate",
					G_CALLBACK (refresh_windowlist_item), NULL);
			submenu = gtk_menu_new();
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);
            gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu),
                menu_item);
            break;

        case DESKMENU_ITEM_VIEWPORTLIST:
            menu_item = gtk_image_menu_item_new_with_mnemonic ("_Viewports");
            gboolean wrap, file;
            wrap = FALSE;
            images = FALSE;
            file = FALSE;
            if (item->wrap
                && strcmp (g_strstrip (item->wrap->str), "true") == 0)
                wrap = TRUE;
            if (item->icon)
            {
				images = TRUE;
                icon = g_strstrip (item->icon->str);
                if (item->icon_file) {
					gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
                	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
					   (menu_item), gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_file_at_size (parse_expand_tilde(icon), w, h, NULL)));
				}
				else {
					gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
						gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU));
				}
            }
            if (item->vpicon)
            {
                vpicon = g_strstrip (parse_expand_tilde(item->vpicon->str));
                if (item->vpicon_file) {
					file = TRUE;
				}
            }
            else
	    {
	       vpicon = "";
	    }
            g_object_set_data(G_OBJECT(menu_item), "vplist", deskmenu_vplist_initialize (wrap, images, file, vpicon));
            g_signal_connect (G_OBJECT (menu_item), "activate",
					G_CALLBACK (refresh_viewportlist_item), NULL);
			submenu = gtk_menu_new();
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), submenu);
            gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu),
                menu_item);
            break;
#endif
        case DESKMENU_ITEM_RELOAD:
            menu_item = gtk_image_menu_item_new_with_mnemonic ("Reload");

            if (item->icon)
            {
                icon = g_strstrip (item->icon->str);
                if (item->icon_file) {
					gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
                	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
					   (menu_item), gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_file_at_size (parse_expand_tilde(icon), w, h, NULL)));
				}
				else {
					gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
						gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU));
				}
            }
            g_signal_connect (G_OBJECT (menu_item), "activate",
                G_CALLBACK (quit), NULL);
            gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu),
                menu_item);
            break;

        case DESKMENU_ITEM_DOCUMENTS:
            menu_item = gtk_image_menu_item_new_with_mnemonic ("Recent Doc_uments");
			gint limit, age;
			gchar *sort_type;
			images = FALSE;
			sort_type = "least used";
			age = 25;
            if (item->icon)
            {
				images = TRUE;
                icon = g_strstrip (item->icon->str);
                if (item->icon_file) {
					gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
                	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
					   (menu_item), gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_file_at_size (parse_expand_tilde(icon), w, h, NULL)));
				}
				else {
					gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
						gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU));
				}
            }
            if (item->age)
            {
                age = atoi(g_strstrip (item->age->str));
            }
            if (item->sort_type) {
				sort_type = g_strstrip (item->sort_type->str);
			}
            if (item->quantity)
            {
                limit = atoi(g_strstrip (item->quantity->str));
            }
			else
			{
				limit = -1;
			}
			if (item->command)
			{
				command = g_strstrip (item->command->str);
			}
			else
			{
				command = g_strdup ("xdg-open");
			}
			GtkWidget *docs = make_recent_documents_list(images, command, limit, age, sort_type);
			gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                docs);
            gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu),
                menu_item);
            break;

        default:
            break;
    }

}

static void
deskmenu_free_item (DeskmenuObject *dm_object) {
	/* free data used to make it */
    if (dm_object->current_item->name)
        g_string_free (dm_object->current_item->name, TRUE);
    if (dm_object->current_item->icon)
        g_string_free (dm_object->current_item->icon, TRUE);
    if (dm_object->current_item->command)
        g_string_free (dm_object->current_item->command, TRUE);
    if (dm_object->current_item->wrap)
        g_string_free (dm_object->current_item->wrap, TRUE);
    if (dm_object->current_item->vpicon)
        g_string_free (dm_object->current_item->vpicon, TRUE);
    if (dm_object->current_item->mini_only)
        g_string_free (dm_object->current_item->mini_only, TRUE);
    if (dm_object->current_item->thisvp)
        g_string_free (dm_object->current_item->thisvp, TRUE);
    if (dm_object->current_item->sort_type)
        g_string_free (dm_object->current_item->sort_type, TRUE);
    if (dm_object->current_item->quantity)
        g_string_free (dm_object->current_item->quantity, TRUE);
    if (dm_object->current_item->age)
        g_string_free (dm_object->current_item->age, TRUE);
    dm_object->current_item = NULL;
}

/* The handler functions. */

static void
start_element (GMarkupParseContext *context,
               const gchar         *element_name,
               const gchar        **attr_names,
               const gchar        **attr_values,
               gpointer             user_data,
               GError             **error)
{
	DeskmenuObject *dm_object = user_data;

    DeskmenuElementType element_type;
    const gchar **ncursor = attr_names, **vcursor = attr_values;
    GtkWidget *item, *menu;
    gint w, h;

    element_type = GPOINTER_TO_INT (g_hash_table_lookup
        (element_hash, element_name));

    if ((dm_object->menu && !dm_object->current_menu)
       || (!dm_object->menu && element_type != DESKMENU_ELEMENT_MENU))
    {
        gint line_num, char_num;
        g_markup_parse_context_get_position (context, &line_num, &char_num);
        g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
            "Error on line %d char %d: Element '%s' declared outside of "
            "toplevel menu element", line_num, char_num, element_name);
        return;
    }

    switch (element_type)
    {
        case DESKMENU_ELEMENT_MENU:

            if (dm_object->current_item != NULL)
            {
                gint line_num, char_num;
                g_markup_parse_context_get_position (context, &line_num,
                    &char_num);
                g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                    "Error on line %d char %d: Element 'menu' cannot be nested "
                    "inside of an item element", line_num, char_num);
                return;
            }
            if (!dm_object->menu)
            {
	            /*if (strcmp (*ncursor, "size") == 0) {
                    deskmenu->w = g_strdup (*vcursor);
                    deskmenu->h = g_strdup (*vcursor);
                    }
                else {
	                gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, deskmenu->w, deskmenu->h);
                }*/
                dm_object->menu = gtk_menu_new ();
                g_object_set_data (G_OBJECT (dm_object->menu), "parent menu",
                    NULL);
                dm_object->current_menu = dm_object->menu;
            }
            else
            {
                gchar *name = NULL;
                gchar *icon = NULL;
                gboolean name_exec = FALSE;
                gboolean icon_file = FALSE;
                while (*ncursor)
                {
                    if (strcmp (*ncursor, "name") == 0)
                        name = g_strdup (*vcursor);
                    else if (strcmp (*ncursor, "icon") == 0)
						icon = g_strdup (*vcursor);
                    else if ((strcmp (*ncursor, "mode") == 0)
                        && (strcmp (*vcursor, "exec") == 0))
                        name_exec = TRUE;
                    else if ((strcmp (*ncursor, "mode1") == 0)
                        && (strcmp (*vcursor, "file") == 0))
                        icon_file = TRUE;
					else
                        g_set_error (error, G_MARKUP_ERROR,
                            G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                            "Unknown attribute: %s", *ncursor);
                    ncursor++;
                    vcursor++;
                }
				if (name_exec)
				{
					GtkWidget *label;
					GHook *hook;

					item = gtk_image_menu_item_new ();
					label = gtk_label_new_with_mnemonic (NULL);
					gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

					g_object_set_data (G_OBJECT (label), "exec", g_strdup (name));
					gtk_container_add (GTK_CONTAINER (item), label);
					hook = g_hook_alloc (dm_object->show_hooks);

					hook->data = (gpointer) label;
					hook->func = (GHookFunc *) launcher_name_exec_update;
					g_hook_append (dm_object->show_hooks, hook);
				}
				else
				{
					if (name)
						item = gtk_image_menu_item_new_with_mnemonic (name);
					else
						item = gtk_image_menu_item_new_with_mnemonic ("");
				}
                if (icon)
				{
					if (icon_file) {
					gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
                	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM
					   (item), gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_file_at_size (parse_expand_tilde(icon), w, h, NULL)));
					}
					else {
					gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item),
						gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU));
					}
				}
				gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu), item);
                menu = gtk_menu_new ();
                g_object_set_data (G_OBJECT (menu), "parent menu",
                    dm_object->current_menu);
                dm_object->current_menu = menu;
                gtk_menu_item_set_submenu (GTK_MENU_ITEM (item),
                    dm_object->current_menu);

                if (!dm_object->make_from_pipe)
				{
					GtkWidget *pin = gtk_tearoff_menu_item_new();
					gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu),
						pin); //add a pin menu item
					dm_object->pin_items = g_slist_prepend (dm_object->pin_items, pin);
				}
				else
				{
					if (gtk_menu_get_tearoff_state (GTK_MENU(dm_object->menu)))
					{
						GtkWidget *pin = gtk_tearoff_menu_item_new();
						gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu),
							pin); //add a pin menu item
					}
				}

                g_free (name);
                g_free (icon);
            }
            break;

        case DESKMENU_ELEMENT_SEPARATOR:
        if (dm_object->current_item != NULL)
            {
                gint line_num, char_num;
                g_markup_parse_context_get_position (context, &line_num,
                    &char_num);
                g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                    "Error on line %d char %d: Element 'menu' cannot be nested "
                    "inside of an item element", line_num, char_num);
                return;
            }
        else {
                gchar *name = NULL;
                gchar *icon = NULL;
                gboolean name_exec = FALSE;
                gboolean icon_file = FALSE;
                gboolean decorate = FALSE;
                gint w, h;
                item = gtk_separator_menu_item_new();
                while (*ncursor)
                {
                    if (strcmp (*ncursor, "name") == 0) {
                        name = g_strdup (*vcursor);
						if (!decorate)
						{
							decorate = TRUE;
						}
					}
                    else if (strcmp (*ncursor, "icon") == 0) {
						icon = g_strdup (*vcursor);
						if (!decorate)
						{
							decorate = TRUE;
						}
					}
                    else if ((strcmp (*ncursor, "mode") == 0)
                        && (strcmp (*vcursor, "exec") == 0))
                        name_exec = TRUE;
                    else if ((strcmp (*ncursor, "mode1") == 0)
                        && (strcmp (*vcursor, "file") == 0))
                        icon_file = TRUE;
					else
                        g_set_error (error, G_MARKUP_ERROR,
                            G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                            "Unknown attribute: %s", *ncursor);
                    ncursor++;
                    vcursor++;
                }
				if (decorate)
				{
					GtkWidget *box = gtk_hbox_new (FALSE, 3);
					gtk_container_add (GTK_CONTAINER(item), GTK_WIDGET(box));
					if (name_exec)
					{
						GtkWidget *label;
						GHook *hook;

						label = gtk_label_new_with_mnemonic (NULL);

						g_object_set_data (G_OBJECT (label), "exec", g_strdup (name));
						gtk_box_pack_end (GTK_BOX(box), label,
															TRUE,
															FALSE,
															0);
						hook = g_hook_alloc (dm_object->show_hooks);

						hook->data = (gpointer) label;
						hook->func = (GHookFunc *) launcher_name_exec_update;
						g_hook_append (dm_object->show_hooks, hook);
					}
					else
					{
						gtk_box_pack_end (GTK_BOX(box), gtk_label_new_with_mnemonic (name),
						TRUE,
						FALSE,
						0);
					}
					if (icon)
					{
						GtkWidget *image;
						if (icon_file) {
							gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
							image = gtk_image_new_from_pixbuf (gdk_pixbuf_new_from_file_at_size (parse_expand_tilde(icon), w, h, NULL));
						}
						else {
							image = gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_MENU);
						}
						gtk_box_pack_start (GTK_BOX(box), image,
                                 FALSE,
                                 FALSE,
                                 0);
					}
					gtk_widget_set_state (item, GTK_STATE_PRELIGHT); /*derive colors from menu hover*/
					g_free (name);
					g_free (icon);
				}
				gtk_menu_shell_append (GTK_MENU_SHELL (dm_object->current_menu), item);
			}
            break;

        case DESKMENU_ELEMENT_ITEM:

            if (dm_object->current_item != NULL)
            {
                gint line_num, char_num;
                g_markup_parse_context_get_position (context, &line_num,
                    &char_num);
                g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                    "Error on line %d char %d: Element 'item' cannot be nested "
                    "inside of another item element", line_num, char_num);
                return;
            }

            dm_object->current_item = g_slice_new0 (DeskmenuItem);
                while (*ncursor)
                {
                    if (strcmp (*ncursor, "type") == 0)
                        dm_object->current_item->type = GPOINTER_TO_INT
                        (g_hash_table_lookup (item_hash, *vcursor));
                    else
                        g_set_error (error, G_MARKUP_ERROR,
                            G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                            "Unknown attribute: %s", *ncursor);
                    ncursor++;
                    vcursor++;
                }
            break;

        case DESKMENU_ELEMENT_NAME:
             while (*ncursor)
                {
                    if ((strcmp (*ncursor, "mode") == 0)
                        && (strcmp (*vcursor, "exec") == 0))
                        dm_object->current_item->name_exec = TRUE;
                    ncursor++;
                    vcursor++;
                } /* no break here to let it fall through */
        case DESKMENU_ELEMENT_ICON:
                while (*ncursor)
                {
                    if ((strcmp (*ncursor, "mode1") == 0)
                        && (strcmp (*vcursor, "file") == 0))
                        dm_object->current_item->icon_file = TRUE;
                    ncursor++;
                    vcursor++;
                } /* no break here to let it fall through */
        case DESKMENU_ELEMENT_VPICON:
                while (*ncursor)
                {
                    if ((strcmp (*ncursor, "mode1") == 0)
                        && (strcmp (*vcursor, "file") == 0))
                        dm_object->current_item->vpicon_file = TRUE;
                    ncursor++;
                    vcursor++;
                } /* no break here to let it fall through */
        case DESKMENU_ELEMENT_COMMAND:
                while (*ncursor)
                {
                    if ((strcmp (*ncursor, "mode2") == 0)
                        && (strcmp (*vcursor, "pipe") == 0))
                        dm_object->current_item->command_pipe = TRUE;
                    if (dm_object->current_item->command_pipe == TRUE
                        && (strcmp (*ncursor, "cache") == 0)
                        && (strcmp (*vcursor, "true") == 0))
                        dm_object->current_item->cache_output = TRUE;
                    ncursor++;
                    vcursor++;
                } /* no break here to let it fall through */
        case DESKMENU_ELEMENT_WRAP:
            if (dm_object->current_item)
                dm_object->current_item->current_element = element_type;
            break;
        case DESKMENU_ELEMENT_THISVP:
            if (dm_object->current_item)
                dm_object->current_item->current_element = element_type;
            break;
        case DESKMENU_ELEMENT_MINIONLY:
            if (dm_object->current_item)
                dm_object->current_item->current_element = element_type;
            break;
        case DESKMENU_ELEMENT_QUANTITY:
            if (dm_object->current_item)
                dm_object->current_item->current_element = element_type;
            break;
        case DESKMENU_ELEMENT_SORT:
            if (dm_object->current_item)
                dm_object->current_item->current_element = element_type;
            break;
        case DESKMENU_ELEMENT_AGE:
            if (dm_object->current_item)
                dm_object->current_item->current_element = element_type;
            break;

        default:
            g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
                "Unknown element: %s", element_name);
            break;
    }
}
//dealing with empty attributes
static void
text (GMarkupParseContext *context,
      const gchar         *text,
      gsize                text_len,
      gpointer             user_data,
      GError             **error)
{
    DeskmenuObject *dm_object = user_data;
    DeskmenuItem *item = dm_object->current_item;

    if (!(item && item->current_element))
        return;

    switch (item->current_element)
    {
        case DESKMENU_ELEMENT_NAME:
            if (!item->name)
                item->name = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->name, text, text_len);
            break;

        case DESKMENU_ELEMENT_ICON:
            if (!item->icon)
                item->icon = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->icon, text, text_len);
            break;

        case DESKMENU_ELEMENT_VPICON:
            if (!item->vpicon)
                item->vpicon = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->vpicon, text, text_len);
            break;

        case DESKMENU_ELEMENT_COMMAND:
            if (!item->command)
                item->command = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->command, text, text_len);
            break;

        case DESKMENU_ELEMENT_WRAP:
            if (!item->wrap)
                item->wrap = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->wrap, text, text_len);
            break;

        case DESKMENU_ELEMENT_THISVP:
            if (!item->thisvp)
                item->thisvp = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->thisvp, text, text_len);
            break;

        case DESKMENU_ELEMENT_MINIONLY:
            if (!item->mini_only)
                item->mini_only = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->mini_only, text, text_len);
            break;

        case DESKMENU_ELEMENT_AGE:
            if (!item->age)
                item->age = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->age, text, text_len);
            break;

        case DESKMENU_ELEMENT_QUANTITY:
            if (!item->quantity)
                item->quantity = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->quantity, text, text_len);
            break;

        case DESKMENU_ELEMENT_SORT:
            if (!item->sort_type)
                item->sort_type = g_string_new_len (text, text_len);
            else
                g_string_append_len (item->sort_type, text, text_len);
            break;

        default:
            break;
    }
}

static void
end_element (GMarkupParseContext *context,
             const gchar         *element_name,
             gpointer             user_data,
             GError             **error)
{

    DeskmenuElementType element_type;
    DeskmenuObject *dm_object = user_data;

    GtkWidget *parent;
    element_type = GPOINTER_TO_INT (g_hash_table_lookup
        (element_hash, element_name));

    switch (element_type)
    {
        case DESKMENU_ELEMENT_MENU:

            g_return_if_fail (dm_object->current_item == NULL);

            parent = g_object_get_data (G_OBJECT (dm_object->current_menu),
                "parent menu");

            dm_object->current_menu = parent;

            break;
/* separator building is now dealt with in the beginning */
        case DESKMENU_ELEMENT_ITEM:

            g_return_if_fail (dm_object->current_item != NULL);

            /* finally make the item ^_^ */
            deskmenu_construct_item (dm_object);

			deskmenu_free_item(dm_object);
            break;
        default:
            break;
    }
}

/* Class init */
static void
deskmenu_class_init (DeskmenuClass *deskmenu_class)
{
    dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (deskmenu_class),
        &dbus_glib_deskmenu_object_info);
}


/* Instance init, matches up words to types,
note how there's no handler for pipe since it's
replaced in its own chunk */
static void
deskmenu_init (Deskmenu *deskmenu)
{
    deskmenu->pinnable = FALSE;
    deskmenu->file_cache = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
set_up_item_hash (void) {
	item_hash= g_hash_table_new (g_str_hash, g_str_equal);

    g_hash_table_insert (item_hash, "launcher",
        GINT_TO_POINTER (DESKMENU_ITEM_LAUNCHER));
#if HAVE_WNCK
    g_hash_table_insert (item_hash, "windowlist",
        GINT_TO_POINTER (DESKMENU_ITEM_WINDOWLIST));
    g_hash_table_insert (item_hash, "viewportlist",
        GINT_TO_POINTER (DESKMENU_ITEM_VIEWPORTLIST));
#endif
    g_hash_table_insert (item_hash, "documents",
        GINT_TO_POINTER (DESKMENU_ITEM_DOCUMENTS));
    g_hash_table_insert (item_hash, "reload",
        GINT_TO_POINTER (DESKMENU_ITEM_RELOAD));
}

static void
set_up_element_hash (void) {
	element_hash = g_hash_table_new (g_str_hash, g_str_equal);

    g_hash_table_insert (element_hash, "menu",
        GINT_TO_POINTER (DESKMENU_ELEMENT_MENU));
    g_hash_table_insert (element_hash, "separator",
        GINT_TO_POINTER (DESKMENU_ELEMENT_SEPARATOR));
    g_hash_table_insert (element_hash, "item",
        GINT_TO_POINTER (DESKMENU_ELEMENT_ITEM));
    g_hash_table_insert (element_hash, "name",
        GINT_TO_POINTER (DESKMENU_ELEMENT_NAME));
    g_hash_table_insert (element_hash, "icon",
        GINT_TO_POINTER (DESKMENU_ELEMENT_ICON));
    g_hash_table_insert (element_hash, "vpicon",
        GINT_TO_POINTER (DESKMENU_ELEMENT_VPICON));
    g_hash_table_insert (element_hash, "command",
        GINT_TO_POINTER (DESKMENU_ELEMENT_COMMAND));
    g_hash_table_insert (element_hash, "thisvp",
        GINT_TO_POINTER (DESKMENU_ELEMENT_THISVP));
    g_hash_table_insert (element_hash, "minionly",
        GINT_TO_POINTER (DESKMENU_ELEMENT_MINIONLY));
    g_hash_table_insert (element_hash, "wrap",
        GINT_TO_POINTER (DESKMENU_ELEMENT_WRAP));
    g_hash_table_insert (element_hash, "sort",
        GINT_TO_POINTER (DESKMENU_ELEMENT_SORT));
    g_hash_table_insert (element_hash, "quantity",
        GINT_TO_POINTER (DESKMENU_ELEMENT_QUANTITY));
    g_hash_table_insert (element_hash, "age",
        GINT_TO_POINTER (DESKMENU_ELEMENT_AGE));
}

static DeskmenuObject
*deskmenu_object_init (void) {
	DeskmenuObject *dm_object = g_slice_new0 (DeskmenuObject);

	dm_object->menu = NULL;
    dm_object->current_menu = NULL;
    dm_object->current_item = NULL;
    dm_object->make_from_pipe = FALSE;
    dm_object->pin_items = NULL;
	dm_object->show_hooks = g_slice_new0 (GHookList);

    g_hook_list_init (dm_object->show_hooks, sizeof (GHook));

	return dm_object;
}

static DeskmenuObject
*deskmenu_parse_file (gchar *filename)
{
    GError *error = NULL;
	DeskmenuObject *dm_object = deskmenu_object_init();
    GMarkupParseContext *context = g_markup_parse_context_new (&parser,
        0, dm_object, NULL);

	gchar *text;
    gsize length;

    g_file_get_contents (filename, &text, &length, NULL); //cache already handled file existence check

	if (!g_markup_parse_context_parse (context, text, strlen(text), &error)
        || !g_markup_parse_context_end_parse (context, &error))
    {
        g_printerr ("Parse failed with message: %s \n", error->message);
        g_error_free (error);
        exit (1);
    }

	g_free(text); //free the joined array
    g_markup_parse_context_free (context); //free the parser

    gtk_widget_show_all (dm_object->menu);
    return dm_object;
}


static DeskmenuObject
*check_file_cache (Deskmenu *deskmenu, gchar *filename) {
	DeskmenuObject *dm_object;
	gchar *user_default = g_build_path (G_DIR_SEPARATOR_S,  g_get_user_config_dir (),
									"compiz",
									"boxmenu",
									"menu.xml",
									NULL);

	//TODO: add a size column to cache for possible autorefresh
		g_print("Checking cache...\n");
	if (strlen(filename) == 0)
    filename = g_build_path (G_DIR_SEPARATOR_S,
                               g_get_user_config_dir (),
                               "compiz",
                               "boxmenu",
                               "menu.xml",
                               NULL);
	if (strcmp(filename, user_default) == 0) {
		g_print("Looking up default menu...\n");
			/*
			set default filename to be [configdir]/compiz/boxmenu/menu.xml
			*/
			gboolean success = FALSE;
		if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
				g_print("Getting default system menu...\n");
				const gchar* const *cursor = g_get_system_config_dirs ();
				gchar *path = NULL;
				while (*cursor)
				{
					g_free (path);
					path = g_strdup (*cursor);
					filename = g_build_path (G_DIR_SEPARATOR_S,
											path,
											"compiz",
											"boxmenu",
											"menu.xml",
											NULL);

					if (g_file_test(filename, G_FILE_TEST_EXISTS))
					{
						if (g_hash_table_lookup(deskmenu->file_cache, filename) == NULL)
						{
							g_hash_table_insert (deskmenu->file_cache, g_strdup(filename), deskmenu_parse_file(filename));
							g_print("Prepared default system menu!\n");
						}
						else
						{
							g_print("Retrieving default system menu!\n");
						}
						g_free (path);
						success = TRUE;
						break;
					}
						cursor++;
				}
			}
		else
			{
				if (g_hash_table_lookup(deskmenu->file_cache, user_default) == NULL)
				{
					if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
						g_print("Preparing default menu!\n");
						g_hash_table_insert (deskmenu->file_cache, g_strdup(filename), deskmenu_parse_file(filename));
						success = TRUE;
					}
				}
				else
				{
					g_print("Retrieving cached default user menu...\n");
					success = TRUE;
				}
			}
		if (!success)
		{
			g_printerr ("Couldn't find a menu file...\n");
			exit (1);
		}
	}
	else {
		if (g_hash_table_lookup(deskmenu->file_cache, filename) == NULL) {
			if (g_file_test(filename, G_FILE_TEST_EXISTS))
			{
				g_print("Preparing new non-default menu...\n");
				g_hash_table_insert (deskmenu->file_cache, g_strdup(filename), deskmenu_parse_file(filename));
			}
			else 
			{
				if (g_hash_table_lookup(deskmenu->file_cache, user_default) != NULL)
				{
					g_print("Couldn't find specified file, loading default...\n");
					filename = user_default;
				}
				else
				{
					g_printerr ("Couldn't find a menu file...\n");
					exit (1);
				}
			}
		}
	}

	dm_object = g_hash_table_lookup (deskmenu->file_cache, filename);

	g_printf("Done loading %s!\n", filename);

	return dm_object;
}

#if HAVE_WNCK
gboolean
deskmenu_vplist (Deskmenu *deskmenu,
				gboolean toggle_wrap,
				gboolean toggle_images,
				gboolean toggle_file,
				gchar *viewport_icon) {
	DeskmenuVplist *vplist = deskmenu_vplist_initialize(toggle_wrap, toggle_images, toggle_file, g_strstrip(viewport_icon));
	deskmenu_vplist_new (vplist);

    gtk_menu_popup (GTK_MENU (vplist->menu),
                    NULL, NULL, NULL, NULL,
                    0, 0);
	return TRUE;
}

gboolean
deskmenu_windowlist (Deskmenu *deskmenu,
					 gboolean images,
					 gboolean thisvp,
					 gboolean mini_only) {
	DeskmenuWindowlist *windowlist = deskmenu_windowlist_initialize (images, thisvp, mini_only);
	deskmenu_windowlist_new(windowlist);

    gtk_menu_popup (GTK_MENU (windowlist->menu),
                    NULL, NULL, NULL, NULL,
                    0, 0);
	return TRUE;
}
#endif

gboolean
deskmenu_documentlist (Deskmenu *deskmenu,
					   gboolean images,
					   gchar *command,
					   int limit,
					   int age,
					   gchar *sort_type) {
	GtkWidget *menu = make_recent_documents_list (images, g_strdup(command), limit, age, g_strstrip(sort_type));

    gtk_menu_popup (GTK_MENU (menu),
                    NULL, NULL, NULL, NULL,
                    0, 0);
	return TRUE;
}

/* The show method */
static void
deskmenu_show (DeskmenuObject *dm_object,
			   Deskmenu *deskmenu,
               GError  **error)
{
	GSList *list = NULL, *iterator = NULL;
	list = dm_object->pin_items;
    g_hook_list_invoke (dm_object->show_hooks, FALSE);
	if (deskmenu->pinnable)
	{
		gtk_menu_set_tearoff_state (GTK_MENU (dm_object->menu), TRUE);
		for (iterator = list; iterator; iterator = iterator->next) {
			gtk_widget_show (iterator->data);
			gtk_widget_set_no_show_all (iterator->data, FALSE);
		}
	}
	else {
		gtk_menu_popup (GTK_MENU (dm_object->menu),
                    NULL, NULL, NULL, NULL,
                    0, 0);
        for (iterator = list; iterator; iterator = iterator->next) {
			gtk_widget_hide (iterator->data);
			gtk_widget_set_no_show_all (iterator->data, TRUE);
		}
	}
}

gboolean
deskmenu_pin (Deskmenu *deskmenu,
			gboolean pin)
{
	deskmenu->pinnable = pin;
	return TRUE;
}

gboolean
deskmenu_reload (Deskmenu *deskmenu,
               GError  **error)
{
    gtk_main_quit ();
    return TRUE;
}

/* The dbus method for binary client */
gboolean
deskmenu_control (Deskmenu *deskmenu, gchar *filename, gchar *workingd, GError  **error)
{
	if(chdir (workingd) == 0) {
		DeskmenuObject *dm_object = check_file_cache (deskmenu, g_strstrip(filename));
		deskmenu_show(dm_object, deskmenu, error);
		return TRUE;
	}
	return FALSE;
}

//precache backend, currently needs GUI
static void
deskmenu_precache (Deskmenu *deskmenu, gchar *filename)
{
	GError *error = NULL;
	GKeyFile *config = g_key_file_new ();
	guint i = 0;
	gsize total = 0;

	check_file_cache(deskmenu, ""); //always cache default menu

	g_print("Attempting to precache files in config...");
	if (!filename)
	{
		filename = g_build_path (G_DIR_SEPARATOR_S,
                                   g_get_user_config_dir (),
                                   "compiz",
                                   "boxmenu",
                                   "precache.ini",
                                   NULL);
	}
	if (!g_key_file_load_from_file (config, filename, G_KEY_FILE_NONE, NULL))
	{
		g_print("Configuration not found, will not precache files...");
	}
	else
	{
		g_print("Configuration found! Starting precache...");
		gchar **files = g_key_file_get_keys (config, "Files", &total, &error);
		gchar *feed = NULL;
		for (i = 0;i < total;i++)
		{
			feed = g_key_file_get_string (config, "Files", files[i], &error);
			if (feed) {
				check_file_cache(deskmenu, parse_expand_tilde(feed));
				g_free(feed);
			}
		}
		g_strfreev(files);
	}
	g_key_file_free (config);
}

int
main (int    argc,
      char **argv)
{
	DBusGConnection *connection;
    GError *error = NULL;
    GObject *deskmenu;

    g_type_init ();

    connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
    if (connection == NULL)
    {
        g_printerr ("Failed to open connection to bus: %s", error->message);
        g_error_free (error);
        exit (1);
    }

	g_print ("Starting the daemon...\n");

	GOptionContext *context;
	gchar *file = NULL;
    GOptionEntry entries[] =
    {
        { "config", 'c', 0, G_OPTION_ARG_FILENAME, &file,
            "Use FILE instead of the default daemon configuration", "FILE" },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, entries, NULL);

	error = NULL;
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_printerr ("option parsing failed: %s", error->message);
        g_error_free (error);
        return 1;
    }
    g_set_prgname ("Compiz Boxmenu");
	g_option_context_free (context);

#if HAVE_WNCK
    wnck_set_client_type (WNCK_CLIENT_TYPE_PAGER);
#endif

    gtk_init (&argc, &argv);

    deskmenu = g_object_new (DESKMENU_TYPE, NULL);

    dbus_g_connection_register_g_object (connection, DESKMENU_PATH_DBUS, deskmenu);

	if (!dbus_bus_request_name (dbus_g_connection_get_connection (connection),
						        DESKMENU_SERVICE_DBUS,
                                DBUS_NAME_FLAG_REPLACE_EXISTING,
						        NULL))
        return 1;

	set_up_element_hash();
	set_up_item_hash();
	deskmenu_precache(DESKMENU(deskmenu), file);

    gtk_main ();

    return 0;
}
