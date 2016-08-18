/*
 * geany_json_prettifier.c - a Geany plugin to format not formatted
 *                            JSON files
 *
 *  Copyright 2016 zhgzhg @ github.com
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <geanyplugin.h>
#include <gdk/gdkkeysyms.h> /* for the key bindings */

#include "lloyd-yajl-66cb08c/src/api/yajl_parse.h" /*yajl/yajl_parse.h*/
#include "lloyd-yajl-66cb08c/src/api/yajl_gen.h" /*yajl/yajl_gen.h*/

#ifdef HAVE_LOCALE_H
	#include <locale.h>
#endif

GeanyPlugin *geany_plugin;
struct GeanyKeyGroup *geany_key_group;
GeanyData *geany_data;
GeanyData *geany_data;

static gchar *plugin_config_path = NULL;
static GKeyFile *keyfile_plugin = NULL;

PLUGIN_VERSION_CHECK(224)

PLUGIN_SET_TRANSLATABLE_INFO(LOCALEDIR,
	GETTEXT_PACKAGE,
	_("JSON Prettifier"),
	_("JSON file format prettifier, minifier and validator.\n\
https://github.com/zhgzhg/Geany-JSON-Prettifier"),
	"1.4.0",
	"zhgzhg @@ github.com\n\
https://github.com/zhgzhg/Geany-JSON-Prettifier"
);

static GtkWidget *main_menu_item = NULL;
static GtkWidget *main_menu_item2 = NULL;
static GtkWidget *escape_forward_slashes_btn = NULL;
static GtkWidget *allow_invalid_utf8_text_strings_btn = NULL;
static GtkWidget *reformat_multiple_json_entities_at_once_btn = NULL;

static gboolean escapeForwardSlashes = FALSE;
static gboolean allowInvalidStringsInUtf8 = TRUE;
static gboolean reformatMultipleJsonEntities = FALSE;

/* JSON Prettifier Code - yajl example used as a basis */

#define GEN_AND_RETURN(func)                                           \
{                                                                      \
	yajl_gen_status __stat = func;                                     \
	if (__stat == yajl_gen_generation_complete &&                      \
			reformatMultipleJsonEntities)                              \
	{                                                                  \
		yajl_gen_reset(g, "\n");                                       \
		__stat = func;                                                 \
	}                                                                  \
	return __stat == yajl_gen_status_ok;                               \
}

static int reformat_null(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    GEN_AND_RETURN(yajl_gen_null(g));
}


static int reformat_boolean(void * ctx, int boolean)
{
    yajl_gen g = (yajl_gen) ctx;
    GEN_AND_RETURN(yajl_gen_bool(g, boolean));
}


static int reformat_number(void * ctx, const char * s, size_t l)
{
    yajl_gen g = (yajl_gen) ctx;
    GEN_AND_RETURN(yajl_gen_number(g, s, l));
}


static int reformat_string(void * ctx, const unsigned char * stringVal,
							size_t stringLen)
{
    yajl_gen g = (yajl_gen) ctx;
    GEN_AND_RETURN(yajl_gen_string(g, stringVal, stringLen));
}


static int reformat_map_key(void * ctx, const unsigned char * stringVal,
							size_t stringLen)
{
    yajl_gen g = (yajl_gen) ctx;
    GEN_AND_RETURN(yajl_gen_string(g, stringVal, stringLen));
}


static int reformat_start_map(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    GEN_AND_RETURN(yajl_gen_map_open(g));
}


static int reformat_end_map(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    GEN_AND_RETURN(yajl_gen_map_close(g));
}


static int reformat_start_array(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    GEN_AND_RETURN(yajl_gen_array_open(g));
}


static int reformat_end_array(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    GEN_AND_RETURN(yajl_gen_array_close(g));
}


static yajl_callbacks callbacks = {
	reformat_null,
	reformat_boolean,
	NULL,
	NULL,
	reformat_number,
	reformat_string,
	reformat_start_map,
	reformat_map_key,
	reformat_end_map,
	reformat_start_array,
	reformat_end_array
};

