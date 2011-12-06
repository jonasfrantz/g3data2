/*

g3data2 : A program for grabbing data from scanned graphs
Copyright (C) 2011 Jonas Frantz

    This file is part of g3data2.

    g3data2 is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    g3data2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Authors email : jonas@frantz.fi

 */

#include <gtk/gtk.h>									/* Include gtk library */
#include <stdio.h>									/* Include stdio library */
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>									/* Include stdlib library */
#include <string.h>									/* Include string library */
#include <math.h>									/* Include math library */
#include <libgen.h>
#include <unistd.h>
#include "main.h"									/* Include predined variables */
#include "strings.h"									/* Include strings */

#ifdef NOSPACING
#define SECT_SEP 0
#define GROUP_SEP 0
#define ELEM_SEP 0
#define FRAME_INDENT 0
#define WINDOW_BORDER 0
#else
#define SECT_SEP 12
#define GROUP_SEP 12
#define ELEM_SEP 6
#define FRAME_INDENT 18
#define WINDOW_BORDER 12
#endif

// Declaration of gtk variables
GtkWidget *window; /* Window */
GtkWidget *mainnotebook;
GtkActionGroup *tab_action_group;

// Declaration of global variables
//gint ViewedTabNum = -1;
gint NoteBookNumPages = 0;
//gboolean WinFullScreen;
gboolean MovePointMode = FALSE;
gboolean HideLog = FALSE, HideZoomArea = FALSE, HideOpProp = FALSE;
//FILE *FP;

// Declaration of extern functions
extern void SetNumPointsEntry(GtkWidget *np_entry, gint np);
extern gint min(gint x, gint y);
extern void DrawMarker(cairo_t *cr, gint x, gint y, gint type);
extern struct PointValue CalcPointValue(gint Xpos, gint Ypos,
		struct TabData *tabData);
extern void print_results(GtkWidget *widget, gpointer func_data);

// Excplicit declaration of functions
void remove_last(GtkWidget *widget, gpointer data);

/****************************************************************/
/* This function closes the window when the application is 	*/
/* killed.							*/
/****************************************************************/
gint close_application(GtkWidget *widget, GdkEvent *event, gpointer data) {
	gtk_main_quit(); /* Quit gtk */
	return FALSE;
}

gboolean draw_zoom_callback(GtkWidget *widget, cairo_t *cr, gpointer data) {
	cairo_t *first_cr;
	cairo_surface_t *first;
	struct TabData *tabData;

	tabData = (struct TabData *) data;

	if (tabData->mousePointerCoords[0] != -1) {

		first = cairo_surface_create_similar(cairo_get_target(cr),
				CAIRO_CONTENT_COLOR, ZOOMPIXSIZE, ZOOMPIXSIZE);

		first_cr = cairo_create(first);
		cairo_scale(first_cr, ZOOMFACTOR, ZOOMFACTOR);
		cairo_set_source_surface(
				first_cr,
				tabData->image,
				-tabData->mousePointerCoords[0] + ZOOMPIXSIZE
						/ (2 * ZOOMFACTOR),
				-tabData->mousePointerCoords[1] + ZOOMPIXSIZE
						/ (2 * ZOOMFACTOR));
		cairo_paint(first_cr);
		cairo_scale(first_cr, 1.0 / ZOOMFACTOR, 1.0 / ZOOMFACTOR);

		DrawMarker(first_cr, ZOOMPIXSIZE / 2, ZOOMPIXSIZE / 2, 2);

		cairo_set_source_surface(cr, first, 0, 0);
		cairo_paint(cr);

		cairo_surface_destroy(first);

		cairo_destroy(first_cr);
	}

	return TRUE;
}

gboolean draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data) {
	guint width, height;
	gint i;
	cairo_t *first_cr;
	cairo_surface_t *first;
	struct TabData *tabData;

	tabData = (struct TabData *) data;

	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);

	first = cairo_surface_create_similar(cairo_get_target(cr),
			CAIRO_CONTENT_COLOR, width, height);

	first_cr = cairo_create(first);
	cairo_set_source_surface(first_cr, tabData->image, 0, 0);
	cairo_paint(first_cr);

	for (i = 0; i < 4; i++) {
		if (tabData->bpressed[i]) {
			DrawMarker(first_cr, tabData->axiscoords[i][0],
					tabData->axiscoords[i][1], i / 2);
		}
	}

	for (i = 0; i < tabData->numpoints; i++) {
		DrawMarker(first_cr, tabData->points[i][0], tabData->points[i][1], 2);
	}

	cairo_set_source_surface(cr, first, 0, 0);
	cairo_paint(cr);

	cairo_surface_destroy(first);

	cairo_destroy(first_cr);

	return TRUE;
}
/****************************************************************/
/* This function sets the sensitivity of the buttons depending	*/
/* the control variables.					*/
/****************************************************************/
void SetButtonSensitivity(struct TabData *tabData) {
	char ttbuf[256];

	if (tabData->Action == PRINT2FILE) {
		snprintf(ttbuf, sizeof(ttbuf), printfilett,
				gtk_entry_get_text(GTK_ENTRY (tabData->file_entry)));
		gtk_widget_set_tooltip_text(tabData->exportbutton, ttbuf);

		gtk_widget_set_sensitive(tabData->file_entry, TRUE);
		if (tabData->valueset[0] && tabData->valueset[1]
				&& tabData->valueset[2] && tabData->valueset[3]
				&& tabData->bpressed[0] && tabData->bpressed[1]
				&& tabData->bpressed[2] && tabData->bpressed[3]
				&& tabData->numpoints > 0 && tabData->file_name_length > 0)
			gtk_widget_set_sensitive(tabData->exportbutton, TRUE);
		else
			gtk_widget_set_sensitive(tabData->exportbutton, FALSE);
	} else {
		gtk_widget_set_tooltip_text(tabData->exportbutton, printrestt);
		gtk_widget_set_sensitive(tabData->file_entry, FALSE);
		if (tabData->valueset[0] && tabData->valueset[1]
				&& tabData->valueset[2] && tabData->valueset[3]
				&& tabData->bpressed[0] && tabData->bpressed[1]
				&& tabData->bpressed[2] && tabData->bpressed[3]
				&& tabData->numpoints > 0)
			gtk_widget_set_sensitive(tabData->exportbutton, TRUE);
		else
			gtk_widget_set_sensitive(tabData->exportbutton, FALSE);
	}

	if (tabData->numlastpoints == 0) {
		gtk_widget_set_sensitive(tabData->remlastbutton, FALSE);
		gtk_widget_set_sensitive(tabData->remallbutton, FALSE);
	} else {
		gtk_widget_set_sensitive(tabData->remlastbutton, TRUE);
		gtk_widget_set_sensitive(tabData->remallbutton, TRUE);
	}
}

/****************************************************************/
/* When a button is pressed inside the drawing area this 	*/
/* function is called, it handles axispoints and graphpoints	*/
/* and paints a square in that position.			*/
/****************************************************************/
gint button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data) {
	GdkModifierType state;
	gint x, y, i, j;
	struct TabData *tabData;

	tabData = (struct TabData *) data;

	gdk_window_get_pointer(event->window, &x, &y, &state); /* Get pointer state */

	if (event->button == 1) { /* If button 1 (leftmost) is pressed */
		if (MovePointMode) {
			for (i = 0; i < tabData->numpoints; i++) {
				if (abs(tabData->points[i][0] - x) < MOVETRESHOLD && abs(
						tabData->points[i][1] - y) < MOVETRESHOLD) {
					printf("Moving point %d\n", i);
				}
			}
		} else {
			/* If none of the set axispoint buttons been pressed */
			if (!tabData->setxypressed[0] && !tabData->setxypressed[1]
					&& !tabData->setxypressed[2] && !tabData->setxypressed[3]) {
				if (tabData->numpoints > tabData->MaxPoints - 1) {
					i = tabData->MaxPoints;
					tabData->MaxPoints += MAXPOINTS;
					tabData->lastpoints = realloc(tabData->lastpoints,
							sizeof(gint) * (tabData->MaxPoints + 4));
					if (tabData->lastpoints == NULL) {
						printf(
								"Error reallocating memory for lastpoints. Exiting.\n");
						exit(-1);
					}
					tabData->points = realloc(tabData->points,
							sizeof(gint *) * tabData->MaxPoints);
					if (tabData->points == NULL) {
						printf(
								"Error reallocating memory for points. Exiting.\n");
						exit(-1);
					}
					for (; i < tabData->MaxPoints; i++) {
						tabData->points[i] = malloc(sizeof(gint) * 2);
						if (tabData->points[i] == NULL) {
							printf(
									"Error allocating memory for points[%d]. Exiting.\n",
									i);
							exit(-1);
						}
					}
				}
				tabData->points[tabData->numpoints][0] = x; /* Save x coordinate */
				tabData->points[tabData->numpoints][1] = y; /* Save x coordinate */
				tabData->lastpoints[tabData->numlastpoints]
						= tabData->numpoints; /* Save index of point */
				tabData->numlastpoints++; /* Increase lastpoint index */
				tabData->numpoints++; /* Increase point counter */
				SetNumPointsEntry(tabData->nump_entry, tabData->numpoints);

			} else {
				for (i = 0; i < 4; i++)
					if (tabData->setxypressed[i]) { /* If the "Set point 1 on x axis" button is pressed */
						tabData->axiscoords[i][0] = x; /* Save coordinates */
						tabData->axiscoords[i][1] = y;
						for (j = 0; j < 4; j++)
							if (i != j)
								gtk_widget_set_sensitive(
										tabData->setxybutton[j], TRUE);
						gtk_widget_set_sensitive(tabData->xyentry[i], TRUE); /* Sensitize the entry */
						gtk_editable_set_editable(
								(GtkEditable *) tabData->xyentry[i], TRUE);
						gtk_widget_grab_focus(tabData->xyentry[i]); /* Focus on entry */
						tabData->setxypressed[i] = FALSE; /* Mark the button as not pressed */
						tabData->bpressed[i] = TRUE; /* Mark that axis point's been set */
						gtk_toggle_button_set_active(
								GTK_TOGGLE_BUTTON(tabData->setxybutton[i]),
								FALSE); /* Pop up the button */
						tabData->lastpoints[tabData->numlastpoints] = -(i + 1); /* Remember that the points been put out */
						tabData->numlastpoints++; /* Increase index of lastpoints */

					}
			}
			SetButtonSensitivity(tabData);
		}
	} else if (event->button == 2) { /* Is the middle button pressed ? */
		for (i = 0; i < 2; i++)
			if (!tabData->bpressed[i]) {
				tabData->axiscoords[i][0] = x;
				tabData->axiscoords[i][1] = y;
				for (j = 0; j < 4; j++)
					if (i != j)
						gtk_widget_set_sensitive(tabData->setxybutton[j], TRUE);
				gtk_widget_set_sensitive(tabData->xyentry[i], TRUE);
				gtk_editable_set_editable((GtkEditable *) tabData->xyentry[i],
						TRUE);
				gtk_widget_grab_focus(tabData->xyentry[i]);
				tabData->setxypressed[i] = FALSE;
				tabData->bpressed[i] = TRUE;
				gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(tabData->setxybutton[i]), FALSE);
				tabData->lastpoints[tabData->numlastpoints] = -(i + 1);
				tabData->numlastpoints++;

				break;
			}
	} else if (event->button == 3) { /* Is the right button pressed ? */
		for (i = 2; i < 4; i++)
			if (!tabData->bpressed[i]) {
				tabData->axiscoords[i][0] = x;
				tabData->axiscoords[i][1] = y;
				for (j = 0; j < 4; j++)
					if (i != j)
						gtk_widget_set_sensitive(tabData->setxybutton[j], TRUE);
				gtk_widget_set_sensitive(tabData->xyentry[i], TRUE);
				gtk_editable_set_editable((GtkEditable *) tabData->xyentry[i],
						TRUE);
				gtk_widget_grab_focus(tabData->xyentry[i]);
				tabData->setxypressed[i] = FALSE;
				tabData->bpressed[i] = TRUE;
				gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(tabData->setxybutton[i]), FALSE);
				tabData->lastpoints[tabData->numlastpoints] = -(i + 1);
				tabData->numlastpoints++;

				break;
			}
	}

	gtk_widget_queue_draw(tabData->drawing_area);

	SetButtonSensitivity(tabData);
	return TRUE;
}

