/*
 *  Wikipedia Lookup - A third-party Pidgin plug-in which offers 
 *					  you the possibility to look up received and
 *					  typed words on Wikipedia.
 *
 *  Copyright (C) 2011 Hendrik Kunert kunerd@users.sourceforge.net
 *
 *  This file is part of wplookup.
 *
 *  wplookup is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with wplookup.  If not, see <http://www.gnu.org/licenses/>.
 */

#define WPLOOKUP_PLUGIN_ID "gtk-hendrik_kunert-wikipedia-lookup"

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "wpconf.h"

#include "wplookup.h"

static void wpl_save_settings()
{
	gchar *filename = NULL;
	xmlDocPtr doc = NULL;       	/* document pointer */
    xmlNodePtr root_node = NULL, 
				url_node = NULL;	/* node pointers */
	
    /* 
     * Creates a new document, a node and set it as a root node
     */
    doc = xmlNewDoc(BAD_CAST "1.0");
    root_node = xmlNewNode(NULL, BAD_CAST "settings");
    xmlDocSetRootElement(doc, root_node);

    /* 
     * xmlNewChild() creates a new node, which is "attached" as child node
     * of root_node node. 
     */
    url_node = xmlNewChild(root_node, NULL, BAD_CAST "url",
                BAD_CAST wikipedia_search_url);
	
    xmlNewProp(url_node, BAD_CAST "language", BAD_CAST "not set");

    /* 
     * Dumping document to stdio or file
     */
	filename = g_build_filename(purple_user_dir(),"settings.xml",NULL);
    xmlSaveFormatFileEnc(filename, doc, "UTF-8", 1);

	g_free(filename);

    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();
}

static void wpl_load_settings()
{	
	gchar *filename = NULL;
	xmlDocPtr doc = NULL;
	xmlChar *xpathUrl = (xmlChar*) "/settings/url";

	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	filename = g_build_filename(purple_user_dir(),"settings.xml",NULL);
	doc = xmlReadFile(filename, NULL, 0);
	g_free(filename);
	
    if (doc == NULL) {
		return;
	}

	result = getnodeset (doc, xpathUrl);
	if(result)
	{
		wikipedia_search_url = xmlNodeListGetString(doc, result->nodesetval->nodeTab[0]->xmlChildrenNode, 1);
		xmlXPathFreeObject (result);
	}

    /*free the document */
    xmlFreeDoc(doc);

    /*
     *Free the global variables that may
     *have been allocated by the parser.
     */
    xmlCleanupParser();

}

static void GetActiveConversation(PidginConversation **conv){
  GList *windows;

  /* Attach to existing conversations */
  for (windows = pidgin_conv_windows_get_list(); windows != NULL; windows = windows->next)
  {
    PidginWindow *CurrentWindow = (PidginWindow*)windows->data;
    if(pidgin_conv_window_has_focus(CurrentWindow)){
      *conv = pidgin_conv_window_get_active_gtkconv(CurrentWindow);
      return;
    }
  }
  *conv = NULL;
}

void wpl_set_url(gchar *url)
{
	int size = 0;

	/* add 1 for \0 and the other for / in the url */
	size = strlen(url)+strlen(WIKIPEDIA_PATH)+1;

	if(wikipedia_search_url != NULL)
		g_free(wikipedia_search_url);

	wikipedia_search_url = (gchar *) malloc(size*sizeof(gchar));
	
	if(wikipedia_search_url != NULL)
	{
		g_sprintf(wikipedia_search_url,"%s%s", url, WIKIPEDIA_PATH);
	}
}

static void show_wikipedia(gchar *search_text)
{
	//const gchar *wikipedia = "http://en.wikipedia.org/wiki/";
	gchar *search_url = NULL;
	int size = 0;

	size = strlen(wikipedia_search_url)+strlen(search_text)+1;
	
	search_url = (gchar *) malloc(size*sizeof(gchar));
	if(search_url != NULL)
	{
		g_sprintf(search_url,"%s%s", wikipedia_search_url, search_text);
		purple_notify_uri(wplookup_plugin_handle, search_url);
	}
	if(search_text != NULL)
		g_free(search_text);
	if(search_url != NULL)
		g_free(search_url);
}

static void menu_popup(GtkTextView *text_view, GtkMenu *menu)
{
	GtkTextBuffer *buffer = NULL;
	GtkWidget *menu_entry = NULL;
	gchar *search_text = NULL;
	GtkTextIter start_selection;
	GtkTextIter end_selection;
    
  	buffer = gtk_text_view_get_buffer(text_view);
  	
  	// somthing selected?
  	if(gtk_text_buffer_get_selection_bounds(buffer, &start_selection, &end_selection))
  	{
		  /* get selected text */
		  search_text = gtk_text_buffer_get_text(buffer, &start_selection, &end_selection, FALSE);
		  
		  /* add menu entry to popup menuy */
		  menu_entry = gtk_menu_item_new_with_label("Wikipedia");
		  gtk_menu_append(GTK_MENU(menu),menu_entry);
		  
		  /* Attach the callback functions to the activate signal */
		  g_signal_connect_swapped( GTK_OBJECT(menu_entry), "activate", GTK_SIGNAL_FUNC(show_wikipedia), (gpointer) search_text);

		  gtk_widget_show(menu_entry);
	}
}