static void my_json_prettify(GeanyDocument *doc, gboolean beautify)
{
	yajl_handle hand;

	/* yajl generator config */

	yajl_gen g;
	yajl_status stat;

	gint text_len = 0;
	gchar *text_string = NULL;

	if (!doc) return;

	/* load the text and preparse it */

	gboolean workWithTextSelection = FALSE;

	/* first try to work only with a text selection (if any) */
	if (sci_has_selection(doc->editor->sci))
	{
		text_string = sci_get_selection_contents(doc->editor->sci);
		if (text_string != NULL)
		{
			workWithTextSelection = TRUE;
			text_len = sci_get_selected_text_length(doc->editor->sci) + 1;
		}
	}
	else
	{	/* Work with the entire file */
		text_len = sci_get_length(doc->editor->sci);
		if (text_len == 0) return;
		++text_len;
		text_string = sci_get_contents(doc->editor->sci, -1);
	}

	if (text_string == NULL) return;

	/* filter characters that may break the formatting */

	utils_str_remove_chars(text_string,	(const gchar *)"\r\n");

	for (text_len = 0; text_string[text_len]; ++text_len);
	++text_len;


	/* begin the prettifying process */

	g = yajl_gen_alloc(NULL);
	yajl_gen_config(g, yajl_gen_beautify, beautify);
	yajl_gen_config(g, yajl_gen_validate_utf8, 1);
	yajl_gen_config(g, yajl_gen_escape_solidus,
			escapeForwardSlashes);


	hand = yajl_alloc(&callbacks, NULL, (void *) g);
	yajl_config(hand, yajl_dont_validate_strings,
			allowInvalidStringsInUtf8);
	yajl_config(hand, yajl_allow_multiple_values,
			reformatMultipleJsonEntities);

	stat = yajl_parse(hand, (unsigned char*)text_string,
						(size_t)text_len - 1);
	stat = yajl_complete_parse(hand);

	if (stat == yajl_status_ok)
	{
		{
			const unsigned char * buf;
			size_t len;
			yajl_gen_get_buf(g, &buf, &len);

			gint spos;
			gboolean insertNewLineAtTheEnd = FALSE;

			if (!workWithTextSelection)
			{ sci_set_text(doc->editor->sci, (const gchar*) buf); }
			else
			{
				spos = sci_get_selection_start(doc->editor->sci);

				sci_replace_sel(doc->editor->sci, (const gchar*) buf);

				/* Insert additional new lines for user's ease */
				if (beautify && spos > 0)
				{
					sci_insert_text(doc->editor->sci, spos,
						editor_get_eol_char(doc->editor));
				}
			}
			yajl_gen_clear(g);
		}

		msgwin_msg_add(COLOR_BLUE, -1, doc,
			"Prettifying of %s succeeded! (%s)",
			document_get_basename_for_display(doc, -1),
			DOC_FILENAME(doc)
		);
	}
	else
	{
		unsigned char *err_str = yajl_get_error(hand, 1,
					(unsigned char*)text_string, (size_t)text_len - 1);

		msgwin_msg_add(COLOR_RED, -1, doc,
			"Prettifying of %s failed!\n%s\n\
Probably improper format or odd symbols! (%s)",
			document_get_basename_for_display(doc, -1),
			err_str,
			DOC_FILENAME(doc));

		yajl_free_error(hand, err_str);
	}

	yajl_gen_free(g);
	yajl_free(hand);

	g_free(text_string);
}

/* Plugin settings */

static void config_save_setting(GKeyFile *keyfile, const gchar *filePath)
{
	if (keyfile && filePath)
		g_key_file_save_to_file(keyfile, filePath, NULL);
}


static gboolean config_get_setting(GKeyFile *keyfile, const gchar *name)
{
	if (keyfile)
		return g_key_file_get_boolean(keyfile, "settings", name, NULL);

	return FALSE;
}


static void config_set_setting(GKeyFile *keyfile, const gchar *name,
								gboolean value)
{
	if (keyfile)
		g_key_file_set_boolean(keyfile, "settings", name, value);
}

static void on_configure_response(GtkDialog* dialog, gint response,
									gpointer user_data)
{
	gboolean value = FALSE;

	if (keyfile_plugin &&
		(response == GTK_RESPONSE_OK || response == GTK_RESPONSE_APPLY))
	{
		value = gtk_toggle_button_get_active(
							GTK_TOGGLE_BUTTON(
								escape_forward_slashes_btn));
		escapeForwardSlashes = value;
		config_set_setting(keyfile_plugin,
							"escape_forward_slashes", value);


		value = gtk_toggle_button_get_active(
							GTK_TOGGLE_BUTTON(
								allow_invalid_utf8_text_strings_btn));
		allowInvalidStringsInUtf8 = value;
		config_set_setting(keyfile_plugin,
							"allow_invalid_utf8_text_strings", value);


		value = gtk_toggle_button_get_active(
					GTK_TOGGLE_BUTTON(
						reformat_multiple_json_entities_at_once_btn));
		reformatMultipleJsonEntities = value;
		config_set_setting(keyfile_plugin,
							"reformat_multiple_json_entities_at_once",
							value);


		config_save_setting(keyfile_plugin, plugin_config_path);
	}
}

static void config_set_defaults(GKeyFile *keyfile)
{
	if (!keyfile) return;
	config_set_setting(keyfile,	"escape_forward_slashes", FALSE);
	escapeForwardSlashes = FALSE;
	config_set_setting(keyfile, "allow_invalid_utf8_text_strings",
		TRUE);
	allowInvalidStringsInUtf8 = TRUE;
	config_set_setting(keyfile,
		"reformat_multiple_json_entities_at_once", FALSE);
	reformatMultipleJsonEntities = FALSE;
}

