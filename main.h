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
#include <gtk/gtk.h>					/* Include gtk library */

#define VERSION "1.0.0"					/* Version number */

#define ZOOMPIXSIZE 200					/* Size of zoom in window */
#define ZOOMFACTOR 4					/* Zoom factor of zoom window */
#define MARKERSIZE 3					/* Size of point marker red outer square */
#define MARKERLENGTH 6					/* Axis marker length */
#define MARKERTHICKNESS 2				/* Marker line thickness */
#define MAXPOINTS 64					/* Number of points to allocate at once */
#define MAXNUMFILES 256
#define GRABTRESHOLD MARKERSIZE*2

#define ACTIONBNUM 2
#define ORDERBNUM 3
#define LOGBNUM 2

#define NONESELECTED -1

#define URI_IDENTIFIER "file://"

struct PointValue {
	double Xv, Yv, Xerr, Yerr;
};

typedef enum {
	PRINT2STDOUT = 0, PRINT2FILE
} ACTION;

typedef enum {
	URI_LIST, PNG_DATA, JPEG_DATA, APP_X_COLOR, NUM_IMAGE_DATA,
} UI_DROP_TARGET_INFO;

struct TabData {
	GtkWidget *drawing_area;
	GtkWidget *zoom_area; 					// Drawing areas
	GtkWidget *xyentry[4];
	GtkWidget *exportbutton;
	GtkWidget *remlastbutton; 				// Various buttons
	GtkWidget *setxybutton[4];
	GtkWidget *remallbutton; 				// Even more various buttons
	GtkWidget *xc_entry, *yc_entry;
	GtkWidget *file_entry, *nump_entry;
	GtkWidget *xerr_entry, *yerr_entry; 	// Coordinate and filename entries
	GtkWidget *logbox;
	GtkWidget *zoomareabox;
	GtkWidget *oppropbox;
	GtkWidget *ViewPort;

	cairo_surface_t *image;

	gint axiscoords[4][2]; 					// X,Y coordinates of axispoints
	gint **points; 							// Indexes of graphpoints and their coordinates
	gint *lastpoints; 						// Indexes of last points put out
	gint numpoints;
	gint numlastpoints; 					// Number of points on graph and last put out
	gint ordering; 							// Various control variables
	gint XSize, YSize;
	gint file_name_length;
	gint MaxPoints; // = MAXPOINTS;
	gint Action;
	gdouble realcoords[4]; 					// X,Y coords on graph
	gboolean UseErrors;
	gboolean setxypressed[4];
	gboolean bpressed[4]; 					// What axispoints have been set out ?
	gboolean valueset[4];
	gboolean logxy[2]; // = { FALSE, FALSE };
	gchar *file_name; 						// Pointer to filename
	gchar FileNames[256];

	gint mousePointerCoords[2];

	gint movedPointIndex;
	gint movedOrigCoords[2];
	gint movedOrigMousePtrCoords[2];
};

struct ButtonData {
	struct TabData *tabData;
	gint index;
};

struct GtkSelectionData_x {
  GdkAtom       selection;
  GdkAtom       target;
  GdkAtom       type;
  gint          format;
  guchar       *data;
  gint          length;
  GdkDisplay   *display;
};
