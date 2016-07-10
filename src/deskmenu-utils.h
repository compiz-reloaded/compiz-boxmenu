gchar *special_to_actual_chars (const gchar *file);
gchar *parse_expand_tilde(const gchar *f);
gchar *get_file_path (const gchar *file);
gchar *grab_only_path (const gchar *file);
gchar *get_full_command(const gchar *command, const gchar *file);
void deskmenu_widget_error (GError *error);
