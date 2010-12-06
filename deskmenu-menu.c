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

#define HAVE_WNCK 1

#if HAVE_WNCK
#include "deskmenu-wnck.h"
#endif

#include "deskmenu-menu.h"
#include "deskmenu-glue.h"


G_DEFINE_TYPE(Deskmenu, deskmenu, G_TYPE_OBJECT) //this is calling deskmenu_class_init

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

//stolen from openbox
//optimize code to reduce calls to this, should only be called once per parse
static
gchar *parse_expand_tilde(const gchar *f)
{
gchar *ret;
GRegex *regex;

if (!f)
    return NULL;
regex = g_regex_new("(?:^|(?<=[ \\t]))~(?:(?=[/ \\t])|$)",
                    G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);
ret = g_regex_replace_literal(regex, f, -1, 0, g_get_home_dir(), 0, NULL);
g_regex_unref(regex);

return ret;
}
//end stolen

//This is how menu command is launched
static void
launcher_activated (GtkWidget *widget,
                    gchar     *command)
{
    GError *error = NULL;

	if (!gdk_spawn_command_line_on_screen (gdk_screen_get_default (), parse_expand_tilde(command), &error))
    {
        GtkWidget *message = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE, "%s", error->message);
        gtk_dialog_run (GTK_DIALOG (message));
        gtk_widget_destroy (message);
    }

}

