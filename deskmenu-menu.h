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

#include "deskmenu-common.h"

typedef struct Deskmenu Deskmenu;
typedef struct DeskmenuClass DeskmenuClass;
typedef struct DeskmenuItem DeskmenuItem;

GType deskmenu_get_type (void);

typedef enum
{
    DESKMENU_ITEM_NONE = 0,
    DESKMENU_ITEM_LAUNCHER,
    DESKMENU_ITEM_WINDOWLIST,
    DESKMENU_ITEM_VIEWPORTLIST,
    DESKMENU_ITEM_RELOAD,
    DESKMENU_ITEM_DOCUMENTS //use native GTK+ support for recent documents
} DeskmenuItemType;

typedef enum
{
    DESKMENU_ELEMENT_NONE = 0,
    DESKMENU_ELEMENT_MENU,
    DESKMENU_ELEMENT_ITEM,
    DESKMENU_ELEMENT_SEPARATOR,
    DESKMENU_ELEMENT_NAME,
    DESKMENU_ELEMENT_ICON,
    DESKMENU_ELEMENT_COMMAND,
    DESKMENU_ELEMENT_THISVP,
    DESKMENU_ELEMENT_MINIONLY,
    DESKMENU_ELEMENT_WRAP,
    DESKMENU_ELEMENT_VPICON,
    DESKMENU_ELEMENT_AGE, //age of dox
    DESKMENU_ELEMENT_QUANTITY, //# of dox
    DESKMENU_ELEMENT_SORT //sort method
} DeskmenuElementType;


struct DeskmenuItem
{
    DeskmenuItemType type;
    DeskmenuElementType current_element;
    GString *name;
    gboolean name_exec;
    gboolean command_pipe;
    gboolean cache_output;
    gboolean icon_file;
    gboolean vpicon_file;
    GString *icon;
    GString *vpicon;
    GString *command;
    GString *wrap;
    GString *thisvp;
    GString *mini_only;
    GString *sort_type; //get sort type for document items
	GString *age;  //get age limit for document items
	GString *quantity; //get item limit for document items
};

struct Deskmenu
{
    GObject parent;
#if HAVE_WNCK
    DeskmenuWindowlist *windowlist;
#endif
    gboolean pinnable;
    GHashTable *file_cache;
};

struct DeskmenuClass
{
    GObjectClass parent;
};

typedef struct DeskmenuObject
{
    GtkWidget *menu;
    GtkWidget *current_menu;
    DeskmenuItem *current_item;
    Deskmenu *deskmenu;
    gboolean make_from_pipe;
    GSList *pin_items;
    GHookList *show_hooks;
} DeskmenuObject;

#define DESKMENU_TYPE              (deskmenu_get_type ())
#define DESKMENU(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DESKMENU_TYPE, Deskmenu))
#define DESKMENU_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), DESKMENU_TYPE, DeskmenuClass))
#define IS_DESKMENU(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), DESKMENU_TYPE))
#define IS_DESKMENU_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), DESKMENU_TYPE))
#define DESKMENU_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), DESKMENU_TYPE, DeskMenu))

typedef enum
{
    DESKMENU_ERROR_GENERIC
} DeskmenuError;

GQuark deskmenu_error_quark (void);
#define DESKMENU_ERROR deskmenu_error_quark ()

gboolean deskmenu_reload (Deskmenu *deskmenu, GError **error);
gboolean deskmenu_control (Deskmenu *deskmenu, gchar *filename, gchar *workingd, GError  **error);
gboolean deskmenu_pin (Deskmenu *deskmenu, gboolean pin);
gboolean deskmenu_windowlist (Deskmenu *deskmenu, gboolean images, gboolean thisvp, gboolean mini_only);
gboolean deskmenu_vplist (Deskmenu *deskmenu, gboolean toggle_wrap, gboolean toggle_images, gboolean toggle_file, gchar *viewport_icon);
gboolean deskmenu_documentlist (Deskmenu *deskmenu, gboolean images, gchar *command, int limit, int age, gchar *sort_type);