/****************************************************************/
/* This function is called when a button is released on the	*/
/* drawing area, currently this function does not perform any	*/
/* task.							*/
/****************************************************************/
gint button_release_event(GtkWidget *widget, GdkEventButton *event,
		gpointer data) {
// For future reference:
//	struct TabData *tabData;
//
//	tabData = (struct TabData *) data;

	if (event->button == 1) {
	} else if (event->button == 2) {
	} else if (event->button == 3) {
	}
	return TRUE;
}

/****************************************************************/
/* This function is called when movement is detected in the	*/
/* drawing area, it captures the coordinates and zoom in om the */
/* position and plots it on the zoom area.			*/
/****************************************************************/
gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event,
		gpointer data) {
	gint x, y;
	gchar buf[32];
	GdkModifierType state;
	struct PointValue CalcVal;
	struct TabData *tabData;

	tabData = (struct TabData *) data;

	gdk_window_get_pointer(event->window, &x, &y, &state); /* Grab mousepointers coordinates */
	/* on drawing area. */

	if (x >= 0 && y >= 0 && x < tabData->XSize && y < tabData->YSize) {

		tabData->mousePointerCoords[0] = x;
		tabData->mousePointerCoords[1] = y;
		gtk_widget_queue_draw(tabData->zoom_area);

		if (tabData->valueset[0] && tabData->valueset[1]
				&& tabData->valueset[2] && tabData->valueset[3]) {
			CalcVal = CalcPointValue(x, y, tabData);

			sprintf(buf, "%16.10g", CalcVal.Xv);
			gtk_entry_set_text(GTK_ENTRY(tabData->xc_entry), buf); /* Put out coordinates in entries */
			sprintf(buf, "%16.10g", CalcVal.Yv);
			gtk_entry_set_text(GTK_ENTRY(tabData->yc_entry), buf);
			sprintf(buf, "%16.10g", CalcVal.Xerr);
			gtk_entry_set_text(GTK_ENTRY(tabData->xerr_entry), buf); /* Put out coordinates in entries */
			sprintf(buf, "%16.10g", CalcVal.Yerr);
			gtk_entry_set_text(GTK_ENTRY(tabData->yerr_entry), buf);
		} else {
			gtk_entry_set_text(GTK_ENTRY(tabData->xc_entry), ""); /* Else clear entries */
			gtk_entry_set_text(GTK_ENTRY(tabData->yc_entry), "");
			gtk_entry_set_text(GTK_ENTRY(tabData->xerr_entry), "");
			gtk_entry_set_text(GTK_ENTRY(tabData->yerr_entry), "");
		}
	} else {
		gtk_entry_set_text(GTK_ENTRY(tabData->xc_entry), ""); /* Else clear entries */
		gtk_entry_set_text(GTK_ENTRY(tabData->yc_entry), "");
		gtk_entry_set_text(GTK_ENTRY(tabData->xerr_entry), "");
		gtk_entry_set_text(GTK_ENTRY(tabData->yerr_entry), "");
	}
	return TRUE;
}

/****************************************************************/
/* This function is called when the "Set point 1/2 on x/y axis"	*/
/* button is pressed. It inactivates the other "Set" buttons	*/
/* and makes sure the button stays down even when pressed on.	*/
/****************************************************************/
void toggle_xy(GtkToggleButton *widget, gpointer data) {
	gint index, i;
	struct ButtonData *buttonData;
	struct TabData *tabData;

	buttonData = (struct ButtonData *) data;
	index = buttonData->index;
	tabData = buttonData->tabData;

	if (gtk_toggle_button_get_active(widget)) { /* Is the button pressed on ? */
		tabData->setxypressed[index] = TRUE; /* The button is pressed down */
		for (i = 0; i < 4; i++) {
			if (index != i)
				gtk_widget_set_sensitive(tabData->setxybutton[i], FALSE);
		}
		if (tabData->bpressed[index]) { /* If the x axis point is already set */
			//			remthis = -(index + 1); /* remove the square */
			//			remove_last(GTK_WIDGET(widget), NULL);
		}
		tabData->bpressed[index] = FALSE; /* Set x axis point 1 to unset */
	} else { /* If button is trying to get unpressed */
		if (tabData->setxypressed[index])
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE); /* Set button down */
	}
}

/****************************************************************/
/* Set type of ordering at output of data.			*/
/****************************************************************/
void SetOrdering(GtkWidget *widget, gpointer data) {
	gint ordering;
	struct ButtonData *buttonData;
	struct TabData *tabData;

	buttonData = (struct ButtonData *) data;
	ordering = buttonData->index;
	tabData = buttonData->tabData;
	tabData->ordering = ordering; /* Set ordering control variable */
}

/****************************************************************/
/****************************************************************/
void SetAction(GtkWidget *widget, gpointer data) {
	gint action;
	struct ButtonData *buttonData;
	struct TabData *tabData;

	buttonData = (struct ButtonData *) data;
	action = buttonData->index;
	tabData = buttonData->tabData;
	tabData->Action = action;
	SetButtonSensitivity(tabData);
}

/****************************************************************/
/* Set whether to use error evaluation and printing or not.	*/
/****************************************************************/
void UseErrCB(GtkToggleButton *widget, gpointer data) {
	struct TabData *tabData;

	tabData = (struct TabData *) data;
	tabData->UseErrors = gtk_toggle_button_get_active(widget);
}

/****************************************************************/
/* When the value of the entry of any axis point is changed, 	*/
/* this function gets called.					*/
/****************************************************************/
void read_xy_entry(GtkWidget *entry, gpointer data) {
	gchar *xy_text;
	gint index;
	struct ButtonData *buttonData;
	struct TabData *tabData;

	buttonData = (struct ButtonData *) data;
	index = buttonData->index;
	tabData = buttonData->tabData;

	xy_text = (gchar *) gtk_entry_get_text(GTK_ENTRY (entry));
	sscanf(xy_text, "%lf", &(tabData->realcoords[index]));
	if (tabData->logxy[index / 2] && tabData->realcoords[index] > 0)
		tabData->valueset[index] = TRUE;
	else if (tabData->logxy[index / 2])
		tabData->valueset[index] = FALSE;
	else
		tabData->valueset[index] = TRUE;

	SetButtonSensitivity(tabData);
}

/****************************************************************/
/* If all the axispoints has been put out, values for these	*/
/* have been assigned and at least one point has been set on	*/
/* the graph activate the write to file button.			*/
/****************************************************************/
void read_file_entry(GtkWidget *entry, gpointer data) {
	struct TabData *tabData;

	tabData = (struct TabData *) data;

	tabData->file_name = (gchar *) gtk_entry_get_text(GTK_ENTRY (entry));
	tabData->file_name_length = strlen(tabData->file_name); /* Get length of string */

	if (tabData->bpressed[0] && tabData->bpressed[1] && tabData->bpressed[2]
			&& tabData->bpressed[3] && tabData->valueset[0]
			&& tabData->valueset[1] && tabData->valueset[2]
			&& tabData->valueset[3] && tabData->numpoints > 0
			&& tabData->file_name_length > 0) {
		gtk_widget_set_sensitive(tabData->exportbutton, TRUE);
	} else
		gtk_widget_set_sensitive(tabData->exportbutton, FALSE);

}

