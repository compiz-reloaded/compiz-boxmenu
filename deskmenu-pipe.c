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
#include <stdio.h>

#include "deskmenu-common.h"

int main (int argc, char *argv[])
{
	DBusGConnection *connection;
	GError *error;
	DBusGProxy *proxy;
	gboolean pin = FALSE;
	
    usleep (200000);

	#if !GLIB_CHECK_VERSION(2,36,0)
		g_type_init ();
	#endif

    GOptionContext *context;
    GOptionEntry entries[] =
    {
        { "pin", 'p', 0, G_OPTION_ARG_NONE, &pin,
            "PIN the root menu as soon as it's generated", NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
    };

    context = g_option_context_new (NULL);
    g_option_context_add_main_entries (context, entries, NULL);

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
    
	gchar *intext;
	gsize length;
	if (!isatty(fileno(stdin))) {
		GIOChannel *gioch = g_io_channel_unix_new(fileno(stdin));
		GIOStatus ret_value = g_io_channel_read_line (gioch, &intext, &length, NULL, NULL);
	}
	else {
		g_printerr("Cannot pipe a tty as menu contents!");
		return 1;
	}
    
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_printerr ("option parsing failed: %s", error->message);
        g_error_free (error);
        return 1;
    }
    if (!dbus_g_proxy_call (proxy, "pin", &error, G_TYPE_BOOLEAN, pin,
        G_TYPE_INVALID, G_TYPE_INVALID))
    {
        g_printerr ("Error: %s\n", error->message);
        g_error_free (error);
        return 1;
    }    
    if (!dbus_g_proxy_call (proxy, "tmppipe", &error, 
        G_TYPE_STRING, intext,
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