//This is how a recent document is opened
static void
recent_activated (GtkRecentChooser *chooser,
                  gchar     *command)
{
    GError *error = NULL;
	
	gchar *full_command;
	gchar *file;
	GRegex *regex, *regex2;

	regex = g_regex_new("file:///", G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);
	regex2 = g_regex_new("%f", G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);

	file = g_strstrip(g_regex_replace_literal(regex, gtk_recent_chooser_get_current_uri (chooser), -1, 0, "/", 0, NULL));
	
	if (g_regex_match (regex2,command,0,0))
	{
		//if using custom complex command, replace %f with filename
		full_command = g_strstrip(g_regex_replace_literal(regex2, command, -1, 0, file, 0, NULL));
	}
	else
	{
		full_command = g_strjoin (" ", command, file, NULL);
	}
	
	g_regex_unref(regex);
	g_regex_unref(regex2);
	
	if (!gdk_spawn_command_line_on_screen (gdk_screen_get_default (), parse_expand_tilde(full_command), &error))
    {
        GtkWidget *message = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE, "%s", error->message);
        gtk_dialog_run (GTK_DIALOG (message));
        gtk_widget_destroy (message);
    }

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
deskmenu_construct_item (Deskmenu *deskmenu)
{
    DeskmenuItem *item = deskmenu->current_item;
    GtkWidget *menu_item;
    gchar *name, *icon, *command, *vpicon;
    gboolean images;
    gint w, h;
//constructs the items in menu
    switch (item->type)
    {
        case DESKMENU_ITEM_LAUNCHER:
            if (item->name_exec)
            {
                gchar *stdout;
                name = g_strstrip (item->name->str);
				if (g_spawn_command_line_sync (parse_expand_tilde(name), &stdout, NULL, NULL, NULL))
					menu_item = gtk_image_menu_item_new_with_mnemonic (g_strstrip(stdout));
				else
					menu_item = gtk_image_menu_item_new_with_mnemonic ("Unable to get output");
				g_free (stdout);
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

            if (item->command)
            {
				
                command = g_strstrip (item->command->str);
                g_signal_connect (G_OBJECT (menu_item), "activate",
                    G_CALLBACK (launcher_activated), g_strdup (command));
            }

            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu),
                menu_item);
            break;
#if HAVE_WNCK
        case DESKMENU_ITEM_WINDOWLIST:
            menu_item = gtk_image_menu_item_new_with_mnemonic ("_Windows");
			images = FALSE;
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
            DeskmenuWindowlist *windowlist = deskmenu_windowlist_new (images);
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                windowlist->menu);
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu),
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
            DeskmenuVplist *vplist = deskmenu_vplist_new (wrap, images, file, vpicon);
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                vplist->menu);
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu),
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
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu),
                menu_item);
            break;

        case DESKMENU_ITEM_DOCUMENTS:
            menu_item = gtk_image_menu_item_new_with_mnemonic ("Recent Doc_uments");
			gint limit, age;
			gchar *sort_type;
			images = FALSE;
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
            gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu),
                menu_item);
            break;
            
        default:
            break;
    }

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
    Deskmenu *deskmenu = DESKMENU (user_data);
    DeskmenuElementType element_type;
    const gchar **ncursor = attr_names, **vcursor = attr_values;
    GtkWidget *item, *menu;
    gint w, h;

    element_type = GPOINTER_TO_INT (g_hash_table_lookup
        (deskmenu->element_hash, element_name));

    if ((deskmenu->menu && !deskmenu->current_menu)
       || (!deskmenu->menu && element_type != DESKMENU_ELEMENT_MENU))
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

            if (deskmenu->current_item != NULL)
            {
                gint line_num, char_num;
                g_markup_parse_context_get_position (context, &line_num,
                    &char_num);
                g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                    "Error on line %d char %d: Element 'menu' cannot be nested "
                    "inside of an item element", line_num, char_num);
                return;
            }
            if (!deskmenu->menu)
            {
	            /*if (strcmp (*ncursor, "size") == 0) {
                    deskmenu->w = g_strdup (*vcursor);
                    deskmenu->h = g_strdup (*vcursor);
                    }
                else {
	                gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, deskmenu->w, deskmenu->h);
                }*/
                deskmenu->menu = gtk_menu_new ();
                g_object_set_data (G_OBJECT (deskmenu->menu), "parent menu",
                    NULL);
                deskmenu->current_menu = deskmenu->menu;
				if (deskmenu->pinnable)
				{
					gtk_menu_set_title (GTK_MENU (deskmenu->menu), "Compiz Boxmenu");
				}
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
					gchar *stdout;
					if (g_spawn_command_line_sync (parse_expand_tilde(name), &stdout, NULL, NULL, NULL))
						item = gtk_image_menu_item_new_with_mnemonic (g_strstrip(stdout));
					else
						item = gtk_image_menu_item_new_with_mnemonic ("Unable to get output");
					g_free (stdout);
				}
				else
				{
					if (name)
						item = gtk_image_menu_item_new_with_mnemonic (name); //allow menus to have icons
				
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
				gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), item);
                menu = gtk_menu_new ();
                g_object_set_data (G_OBJECT (menu), "parent menu",
                    deskmenu->current_menu);
                deskmenu->current_menu = menu;
                gtk_menu_item_set_submenu (GTK_MENU_ITEM (item),
                    deskmenu->current_menu);
                if (deskmenu->pinnable)
				{
					gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), gtk_tearoff_menu_item_new()); //add a pin menu item
				}
                g_free (name);
                g_free (icon);
            }
            break;

        case DESKMENU_ELEMENT_SEPARATOR:
        if (deskmenu->current_item != NULL)
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
					GtkHBox *box = gtk_hbox_new (FALSE, 3);
					gtk_container_add (GTK_CONTAINER(item), GTK_WIDGET(box));
					if (name_exec)
					{
						gchar *stdout;
						if (g_spawn_command_line_sync (parse_expand_tilde(name), &stdout, NULL, NULL, NULL))
							gtk_box_pack_end (GTK_BOX(box), gtk_label_new_with_mnemonic (g_strstrip(stdout)),
                                                         TRUE,
                                                         FALSE,
                                                         0);
						else
							gtk_box_pack_end (GTK_BOX(box), gtk_label_new_with_mnemonic ("Unable to get output"),
                                 TRUE,
                                 FALSE,
                                 0);
						g_free (stdout);
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
				gtk_menu_shell_append (GTK_MENU_SHELL (deskmenu->current_menu), item);
			}
            break;

        case DESKMENU_ELEMENT_ITEM:

            if (deskmenu->current_item != NULL)
            {
                gint line_num, char_num;
                g_markup_parse_context_get_position (context, &line_num,
                    &char_num);
                g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_PARSE,
                    "Error on line %d char %d: Element 'item' cannot be nested "
                    "inside of another item element", line_num, char_num);
                return;
            }

            deskmenu->current_item = g_slice_new0 (DeskmenuItem);
                while (*ncursor)
                {
                    if (strcmp (*ncursor, "type") == 0)
                        deskmenu->current_item->type = GPOINTER_TO_INT
                        (g_hash_table_lookup (deskmenu->item_hash, *vcursor));
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
                        deskmenu->current_item->name_exec = TRUE;
                    ncursor++;
                    vcursor++;
                } /* no break here to let it fall through */
        case DESKMENU_ELEMENT_ICON:
                while (*ncursor)
                {
                    if ((strcmp (*ncursor, "mode1") == 0)
                        && (strcmp (*vcursor, "file") == 0))
                        deskmenu->current_item->icon_file = TRUE;
                    ncursor++;
                    vcursor++;
                } /* no break here to let it fall through */
        case DESKMENU_ELEMENT_VPICON:
                while (*ncursor)
                {
                    if ((strcmp (*ncursor, "mode1") == 0)
                        && (strcmp (*vcursor, "file") == 0))
                        deskmenu->current_item->vpicon_file = TRUE;
                    ncursor++;
                    vcursor++;
                } /* no break here to let it fall through */
        case DESKMENU_ELEMENT_COMMAND:
        case DESKMENU_ELEMENT_WRAP:
            if (deskmenu->current_item)
                deskmenu->current_item->current_element = element_type;
            break;
        case DESKMENU_ELEMENT_QUANTITY:
            if (deskmenu->current_item)
                deskmenu->current_item->current_element = element_type;
            break;
        case DESKMENU_ELEMENT_SORT:
            if (deskmenu->current_item)
                deskmenu->current_item->current_element = element_type;
            break;
        case DESKMENU_ELEMENT_AGE:
            if (deskmenu->current_item)
                deskmenu->current_item->current_element = element_type;
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
    Deskmenu *deskmenu = DESKMENU (user_data);
    DeskmenuItem *item = deskmenu->current_item;

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
    Deskmenu *deskmenu = DESKMENU (user_data);
    GtkWidget *parent;
    element_type = GPOINTER_TO_INT (g_hash_table_lookup
        (deskmenu->element_hash, element_name));

    switch (element_type)
    {
        case DESKMENU_ELEMENT_MENU:

            g_return_if_fail (deskmenu->current_item == NULL);

            parent = g_object_get_data (G_OBJECT (deskmenu->current_menu),
                "parent menu");

            deskmenu->current_menu = parent;

            break;
/* separator building is now dealt with in the beginning */
        case DESKMENU_ELEMENT_ITEM:

            g_return_if_fail (deskmenu->current_item != NULL);

            /* finally make the item ^_^ */
            deskmenu_construct_item (deskmenu);

            /* free data used to make it */
            if (deskmenu->current_item->name)
                g_string_free (deskmenu->current_item->name, TRUE);
            if (deskmenu->current_item->icon)
                g_string_free (deskmenu->current_item->icon, TRUE);
            if (deskmenu->current_item->command)
                g_string_free (deskmenu->current_item->command, TRUE);
            if (deskmenu->current_item->wrap)
                g_string_free (deskmenu->current_item->wrap, TRUE);
            if (deskmenu->current_item->vpicon)
                g_string_free (deskmenu->current_item->vpicon, TRUE);
            if (deskmenu->current_item->sort_type)
                g_string_free (deskmenu->current_item->sort_type, TRUE);
            if (deskmenu->current_item->quantity)
                g_string_free (deskmenu->current_item->quantity, TRUE);
            if (deskmenu->current_item->age)
                g_string_free (deskmenu->current_item->age, TRUE);
            g_slice_free (DeskmenuItem, deskmenu->current_item);
            deskmenu->current_item = NULL;
            break;
        default:
            break;
    }
}