/****************************************************************/
/* If the "X/Y axis is logarithmic" check button is toggled	*/
/* this function gets called. It sets the logx variable to its	*/
/* correct value corresponding to the buttons state.		*/
/****************************************************************/
void islogxy(GtkToggleButton *widget, gpointer data) {
	gint index;
	struct ButtonData *buttonData;
	struct TabData *tabData;

	buttonData = (struct ButtonData *) data;
	index = buttonData->index;
	tabData = buttonData->tabData;

	tabData->logxy[index] = (gtk_toggle_button_get_active(widget)); /* If checkbutton is pressed down */
	/* logxy = TRUE else FALSE. */
	if (tabData->logxy[index]) {
		if (tabData->realcoords[index * 2] <= 0) { /* If a negative value has been insert */
			tabData->valueset[index * 2] = FALSE;
			gtk_entry_set_text(GTK_ENTRY(tabData->xyentry[index*2]), ""); /* Zero it */
		}
		if (tabData->realcoords[index * 2 + 1] <= 0) { /* If a negative value has been insert */
			tabData->valueset[index * 2 + 1] = FALSE;
			gtk_entry_set_text(GTK_ENTRY(tabData->xyentry[index*2+1]), ""); /* Zero it */
		}
	}
}

/****************************************************************/
/* This function removes the last inserted point or the point	*/
/* indexed by remthis (<0).					*/
/****************************************************************/
void remove_last(GtkWidget *widget, gpointer data) {
	gint i;
	struct TabData *tabData;

	tabData = (struct TabData *) data;

	/* First redraw the drawing_area with the original image, to clean it. */

	if (tabData->numlastpoints > 0) { /* If points been put out, remove last one */
		//		if (remthis == 0) { /* If remthis is 0, ignore it.		*/
		tabData->numlastpoints--;
		for (i = 0; i < 4; i++)
			if (tabData->lastpoints[tabData->numlastpoints] == -(i + 1)) { /* If point to be removed is axispoint 1-4 */
				tabData->bpressed[i] = FALSE; /* Mark it unpressed.			*/
				gtk_widget_set_sensitive(tabData->xyentry[i], FALSE); /* Inactivate entry for point.		*/
				break;
			}
		if (i == 4)
			tabData->numpoints--; /* If its none of the X/Y markers then	*/
		SetNumPointsEntry(tabData->nump_entry, tabData->numpoints); /* its an ordinary marker, remove it.	 */
		//		}
	}

	gtk_widget_queue_draw(tabData->drawing_area);

	SetButtonSensitivity(tabData);
	//	remthis = 0; /* Reset remthis variable */
}

/****************************************************************/
/* This function sets the proper variables and then calls 	*/
/* remove_last, to remove all points except the axis points.	*/
/****************************************************************/
void remove_all(GtkWidget *widget, gpointer data) {
	gint i, j, index;
	struct TabData *tabData;

	tabData = (struct TabData *) data;

	if (tabData->numlastpoints > 0 && tabData->numpoints > 0) {
		index = 0;
		for (i = 0; i < tabData->numlastpoints; i++)
			for (j = 0; j < 4; j++) { /* Search for axispoints and store them in */
				if (tabData->lastpoints[i] == -(j + 1)) { /* lastpoints at the first positions.      */
					tabData->lastpoints[index] = -(j + 1);
					index++;
				}
			}
		tabData->lastpoints[index] = 0;

		tabData->numlastpoints = index + 1;
		tabData->numpoints = 1;
		SetNumPointsEntry(tabData->nump_entry, tabData->numpoints);

		remove_last(widget, data); /* Call remove_last() for housekeeping */
	} else if (tabData->numlastpoints > 0 && tabData->numpoints == 0) {
		tabData->numlastpoints = 0; /* Nullify amount of points */
		for (i = 0; i < 4; i++) {
			tabData->valueset[i] = FALSE;
			tabData->bpressed[i] = FALSE;
			gtk_entry_set_text((GtkEntry *) tabData->xyentry[i], "");
		}
		remove_last(widget, data); /* Call remove_last() for housekeeping */
	}
}

/****************************************************************/
/* This function handles all of the keypresses done within the	*/
/* main window and handles the  appropriate measures.		*/
/****************************************************************/
gint key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	GtkAdjustment *adjustment;
	gdouble adj_val;
	GdkCursor *cursor;
	struct TabData *tabData;

	tabData = (struct TabData *) data;

	if (event->keyval == GDK_KEY_Left) {
		adjustment = gtk_viewport_get_hadjustment(
				(GtkViewport *) tabData->ViewPort);
		adj_val = gtk_adjustment_get_value(adjustment);
		adj_val -= gtk_adjustment_get_page_size(adjustment) / 10.0;
		if (adj_val < gtk_adjustment_get_lower(adjustment))
			adj_val = gtk_adjustment_get_lower(adjustment);
		gtk_adjustment_set_value(adjustment, adj_val);
		gtk_viewport_set_hadjustment((GtkViewport *) tabData->ViewPort,
				adjustment);
	} else if (event->keyval == GDK_KEY_Right) {
		adjustment = gtk_viewport_get_hadjustment(
				(GtkViewport *) tabData->ViewPort);
		adj_val = gtk_adjustment_get_value(adjustment);
		adj_val += gtk_adjustment_get_page_size(adjustment) / 10.0;
		if (adj_val > (gtk_adjustment_get_upper(adjustment)
				- gtk_adjustment_get_page_size(adjustment)))
			adj_val = (gtk_adjustment_get_upper(adjustment)
					- gtk_adjustment_get_page_size(adjustment));
		gtk_adjustment_set_value(adjustment, adj_val);
		gtk_viewport_set_hadjustment((GtkViewport *) tabData->ViewPort,
				adjustment);
	} else if (event->keyval == GDK_KEY_Up) {
		adjustment = gtk_viewport_get_vadjustment(
				(GtkViewport *) tabData->ViewPort);
		adj_val = gtk_adjustment_get_value(adjustment);
		adj_val -= gtk_adjustment_get_page_size(adjustment) / 10.0;
		if (adj_val < gtk_adjustment_get_lower(adjustment))
			adj_val = gtk_adjustment_get_lower(adjustment);
		gtk_adjustment_set_value(adjustment, adj_val);
		gtk_viewport_set_vadjustment((GtkViewport *) tabData->ViewPort,
				adjustment);
	} else if (event->keyval == GDK_KEY_Down) {
		adjustment = gtk_viewport_get_vadjustment(
				(GtkViewport *) tabData->ViewPort);
		adj_val = gtk_adjustment_get_value(adjustment);
		adj_val += gtk_adjustment_get_page_size(adjustment) / 10.0;
		if (adj_val > (gtk_adjustment_get_upper(adjustment)
				- gtk_adjustment_get_page_size(adjustment)))
			adj_val = (gtk_adjustment_get_upper(adjustment)
					- gtk_adjustment_get_page_size(adjustment));
		gtk_adjustment_set_value(adjustment, adj_val);
		gtk_viewport_set_vadjustment((GtkViewport *) tabData->ViewPort,
				adjustment);
	} else if (event->keyval == GDK_KEY_Control_L) {
		cursor = gdk_cursor_new(GDK_HAND2);
		gdk_window_set_cursor(
				gtk_widget_get_parent_window(tabData->drawing_area), cursor);
		MovePointMode = TRUE;
	}

	return 0;
}

/****************************************************************/
/****************************************************************/
gint key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer data) {
	GdkCursor *cursor;
	struct TabData *tabData;

	tabData = (struct TabData *) data;

	if (event->keyval == GDK_KEY_Control_L) {
		//		if (ViewedTabNum != -1) {
		cursor = gdk_cursor_new(GDK_CROSSHAIR);
		gdk_window_set_cursor(
				gtk_widget_get_parent_window(tabData->drawing_area), cursor);
		MovePointMode = FALSE;
		//		}
	}

	return 0;
}

