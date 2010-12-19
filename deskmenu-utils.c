#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "deskmenu-utils.h"

/* common functions used throughout compiz boxmenu */

//stolen from openbox
//optimize code to reduce calls to this, should only be called once per parse

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

gchar *get_file_path (const gchar *file)
{
	GRegex *regex;
	gchar *f;
	
	if (!file)
		return NULL;
	regex = g_regex_new("^file:///", G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);
	f = g_strstrip(special_to_actual_chars(g_regex_replace_literal(regex, file, -1, 0, "/", 0, NULL)));
	
	g_regex_unref(regex);

	return f;
}

gchar *special_to_actual_chars (const gchar *file) {
	GRegex *regex;
	gchar *f;
	
	if (!file)
		return NULL;
	regex = g_regex_new("%20", G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);
	f = g_strstrip(g_regex_replace_literal(regex, file, -1, 0, " ", 0, NULL));
	
	return f;
}

gchar *grab_only_path (const gchar *file)
{
	GRegex *regex;
	gchar *f;

	if (!file)
		return NULL;
	regex = g_regex_new(" .*", G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);
	
	f = g_strstrip(special_to_actual_chars(g_regex_replace_literal(regex, get_file_path(file), -1, 0, "", 0, NULL)));

	g_regex_unref(regex);
	return f;
}

//make sure we don't have file:///
gchar *get_full_command(const gchar *command, const gchar *file)
{
	gchar *fc;
	GRegex *regex;

	regex = g_regex_new("%f", G_REGEX_MULTILINE | G_REGEX_RAW, 0, NULL);

	if (g_regex_match (regex,command,0,0))
	{
		//if using custom complex command, replace %f with filename
		fc = g_strstrip(g_regex_replace_literal(regex, command, -1, 0, get_file_path(file), 0, NULL));
	}
	else
	{
		fc = g_strjoin (" ", command, g_strjoin("", "\"", get_file_path(file), "\"", NULL), NULL);
	}
	
	g_regex_unref(regex);

	return fc;
}

//generic function to show dialog on error
void deskmenu_widget_error (GError *error)
{
	GtkWidget *message = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
	GTK_BUTTONS_CLOSE, "%s", error->message);
	gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);
}