/* The list of what handler does what. */
//this parses menus
static GMarkupParser parser = {
    start_element,
    end_element,
    text,
    NULL,
    NULL
};


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

    deskmenu->menu = NULL;
    deskmenu->current_menu = NULL;
    deskmenu->current_item = NULL;
    deskmenu->pinnable = FALSE;

    deskmenu->item_hash = g_hash_table_new (g_str_hash, g_str_equal);
    deskmenu->file_cache = g_hash_table_new (g_str_hash, g_str_equal);
    deskmenu->split_cache = g_hash_table_new (g_str_hash, g_str_equal);
    deskmenu->chunk_marks = g_hash_table_new (g_str_hash, g_str_equal);

    g_hash_table_insert (deskmenu->item_hash, "launcher",
        GINT_TO_POINTER (DESKMENU_ITEM_LAUNCHER));
#if HAVE_WNCK
    g_hash_table_insert (deskmenu->item_hash, "windowlist",
        GINT_TO_POINTER (DESKMENU_ITEM_WINDOWLIST));
    g_hash_table_insert (deskmenu->item_hash, "viewportlist",
        GINT_TO_POINTER (DESKMENU_ITEM_VIEWPORTLIST));
#endif
    g_hash_table_insert (deskmenu->item_hash, "documents",
        GINT_TO_POINTER (DESKMENU_ITEM_DOCUMENTS));
    g_hash_table_insert (deskmenu->item_hash, "reload",
        GINT_TO_POINTER (DESKMENU_ITEM_RELOAD));

    deskmenu->element_hash = g_hash_table_new (g_str_hash, g_str_equal);
    
    g_hash_table_insert (deskmenu->element_hash, "menu", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_MENU));
    g_hash_table_insert (deskmenu->element_hash, "separator",
        GINT_TO_POINTER (DESKMENU_ELEMENT_SEPARATOR));
    g_hash_table_insert (deskmenu->element_hash, "item", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_ITEM));
    g_hash_table_insert (deskmenu->element_hash, "name", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_NAME));
    g_hash_table_insert (deskmenu->element_hash, "icon", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_ICON));
    g_hash_table_insert (deskmenu->element_hash, "vpicon", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_VPICON));
    g_hash_table_insert (deskmenu->element_hash, "command", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_COMMAND));
    g_hash_table_insert (deskmenu->element_hash, "wrap", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_WRAP));
    g_hash_table_insert (deskmenu->element_hash, "sort", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_SORT));
    g_hash_table_insert (deskmenu->element_hash, "quantity", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_QUANTITY));
    g_hash_table_insert (deskmenu->element_hash, "age", 
        GINT_TO_POINTER (DESKMENU_ELEMENT_AGE));
}

