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
#include <dbus/dbus-glib.h>
#include <string.h>
#include <unistd.h>

#include "deskmenu-common.h"

int main (int argc, char *argv[])
{
	DBusGConnection *connection;
	GError *error;
	DBusGProxy *proxy;
	gchar *icon = NULL;
	gboolean images = FALSE;
	gboolean toggle_file = FALSE;
	gboolean toggle_wrap = FALSE;
	
    usleep (200000);

    g_type_init ();

    GOptionContext *context;
    GOptionEntry entries[] =
    {
		{ "images", 'i', 0, G_OPTION_ARG_NONE, &images,
            "Use IMAGES in the viewportlist. Change the viewport icon with '-v <ICON>.'", NULL },
        { "viewport-icon", 'v', 0, G_OPTION_ARG_STRING, &icon,
            "Use ICON/FILENAME as the viewport icon.", "ICON" },
        { "wrap", 'w', 0, G_OPTION_ARG_NONE, &toggle_wrap,
            "Turn on viewport wrap navigation", NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_summary (context, "Calls the daemon to display just a viewportlist");

    error = NULL;
    connection = dbus_g_bus_get (DBUS_BUS_SESSION,
                               &error);
    if (connection == NULL)
    {
        g_printerr ("Failed to open connection to bus: %s\n",
                  error->message);
        g_error_free (error);
        return 1;
    }

    proxy = dbus_g_proxy_new_for_name (connection,
                                       DESKMENU_SERVICE_DBUS,
                                       DESKMENU_PATH_DBUS,
                                       DESKMENU_INTERFACE_DBUS);

    error = NULL;
    
    
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_printerr ("option parsing failed: %s", error->message);
        g_error_free (error);
        return 1;
    }

	if (images)
	{
		if (icon) {
			toggle_file = g_regex_match_simple ("/", icon, 0, 0);
		}
		else
		{
			icon = "user-desktop";
		}
		
	}

    if (!dbus_g_proxy_call (proxy, "vplist", &error, 
		G_TYPE_BOOLEAN, toggle_wrap,
		G_TYPE_BOOLEAN, images,
		G_TYPE_BOOLEAN, toggle_file,
		G_TYPE_STRING, icon,
        G_TYPE_INVALID, G_TYPE_INVALID))
    {
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
        return 1;
    }
  
  g_option_context_free (context);
  g_object_unref (proxy);

    return 0;
}