/****************************************************************/
/* This function loads the image, and inserts it into the tab	*/
/* and sets up all of the different signals associated with it.	*/
/****************************************************************/
gint InsertImage(GtkWidget *drawing_area_alignment, char *filename,
		gdouble Scale, gdouble maxX, gdouble maxY, struct TabData *tabData) {

	gdouble mScale;
	GdkCursor *cursor;
	GtkWidget *dialog;
	gchar buf[256]; /* Text buffer for window title */

	tabData->image = cairo_image_surface_create_from_png(filename);
	if (cairo_surface_status(tabData->image) != CAIRO_STATUS_SUCCESS) {
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), /* Notify user of the error */
		GTK_DIALOG_DESTROY_WITH_PARENT, /* with a dialog */
		GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error loading file '%s'",
				filename);
		gtk_dialog_run(GTK_DIALOG (dialog));
		gtk_widget_destroy(dialog);

		return -1; /* exit */
	}

	tabData->XSize = cairo_image_surface_get_width(tabData->image);
	tabData->YSize = cairo_image_surface_get_height(tabData->image);

	sprintf(buf, Window_Title, filename); /* Print window title in buffer */
	gtk_window_set_title(GTK_WINDOW (window), buf); /* Set window title */

	mScale = -1;
	if (maxX != -1 && maxY != -1) {
		if (tabData->XSize > maxX) {
			mScale = (double) maxX / tabData->XSize;
		}
		if (tabData->YSize > maxY && (double) maxY / tabData->YSize < mScale)
			mScale = (double) maxY / tabData->YSize;
	}

	if (Scale == -1 && mScale != -1)
		Scale = mScale;

	if (Scale != -1) {
		tabData->XSize *= Scale;
		tabData->YSize *= Scale;

		// flush to ensure all writing to the image was done
		cairo_surface_flush(tabData->image);

		cairo_t *cr;
		cr = cairo_create(tabData->image);

		cairo_surface_t *first;
		first = cairo_surface_create_similar(cairo_get_target(cr),
				CAIRO_CONTENT_COLOR, tabData->XSize, tabData->YSize);

		cairo_t *first_cr;
		first_cr = cairo_create(first);
		cairo_scale(first_cr, Scale, Scale);
		cairo_set_source_surface(first_cr, tabData->image, 0, 0);
		cairo_paint(first_cr);
		tabData->image = first;

		cairo_destroy(first_cr);
	}

	tabData->drawing_area = gtk_drawing_area_new(); /* Create new drawing area */
	gtk_widget_set_size_request(tabData->drawing_area, tabData->XSize,
			tabData->YSize);

	g_signal_connect (G_OBJECT (tabData->drawing_area), "draw",
			G_CALLBACK (draw_callback), tabData);

	g_signal_connect (G_OBJECT (tabData->drawing_area), "button_press_event", /* Connect drawing area to */
			G_CALLBACK (button_press_event), tabData); /* button_press_event. */

	g_signal_connect (G_OBJECT (tabData->drawing_area), "button_release_event", /* Connect drawing area to */
			G_CALLBACK (button_release_event), tabData); /* button_release_event */

	g_signal_connect (G_OBJECT (tabData->drawing_area), "motion_notify_event", /* Connect drawing area to */
			G_CALLBACK (motion_notify_event), tabData); /* motion_notify_event. */

	gtk_widget_set_events(
			tabData->drawing_area,
			GDK_EXPOSURE_MASK | /* Set the events active */
			GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
					| GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

	gtk_container_add((GtkContainer *) drawing_area_alignment,
			tabData->drawing_area);

	gtk_widget_show(tabData->drawing_area);

	cursor = gdk_cursor_new(GDK_CROSSHAIR);
	gdk_window_set_cursor(gtk_widget_get_parent_window(tabData->drawing_area),
			cursor);

	return 0;
}

/****************************************************************/
/* This callback is called when the file - exit menuoptioned is */
/* selected.							*/
/****************************************************************/
GCallback menu_file_exit(void) {
	close_application(NULL, NULL, NULL);

	return NULL;
}

/****************************************************************/
/* This callback sets up the thumbnail in the Fileopen dialog.	*/
/****************************************************************/
static void update_preview_cb(GtkFileChooser *file_chooser, gpointer data) {
	GtkWidget *preview;
	char *filename;
	GdkPixbuf *pixbuf;
	gboolean have_preview;

	preview = GTK_WIDGET (data);
	filename = gtk_file_chooser_get_preview_filename(file_chooser);

	pixbuf = gdk_pixbuf_new_from_file_at_size(filename, 128, 128, NULL);
	have_preview = (pixbuf != NULL);
	g_free(filename);

	gtk_image_set_from_pixbuf(GTK_IMAGE (preview), pixbuf);
	if (pixbuf)
		g_object_unref(pixbuf);

	gtk_file_chooser_set_preview_widget_active(file_chooser, have_preview);
}

int allocateTabMemory(struct TabData ***tabsData, gint newTabNumber) {
	static gint maxNumTabs = 0;
	static gint numTabsAllocated = 0;

	if (*tabsData == NULL) {
		*tabsData = malloc(sizeof(struct TabData *) * MAXNUMTABS);
		if (*tabsData == NULL) {
			return FALSE;
		}
		maxNumTabs = MAXNUMTABS;
	}
	if (newTabNumber >= maxNumTabs) {
		maxNumTabs += MAXNUMTABS;
		*tabsData = realloc(*tabsData, sizeof(struct TabData *) * MAXNUMTABS);
		if (*tabsData == NULL) {
			return FALSE;
		}
	}
	if (newTabNumber >= numTabsAllocated) {
		(*tabsData)[newTabNumber] = malloc(sizeof(struct TabData));
		if ((*tabsData)[newTabNumber] == NULL) {
			return FALSE;
		}
		numTabsAllocated++;
	}
	return TRUE;
}

