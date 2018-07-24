/* Compile with: cc `pkg-config --cflags --libs gtk+-2.0 libgvc` */
#include <gtk/gtk.h>
#include <gvc.h>

static GtkWidget *text_view;
static GtkWidget *canvas;
static GVC_t *gvc;
static Agraph_t *gv_graph;

static gboolean
main_window_delete_event(GtkWidget *w, GdkEvent *e, gpointer p)
{
	gtk_main_quit();
	return FALSE;
}

static gboolean
graph_window_delete_event(GtkWidget *w, GdkEvent *e, gpointer p)
{
	gvFreeLayout(gvc, gv_graph);
	agclose(gv_graph);
	gv_graph = NULL;
	canvas = NULL;
	return FALSE;
}

static void
draw_graph(cairo_t *cr, Agraph_t *g)
{
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_paint(cr);

	cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

	boxf bb = GD_bb(g);
	double gw = bb.UR.x;
	double gh = bb.UR.y;
	for (Agnode_t *n = agfstnode(g); n; n = agnxtnode(g, n)) {
		char *name = agnameof(n);
		pointf coord = ND_coord(n);
		double ht, lw, rw;
		ht = ND_ht(n);
		lw = ND_lw(n);
		rw = ND_rw(n);
		double x, y, w, h;
		x = coord.x - lw;
		y = gh - coord.y - ht*.5;
		w = lw+rw;
		h = ht;
		cairo_rectangle(cr, x, y, w, h);
		cairo_stroke(cr);
		for (Agedge_t *e = agfstout(g, n); e; e = agnxtout(g, e)) {
			splines *spl = ED_spl(e);
			for (int b=0; b<spl->size; b++) {
				bezier* bez = &spl->list[b];
				cairo_move_to(cr,
					      bez->list[0].x,
					      gh - bez->list[0].y);
				for (int i=1; i+2<bez->size; i+=3) {
					cairo_curve_to(cr,
						       bez->list[i].x,
						       gh - bez->list[i].y,
						       bez->list[i+1].x,
						       gh - bez->list[i+1].y,
						       bez->list[i+2].x,
						       gh - bez->list[i+2].y);
				}
				cairo_stroke(cr);
			}
		}
	}
}

static gboolean
expose_event(GtkWidget *w, GdkEvent *e, gpointer p)
{
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(w));
	draw_graph(cr, gv_graph);
	cairo_destroy(cr);
	return TRUE;
}

static void
show_graph(void)
{
	gvLayout(gvc, gv_graph, "dot");
	boxf bb = GD_bb(gv_graph);
	if (canvas) {
		gdk_window_invalidate_rect
			(gtk_widget_get_window(canvas), NULL, FALSE);
	} else {
		GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		g_signal_connect(window, "delete-event",
				 G_CALLBACK(graph_window_delete_event), NULL);
		GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
		canvas = gtk_drawing_area_new();
		g_signal_connect(canvas, "expose-event",
				 G_CALLBACK(expose_event), NULL);
		gtk_scrolled_window_add_with_viewport
			(GTK_SCROLLED_WINDOW(sw), canvas);
		gtk_container_add(GTK_CONTAINER(window), sw);
		gtk_widget_show_all(window);
	}
	gtk_widget_set_size_request(canvas, bb.UR.x, bb.UR.y);
}

static void
render_button_clicked(void)
{
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buf, &start, &end);
	char *text = gtk_text_buffer_get_text(buf, &start, &end, TRUE);

	Agraph_t *g = agmemread(text);
	g_free(text);
	if (g) {
		if (gv_graph) {
			gvFreeLayout(gvc, gv_graph);
			agclose(gv_graph);
		}
		gv_graph = g;
		show_graph();
	}
}

int
main(int argc, char *argv[])
{
	gvc = gvContext();

	GtkWidget *window;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
	g_signal_connect(window, "delete-event",
			 G_CALLBACK(main_window_delete_event), NULL);
	/* populate main window */
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	text_view = gtk_text_view_new();
	GtkWidget *toolbar = gtk_toolbar_new();
	gtk_toolbar_append_item
		(GTK_TOOLBAR(toolbar), "Render", NULL, NULL, NULL,
		 render_button_clicked, NULL);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);

	gtk_widget_show_all(window);

	gtk_main();

	gvFreeContext(gvc);

	return 0;
}