static
gchar *check_file_cache (Deskmenu *deskmenu, gchar *filename) {
	gchar *t = NULL;
	gchar *f = NULL;
	gchar *user_default = g_build_path (G_DIR_SEPARATOR_S,  g_get_user_config_dir (),
									"compiz",
									"boxmenu",
									"menu.xml",
									NULL);
	
	//TODO: add a size column to cache for possible autorefresh
		g_print("Checking cache...\n");	
	
	if (strlen(filename) == 0) {
		g_print("No filename supplied, looking up default menu...\n");
			/*
			set default filename to be [configdir]/compiz/boxmenu/menu.xml
			*/
			filename = user_default;
			gboolean success = FALSE;
		if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
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
			
					if (g_file_get_contents (filename, &f, NULL, NULL))
					{
						g_hash_table_insert (deskmenu->file_cache, g_strdup(filename), g_strdup(f));
						g_free (path);
						g_print("Got it!\n");
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
					g_file_get_contents (filename, &f, NULL, NULL);
					g_print("Cacheing default user file...\n");
					g_hash_table_insert (deskmenu->file_cache, g_strdup(filename), g_strdup(f));
				}
				g_print("Retrieving cached default user file...\n");
				success = TRUE;
			}
		if (!success)
		{
			g_printerr ("Couldn't find a menu file...\n");
			exit (1);		
		}
	}
	else {
		if (g_hash_table_lookup(deskmenu->file_cache, user_default) == NULL) {
			if (g_file_get_contents (filename, &f, NULL, NULL))
				{
					g_print("Cacheing new non-default file...\n");
					g_hash_table_insert (deskmenu->file_cache, g_strdup(filename), g_strdup(f));
				}
				else {
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
	
	t = g_hash_table_lookup (deskmenu->file_cache, filename);
	
	g_printf("Done loading %s!\n", filename);
	g_free (f);
	g_free (filename);
	
	return t;
}

static void
pipe_execute (gint i, gchar **menu_chunk) {
	gchar *exec, *stdout, *pipe_error;
	GRegex *command;
	command = g_regex_new("<pipe command=\"|\"/>", G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);
	pipe_error = g_strdup ("<item type=\"launcher\"><name>Cannot retrieve pipe output</name></item>");
	
    exec = parse_expand_tilde(g_strstrip(g_regex_replace_literal(command, menu_chunk[i], -1, 0, "", 0, NULL)));
	if (g_spawn_command_line_sync (exec, &stdout, NULL, NULL, NULL))
	{
		menu_chunk[i] = stdout;
	}
	else
	{
		menu_chunk[i] = pipe_error;
	}
	g_regex_unref(command); //free the pipe command extractor
    //fix this to be able to replace faulty pipe with an item that says it can't be executed
    //needs validator in order to make sure it can be parsed, if not, set parsed error
}

static void
deskmenu_parse_text (Deskmenu *deskmenu, gchar *text)
{
    GError *error = NULL;
	gchar **menu_chunk;

    GMarkupParseContext *context = g_markup_parse_context_new (&parser,
        0, deskmenu, NULL);
    

	
	GList* list = NULL, *iterator = NULL;
	list = g_hash_table_lookup (deskmenu->chunk_marks, text);
    
	if (list)
	{
		menu_chunk = g_strdupv(g_hash_table_lookup (deskmenu->split_cache, text));
		for (iterator = list; iterator; iterator = iterator->next) {
			 pipe_execute (atoi(iterator->data), menu_chunk);
		}
	}
	else
	{
		GRegex *regex = g_regex_new("(<pipe command=\".*\"/>)", G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);
		menu_chunk = g_regex_split (regex, text, 0); //this splits the menu into parsable chunks, needed for pipe item capabilities
		g_hash_table_insert(deskmenu->split_cache, g_strdup(text), g_strdupv(menu_chunk));
		int i = 0;
		//this loop will replace the pipeitem chunk with its output, other chunks are let through as is
		while (menu_chunk[i])
		{
			if (g_regex_match (regex,menu_chunk[i],0,0))
			{
				g_hash_table_insert(deskmenu->chunk_marks, g_strdup(text), 
				g_slist_append(g_hash_table_lookup(deskmenu->chunk_marks, g_strdup(text)), g_strdup_printf ("%i", i)));
				pipe_execute(i, menu_chunk);
			}
			i++;
		}
		g_regex_unref(regex); //free the pipeitem chunk checker
	}
	
	text = g_strjoinv (NULL, menu_chunk); //stitch the text so we can get error reporting back to normal
	
	if (!g_markup_parse_context_parse (context, text, strlen(text), &error)
        || !g_markup_parse_context_end_parse (context, &error))
    {
        g_printerr ("Parse failed with message: %s \n", error->message);
        g_error_free (error);
        exit (1);
    }

	g_free(text); //free the joined array
	g_strfreev(menu_chunk); //free the menu chunks and their container
    g_markup_parse_context_free (context); //free the parser

    gtk_widget_show_all (deskmenu->menu);
}

gboolean
deskmenu_vplist (Deskmenu *deskmenu,gboolean toggle_wrap, gboolean toggle_images, gboolean toggle_file, gchar *viewport_icon) {
	DeskmenuVplist *vplist = deskmenu_vplist_new(toggle_wrap, toggle_images, toggle_file, g_strdup(viewport_icon));

    gtk_menu_popup (GTK_MENU (vplist->menu),
                    NULL, NULL, NULL, NULL,
                    0, 0);

	return TRUE;
}

gboolean
deskmenu_windowlist (Deskmenu *deskmenu, gboolean images) {
	DeskmenuWindowlist *windowlist = deskmenu_windowlist_new (images);

    gtk_menu_popup (GTK_MENU (windowlist->menu),
                    NULL, NULL, NULL, NULL,
                    0, 0);
	return TRUE;
}

gboolean
deskmenu_documentlist (Deskmenu *deskmenu, gboolean images, gchar *command, int limit, int age, gchar *sort_type) {
	GtkWidget *menu = make_recent_documents_list (images, g_strdup(command), limit, age, g_strdup(sort_type));
	
    gtk_menu_popup (GTK_MENU (menu),
                    NULL, NULL, NULL, NULL,
                    0, 0);

	return TRUE;
}
/* The show method */
static void
deskmenu_show (Deskmenu *deskmenu,
               GError  **error)
{
	if (deskmenu->pinnable)
	{
		gtk_menu_set_tearoff_state (GTK_MENU (deskmenu->menu), TRUE); 
	}
	else {
    gtk_menu_popup (GTK_MENU (deskmenu->menu),
                    NULL, NULL, NULL, NULL,
                    0, 0);
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
deskmenu_control (Deskmenu *deskmenu, gchar *filename, GError  **error)
{
	if (deskmenu->menu)
	{
		gtk_widget_destroy (deskmenu->menu); //free mem
		deskmenu->menu = NULL; //destroy menu
	}
	deskmenu_parse_text(deskmenu, check_file_cache (deskmenu, 
		g_strdup(filename))); //recreate the menu, check caches for data
    
	deskmenu_show(deskmenu, error);
	return TRUE;
}


//precache backend, currently needs GUI
static void
deskmenu_precache (Deskmenu *deskmenu, gchar *filename)
{
	GError *error = NULL;
	GKeyFile *config = g_key_file_new ();
	int i;
	
 	(void *)check_file_cache(deskmenu, ""); //always cache default menu
	
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
		gchar **files = g_key_file_get_keys (config, "Files", NULL, &error);
		gchar *feed;
		
		while (files[i])
		{
			feed = g_key_file_get_string (config, "Files", files[i], &error);
			(void *)check_file_cache(deskmenu, parse_expand_tilde(feed));
			i++;
		}
		g_strfreev(files);
		g_free(feed);
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

	deskmenu_precache(DESKMENU(deskmenu), file);

    gtk_main ();

    return 0;
}