/****************************************************************/
/* This function sets up a new tab, sets up all of the widgets 	*/
/* needed.							*/
/****************************************************************/
gint SetupNewTab(char *filename, gdouble Scale, gdouble maxX, gdouble maxY,
		gboolean UsePreSetCoords, gdouble *TempCoords, gboolean *Uselogxy,
		gboolean *UseError) {
	GtkWidget *table; /* GTK table/box variables for packing */
	GtkWidget *tophbox, *bottomhbox;
	GtkWidget *trvbox, *tlvbox, *brvbox, *blvbox, *subvbox;
	GtkWidget *xy_label[4]; /* Labels for texts in window */
	GtkWidget *logcheckb[2]; /* Logarithmic checkbuttons */
	GtkWidget *nump_label, *ScrollWindow; /* Various widgets */
	GtkWidget *APlabel, *PIlabel, *ZAlabel, *Llabel, *tab_label;
	GtkWidget *alignment, *fixed;
	GtkWidget *x_label, *y_label, *tmplabel;
	GtkWidget *ordercheckb[3], *UseErrCheckB, *actioncheckb[2];
	GtkWidget *Olabel, *Elabel, *Alabel;
	GSList *group;
	GtkWidget *dialog;
	GtkWidget *pm_label, *pm_label2;
	GtkWidget *drawing_area_alignment;

	gchar buf[256], buf2[256];
	gint i, TabNum;
	gboolean FileInCwd;
	static gint NumberOfTabs = 0;

	// Declaration of tab specific data structure
	static struct TabData **allTabsData = NULL;

	if (!allocateTabMemory(&allTabsData, NumberOfTabs)) {
		dialog = gtk_message_dialog_new(GTK_WINDOW(window), /* Notify user of the error */
		GTK_DIALOG_DESTROY_WITH_PARENT, /* with a dialog */
		GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				"Cannot open more tabs, memory allocation failed (%d)",
				MAXNUMTABS);
		gtk_dialog_run(GTK_DIALOG (dialog));
		gtk_widget_destroy(dialog);
		return -1;
	}
	NumberOfTabs++;

	strncpy(buf2, filename, 256);
	if (strcmp(dirname(buf2), getcwd(buf, 256)) == 0) {
		tab_label = gtk_label_new(basename(filename));
		FileInCwd = TRUE;
	} else {
		tab_label = gtk_label_new(filename);
		FileInCwd = FALSE;
	}

	table = gtk_table_new(1, 2, FALSE); /* Create table */
	gtk_container_set_border_width(GTK_CONTAINER (table), WINDOW_BORDER);
	gtk_table_set_row_spacings(GTK_TABLE(table), SECT_SEP); /* Set spacings */
	gtk_table_set_col_spacings(GTK_TABLE(table), 0);
	TabNum = gtk_notebook_append_page((GtkNotebook *) mainnotebook, table,
			tab_label);
	if (TabNum == -1) {
		return -1;
	}

	if (TempCoords != NULL) {
		allTabsData[TabNum]->realcoords[0] = TempCoords[0];
		allTabsData[TabNum]->realcoords[2] = TempCoords[1];
		allTabsData[TabNum]->realcoords[1] = TempCoords[2];
		allTabsData[TabNum]->realcoords[3] = TempCoords[3];
	}
	if (Uselogxy != NULL) {
		allTabsData[TabNum]->logxy[0] = Uselogxy[0];
		allTabsData[TabNum]->logxy[1] = Uselogxy[1];
	}
	if (UseError != NULL) {
		allTabsData[TabNum]->UseErrors = *UseError;
	} else {
		allTabsData[TabNum]->UseErrors = FALSE;
	}

	/* Init datastructures */

	allTabsData[TabNum]->bpressed[0] = FALSE;
	allTabsData[TabNum]->bpressed[1] = FALSE;
	allTabsData[TabNum]->bpressed[2] = FALSE;
	allTabsData[TabNum]->bpressed[3] = FALSE;

	allTabsData[TabNum]->valueset[0] = FALSE;
	allTabsData[TabNum]->valueset[1] = FALSE;
	allTabsData[TabNum]->valueset[2] = FALSE;
	allTabsData[TabNum]->valueset[3] = FALSE;

	allTabsData[TabNum]->numpoints = 0;
	allTabsData[TabNum]->numlastpoints = 0;
	allTabsData[TabNum]->ordering = 0;

	allTabsData[TabNum]->mousePointerCoords[0] = -1;
	allTabsData[TabNum]->mousePointerCoords[1] = -1;

	allTabsData[TabNum]->logxy[0] = FALSE;
	allTabsData[TabNum]->logxy[1] = FALSE;

	allTabsData[TabNum]->MaxPoints = MAXPOINTS;

	allTabsData[TabNum]->setxypressed[0] = FALSE;
	allTabsData[TabNum]->setxypressed[1] = FALSE;
	allTabsData[TabNum]->setxypressed[2] = FALSE;
	allTabsData[TabNum]->setxypressed[3] = FALSE;

	allTabsData[TabNum]->lastpoints = (gint *) malloc(
			sizeof(gint) * (allTabsData[TabNum]->MaxPoints + 4));
	if (allTabsData[TabNum]->lastpoints == NULL) {
		printf("Error allocating memory for lastpoints. Exiting.\n");
		return -1;
	}
	allTabsData[TabNum]->points = (void *) malloc(
			sizeof(gint *) * allTabsData[TabNum]->MaxPoints);
	if (allTabsData[TabNum]->points == NULL) {
		printf("Error allocating memory for points. Exiting.\n");
		return -1;
	}
	for (i = 0; i < allTabsData[TabNum]->MaxPoints; i++) {
		allTabsData[TabNum]->points[i] = (gint *) malloc(sizeof(gint) * 2);
		if (allTabsData[TabNum]->points[i] == NULL) {
			printf("Error allocating memory for points[%d]. Exiting.\n", i);
			return -1;
		}
	}

	g_signal_connect (G_OBJECT (table), "key_press_event",
			G_CALLBACK (key_press_event), allTabsData[TabNum]);
	g_signal_connect (G_OBJECT (table), "key_release_event",
			G_CALLBACK (key_release_event), allTabsData[TabNum]);

	for (i = 0; i < 4; i++) {
		allTabsData[TabNum]->xyentry[i] = gtk_entry_new(); /* Create text entry */
		gtk_entry_set_max_length(GTK_ENTRY (allTabsData[TabNum]->xyentry[i]),
				20);
		gtk_editable_set_editable(
				(GtkEditable *) allTabsData[TabNum]->xyentry[i], FALSE);
		gtk_widget_set_sensitive(allTabsData[TabNum]->xyentry[i], FALSE); /* Inactivate it */
		struct ButtonData *buttonData;
		buttonData = malloc(sizeof(struct ButtonData));
		buttonData->tabData = allTabsData[TabNum];
		buttonData->index = i;
		g_signal_connect (G_OBJECT (allTabsData[TabNum]->xyentry[i]), "changed", /* Init the entry to call */
				G_CALLBACK (read_xy_entry), buttonData); /* read_x1_entry whenever */
		gtk_widget_set_tooltip_text(allTabsData[TabNum]->xyentry[i],
				entryxytt[i]);
	}

	x_label = gtk_label_new(x_string);
	y_label = gtk_label_new(y_string);
	allTabsData[TabNum]->xc_entry = gtk_entry_new(); /* Create text entry */
	gtk_entry_set_max_length(GTK_ENTRY (allTabsData[TabNum]->xc_entry), 16);
	gtk_editable_set_editable((GtkEditable *) allTabsData[TabNum]->xc_entry,
			FALSE);
	allTabsData[TabNum]->yc_entry = gtk_entry_new(); /* Create text entry */
	gtk_entry_set_max_length(GTK_ENTRY (allTabsData[TabNum]->yc_entry), 16);
	gtk_editable_set_editable((GtkEditable *) allTabsData[TabNum]->yc_entry,
			FALSE);

	pm_label = gtk_label_new(pm_string);
	pm_label2 = gtk_label_new(pm_string);
	allTabsData[TabNum]->xerr_entry = gtk_entry_new(); /* Create text entry */
	gtk_entry_set_max_length(GTK_ENTRY (allTabsData[TabNum]->xerr_entry), 16);
	gtk_editable_set_editable((GtkEditable *) allTabsData[TabNum]->xerr_entry,
			FALSE);
	allTabsData[TabNum]->yerr_entry = gtk_entry_new(); /* Create text entry */
	gtk_entry_set_max_length(GTK_ENTRY (allTabsData[TabNum]->yerr_entry), 16);
	gtk_editable_set_editable((GtkEditable *) allTabsData[TabNum]->yerr_entry,
			FALSE);

	nump_label = gtk_label_new(nump_string);
	allTabsData[TabNum]->nump_entry = gtk_entry_new(); /* Create text entry */
	gtk_entry_set_max_length(GTK_ENTRY (allTabsData[TabNum]->nump_entry), 10);
	gtk_editable_set_editable((GtkEditable *) allTabsData[TabNum]->nump_entry,
			FALSE);
	SetNumPointsEntry(allTabsData[TabNum]->nump_entry,
			allTabsData[TabNum]->numpoints);

	allTabsData[TabNum]->zoom_area = gtk_drawing_area_new(); /* Create new drawing area */
	gtk_widget_set_size_request(allTabsData[TabNum]->zoom_area, ZOOMPIXSIZE,
			ZOOMPIXSIZE);
	g_signal_connect (G_OBJECT (allTabsData[TabNum]->zoom_area), "draw",
			G_CALLBACK (draw_zoom_callback), allTabsData[TabNum]);

	for (i = 0; i < 4; i++) {
		xy_label[i] = gtk_label_new(NULL);
		gtk_label_set_markup((GtkLabel *) xy_label[i], xy_label_text[i]);
	}

	for (i = 0; i < 4; i++) {
		tmplabel = gtk_label_new(NULL);
		gtk_label_set_markup_with_mnemonic((GtkLabel *) tmplabel, setxylabel[i]);
		allTabsData[TabNum]->setxybutton[i] = gtk_toggle_button_new(); /* Create button */
		gtk_container_add((GtkContainer *) allTabsData[TabNum]->setxybutton[i],
				tmplabel);
		struct ButtonData *buttonData;
		buttonData = malloc(sizeof(struct ButtonData));
		buttonData->tabData = allTabsData[TabNum];
		buttonData->index = i;
		g_signal_connect (G_OBJECT (allTabsData[TabNum]->setxybutton[i]), "toggled", /* Connect button */
				G_CALLBACK (toggle_xy), buttonData);
		gtk_widget_set_tooltip_text(allTabsData[TabNum]->setxybutton[i],
				setxytts[i]);
	}

	allTabsData[TabNum]->remlastbutton = gtk_button_new_with_mnemonic(
			RemLastBLabel); /* Create button */
	g_signal_connect (G_OBJECT (allTabsData[TabNum]->remlastbutton), "clicked", /* Connect button */
			G_CALLBACK (remove_last), allTabsData[TabNum]);
	gtk_widget_set_sensitive(allTabsData[TabNum]->remlastbutton, FALSE);
	gtk_widget_set_tooltip_text(allTabsData[TabNum]->remlastbutton, removeltt);

	allTabsData[TabNum]->remallbutton = gtk_button_new_with_mnemonic(
			RemAllBLabel); /* Create button */
	g_signal_connect (G_OBJECT (allTabsData[TabNum]->remallbutton), "clicked", /* Connect button */
			G_CALLBACK (remove_all), allTabsData[TabNum]);
	gtk_widget_set_sensitive(allTabsData[TabNum]->remallbutton, FALSE);
	gtk_widget_set_tooltip_text(allTabsData[TabNum]->remallbutton, removeatts);

	for (i = 0; i < 2; i++) {
		logcheckb[i] = gtk_check_button_new_with_mnemonic(loglabel[i]); /* Create check button */
		struct ButtonData *buttonData;
		buttonData = malloc(sizeof(struct ButtonData));
		buttonData->tabData = allTabsData[TabNum];
		buttonData->index = i;
		g_signal_connect (G_OBJECT (logcheckb[i]), "toggled", /* Connect button */
				G_CALLBACK (islogxy), buttonData);
		gtk_widget_set_tooltip_text(logcheckb[i], logxytt[i]);
		gtk_toggle_button_set_active((GtkToggleButton *) logcheckb[i],
				allTabsData[TabNum]->logxy[i]);
	}

	tophbox = gtk_hbox_new(FALSE, SECT_SEP);
	alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), alignment, 0, 1, 0, 1, 5, 0, 0, 0);
	gtk_container_add((GtkContainer *) alignment, tophbox);

	bottomhbox = gtk_hbox_new(FALSE, SECT_SEP);
	alignment = gtk_alignment_new(0, 0, 1, 1);
	gtk_table_attach(GTK_TABLE(table), alignment, 0, 1, 1, 2, 5, 5, 0, 0);
	gtk_container_add((GtkContainer *) alignment, bottomhbox);

	tlvbox = gtk_vbox_new(FALSE, ELEM_SEP);
	gtk_box_pack_start(GTK_BOX (tophbox), tlvbox, FALSE, FALSE, ELEM_SEP);
	APlabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL (APlabel), APheader);
	alignment = gtk_alignment_new(0, 1, 0, 0);
	gtk_container_add((GtkContainer *) alignment, APlabel);
	gtk_box_pack_start(GTK_BOX (tlvbox), alignment, FALSE, FALSE, 0);
	table = gtk_table_new(3, 4, FALSE);
	fixed = gtk_fixed_new();
	gtk_fixed_put((GtkFixed *) fixed, table, FRAME_INDENT, 0);
	gtk_table_set_row_spacings(GTK_TABLE(table), ELEM_SEP);
	gtk_table_set_col_spacings(GTK_TABLE(table), ELEM_SEP);
	gtk_box_pack_start(GTK_BOX (tlvbox), fixed, FALSE, FALSE, 0);
	for (i = 0; i < 4; i++) {
		gtk_table_attach(GTK_TABLE(table), allTabsData[TabNum]->setxybutton[i],
				0, 1, i, i + 1, 5, 0, 0, 0);
		gtk_table_attach(GTK_TABLE(table), xy_label[i], 1, 2, i, i + 1, 0, 0,
				0, 0);
		gtk_table_attach(GTK_TABLE(table), allTabsData[TabNum]->xyentry[i], 2,
				3, i, i + 1, 0, 0, 0, 0);
	}

	trvbox = gtk_vbox_new(FALSE, ELEM_SEP);
	gtk_box_pack_start(GTK_BOX (tophbox), trvbox, FALSE, FALSE, ELEM_SEP);

	PIlabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL (PIlabel), PIheader);
	alignment = gtk_alignment_new(0, 1, 0, 0);
	gtk_container_add((GtkContainer *) alignment, PIlabel);
	gtk_box_pack_start(GTK_BOX (trvbox), alignment, FALSE, FALSE, 0);

	table = gtk_table_new(4, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), ELEM_SEP);
	gtk_table_set_col_spacings(GTK_TABLE(table), ELEM_SEP);
	fixed = gtk_fixed_new();
	gtk_fixed_put((GtkFixed *) fixed, table, FRAME_INDENT, 0);
	gtk_box_pack_start(GTK_BOX (trvbox), fixed, FALSE, FALSE, 0);
	gtk_table_attach(GTK_TABLE(table), x_label, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), allTabsData[TabNum]->xc_entry, 1, 2, 0,
			1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), pm_label, 2, 3, 0, 1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), allTabsData[TabNum]->xerr_entry, 3, 4,
			0, 1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), y_label, 0, 1, 1, 2, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), allTabsData[TabNum]->yc_entry, 1, 2, 1,
			2, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), pm_label2, 2, 3, 1, 2, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), allTabsData[TabNum]->yerr_entry, 3, 4,
			1, 2, 0, 0, 0, 0);

	table = gtk_table_new(3, 1, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	fixed = gtk_fixed_new();
	gtk_fixed_put((GtkFixed *) fixed, table, FRAME_INDENT, 0);
	gtk_box_pack_start(GTK_BOX (trvbox), fixed, FALSE, FALSE, 0);
	alignment = gtk_alignment_new(0, 1, 0, 0);
	gtk_container_add((GtkContainer *) alignment, nump_label);
	gtk_table_attach(GTK_TABLE(table), alignment, 0, 1, 0, 1, 0, 0, 0, 0);
	gtk_table_attach(GTK_TABLE(table), allTabsData[TabNum]->nump_entry, 1, 2,
			0, 1, 0, 0, 0, 0);

	blvbox = gtk_vbox_new(FALSE, GROUP_SEP);
	gtk_box_pack_start(GTK_BOX (bottomhbox), blvbox, FALSE, FALSE, ELEM_SEP);

	subvbox = gtk_vbox_new(FALSE, ELEM_SEP);
	gtk_box_pack_start(GTK_BOX (blvbox), subvbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (subvbox), allTabsData[TabNum]->remlastbutton,
			FALSE, FALSE, 0); /* Pack button in vert. box */
	gtk_box_pack_start(GTK_BOX (subvbox), allTabsData[TabNum]->remallbutton,
			FALSE, FALSE, 0); /* Pack button in vert. box */

	subvbox = gtk_vbox_new(FALSE, ELEM_SEP);
	allTabsData[TabNum]->zoomareabox = subvbox;
	gtk_box_pack_start(GTK_BOX (blvbox), subvbox, FALSE, FALSE, 0);
	ZAlabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL (ZAlabel), ZAheader);
	alignment = gtk_alignment_new(0, 1, 0, 0);
	gtk_container_add((GtkContainer *) alignment, ZAlabel);
	gtk_box_pack_start(GTK_BOX (subvbox), alignment, FALSE, FALSE, 0);
	fixed = gtk_fixed_new();
	gtk_fixed_put((GtkFixed *) fixed, allTabsData[TabNum]->zoom_area,
			FRAME_INDENT, 0);
	gtk_box_pack_start(GTK_BOX (subvbox), fixed, FALSE, FALSE, 0);

	subvbox = gtk_vbox_new(FALSE, ELEM_SEP);
	allTabsData[TabNum]->logbox = subvbox;
	gtk_box_pack_start(GTK_BOX (blvbox), subvbox, FALSE, FALSE, 0);
	Llabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL (Llabel), Lheader);
	alignment = gtk_alignment_new(0, 1, 0, 0);
	gtk_container_add((GtkContainer *) alignment, Llabel);
	gtk_box_pack_start(GTK_BOX (subvbox), alignment, FALSE, FALSE, 0);
	for (i = 0; i < 2; i++) {
		fixed = gtk_fixed_new();
		gtk_fixed_put((GtkFixed *) fixed, logcheckb[i], FRAME_INDENT, 0);
		gtk_box_pack_start(GTK_BOX (subvbox), fixed, FALSE, FALSE, 0); /* Pack checkbutton in vert. box */
	}

	group = NULL;
	for (i = 0; i < ORDERBNUM; i++) {
		ordercheckb[i] = gtk_radio_button_new_with_label(group, orderlabel[i]); /* Create radio button */
		struct ButtonData *buttonData;
		buttonData = malloc(sizeof(struct ButtonData));
		buttonData->tabData = allTabsData[TabNum];
		buttonData->index = i;
		g_signal_connect (G_OBJECT (ordercheckb[i]), "toggled", /* Connect button */
				G_CALLBACK (SetOrdering), buttonData);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (ordercheckb[i])); /* Get buttons group */
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (ordercheckb[0]), TRUE); /* Set no ordering button active */

	subvbox = gtk_vbox_new(FALSE, ELEM_SEP);
	allTabsData[TabNum]->oppropbox = subvbox;
	gtk_box_pack_start(GTK_BOX (blvbox), subvbox, FALSE, FALSE, 0);
	Olabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL (Olabel), Oheader);
	alignment = gtk_alignment_new(0, 1, 0, 0);
	gtk_container_add((GtkContainer *) alignment, Olabel);
	gtk_box_pack_start(GTK_BOX (subvbox), alignment, FALSE, FALSE, 0);
	for (i = 0; i < ORDERBNUM; i++) {
		fixed = gtk_fixed_new();
		gtk_fixed_put((GtkFixed *) fixed, ordercheckb[i], FRAME_INDENT, 0);
		gtk_box_pack_start(GTK_BOX (subvbox), fixed, FALSE, FALSE, 0); /* Pack radiobutton in vert. box */
	}

	UseErrCheckB = gtk_check_button_new_with_mnemonic(PrintErrCBLabel);
	g_signal_connect (G_OBJECT (UseErrCheckB), "toggled",
			G_CALLBACK (UseErrCB), allTabsData[TabNum]);
	gtk_widget_set_tooltip_text(UseErrCheckB, uetts);
	gtk_toggle_button_set_active((GtkToggleButton *) UseErrCheckB,
			allTabsData[TabNum]->UseErrors);

	Elabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL (Elabel), Eheader);
	alignment = gtk_alignment_new(0, 1, 0, 0);
	gtk_container_add((GtkContainer *) alignment, Elabel);
	gtk_box_pack_start(GTK_BOX (subvbox), alignment, FALSE, FALSE, 0);
	fixed = gtk_fixed_new();
	gtk_fixed_put((GtkFixed *) fixed, UseErrCheckB, FRAME_INDENT, 0);
	gtk_box_pack_start(GTK_BOX (subvbox), fixed, FALSE, FALSE, 0);

	subvbox = gtk_vbox_new(FALSE, ELEM_SEP);
	gtk_box_pack_start(GTK_BOX (blvbox), subvbox, FALSE, FALSE, 0);
	group = NULL;
	for (i = 0; i < ACTIONBNUM; i++) {
		actioncheckb[i]
				= gtk_radio_button_new_with_label(group, actionlabel[i]); /* Create radio button */
		struct ButtonData *buttonData;
		buttonData = malloc(sizeof(struct ButtonData));
		buttonData->tabData = allTabsData[TabNum];
		buttonData->index = i;
		g_signal_connect (G_OBJECT (actioncheckb[i]), "toggled", /* Connect button */
				G_CALLBACK (SetAction), buttonData);
		group = gtk_radio_button_get_group(GTK_RADIO_BUTTON (actioncheckb[i])); /* Get buttons group */
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (actioncheckb[0]), TRUE); /* Set no ordering button active */

	Alabel = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL (Alabel), Aheader);
	alignment = gtk_alignment_new(0, 1, 0, 0);
	gtk_container_add((GtkContainer *) alignment, Alabel);
	gtk_box_pack_start(GTK_BOX (subvbox), alignment, FALSE, FALSE, 0);
	for (i = 0; i < ACTIONBNUM; i++) {
		fixed = gtk_fixed_new();
		gtk_fixed_put((GtkFixed *) fixed, actioncheckb[i], FRAME_INDENT, 0);
		gtk_box_pack_start(GTK_BOX (subvbox), fixed, FALSE, FALSE, 0);
	}

	allTabsData[TabNum]->file_entry = gtk_entry_new(); /* Create text entry */
	gtk_entry_set_max_length(GTK_ENTRY (allTabsData[TabNum]->file_entry), 256);
	gtk_editable_set_editable((GtkEditable *) allTabsData[TabNum]->file_entry,
			TRUE);
	g_signal_connect (G_OBJECT (allTabsData[TabNum]->file_entry), "changed", /* Init the entry to call */
			G_CALLBACK (read_file_entry), allTabsData[TabNum]);
	gtk_widget_set_tooltip_text(allTabsData[TabNum]->file_entry, filenamett);

	if (FileInCwd) {
		snprintf(buf2, 256, "%s.dat", basename(filename));
		strncpy(allTabsData[TabNum]->FileNames, basename(filename), 256);
	} else {
		snprintf(buf2, 256, "%s.dat", filename);
		strncpy(allTabsData[TabNum]->FileNames, filename, 256);
	}

	snprintf(buf, 256, Window_Title, allTabsData[TabNum]->FileNames); /* Print window title in buffer */
	gtk_window_set_title(GTK_WINDOW (window), buf); /* Set window title */

	fixed = gtk_fixed_new();
	gtk_fixed_put((GtkFixed *) fixed, allTabsData[TabNum]->file_entry,
			FRAME_INDENT, 0);
	gtk_box_pack_start(GTK_BOX (subvbox), fixed, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(allTabsData[TabNum]->file_entry, FALSE);

	allTabsData[TabNum]->exportbutton = gtk_button_new_with_mnemonic(
			PrintBLabel); /* Create button */

	gtk_box_pack_start(GTK_BOX (subvbox), allTabsData[TabNum]->exportbutton,
			FALSE, FALSE, 0);
	gtk_widget_set_sensitive(allTabsData[TabNum]->exportbutton, FALSE);

	g_signal_connect (G_OBJECT (allTabsData[TabNum]->exportbutton), "clicked",
			G_CALLBACK (print_results), allTabsData[TabNum]);
	gtk_widget_set_tooltip_text(allTabsData[TabNum]->exportbutton, printrestt);

	brvbox = gtk_vbox_new(FALSE, GROUP_SEP);
	gtk_box_pack_start(GTK_BOX (bottomhbox), brvbox, TRUE, TRUE, 0);

	gtk_entry_set_text(GTK_ENTRY (allTabsData[TabNum]->file_entry), buf2); /* Set text of text entry to filename */

	ScrollWindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy((GtkScrolledWindow *) ScrollWindow,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	allTabsData[TabNum]->ViewPort = gtk_viewport_new(NULL, NULL);

	gtk_box_pack_start(GTK_BOX (brvbox), ScrollWindow, TRUE, TRUE, 0);
	drawing_area_alignment = gtk_alignment_new(0, 0, 0, 0);
	gtk_container_add(GTK_CONTAINER (allTabsData[TabNum]->ViewPort),
			drawing_area_alignment);
	gtk_container_add(GTK_CONTAINER (ScrollWindow),
			allTabsData[TabNum]->ViewPort);

	gtk_widget_show_all(window);

	gtk_notebook_set_current_page((GtkNotebook *) mainnotebook, TabNum);

	if (InsertImage(drawing_area_alignment, filename, Scale, maxX, maxY,
			allTabsData[TabNum]) == -1) {
		gtk_notebook_remove_page((GtkNotebook *) mainnotebook, TabNum);
		return -1;
	}

	if (UsePreSetCoords) {
		allTabsData[TabNum]->axiscoords[0][0] = 0;
		allTabsData[TabNum]->axiscoords[0][1] = allTabsData[TabNum]->YSize - 1;
		allTabsData[TabNum]->axiscoords[1][0] = allTabsData[TabNum]->XSize - 1;
		allTabsData[TabNum]->axiscoords[1][1] = allTabsData[TabNum]->YSize - 1;
		allTabsData[TabNum]->axiscoords[2][0] = 0;
		allTabsData[TabNum]->axiscoords[2][1] = allTabsData[TabNum]->YSize - 1;
		allTabsData[TabNum]->axiscoords[3][0] = 0;
		allTabsData[TabNum]->axiscoords[3][1] = 0;
		for (i = 0; i < 4; i++) {
			gtk_widget_set_sensitive(allTabsData[TabNum]->xyentry[i], TRUE);
			gtk_editable_set_editable(
					(GtkEditable *) allTabsData[TabNum]->xyentry[i], TRUE);
			sprintf(buf, "%lf", allTabsData[TabNum]->realcoords[i]);
			gtk_entry_set_text((GtkEntry *) allTabsData[TabNum]->xyentry[i],
					buf);
			allTabsData[TabNum]->lastpoints[allTabsData[TabNum]->numlastpoints]
					= -(i + 1);
			allTabsData[TabNum]->numlastpoints++;
			allTabsData[TabNum]->valueset[i] = TRUE;
			allTabsData[TabNum]->bpressed[i] = TRUE;
			allTabsData[TabNum]->setxypressed[i] = FALSE;
		}
		gtk_widget_set_sensitive(allTabsData[TabNum]->exportbutton, TRUE);
	}

	gtk_action_group_set_sensitive(tab_action_group, TRUE);
//	NoteBookNumPages++;

	if (HideZoomArea)
		for (i = 0; i < MAXNUMTABS; i++)
			if (allTabsData[i]->zoomareabox != NULL)
				gtk_widget_hide(allTabsData[i]->zoomareabox);
	if (HideLog)
		for (i = 0; i < MAXNUMTABS; i++)
			if (allTabsData[i]->logbox != NULL)
				gtk_widget_hide(allTabsData[i]->logbox);
	if (HideOpProp)
		for (i = 0; i < MAXNUMTABS; i++)
			if (allTabsData[i]->oppropbox != NULL)
				gtk_widget_hide(allTabsData[i]->oppropbox);

	return 0;
}

/****************************************************************/
/****************************************************************/
void drag_data_received(GtkWidget *widget, GdkDragContext *drag_context,
		gint x, gint y, GtkSelectionData *data, guint info, guint event_time,
		gpointer user_data) {
	gchar filename[256], *c;
	gint i;
	GtkWidget *dialog;

	switch (info) {
	case URI_LIST: {
		//		//	    printf("Received uri : %s\n", (gchar *) data->data);
		//		if ((c = strstr((gchar *) data->data, URI_IDENTIFIER)) == NULL) {
		//			dialog = gtk_message_dialog_new(GTK_WINDOW(window), /* Notify user of the error */
		//			GTK_DIALOG_DESTROY_WITH_PARENT, /* with a dialog */
		//			GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
		//					"Cannot extract filename from uri '%s'", filename);
		//			gtk_dialog_run(GTK_DIALOG (dialog));
		//			gtk_widget_destroy(dialog);
		//			break;
		//		}
		//		strncpy(filename, &(c[strlen(URI_IDENTIFIER)]), 256);
		//		for (i = 0; i < strlen(filename); i++)
		//			if (filename[i] == '\n')
		//				filename[i] = '\0';
		//		SetupNewTab(filename, 1.0, -1, -1, FALSE);
		break;
	}
	case JPEG_DATA:
	case PNG_DATA: {
		//		printf("Received drag-and-drop jpeg_data or png_data\n");
		//		GError *error = NULL;
		//		GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_mime_type(
		//				gdk_atom_name(data->type), &error);
		//		if (loader) {
		//			error = NULL;
		//			if (gdk_pixbuf_loader_write(loader, data->data, data->length,
		//					&error)) {
		//				GdkPixbuf *pbuf = gdk_pixbuf_loader_get_pixbuf(loader);
		//				if (pbuf) {
		//					int width = gdk_pixbuf_get_width(pbuf);
		//					int height = gdk_pixbuf_get_height(pbuf);
		//					printf("Received image of size %d x %d\n", width, height); // Print debugging information
		//				}
		//			}
		//		}
		break;
	}
	case APP_X_COLOR: {
		printf("Received drag-and-drop app-x-color\n");
		break;
	}
	}
	gtk_drag_finish(drag_context, TRUE, FALSE, event_time);
}

/****************************************************************/
/* This callback handles the file - open dialog.		*/
/****************************************************************/
GCallback menu_file_open(void) {
	GtkWidget *dialog, *scalespinbutton, *hboxextra, *scalelabel;
	GtkImage *preview;
	GtkAdjustment *scaleadj;
	GtkFileFilter *filefilter;

	dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW (window),
			GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

	// Set filtering of files to open to filetypes gdk_pixbuf can handle
	filefilter = gtk_file_filter_new();
	gtk_file_filter_add_pixbuf_formats(filefilter);
	gtk_file_chooser_set_filter((GtkFileChooser *) dialog,
			(GtkFileFilter *) filefilter);

	hboxextra = gtk_hbox_new(FALSE, ELEM_SEP);

	scalelabel = gtk_label_new(scale_string);

	scaleadj = (GtkAdjustment *) gtk_adjustment_new(1, 0.1, 100, 0.1, 0.1, 1);
	scalespinbutton = gtk_spin_button_new(scaleadj, 0.1, 1);

	gtk_box_pack_start(GTK_BOX (hboxextra), scalelabel, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX (hboxextra), scalespinbutton, FALSE, FALSE, 0);

	gtk_file_chooser_set_extra_widget((GtkFileChooser *) dialog, hboxextra);

	gtk_widget_show(hboxextra);
	gtk_widget_show(scalelabel);
	gtk_widget_show(scalespinbutton);

	preview = (GtkImage *) gtk_image_new();
	gtk_file_chooser_set_preview_widget((GtkFileChooser *) dialog,
			(GtkWidget *) preview);
	g_signal_connect (dialog, "update-preview",
			G_CALLBACK (update_preview_cb), preview);

	if (gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename;

		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (dialog));
		SetupNewTab(filename,
				gtk_spin_button_get_value((GtkSpinButton *) scalespinbutton),
				-1, -1, FALSE, NULL, NULL, NULL);

		g_free(filename);
	}

	gtk_widget_destroy(dialog);

	return NULL;
}

