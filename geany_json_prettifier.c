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

PLUGIN_VERSION_CHECK(224)

PLUGIN_SET_TRANSLATABLE_INFO(LOCALEDIR,
	GETTEXT_PACKAGE,
	_("JSON Prettifier"),
	_("JSON file format prettifier and validator. \
https://github.com/zhgzhg/Geany-JSON-Prettifier"),
	"1.3.0",
	"zhgzhg @@ github.com\n\
https://github.com/zhgzhg/Geany-JSON-Prettifier"
);
//PLUGIN_KEY_GROUP(json_prettifier, 1)

static GtkWidget *main_menu_item = NULL;

/* JSON Prettifier Code - yajl example used as a basis */

static int reformat_null(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_null(g);
}


static int reformat_boolean(void * ctx, int boolean)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_bool(g, boolean);
}


static int reformat_number(void * ctx, const char * s, size_t l)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_number(g, s, l);
}


static int reformat_string(void * ctx, const unsigned char * stringVal,
							size_t stringLen)
{
    yajl_gen g = (yajl_gen) ctx;
    return (yajl_gen_status_ok ==
			yajl_gen_string(g, stringVal, stringLen));
}


static int reformat_map_key(void * ctx, const unsigned char * stringVal,
							size_t stringLen)
{
    yajl_gen g = (yajl_gen) ctx;
    return (yajl_gen_status_ok ==
			yajl_gen_string(g, stringVal, stringLen));
}


static int reformat_start_map(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_map_open(g);
}


static int reformat_end_map(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_map_close(g);
}


static int reformat_start_array(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_array_open(g);
}


static int reformat_end_array(void * ctx)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_array_close(g);
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

static void my_json_prettify(GeanyDocument *doc)
{
	yajl_handle hand;

	/* yajl generator config */

	yajl_gen g;
	yajl_status stat;

	gint text_len = 0;
	gchar *text_string = NULL;

	if (!doc) return;

	/* load the text and preparse it */

	text_len = sci_get_length(doc->editor->sci);
	if (text_len == 0) return;
	++text_len;

	text_string = g_malloc(text_len);
	if (text_string == NULL) return;

	text_string = sci_get_contents(doc->editor->sci, text_len);

	/* filter characters that may break the formatting */

	utils_str_remove_chars(text_string,	(const gchar *)"\r\n");

	for (text_len = 0; text_string[text_len]; ++text_len);
	++text_len;

	/* begin the prettifying process */

	g = yajl_gen_alloc(NULL);
	yajl_gen_config(g, yajl_gen_beautify, 1);
	yajl_gen_config(g, yajl_gen_validate_utf8, 1);

	hand = yajl_alloc(&callbacks, NULL, (void *) g);
	yajl_config(hand, yajl_allow_comments, 1);

	stat = yajl_parse(hand, (unsigned char*)text_string,
						(size_t)text_len - 1);
	stat = yajl_complete_parse(hand);

	if (stat == yajl_status_ok)
	{
		{
			const unsigned char * buf;
			size_t len;
			yajl_gen_get_buf(g, &buf, &len);

			sci_set_text(doc->editor->sci, (const gchar*) buf);
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


/* Geany plugin EP code */

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata)
{
	my_json_prettify(document_get_current());
}

static void kb_run_json_prettifier(G_GNUC_UNUSED guint key_id)
{
	my_json_prettify(document_get_current());
}


void plugin_init(GeanyData *data)
{
	main_menu_item = gtk_menu_item_new_with_mnemonic("JSON Prettifier");
	gtk_widget_show(main_menu_item);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
						main_menu_item);
	g_signal_connect(main_menu_item, "activate",
						G_CALLBACK(item_activate_cb), NULL);

	/* do not activate if there are do documents opened */
	ui_add_document_sensitive(main_menu_item);


	/* Register shortcut key group */
	geany_key_group = plugin_set_key_group(
						geany_plugin, _("json_prettifier"), 1, NULL);

	/* Ctrl + Alt + j */
	keybindings_set_item(geany_key_group, 0, kb_run_json_prettifier,
                         GDK_j, GDK_CONTROL_MASK | GDK_MOD1_MASK,
                         "run_json_prettifier",
                         _("Run the JSON Prettifier"),
                         main_menu_item);
}


void plugin_cleanup(void)
{
	gtk_widget_destroy(main_menu_item);
}