static void wplookup_attach_conv(PurpleConversation *conv){
  PidginConversation *gtkconv;
  GtkTextView *view;

  gtkconv = PIDGIN_CONVERSATION(conv);
  
  view = GTK_TEXT_VIEW(gtkconv->imhtml);
  g_signal_connect(G_OBJECT(view),"populate-popup", G_CALLBACK(menu_popup), NULL);
  
  view = GTK_TEXT_VIEW(gtkconv->entry);
  g_signal_connect(G_OBJECT(view),"populate-popup", G_CALLBACK(menu_popup), NULL);
}

static void wplookup_remove_from_conv(PidginConversation *gtkconv){
  GtkTextView *view;
  
  view = GTK_TEXT_VIEW(gtkconv->imhtml);
  g_signal_handlers_disconnect_matched(G_OBJECT(view),G_SIGNAL_MATCH_FUNC, 0, 0, NULL, G_CALLBACK(menu_popup), NULL);
  
  view = GTK_TEXT_VIEW(gtkconv->entry);
  g_signal_handlers_disconnect_matched(G_OBJECT(view),G_SIGNAL_MATCH_FUNC, 0, 0, NULL, G_CALLBACK(menu_popup), NULL);
}

//Function is called on loading of the plugin
static gboolean
plugin_load(PurplePlugin *plugin) {
    void *conv_handle;
    GList *convs;

    conv_handle = purple_conversations_get_handle();

	/* load settings */
	wpl_load_settings();

    /* Attach to existing conversations */
    for (convs = purple_get_conversations(); convs != NULL; convs = convs->next)
	{
		wplookup_attach_conv((PurpleConversation *)convs->data);
	}

	purple_signal_connect(conv_handle, "conversation-created",
			    plugin, PURPLE_CALLBACK(wplookup_attach_conv), NULL);

    wplookup_plugin_handle = plugin;

    return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin){
  void *conv_handle;
  GList *convs;

  conv_handle = purple_conversations_get_handle();

  /*save settings */
  wpl_save_settings();

  /* free wikipedia_search_url */
	if(wikipedia_search_url != NULL)
		g_free(wikipedia_search_url);

  /* Delete Labels */
  for (convs = purple_get_conversations(); convs != NULL; convs = convs->next)
  {
    PurpleConversation *conv = (PurpleConversation *)convs->data;
    if(PIDGIN_IS_PIDGIN_CONVERSATION(conv)){
      wplookup_remove_from_conv(PIDGIN_CONVERSATION(conv));
    }
  }

  return TRUE;
}

static GtkWidget *
get_config_frame(PurplePlugin *plugin)
{
	GtkWidget *ret, *vbox, *win, *language_list;

	ret = gtk_vbox_new(FALSE, PIDGIN_HIG_CAT_SPACE);
	gtk_container_set_border_width (GTK_CONTAINER(ret), PIDGIN_HIG_BORDER);

	vbox = pidgin_make_frame(ret, "Settings");
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 4);
	gtk_widget_show(vbox);
	
	win = gtk_scrolled_window_new(0, 0);
	gtk_box_pack_start(GTK_BOX(vbox), win, TRUE, TRUE, 0);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(win),
										GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(win),
			GTK_POLICY_NEVER,
			GTK_POLICY_ALWAYS);

	gtk_widget_show(win);

	language_list = create_view_and_model();
	
	gtk_container_add(GTK_CONTAINER(win), language_list);
	gtk_widget_show(language_list);
	

	gtk_widget_show_all(ret);
	return ret;
}


static PidginPluginUiInfo settings =
{
	get_config_frame,
	0, /* page_num (Reserved) */

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    PIDGIN_PLUGIN_TYPE,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,
    WPLOOKUP_PLUGIN_ID,
    "Wikipedia Lookup",
    VERSION_NUMBER,
    "Wikipedia-Lookup Plugin",
    "Plugin that show you Wikipedia articles for recieved or typed words.",
    "Hendrik Kunert <kunerd@users.sourceforge.net>",
    "https://sourceforge.net/projects/pidginpluginfor/",
    plugin_load,  //on load
    plugin_unload,//on unload   gboolean plugin_unload(PurplePlugin *plugin)
    NULL,         //on destroy  void plugin_destroy(PurplePlugin *plugin)
    &settings,         //UI info struct pointer
    NULL,         //unknown
    NULL,         //Configuration Frame
    NULL,         //UI functions
    NULL,         //placeholder
    NULL,         //placeholder
    NULL,         //placeholder
    NULL          //placeholder
};

static void
init_plugin(PurplePlugin *plugin)
{
}

PURPLE_INIT_PLUGIN(wplookup, init_plugin, info)