/****************************************************************/
/* This function destroys a dialog.				*/
/****************************************************************/
void dialog_destroy(GtkWidget *widget, gpointer data) {
	gtk_grab_remove(GTK_WIDGET(widget));
}

/****************************************************************/
/* This function closes a dialog.				*/
/****************************************************************/
void dialog_close(GtkWidget *widget, gpointer data) {
	gtk_widget_destroy(GTK_WIDGET(data));
}

/****************************************************************/
/* This Callback generates the help - about dialog.		*/
/****************************************************************/
GCallback menu_help_about(void) {
	gchar *authors[] = AUTHORS;

	gtk_show_about_dialog((GtkWindow *) window, "authors", authors, "comments",
			COMMENTS, "copyright", COPYRIGHT, "license", LICENSE, "name",
			PROGNAME, "version", VERSION, "website", HOMEPAGEURL,
			"website-label", HOMEPAGELABEL, NULL);

	return NULL;
}

/****************************************************************/
/* This function is called when a tab is closed. It removes the	*/
/* page from the notebook, all widgets within the page are	*/
/* destroyed.							*/
/****************************************************************/
GCallback menu_tab_close(void) {
	gtk_notebook_remove_page((GtkNotebook *) mainnotebook,
			gtk_notebook_get_current_page((GtkNotebook *) mainnotebook)); /* This appearently takes care of everything */

//	NoteBookNumPages--;

	if (gtk_notebook_get_n_pages((GtkNotebook *) mainnotebook) == 0)
		gtk_action_group_set_sensitive(tab_action_group, FALSE);

	return NULL;
}

