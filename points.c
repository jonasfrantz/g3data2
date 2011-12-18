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

#include <stdlib.h>
#include <gtk/gtk.h>
#include <math.h>
#include "main.h"

/* Extern functions */

extern void orderPoints(struct PointValue *RealPos, gint left, gint right,
		gint ordering);

/****************************************************************/
/* This function returns the integer with the lesser value.	*/
/****************************************************************/
gint min(gint x, gint y) {
	if (x < y)
		return x;
	else
		return y;
}

/****************************************************************/
/* This function calculates the true value of the point based	*/
/* on the coordinates of the point on the bitmap.		*/
/****************************************************************/
struct PointValue calculatePointValue(gint Xpos, gint Ypos, struct TabData *tabData) {
	double alpha, beta, x21, x43, y21, y43, rlc[4]; /* Declare help variables */
	struct PointValue pointValue;

	x21 = (double) tabData->axiscoords[1][0] - tabData->axiscoords[0][0]; /* Calculate deltax of x axis points */
	y21 = (double) tabData->axiscoords[1][1] - tabData->axiscoords[0][1]; /* Calculate deltay of x axis points */
	x43 = (double) tabData->axiscoords[3][0] - tabData->axiscoords[2][0]; /* Calculate deltax of y axis points */
	y43 = (double) tabData->axiscoords[3][1] - tabData->axiscoords[2][1]; /* Calculate deltay of y axis points */

	if (tabData->logxy[0]) { /* If x axis is logarithmic, store	*/
		rlc[0] = log(tabData->realcoords[0]); /* recalculated values in rlc.		*/
		rlc[1] = log(tabData->realcoords[1]);
	} else {
		rlc[0] = tabData->realcoords[0]; /* Else store old values in rlc.	*/
		rlc[1] = tabData->realcoords[1];
	}

	if (tabData->logxy[1]) {
		rlc[2] = log(tabData->realcoords[2]); /* If y axis is logarithmic, store      */
		rlc[3] = log(tabData->realcoords[3]); /* recalculated values in rlc.          */
	} else {
		rlc[2] = tabData->realcoords[2]; /* Else store old values in rlc.        */
		rlc[3] = tabData->realcoords[3];
	}

	alpha = ((tabData->axiscoords[0][0] - (double) Xpos)
			- (tabData->axiscoords[0][1] - (double) Ypos) * (x43 / y43)) / (x21
			- ((y21 * x43) / y43));
	beta = ((tabData->axiscoords[2][1] - (double) Ypos)
			- (tabData->axiscoords[2][0] - (double) Xpos) * (y21 / x21)) / (y43
			- ((x43 * y21) / x21));

	if (tabData->logxy[0])
		pointValue.Xv = exp(-alpha * (rlc[1] - rlc[0]) + rlc[0]);
	else
		pointValue.Xv = -alpha * (rlc[1] - rlc[0]) + rlc[0];

	if (tabData->logxy[1])
		pointValue.Yv = exp(-beta * (rlc[3] - rlc[2]) + rlc[2]);
	else
		pointValue.Yv = -beta * (rlc[3] - rlc[2]) + rlc[2];

	alpha = ((tabData->axiscoords[0][0] - (double) (Xpos + 1))
			- (tabData->axiscoords[0][1] - (double) (Ypos + 1)) * (x43 / y43))
			/ (x21 - ((y21 * x43) / y43));
	beta = ((tabData->axiscoords[2][1] - (double) (Ypos + 1))
			- (tabData->axiscoords[2][0] - (double) (Xpos + 1)) * (y21 / x21))
			/ (y43 - ((x43 * y21) / x21));

	if (tabData->logxy[0])
		pointValue.Xerr = exp(-alpha * (rlc[1] - rlc[0]) + rlc[0]);
	else
		pointValue.Xerr = -alpha * (rlc[1] - rlc[0]) + rlc[0];

	if (tabData->logxy[1])
		pointValue.Yerr = exp(-beta * (rlc[3] - rlc[2]) + rlc[2]);
	else
		pointValue.Yerr = -beta * (rlc[3] - rlc[2]) + rlc[2];

	alpha = ((tabData->axiscoords[0][0] - (double) (Xpos - 1))
			- (tabData->axiscoords[0][1] - (double) (Ypos - 1)) * (x43 / y43))
			/ (x21 - ((y21 * x43) / y43));
	beta = ((tabData->axiscoords[2][1] - (double) (Ypos - 1))
			- (tabData->axiscoords[2][0] - (double) (Xpos - 1)) * (y21 / x21))
			/ (y43 - ((x43 * y21) / x21));

	if (tabData->logxy[0])
		pointValue.Xerr -= exp(-alpha * (rlc[1] - rlc[0]) + rlc[0]);
	else
		pointValue.Xerr -= -alpha * (rlc[1] - rlc[0]) + rlc[0];

	if (tabData->logxy[1])
		pointValue.Yerr -= exp(-beta * (rlc[3] - rlc[2]) + rlc[2]);
	else
		pointValue.Yerr -= -beta * (rlc[3] - rlc[2]) + rlc[2];

	pointValue.Xerr = fabs(pointValue.Xerr / 4.0);
	pointValue.Yerr = fabs(pointValue.Yerr / 4.0);

	return pointValue;
}

/****************************************************************/
/* This function is called when the "Print results" button is	*/
/* pressed, it calculate the values of the datapoints and 	*/
/* prints them through stdout.					*/
/****************************************************************/
void outputResultset(GtkWidget *widget, gpointer data) {
	gint i; /* Declare index variable */
	gboolean print2file;
	FILE *FP;
	struct TabData *tabData;
	struct PointValue *realPositions, calculatedValue;

	tabData = (struct TabData *) data;

	print2file = tabData->Action;

	if (print2file == PRINT2FILE) {
		FP = fopen(tabData->file_name, "w"); /* Open file for writing */
		if (FP == NULL) {
			printf("Could not open %s for writing\n", tabData->file_name); /* If unable to open print error */
			return;
		}
	}

	realPositions = (struct PointValue *) malloc(
			sizeof(struct PointValue) * tabData->numpoints);

	/* Next up is recalculating the positions of the points by solving a 2*2 matrix */

	for (i = 0; i < tabData->numpoints; i++) {
		calculatedValue = calculatePointValue(tabData->points[i][0],
				tabData->points[i][1], tabData);
		realPositions[i].Xv = calculatedValue.Xv;
		realPositions[i].Yv = calculatedValue.Yv;
		realPositions[i].Xerr = calculatedValue.Xerr;
		realPositions[i].Yerr = calculatedValue.Yerr;
	}

	if (tabData->ordering != 0) {
		orderPoints(realPositions, 0, tabData->numpoints - 1, tabData->ordering);
	}

	/* Print results to stdout or file */

	for (i = 0; i < tabData->numpoints; i++) {
		if (print2file == PRINT2FILE) {
			fprintf(FP, "%.12g  %.12g", realPositions[i].Xv, realPositions[i].Yv);
			if (tabData->UseErrors) {
				fprintf(FP, "\t%.12g  %.12g\n", realPositions[i].Xerr,
						realPositions[i].Yerr);
			} else
				fprintf(FP, "\n");
		} else {
			printf("%.12g  %.12g", realPositions[i].Xv, realPositions[i].Yv);
			if (tabData->UseErrors) {
				printf("\t%.12g  %.12g\n", realPositions[i].Xerr, realPositions[i].Yerr);
			} else
				printf("\n");
		}
	}
	free(realPositions);

	if (print2file == PRINT2FILE)
		fclose(FP);
}
