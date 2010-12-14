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
#include <unistd.h>

#include "deskmenu-common.h"

int main (int argc, char *argv[])
{
	DBusGConnection *connection;
	GError *error;
	DBusGProxy *proxy;
	gboolean images = FALSE;
	gboolean thisvp = FALSE;
	gboolean mini_only = FALSE;
	
    usleep (200000);

    g_type_init ();

    GOptionContext *context;
    GOptionEntry entries[] =
    {
        { "images", 'i', 0, G_OPTION_ARG_NONE, &images,
            "Use IMAGES in the windowlist", NULL },
        { "minimized", 'm', 0, G_OPTION_ARG_NONE, &mini_only,
            "Show only MINIMIZED windows in the windowlist", NULL },
        { "current-viewport", 'c', 0, G_OPTION_ARG_NONE, &thisvp,
            "Show only windows in CURRENT VIEWPORT in the windowlist", NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, entries, NULL);
    g_option_context_set_summary (context, "Calls the daemon to display just a windowlist");

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
    if (!dbus_g_proxy_call (proxy, "windowlist", &error, 
		G_TYPE_BOOLEAN, images,
		G_TYPE_BOOLEAN, thisvp,
		G_TYPE_BOOLEAN, mini_only,
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