GtkWidget *plugin_configure(GtkDialog *dialog)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
	GtkWidget *_hbox1 = gtk_hbox_new(FALSE, 6);
	GtkWidget *_hbox2 = gtk_hbox_new(FALSE, 6);
	GtkWidget *_hbox3 = gtk_hbox_new(FALSE, 6);

	escape_forward_slashes_btn = gtk_check_button_new_with_label(
		_("Escape any forward slashes."));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(escape_forward_slashes_btn),
		config_get_setting(keyfile_plugin,
							"escape_forward_slashes"));


	allow_invalid_utf8_text_strings_btn =
		gtk_check_button_new_with_label(
				_("Allow invalid UTF-8 in the strings."));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(allow_invalid_utf8_text_strings_btn),
		config_get_setting(keyfile_plugin,
				"allow_invalid_utf8_text_strings"));


	reformat_multiple_json_entities_at_once_btn =
		gtk_check_button_new_with_label(
				_("Reformat multiple JSON entities at once."));
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(reformat_multiple_json_entities_at_once_btn),
		config_get_setting(keyfile_plugin,
				"reformat_multiple_json_entities_at_once"));



	gtk_box_pack_start(GTK_BOX(_hbox1), escape_forward_slashes_btn,
						TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_hbox2),
						allow_invalid_utf8_text_strings_btn, TRUE, TRUE,
						0);
	gtk_box_pack_start(GTK_BOX(_hbox3),
						reformat_multiple_json_entities_at_once_btn,
						TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), _hbox1, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _hbox2, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), _hbox3, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);

	g_signal_connect(dialog, "response",
						G_CALLBACK(on_configure_response), NULL);

	return vbox;
}


/* Geany plugin EP code */

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata)
{
	my_json_prettify(document_get_current(), TRUE);
}

static void kb_run_json_prettifier(G_GNUC_UNUSED guint key_id)
{
	my_json_prettify(document_get_current(), TRUE);
}

static void item_activate_cb2(GtkMenuItem *menuitem, gpointer gdata)
{
	my_json_prettify(document_get_current(), FALSE);
}

static void kb_run_json_minifier(G_GNUC_UNUSED guint key_id)
{
	my_json_prettify(document_get_current(), FALSE);
}

void plugin_init(GeanyData *data)
{
	/* read & prepare configuration */
	gchar *config_dir = g_build_path(G_DIR_SEPARATOR_S,
		geany_data->app->configdir, "plugins", "jsonconverter", NULL);
	plugin_config_path = g_build_path(G_DIR_SEPARATOR_S, config_dir,
										"jsonconverter.conf", NULL);

	g_mkdir_with_parents(config_dir, S_IRUSR | S_IWUSR | S_IXUSR);
	g_free(config_dir);

	keyfile_plugin = g_key_file_new();

	if (!g_key_file_load_from_file(keyfile_plugin, plugin_config_path,
									G_KEY_FILE_NONE, NULL))
	{
		config_set_defaults(keyfile_plugin);
		config_save_setting(keyfile_plugin, plugin_config_path);
	}
	else
	{
		escapeForwardSlashes = config_get_setting(keyfile_plugin,
									"escape_forward_slashes");

		allowInvalidStringsInUtf8 = config_get_setting(keyfile_plugin,
									"allow_invalid_utf8_text_strings");

		reformatMultipleJsonEntities = config_get_setting(
							keyfile_plugin,
							"reformat_multiple_json_entities_at_once");
	}

	/* ---------------------------- */

	main_menu_item = gtk_menu_item_new_with_mnemonic("JSON Prettifier");
	gtk_widget_show(main_menu_item);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
						main_menu_item);
	g_signal_connect(main_menu_item, "activate",
						G_CALLBACK(item_activate_cb), NULL);

	main_menu_item2 = gtk_menu_item_new_with_mnemonic("JSON Minifier");
	gtk_widget_show(main_menu_item2);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
						main_menu_item2);
	g_signal_connect(main_menu_item2, "activate",
						G_CALLBACK(item_activate_cb2), NULL);

	/* do not activate if there are do documents opened */
	ui_add_document_sensitive(main_menu_item);
	ui_add_document_sensitive(main_menu_item2);


	/* Register shortcut key group */
	geany_key_group = plugin_set_key_group(
						geany_plugin, _("json_prettifier"), 2, NULL);


	/* Ctrl + Alt + j to pretify */
	keybindings_set_item(geany_key_group, 0, kb_run_json_prettifier,
                         GDK_j, GDK_CONTROL_MASK | GDK_MOD1_MASK,
                         "run_json_prettifier",
                         _("Run the JSON Prettifier"),
                         main_menu_item);

	/* Ctrl + Alt + m to minify */
	keybindings_set_item(geany_key_group, 1, kb_run_json_minifier,
                         GDK_m, GDK_CONTROL_MASK | GDK_MOD1_MASK,
                         "run_json_minifier",
                         _("Run the JSON Minifier"),
                         main_menu_item2);
}


void plugin_cleanup(void)
{
	g_free(plugin_config_path);
	g_key_file_free(keyfile_plugin);
	gtk_widget_destroy(main_menu_item2);
	gtk_widget_destroy(main_menu_item);
}
