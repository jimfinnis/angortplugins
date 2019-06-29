/**
 * bdfbanner
 *
 * nvBDFlib example
 *
 * Released into the public domain by Giuseppe Gatta, 2014
 */
 
#include <stdio.h>
#include <stdlib.h>
#include "nvbdflib.h"
#include <string.h>
#include <strings.h>

void resize_draw_buffer(int rows);
void bdfbanner_drawing_function(int x, int y, int c);

struct
{
	char **data;
	int numOfRows;
}drawBuffer;

void resize_draw_buffer(int rows)
{
	int i;
		
	drawBuffer.data = realloc(drawBuffer.data, sizeof(char*) * rows);

	for(i = drawBuffer.numOfRows; i < rows; i++)
	{
		drawBuffer.data[i] = malloc(79);
		bzero(drawBuffer.data[i], 79);
	}
	
	drawBuffer.numOfRows = rows;
}

void bdfbanner_drawing_function(int x, int y, int c)
{
	if(x < 0 || y < 0 || x >= 79)
		return;
	
	if(y >= drawBuffer.numOfRows)
		resize_draw_buffer(y+1);
	
	drawBuffer.data[y][x] = c;
}

int main(int argc, char *argv[])
{
	BDF_FONT *bdfFont;
	int x=0, y=0, i;
	
	if(argc < 3)
	{
		printf("Usage: bdfbanner <bdf_font> ... message ...\n");
		return EXIT_SUCCESS;
	}
	
	if(!(bdfFont = bdfReadPath(argv[1])))
	{
		printf("Error: Could not load BDF font from supplied path.\n");
		return EXIT_FAILURE;
	}
	
	drawBuffer.numOfRows = 0;
	drawBuffer.data = NULL;

	bdfSetDrawingFunction(bdfbanner_drawing_function);
	bdfSetDrawingAreaSize(79, 1);
	bdfSetDrawingWrap(1);
	
	for(i = 2; i < argc; i++)
	{
		bdfPrintString(bdfFont, x, y, argv[i]); 
		x = bdfGetDrawingCurrentX();
		y = bdfGetDrawingCurrentY();
		bdfPrintCharacter(bdfFont, x, y, ' ');
		x = bdfGetDrawingCurrentX();
		y = bdfGetDrawingCurrentY();
	}
		
	bdfFree(bdfFont);
	
	for(y = 0; y < drawBuffer.numOfRows; y++)
	{
		for(x = 0; x < 79; x++)
			putchar( (drawBuffer.data[y][x] > 0) ? '#' : ' ');
	
		putchar('\n');
	}
		
	return EXIT_SUCCESS;
}
