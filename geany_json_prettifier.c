/**
 * JSON Prettifier plugin for Geany editor
 * (C) 29.05.2015 zhgzhg @ github.com
 * OS: Linux
 * Dependencies: yajl, geany
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <geanyplugin.h>

#include "lloyd-yajl-66cb08c/src/api/yajl_parse.h"
#include "lloyd-yajl-66cb08c/src/api/yajl_gen.h"

#ifdef HAVE_LOCALE_H
	#include <locale.h>
#endif

GeanyPlugin *geany_plugin;
GeanyData *geany_data;
GeanyFunctions *geany_functions;

PLUGIN_VERSION_CHECK(211)

//PLUGIN_SET_INFO("JSON Prettifier", "JSON file format prettifier tool.", "1.1", "zhgzhg @@ github.com");
PLUGIN_SET_TRANSLATABLE_INFO(LOCALEDIR, GETTEXT_PACKAGE, _("JSON Prettifier"), _("JSON file format prettifier tool."), "1.1", "zhgzhg @@ github.com");

static GtkWidget *main_menu_item = NULL;

/** JSON Prettifier Code **/

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

static int reformat_string(void * ctx, const unsigned char * stringVal, size_t stringLen)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_string(g, stringVal, stringLen);
}

static int reformat_map_key(void * ctx, const unsigned char * stringVal, size_t stringLen)
{
    yajl_gen g = (yajl_gen) ctx;
    return yajl_gen_status_ok == yajl_gen_string(g, stringVal, stringLen);
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

static yajl_callbacks callbacks = { reformat_null, reformat_boolean, NULL, NULL, reformat_number, reformat_string, reformat_start_map, reformat_map_key, reformat_end_map, reformat_start_array, reformat_end_array };

static void my_json_prettify(GeanyDocument *doc)
{
	yajl_handle hand;
	
	/* generator config */
    yajl_gen g;
    yajl_status stat;
    size_t rd;
    int a = 1;

	gint text_len = 0;
	gint i = 0;
	gchar *text_string = NULL;

    if (!doc) return;
	
	text_len = sci_get_length(doc->editor->sci);
	if (text_len == 0) return;
	++text_len;
	
	text_string = g_malloc(text_len);	
	if (text_string == NULL) return;
	
	text_string = sci_get_contents(doc->editor->sci, text_len);
	
	// filter characters that may break the formatting
	
	utils_str_remove_chars(text_string,	(const gchar *)"\r\n");
	
	for (text_len = 0; text_string[text_len]; ++text_len);
	++text_len;
	
	// sci_set_text(doc->editor->sci, text_string); // for debugging
	
	/////////////

	g = yajl_gen_alloc(NULL);
    yajl_gen_config(g, yajl_gen_beautify, 1);
    yajl_gen_config(g, yajl_gen_validate_utf8, 1);

    hand = yajl_alloc(&callbacks, NULL, (void *) g);    
    yajl_config(hand, yajl_allow_comments, 1);
	
	stat = yajl_parse(hand, (unsigned char*)text_string, (size_t)text_len - 1);	
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
	}
	/*else // for debugging
	{
	    unsigned char * str = yajl_get_error(hand, 1, (unsigned char*)text_string, (size_t)text_len - 1);
        dialogs_show_msgbox(GTK_MESSAGE_INFO, (const gchar*) str);
        yajl_free_error(hand, str);
	}*/

    yajl_gen_free(g);
    yajl_free(hand);

	/////////////	
	
	g_free(text_string);
}
 
/** Geany plugin EP code **/

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata)
{
	my_json_prettify(document_get_current());
}

void plugin_init(GeanyData *data)
{
	main_menu_item = gtk_menu_item_new_with_mnemonic("JSON Prettifier");
	gtk_widget_show(main_menu_item);
	gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu), main_menu_item);
	g_signal_connect(main_menu_item, "activate", G_CALLBACK(item_activate_cb), NULL);
}

void plugin_cleanup(void)
{
	gtk_widget_destroy(main_menu_item);
}