/****************************************************************/
/* This callback handles the fullscreen toggling.		*/
/****************************************************************/
GCallback full_screen_action_callback(GtkWidget *widget, gpointer func_data) {
	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget))) {
		gtk_window_fullscreen(GTK_WINDOW (window));
//		WinFullScreen = TRUE;
	} else {
		gtk_window_unfullscreen(GTK_WINDOW (window));
//		WinFullScreen = FALSE;
	}
	return NULL;
}

/****************************************************************/
/* This callback handles the hide zoom area toggling.		*/
/****************************************************************/
GCallback hide_zoom_area_callback(GtkWidget *widget, gpointer func_data) {
	int i;

	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget))) {
		for (i = 0; i < MAXNUMTABS; i++)
			//			if (allTabsData[i]->zoomareabox != NULL)
			//				gtk_widget_hide(allTabsData[i]->zoomareabox);
			HideZoomArea = TRUE;
	} else {
		for (i = 0; i < MAXNUMTABS; i++)
			//			if (allTabsData[i]->zoomareabox != NULL)
			//				gtk_widget_show(allTabsData[i]->zoomareabox);
			HideZoomArea = FALSE;
	}
	return NULL;
}

/****************************************************************/
/* This callback handles the hide axis settings toggling.	*/
/****************************************************************/
GCallback hide_axis_settings_callback(GtkWidget *widget, gpointer func_data) {
	int i;

	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget))) {
		for (i = 0; i < MAXNUMTABS; i++)
			//			if (allTabsData[i]->logbox != NULL)
			//				gtk_widget_hide(allTabsData[i]->logbox);
			HideLog = TRUE;
	} else {
		for (i = 0; i < MAXNUMTABS; i++)
			//			if (allTabsData[i]->logbox != NULL)
			//				gtk_widget_show(allTabsData[i]->logbox);
			HideLog = FALSE;
	}
	return NULL;
}

/****************************************************************/
/* This callback handles the hide output properties toggling.	*/
/****************************************************************/
GCallback hide_output_prop_callback(GtkWidget *widget, gpointer func_data) {
	int i;

	if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget))) {
		for (i = 0; i < MAXNUMTABS; i++)
			//			if (allTabsData[i]->oppropbox != NULL)
			//				gtk_widget_hide(allTabsData[i]->oppropbox);
			HideOpProp = TRUE;
	} else {
		for (i = 0; i < MAXNUMTABS; i++)
			//			if (allTabsData[i]->oppropbox != NULL)
			//				gtk_widget_show(allTabsData[i]->oppropbox);
			HideOpProp = FALSE;
	}
	return NULL;
}

/****************************************************************/
/* This callback is called when the notebook page is changed.	*/
/* It sets up the ViewedTabNum value as well as the title of	*/
/* the window to match the image currently viewed.		*/
/****************************************************************/
GCallback NoteBookTabChange(GtkNotebook *notebook, gpointer page,
		guint page_num, gpointer user_data) {
	gchar buf[256];

	//	sprintf(buf, Window_Title, allTabsData[ViewedTabNum]->FileNames); /* Print window title in buffer */
	gtk_window_set_title(GTK_WINDOW (window), buf); /* Set window title */

	return NULL;
}

/****************************************************************/
/* This is the main function, this function gets called when	*/
/* the program is executed. It allocates the necessary work-	*/
/* spaces and initialized the main window and its widgets.	*/
/****************************************************************/
int main(int argc, char **argv) {
	gint FileIndex[MAXNUMTABS], NumFiles = 0, i, maxX, maxY;
	gdouble Scale;
	gboolean UsePreSetCoords, UseError, Uselogxy[2];
	gdouble TempCoords[4];
	gdouble *TempCoordsPtr;

	GtkWidget *mainvbox;

	GtkWidget *menubar;
	GtkActionGroup *action_group;
	GtkUIManager *ui_manager;
	GtkAccelGroup *accel_group;
	GError *error;

#include "vardefs.h"

	gtk_init(&argc, &argv); /* Init GTK */

	if (argc > 1)
		if (strcmp(argv[1], "-h") == 0 || /* If no parameters given, -h or --help */
		strcmp(argv[1], "--help") == 0) {
			printf("%s", HelpText); /* Print help */
			exit(0); /* and exit */
		}

	maxX = -1;
	maxY = -1;
	Scale = -1;
	UseError = FALSE;
	UsePreSetCoords = FALSE;
	Uselogxy[0] = FALSE;
	Uselogxy[1] = FALSE;
	for (i = 1; i < argc; i++) {
		if (*(argv[i]) == '-') {
			if (strcmp(argv[i], "-scale") == 0) {
				if (argc - i < 2) {
					printf("Too few parameters for -scale\n");
					exit(0);
				}
				if (sscanf(argv[i + 1], "%lf", &Scale) != 1) {
					printf("-scale parameter in invalid form !\n");
					exit(0);
				}
				i++;
				if (i >= argc)
					break;
			} else if (strcmp(argv[i], "-errors") == 0) {
				UseError = TRUE;
			} else if (strcmp(argv[i], "-lnx") == 0) {
				Uselogxy[0] = TRUE;
			} else if (strcmp(argv[i], "-lny") == 0) {
				Uselogxy[1] = TRUE;
			} else if (strcmp(argv[i], "-max") == 0) {
				if (argc - i < 3) {
					printf("Too few parameters for -max\n");
					exit(0);
				}
				if (sscanf(argv[i + 1], "%d", &maxX) != 1) {
					printf("-max first parameter in invalid form !\n");
					exit(0);
				}
				if (sscanf(argv[i + 2], "%d", &maxY) != 1) {
					printf("-max second parameter in invalid form !\n");
					exit(0);
				}
				i += 2;
				if (i >= argc)
					break;
			} else if (strcmp(argv[i], "-coords") == 0) {
				UsePreSetCoords = TRUE;
				if (argc - i < 5) {
					printf("Too few parameters for -coords\n");
					exit(0);
				}
				if (sscanf(argv[i + 1], "%lf", &TempCoords[0]) != 1) {
					printf("-max first parameter in invalid form !\n");
					exit(0);
				}
				if (sscanf(argv[i + 2], "%lf", &TempCoords[1]) != 1) {
					printf("-max second parameter in invalid form !\n");
					exit(0);
				}
				if (sscanf(argv[i + 3], "%lf", &TempCoords[2]) != 1) {
					printf("-max third parameter in invalid form !\n");
					exit(0);
				}
				if (sscanf(argv[i + 4], "%lf", &TempCoords[3]) != 1) {
					printf("-max fourth parameter in invalid form !\n");
					exit(0);
				}
				i += 4;
				if (i >= argc)
					break;
				/*	    } else if (strcmp(argv[i],"-hidelog")==0) {
				 HideLog = TRUE;
				 } else if (strcmp(argv[i],"-hideza")==0) {
				 HideZoomArea = TRUE;
				 } else if (strcmp(argv[i],"-hideop")==0) {
				 HideOpProp = TRUE; */
			} else {
				printf("Unknown parameter : %s\n", argv[i]);
				exit(0);
			}
			continue;
		} else {
			FileIndex[NumFiles] = i;
			NumFiles++;
		}
	}

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL); /* Create window */
	gtk_window_set_default_size((GtkWindow *) window, 640, 480);
	gtk_window_set_title(GTK_WINDOW (window), Window_Title_NoneOpen); /* Set window title */
	gtk_window_set_resizable(GTK_WINDOW (window), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER (window), 0); /* Set borders in window */
	mainvbox = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), mainvbox);

	g_signal_connect(G_OBJECT (window), "delete_event", /* Init delete event of window */
			G_CALLBACK (close_application), NULL);

	gtk_drag_dest_set(window, GTK_DEST_DEFAULT_ALL, ui_drop_target_entries,
			NUM_IMAGE_DATA, (GDK_ACTION_COPY | GDK_ACTION_MOVE));
	g_signal_connect(G_OBJECT (window), "drag-data-received", /* Drag and drop catch */
			G_CALLBACK (drag_data_received), NULL);

	/* Create menues */
	action_group = gtk_action_group_new("MenuActions");
	gtk_action_group_add_actions(action_group, entries, G_N_ELEMENTS (entries),
			window);
	gtk_action_group_add_toggle_actions(action_group, toggle_entries,
			G_N_ELEMENTS (toggle_entries), window);
	tab_action_group = gtk_action_group_new("TabActions");
	gtk_action_group_add_actions(tab_action_group, closeaction,
			G_N_ELEMENTS (closeaction), window);
	gtk_action_group_set_sensitive(tab_action_group, FALSE);

	ui_manager = gtk_ui_manager_new();
	gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
	gtk_ui_manager_insert_action_group(ui_manager, tab_action_group, 0);

	accel_group = gtk_ui_manager_get_accel_group(ui_manager);
	gtk_window_add_accel_group(GTK_WINDOW (window), accel_group);

	error = NULL;
	if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_description, -1,
			&error)) {
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
		exit(EXIT_FAILURE);
	}

	menubar = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
	gtk_box_pack_start(GTK_BOX (mainvbox), menubar, FALSE, FALSE, 0);

	mainnotebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX (mainvbox), mainnotebook, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT (mainnotebook), "switch-page", /* Init switch-page event of notebook */
			G_CALLBACK (NoteBookTabChange), NULL);

	if (UsePreSetCoords) {
		TempCoordsPtr = &(TempCoords[0]);
	} else {
		TempCoordsPtr = NULL;
	}

	for (i = 0; i < NumFiles; i++) {
		SetupNewTab(argv[FileIndex[i]], Scale, maxX, maxY, UsePreSetCoords,
				TempCoordsPtr, &(Uselogxy[0]), &UseError);
	}

	gtk_widget_show_all(window); /* Show all widgets */

	gtk_main(); /* This is where it all starts */

	return (0); /* Exit. */
}
