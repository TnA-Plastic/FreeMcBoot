/*      
  _____     ___ ____ 
   ____|   |    ____|      PS2 Open Source Project
  |     ___|   |____       
  
---------------------------------------------------------------------------

    Copyright (C) 2008 - Neme & jimmikaelkael (www.psx-scene.com) 

    This program is free software; you can redistribute it and/or modify
    it under the terms of the Free McBoot License.
    
	This program and any related documentation is provided "as is"
	WITHOUT ANY WARRANTIES, either express or implied, including, but not
 	limited to, implied warranties of fitness for a particular purpose. The
 	entire risk arising out of use or performance of the software remains
 	with you.
   	In no event shall the author be liable for any damages whatsoever
 	(including, without limitation, damages to your hardware or equipment,
 	environmental damage, loss of health, or any kind of pecuniary loss)
 	arising out of the use of or inability to use this software or
 	documentation, even if the author has been advised of the possibility of
 	such damages.

    You should have received a copy of the Free McBoot License along with
    this program; if not, please report at psx-scene :
    http://psx-scene.com/forums/freevast/

---------------------------------------------------------------------------

 GUI source file
 
--------------------------------------------------------------------------- 
*/

#include "fmcb.h"
#include "sjpcm.h"

// include font specific datas
#include "font_verdana.h"

//#define DEBUG

void Setup_GS(int gs_vmode);
void gfx_set_defaults(void);
void load_Textures(void);
void Clear_Screen(void);
int  Draw_INTRO(void);
int  Draw_GUI(int logo, int selected_button, int highlight_pulse, int highlight_blw, int log, int dialog);
int  Draw_OUTRO(void);
void Render_GUI(void);
void Play_Sound(void);

GSGLOBAL *gsGlobal;
GSTEXTURE tex_bar_up, tex_bar_down, tex_bar_delimiter, tex_highlight, tex_highlight_bw;
GSTEXTURE tex_background, tex_logo;
GSTEXTURE tex_credits_coded, tex_credits_gui;
GSTEXTURE tex_option[6];
GSTEXTURE tex_font_verdana;
GSTEXTURE tex_icon_ok, tex_icon_warning, tex_icon_error;

// screen defaults for NTSC, just in case
int TV_mode       = 2;
int	SCREEN_WIDTH  = 640;
int	SCREEN_HEIGHT = 448;
int SCREEN_X	  = 632;
int SCREEN_Y	  = 50;
int FONT_WIDTH    = 16;
int FONT_HEIGHT   = 15;
int FONT_SPACING  = 1;
int FONT_Y        = 75;

// define colors
#define Black  		GS_SETREG_RGBAQ(0x00,0x00,0x00,0x00,0x00)		
#define Blue  		GS_SETREG_RGBAQ(0x18,0x23,0xFF,0x80,0x00)
#define White  		GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0x80,0x00)		
#define Gray  		GS_SETREG_RGBAQ(0x33,0x33,0x33,0x80,0x00)		
#define TexCol 		GS_SETREG_RGBAQ(0x80,0x80,0x80,0x80,0x00)

// sounds related
static short snd_buf_L[1024] __attribute__((aligned (64)));
static short snd_buf_R[1024] __attribute__((aligned (64)));
u16 *pSampleBuf;
int nSizeSample; 
int snd_pos;
int snd_finished;
int SAMPLES_TICK = 800; // default to NTSC, PAL must use 960

// Common to both GUI & INTRO
int bar_delimiter_x[7];
int option_x[6];

// For GUI
int logo_alpha;
int highlight_alpha;
int amount;
int pause_pulse;
int stop_pulse_done;
int log_alpha;
int dialog_alpha;
int control_alpha;
int control_amount;
int pause_control_pulse;
	
// For both INTRO & OUTRO
int up_panel_y;
int down_panel_y;
int background_alpha;

// For INTRO only
float logo_width;
float logo_height;
float logo_accel;

// For dialogs
#define DIALOG_YES_NO       1
#define DIALOG_OK           2
#define DIALOG_OK_CANCEL    3
#define DIALOG_ABORT     	4
#define DIALOG_1_2     	 	5
#define DIALOG_NONE     	6
#define ICON_DIALOG_OK      1
#define ICON_DIALOG_WARNING 2
#define ICON_DIALOG_ERROR   3
#define ICON_DIALOG_NONE    4
#define MAX_DIALOG_LINES    14

// to be filled before opening a dialog :
int dialog_type;
int dialog_icon;
char* dialog_buffer[MAX_DIALOG_LINES];
int selected_dialog_button;
int internal_dialog_type; // only for internal use !!!

#define MAX_LOG_LINES 22
// to be filled before to print a log
char* log_job_buffer[MAX_LOG_LINES];
int log_result[MAX_LOG_LINES];

//--------------------------------------------------------------
// PNG handling code: from MyPS2 by ntba2
//
typedef struct {
	int width;
	int height;
	int bit_depth;

	void *priv;
} pngData;

typedef struct {
	png_structp	png_ptr;
	png_infop	info_ptr, end_info;

	u8 *buf;
	int pos;

	u8 *data;
} pngPrivate;

//--------------------------------------------------------------

static void read_data_fn(png_structp png_ptr, png_bytep buf, png_size_t size)
{
	pngPrivate *priv = (pngPrivate*)png_get_io_ptr(png_ptr);

	memcpy(buf, priv->buf + priv->pos, size);
	priv->pos += size;
}

//--------------------------------------------------------------

pngData *pngOpenRAW(u8 *data, int size)
{
	pngData		*png;
	pngPrivate	*priv;

	if (png_sig_cmp( data, 0, 8 ) != 0)
		return NULL;
		
	if ((png = malloc(sizeof(pngData))) == NULL)
		return NULL;

	memset (png, 0, sizeof(pngData));

	if ((priv = malloc(sizeof(pngPrivate))) == NULL)
		return NULL;

	png->priv = priv;

	priv->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!priv->png_ptr) {
		#ifdef DEBUG
			printf("PNG Read Struct Init Failed\n");		
		#endif	
		free(png);
		return NULL;
	}

	priv->info_ptr = png_create_info_struct(priv->png_ptr);
	if (!priv->info_ptr) {
		#ifdef DEBUG
			printf("PNG Info Struct Init Failed\n");
		#endif
		free(png);
		png_destroy_read_struct(&priv->png_ptr, NULL, NULL);
		return NULL;
	}

	priv->end_info = png_create_info_struct(priv->png_ptr);
	if (!priv->end_info) {
		free(png);
		png_destroy_read_struct(&priv->png_ptr, &priv->info_ptr, NULL);
		return NULL;
	}
	
	priv->buf	= data;
	priv->pos	= 0;

	png_set_read_fn(priv->png_ptr, (png_voidp)priv, read_data_fn);
	png_read_png(priv->png_ptr, priv->info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	png->width	= priv->info_ptr->width;
	png->height	= priv->info_ptr->height;
	png->bit_depth	= priv->info_ptr->channels * 8;

	#ifdef DEBUG
		printf("PNG info: width=%3d ", png->width);
		printf("height=%3d ", png->height);	
		printf("bit depth=%2d ", png->bit_depth);
		printf("color type=%2d\n", priv->png_ptr->color_type);
	#endif
		
	return png;
}

//--------------------------------------------------------------

int pngReadImage(pngData *png, u8 *dest)
{
	pngPrivate *priv = png->priv;
	u8 **row_pointers;
	int i, row_ptr;

	int y;

	row_pointers = png_get_rows(priv->png_ptr, priv->info_ptr);
	row_ptr = 0;		

	for (i = 0; i < priv->info_ptr->height; i++) {
		memcpy(dest + row_ptr, row_pointers[i], priv->info_ptr->rowbytes);

		// need to normalize alpha channel to ps2 range
		if (priv->info_ptr->channels == 4) {
			for (y = 3; y < priv->info_ptr->rowbytes; y += 4)
				*(dest + row_ptr + y ) /= 2;
		}

		row_ptr += priv->info_ptr->rowbytes;
	}

	return 1;
}

//--------------------------------------------------------------

void pngClose(pngData *png)
{
	pngPrivate *priv = png->priv;

	png_destroy_read_struct(&priv->png_ptr, &priv->info_ptr, &priv->end_info);
	
	if (priv->data)
		free(priv->data);

	free(priv);
	free(png);
}

//----------------------------------------------------

void Play_Sound(void) 
{
	// TO BE CALLED ONCE PER VERTICAL SYNC PERIOD !!!
	// Uses :
	// - pSampleBuf   (u16 *) as sound buffer
	// - nSizeSample  (int) as sound buffer size
	// - SAMPLES_TICK (int) : 800 for NTSC, 960 for PAL
	// - snd_pos      (int) as position in sound buffer
	// - snd_finished (int) as flag if sound finished playing : 0 or 1
	// Sounds must be 48kHZ 16bit mono uncompressed PCM
	// The mono channel is send on both R & L
	//
	// Example of use :
	//		pSampleBuf = (u16 *)&option_snd;
	//		nSizeSample = size_option_snd;
	//		snd_pos = 0;
	//		while (1) {
	//			Play_Sound();
	//			gsKit_vsync();
	//		}
	
	int i;		
	int buffered;
	
	// Skip audio file header (to avoid it to be put in sound buffer and be played,
	// producing crackling effect at begining of sample)
	if (snd_pos == 0) {
		snd_pos += 22; // Skip audio file header (in short int indexing !)
		snd_finished = 0;
		#ifdef DEBUG
			printf("Starting new sound\n");
		#endif
	}

	// fill left & right sound buffer
	for (i=0; i<SAMPLES_TICK; i++) {
		
		// If were are not at end of sound buffer, fills with datas
		if ((snd_pos + i) <= (nSizeSample / 2)) {
			snd_buf_L[i] = pSampleBuf[snd_pos + i];
			snd_buf_R[i] = snd_buf_L[i];
		//if (((snd_pos + i) * 2 + 1) <= (nSizeSample / 2)) {   // stereo			
			//snd_buf_L[i] = pSampleBuf[(snd_pos + i) * 2 + 0]; // stereo
			//snd_buf_R[i] = pSampleBuf[(snd_pos + i) * 2 + 1]; // stereo
			
		}
		else {
			// if no more datas available, then clear left & right buffer 
			snd_buf_L[i] = 0;
			snd_buf_R[i] = 0;
		}
	}

	// Enqueue "SAMPLES_TICK" left & right samples
  	buffered = SjPCM_Buffered();
  	if (buffered < 1*SAMPLES_TICK) 
  		SjPCM_Enqueue(snd_buf_L, snd_buf_R, SAMPLES_TICK, 0); // avoid underrun
  	if (buffered < 2*SAMPLES_TICK) 
  		SjPCM_Enqueue(snd_buf_L, snd_buf_R, SAMPLES_TICK, 0); // regular buffer fill	

	// Update sound position
	if (snd_pos < (nSizeSample / 2)) {	
		snd_pos += SAMPLES_TICK;		
	//if (snd_pos < (nSizeSample / 4)) { // stereo
		//snd_pos += SAMPLES_TICK;		 // stereo
	}
	else {
		snd_finished = 1;	
		#ifdef DEBUG
			printf("Sound position = %d\n", snd_pos);
		#endif
	}
}

//----------------------------------------------------

void drawChar_verdana(u32 x, u32 y, u32 width, u32 height, u64 color, u32 c)
{
	// Draw a character with verdana font
	
	int x1, x2, y1, y2;
	int u1, u2, v1, v2;
	
	x1 = x;
	x2 = x1 + width;	
	y1 = y;
	y2 = y1 + height;	

	// Calculate char coordinates int verdana texture			
	u1 = (c % (tex_font_verdana.Width/16)) * (tex_font_verdana.Width/16);
	u2 = u1 + 16;
	v1 = c - (c % (tex_font_verdana.Height/8)); // careful: 8 rows only !!!
	v2 = v1 + 16;

	// Draw a char using verdana texture
	gsKit_prim_sprite_texture(gsGlobal, &tex_font_verdana,
							x1, // X1
							y1,	// Y1
							u1, // U1
							v1, // V1
							x2, // X2
							y2,	// Y2
							u2, // U2
							v2, // V2
							0,
							color);
	
}

//--------------------------------------------------------------

void drawString_verdana(u32 x, u32 y, u64 color, const char *string)
{
	// Draw a string with verdana font
	
	int l, i, cx;
	int c;

	cx = x;
	
	l = strlen(string);

	for( i = 0; i < l; i++ )
	{
		c = (u8)string[i];
		if (c > 127) c = 127; // security check as the font is incomplete
		
		// Draw the string character by character
		drawChar_verdana(cx, y, FONT_WIDTH, FONT_HEIGHT, color, c);
		
		// Uses width informations for verdana font
		cx += font_verdana_width[c] + FONT_SPACING;
	}
}

//--------------------------------------------------------------

int getStringWidth_verdana(const char *string)
{
	// Calculate and return width in pixels of a string using verdana font
	
	int i, l, c, size;
	
	l = strlen(string);
	
	size = 0;
	
	for( i = 0; i < l; i++ )
	{
		c = (u8)string[i];
		if (c >= 128) c = 127; // security check as the font is incomplete
		
		size += font_verdana_width[c] + FONT_SPACING;
	}
		
	return size;
}

//--------------------------------------------------------------

void draw_log(int alpha)
{
	// Draw the log
	
	int i;	
	
	int pos_x = 40;  // log lines x
	int pos_x2 = 370; // log result x
	int pos_y = FONT_Y;
	
	for (i=0; i<MAX_LOG_LINES; i++) {
		if ((log_job_buffer[i] != NULL) && (strlen(log_job_buffer[i]) > 0)) {
			drawString_verdana(pos_x, pos_y, GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,alpha,0x00), log_job_buffer[i]);
		}
		if (log_result[i] == 0) {
			// Draw icon error
			gsKit_prim_sprite_texture(gsGlobal, &tex_icon_error,
							pos_x2, // X1
							pos_y,	// Y1
							0,  // U1
							0,  // V1
							pos_x2 + FONT_WIDTH + 1, // X2
							pos_y + FONT_HEIGHT,	// Y2
							tex_icon_error.Width,  // U2
							tex_icon_error.Height, // V2
							0,
							GS_SETREG_RGBAQ(0x80,0x80,0x80, alpha,0x00));		
		}
		else if (log_result[i] == 1) {
			// Draw icon OK
			gsKit_prim_sprite_texture(gsGlobal, &tex_icon_ok,
							pos_x2, // X1
							pos_y,	// Y1
							0,  // U1
							0,  // V1
							pos_x2 + FONT_WIDTH + 1, // X2
							pos_y + FONT_HEIGHT,// Y2
							tex_icon_ok.Width,  // U2
							tex_icon_ok.Height, // V2
							0,
							GS_SETREG_RGBAQ(0x80,0x80,0x80, alpha,0x00));		
		}
		else if (log_result[i] == 2) {
			// do nothing
		}
		
		pos_y += FONT_HEIGHT;
	}
}

//--------------------------------------------------------------

void draw_dialog(int alpha_dialog, int alpha_control)
{
	// log must be faded before to call this
	// Draw a dialog using dialog_type, dialog_icon, dialog_buffer vars
	
	int x1, x2, y1, y2;
	int y;
	int i;
	char *msg;
	char *msg1, *msg2;
	u64 color_text, color_texture, color_selected, color_unselected;
	int dialog_start_y = 0;
	int spacing = 12; 
	int width = 0;
	int height = 0;
	int control_spacing = 25;
			
	// Set all needed colors and transparency
	color_text = GS_SETREG_RGBAQ(0xFF,0xFF,0xFF, alpha_dialog,0x00);
	color_texture = GS_SETREG_RGBAQ(0x80,0x80,0x80, alpha_dialog,0x00);
	if (dialog_icon == ICON_DIALOG_ERROR)
		color_selected = GS_SETREG_RGBAQ(0x92,0x00,0x00, alpha_control,0x00);
	else	
		color_selected = GS_SETREG_RGBAQ(0x18,0x23,0xFF, alpha_control,0x00);	
	color_unselected = GS_SETREG_RGBAQ(0x33,0x33,0x33, alpha_dialog,0x00);
		
	y2 = 0;

	// Calculate height needed for dialog to center it in height
	if (dialog_icon == ICON_DIALOG_OK)
		height += tex_icon_ok.Height;
	else if (dialog_icon == ICON_DIALOG_WARNING)
		height += tex_icon_warning.Height;
	else if (dialog_icon == ICON_DIALOG_ERROR)
		height += tex_icon_error.Height;
	height += spacing;	
	for (i=0; i<MAX_DIALOG_LINES; i++) {
		if ((dialog_buffer[i] != NULL) && (strlen(dialog_buffer[i]) > 0))
			height += FONT_HEIGHT;
	}
	height += spacing;
	if (dialog_type != DIALOG_NONE)
		height += FONT_HEIGHT;
	
	dialog_start_y = ((SCREEN_HEIGHT - height) / 2) - 20;
	
	// Draw dialog Icon
	if (dialog_icon == ICON_DIALOG_OK) {
		// Calculates coordinates to center the dialog icon OK
		x1 = (SCREEN_WIDTH - tex_icon_ok.Width) / 2;
		x2 = x1 + tex_icon_ok.Width;
		y1 = dialog_start_y;
		y2 = y1 + tex_icon_ok.Height;

		// Draw icon OK
		gsKit_prim_sprite_texture(gsGlobal, &tex_icon_ok,
							x1, // X1
							y1,	// Y1
							0,  // U1
							0,  // V1
							x2, // X2
							y2,	// Y2
							tex_icon_ok.Width,  // U2
							tex_icon_ok.Height, // V2
							0,
							color_texture);		
	}	
	else if (dialog_icon == ICON_DIALOG_WARNING) {
		// Calculates coordinates to center the dialog icon warning
		x1 = (SCREEN_WIDTH - tex_icon_warning.Width) / 2;
		x2 = x1 + tex_icon_warning.Width;
		y1 = dialog_start_y;
		y2 = y1 + tex_icon_warning.Height;

		// Draw icon warning
		gsKit_prim_sprite_texture(gsGlobal, &tex_icon_warning,
							x1, // X1
							y1,	// Y1
							0,  // U1
							0,  // V1
							x2, // X2
							y2,	// Y2
							tex_icon_warning.Width,  // U2
							tex_icon_warning.Height, // V2
							0,
							color_texture);		
	}						
	else if (dialog_icon == ICON_DIALOG_ERROR) {
		// Calculates coordinates to center the dialog icon warning
		x1 = (SCREEN_WIDTH - tex_icon_error.Width) / 2;
		x2 = x1 + tex_icon_error.Width;
		y1 = dialog_start_y;
		y2 = y1 + tex_icon_error.Height;

		// Draw icon error
		gsKit_prim_sprite_texture(gsGlobal, &tex_icon_error,
							x1, // X1
							y1,	// Y1
							0,  // U1
							0,  // V1
							x2, // X2
							y2,	// Y2
							tex_icon_error.Width,  // U2
							tex_icon_error.Height, // V2
							0,
							color_texture);		
	}						
	else if (dialog_icon == ICON_DIALOG_NONE) {
		y2 = dialog_start_y;
	}
	
	// Spacing from icon	
	y = y2 + spacing;				
	
	// Printing dialog buffer
	for (i=0; i<MAX_DIALOG_LINES; i++) {
		if ((dialog_buffer[i] != NULL) && (strlen(dialog_buffer[i]) > 0)) {
				msg = dialog_buffer[i];		
				// Print centered string 				
				drawString_verdana(((
					SCREEN_WIDTH - getStringWidth_verdana(msg)) / 2),  // X
					y,   // Y
					color_text,     // color
					msg);			// string
				y += FONT_HEIGHT;		
		}
		
	}
				
	// Spacing from dialog text
	y += spacing;
	
	// Drawing dialog controls
	if (internal_dialog_type == DIALOG_OK) {
		
		// set default selected control
		if (selected_dialog_button == 0) selected_dialog_button = 1;
		
		// Draw centered controls
		msg1 = "OK";			
		width = getStringWidth_verdana(msg1);		
		x1 = (SCREEN_WIDTH - width) / 2;

		//if (selected_dialog_button == 1)		
			 drawString_verdana(x1, y, color_selected, msg1);							
			 
	}
	else if (internal_dialog_type == DIALOG_YES_NO) {
		
		// set default selected control		
		if (selected_dialog_button == 0) selected_dialog_button = 2;
		
		// Draw centered controls		
		msg1 = "YES";						
		msg2 = "NO";						
		width = getStringWidth_verdana(msg1) + getStringWidth_verdana(msg2) + control_spacing;		
		x1 = (SCREEN_WIDTH - width) / 2;
		x2 = x1 + width - getStringWidth_verdana(msg2);
		
		if (selected_dialog_button == 1)		
			 drawString_verdana(x1, y, color_selected, msg1);							
		else drawString_verdana(x1, y, color_unselected, msg1);

		if (selected_dialog_button == 2)		
			 drawString_verdana(x2, y, color_selected, msg2);							
		else drawString_verdana(x2, y, color_unselected, msg2);							
		
	}	
	else if (internal_dialog_type == DIALOG_OK_CANCEL) {
		
		// set default selected control		
		if (selected_dialog_button == 0) selected_dialog_button = 2;
		
		// Draw centered controls		
		msg1 = "OK";						
		msg2 = "CANCEL";						
		width = getStringWidth_verdana(msg1) + getStringWidth_verdana(msg2) + control_spacing;		
		x1 = (SCREEN_WIDTH - width) / 2;
		x2 = x1 + width - getStringWidth_verdana(msg2);
				
		if (selected_dialog_button == 1)		
			 drawString_verdana(x1, y, color_selected, msg1);							
		else drawString_verdana(x1, y, color_unselected, msg1);

		if (selected_dialog_button == 2)		
			 drawString_verdana(x2, y, color_selected, msg2);							
		else drawString_verdana(x2, y, color_unselected, msg2);							
	}	
	else if (internal_dialog_type == DIALOG_ABORT) {
		
		// set default selected control
		if (selected_dialog_button == 0) selected_dialog_button = 1;
		
		// Draw centered controls
		msg1 = "ABORT";			
		width = getStringWidth_verdana(msg1);		
		x1 = (SCREEN_WIDTH - width) / 2;

		//if (selected_dialog_button == 1)		
			 drawString_verdana(x1, y, color_selected, msg1);							
			 
	}
	else if (internal_dialog_type == DIALOG_1_2) {
		
		// set default selected control
		if (selected_dialog_button == 0) selected_dialog_button = 1;
		
		// Draw centered controls		
		msg1 = "SLOT 1";						
		msg2 = "SLOT 2";						
		width = getStringWidth_verdana(msg1) + getStringWidth_verdana(msg2) + control_spacing;		
		x1 = (SCREEN_WIDTH - width) / 2;
		x2 = x1 + width - getStringWidth_verdana(msg2);
				
		if (selected_dialog_button == 1)		
			 drawString_verdana(x1, y, color_selected, msg1);							
		else drawString_verdana(x1, y, color_unselected, msg1);

		if (selected_dialog_button == 2)		
			 drawString_verdana(x2, y, color_selected, msg2);							
		else drawString_verdana(x2, y, color_unselected, msg2);							
			 
	}
	else if (internal_dialog_type == DIALOG_NONE) {
		// do nothing
	}
}

//--------------------------------------------------------------

void draw_background(int alpha)
{
	// Draw background texture
	
	gsKit_prim_sprite_texture(gsGlobal, &tex_background,
							0, 	// X1
							0,  // Y1
							0,  // U1
							0,  // V1
							SCREEN_WIDTH,			// X2
							SCREEN_HEIGHT,			// Y2
							tex_background.Width, 	// U2
							tex_background.Height, 	// V2
							0,
							GS_SETREG_RGBAQ(0x80, 0x80, 0x80, alpha, 0x00));
}

//--------------------------------------------------------------

void draw_up_panel(int y, int selected_button, int highlight_pulse, int highlight_blw)
{
	// Draw a complete Up panel including bar, separators, buttons texts and highlight (with pulse control)
	
	int i;
	
	// Draw up bar
	gsKit_prim_sprite_texture(gsGlobal, &tex_bar_up,
							0, 	// X1
							y,  // Y1
							0,  // U1
							0,  // V1
							SCREEN_WIDTH,	  		// X2
							y +	tex_bar_up.Height,	// Y2
							tex_bar_up.Width, 		// U2
							tex_bar_up.Height, 		// V2
							0,
							TexCol);
							
	// Draw delimiters
	for (i=1; i<6; i++) { //skip 1st and last up panel delimiters							
		gsKit_prim_sprite_texture(gsGlobal, &tex_bar_delimiter,
							bar_delimiter_x[i], // X1
							y,  				// Y1
							0,  				// U1
							0,  				// V1
							bar_delimiter_x[i] + tex_bar_delimiter.Width,	// X2
							y + tex_bar_delimiter.Height,					// Y2
							tex_bar_delimiter.Width,						// U2
							tex_bar_delimiter.Height,						// V2
							0,
							TexCol);
	}							

	// Draw buttons text
	for (i=0; i<6; i++) {								
		gsKit_prim_sprite_texture(gsGlobal, &tex_option[i],
							option_x[i],// X1
							y + 10,  	// Y1
							0,  		// U1
							0,  		// V1
							option_x[i] + tex_option[i].Width, 	// X2
							y + 10 + tex_option[i].Height,		// Y2
							tex_option[i].Width,				// U2
							tex_option[i].Height,				// V2
							0,
							TexCol);							
	}

	// Alpha calculation to control Highlight pulse	
	if (highlight_pulse) {
		highlight_alpha += amount;
		if (highlight_alpha >= 0xff) {
			highlight_alpha = 0xff;
			pause_pulse++;
			if (pause_pulse >= 12) {
				amount = -6;
				pause_pulse = 0;
			}
		} else if (highlight_alpha <= 0x40) {
			amount = 6;
			highlight_alpha = 0x40;
		}
	}
	else {
		if (highlight_blw) {
		stop_pulse_done = 1;
		highlight_alpha = 0x80;
		}	
		else {
		stop_pulse_done = 0;
		if (highlight_alpha <= 0xff) {
			amount = 6;
			highlight_alpha += amount;	
			if (highlight_alpha >= 0xff) {
				highlight_alpha = 0xff;
				stop_pulse_done = 1;
			} 	
		}
		}
	}
	
	if (!highlight_pulse && highlight_blw && stop_pulse_done) {
		// Draw highlighting on up panel selected button if needed
		if (selected_button > 0) {
			gsKit_prim_sprite_texture(gsGlobal, &tex_highlight_bw,
							bar_delimiter_x[selected_button-1]+1,// X1
							y,  // Y1
							0,  // U1
							0,  // V1
							bar_delimiter_x[selected_button],	// X2
							y + tex_highlight_bw.Height,			// Y2
							tex_highlight_bw.Width, 				// U2
							tex_highlight_bw.Height, 				// V2
							0,
							GS_SETREG_RGBAQ(0x80, 0x80, 0x80, highlight_alpha, 0x00));
		}						
	}
	else {
		// Draw highlighting on up panel selected button if needed
		if (selected_button > 0) {
			gsKit_prim_sprite_texture(gsGlobal, &tex_highlight,
							bar_delimiter_x[selected_button-1]+1,// X1
							y,  // Y1
							0,  // U1
							0,  // V1
							bar_delimiter_x[selected_button],	// X2
							y + tex_highlight.Height,			// Y2
							tex_highlight.Width, 				// U2
							tex_highlight.Height, 				// V2
							0,
							GS_SETREG_RGBAQ(0x80, 0x80, 0x80, highlight_alpha, 0x00));
		}	
	}					
								
}

//--------------------------------------------------------------	

void draw_down_panel(int y2) // careful : y2 !!!
{
	// Draw a complete down panel including down bar, credits for code & gfx
	
	// Draw down bar
	gsKit_prim_sprite_texture(gsGlobal, &tex_bar_down,
							0, 	// X1
							y2 - tex_bar_down.Height,  // Y1
							0,  // U1
							0,  // V1
							SCREEN_WIDTH, 		 // X2
							y2, 	 	 		 // Y2
							tex_bar_down.Width,  // U2
							tex_bar_down.Height, // V2
							0,
							TexCol);
					
	// Draw credits for code									
	gsKit_prim_sprite_texture(gsGlobal, &tex_credits_coded,
							8, 	// X1
							y2 - 4 - tex_credits_coded.Height,  // Y1
							0,  // U1
							0,  // V1
							8 + tex_credits_coded.Width,  	// X2
							y2 - 4, 	 		  			// Y2
							tex_credits_coded.Width,  	  	// U2
							tex_credits_coded.Height, 	 	// V2
							0,
							TexCol);

	// Draw credits for gfx
	gsKit_prim_sprite_texture(gsGlobal, &tex_credits_gui,
							SCREEN_WIDTH - 12 - tex_credits_gui.Width,   // X1
							y2 - 4 - tex_credits_gui.Height,  			 // Y1
							0,  // U1
							0,  // V1
							SCREEN_WIDTH - 12, 		// X2
							y2 - 4, 	 			// Y2
							tex_credits_gui.Width,  // U2
							tex_credits_gui.Height, // V2
							0,
							TexCol);
}
	
//--------------------------------------------------------------		

void draw_logo(int width, int height, int alpha)
{
	// Draw FMCB logo
	
	int x1, x2, y1, y2;
	
	// Calculates coordinates to center the logo
	x1 = (SCREEN_WIDTH - width) / 2;
	x2 = x1 + width;
	y1 = (SCREEN_HEIGHT - height) / 2;
	y2 = y1 + height;

	// Draw logo	
	gsKit_prim_sprite_texture(gsGlobal, &tex_logo,
							x1, // X1
							y1,	// Y1
							0,  // U1
							0,  // V1
							x2, // X2
							y2,	// Y2
							tex_logo.Width,  // U2
							tex_logo.Height, // V2
							0,
							GS_SETREG_RGBAQ(0x80, 0x80, 0x80, alpha, 0x00));		
							
}

//----------------------------------------------------

void load_Textures(void)
{
	// Load permanently needed textures into VRAM
		
	pngData *pPng;
	u8		*pImgData;

	#ifdef DEBUG				
		printf("1st VRAM Pointer = %08x  \n", gsGlobal->CurrentPointer);	
	#endif	
		
	//gsGlobal->CurrentPointer = vram_pointer;
				
	if ((pPng = pngOpenRAW(&background, size_background)) > 0) { // tex size = 0x140000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_background.PSM 		= GS_PSM_CT32;
				tex_background.Mem 		= (u32 *)pImgData;
				tex_background.VramClut = 0;
				tex_background.Clut		= NULL;
				tex_background.Width    = pPng->width;
				tex_background.Height   = pPng->height;
				tex_background.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_background.Vram 	= gsKit_vram_alloc(gsGlobal,
			 						  		gsKit_texture_size(tex_background.Width, tex_background.Height, tex_background.PSM), 
			 						  		GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_background);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}	
	
	if ((pPng = pngOpenRAW(&bar_up, size_bar_up)) > 0) { // tex size = 0x8000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_bar_up.PSM 	 	= GS_PSM_CT32;
				tex_bar_up.Mem 	 	= (u32 *)pImgData;
				tex_bar_up.VramClut = 0;
				tex_bar_up.Clut 	= NULL;
				tex_bar_up.Width 	= pPng->width;
				tex_bar_up.Height 	= pPng->height;
				tex_bar_up.Filter 	= GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_bar_up.Vram 	= gsKit_vram_alloc(gsGlobal,
			 							gsKit_texture_size(tex_bar_up.Width, tex_bar_up.Height, tex_bar_up.PSM), 
			 							GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_bar_up);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}		
	
	if ((pPng = pngOpenRAW(&bar_down, size_bar_down)) > 0) { // tex size = 0x8000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_bar_down.PSM 	  = GS_PSM_CT32;
				tex_bar_down.Mem 	  = (u32 *)pImgData;
				tex_bar_down.VramClut = 0;
				tex_bar_down.Clut 	  = NULL;
				tex_bar_down.Width    = pPng->width;
				tex_bar_down.Height   = pPng->height;
				tex_bar_down.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_bar_down.Vram 	  = gsKit_vram_alloc(gsGlobal,
			 							  gsKit_texture_size(tex_bar_down.Width, tex_bar_down.Height, tex_bar_down.PSM), 
			 							  GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_bar_down);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}

	if ((pPng = pngOpenRAW(&bar_delimiter, size_bar_delimiter)) > 0) { // tex size = 0x4000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_bar_delimiter.PSM 	   = GS_PSM_CT32;
				tex_bar_delimiter.Mem 	   = (u32 *)pImgData;
				tex_bar_delimiter.VramClut = 0;
				tex_bar_delimiter.Clut 	   = NULL;
				tex_bar_delimiter.Width    = pPng->width;
				tex_bar_delimiter.Height   = pPng->height;
				tex_bar_delimiter.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_bar_delimiter.Vram 	   = gsKit_vram_alloc(gsGlobal,
			 							  		gsKit_texture_size(tex_bar_delimiter.Width, tex_bar_delimiter.Height, tex_bar_delimiter.PSM), 
			 							  		GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_bar_delimiter);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}
	
	if ((pPng = pngOpenRAW(&option_install_normal, size_option_install_normal)) > 0) { // tex size = 0x4000 
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_option[0].PSM 	   = GS_PSM_CT32;
				tex_option[0].Mem 	   = (u32 *)pImgData;
				tex_option[0].VramClut = 0;
				tex_option[0].Clut 	   = NULL;
				tex_option[0].Width    = pPng->width;
				tex_option[0].Height   = pPng->height;
				tex_option[0].Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_option[0].Vram 	   = gsKit_vram_alloc(gsGlobal,
			 				 				gsKit_texture_size(tex_option[0].Width, tex_option[0].Height, tex_option[0].PSM), 
			 				  				GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_option[0]);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}

	if ((pPng = pngOpenRAW(&option_install_multi, size_option_install_multi)) > 0) { // tex size = 0x8000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_option[1].PSM 	   = GS_PSM_CT32;
				tex_option[1].Mem 	   = (u32 *)pImgData;
				tex_option[1].VramClut = 0;
				tex_option[1].Clut 	   = NULL;
				tex_option[1].Width    = pPng->width;
				tex_option[1].Height   = pPng->height;
				tex_option[1].Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_option[1].Vram 	   = gsKit_vram_alloc(gsGlobal,
			 								gsKit_texture_size(tex_option[1].Width, tex_option[1].Height, tex_option[1].PSM), 
			 								GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_option[1]);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}

	if ((pPng = pngOpenRAW(&option_launch_fmcb, size_option_launch_fmcb)) > 0) { // tex size = 0x4000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_option[2].PSM 	   = GS_PSM_CT32;
				tex_option[2].Mem 	   = (u32 *)pImgData;
				tex_option[2].VramClut = 0;
				tex_option[2].Clut 	   = NULL;
				tex_option[2].Width    = pPng->width;
				tex_option[2].Height   = pPng->height;
				tex_option[2].Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_option[2].Vram 	   = gsKit_vram_alloc(gsGlobal,
			 								gsKit_texture_size(tex_option[2].Width, tex_option[2].Height, tex_option[2].PSM), 
			 								GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_option[2]);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}

	if ((pPng = pngOpenRAW(&option_fmcb_cfg, size_option_fmcb_cfg)) > 0) { // tex size = 0x8000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_option[3].PSM 	   = GS_PSM_CT32;
				tex_option[3].Mem 	   = (u32 *)pImgData;
				tex_option[3].VramClut = 0;
				tex_option[3].Clut 	   = NULL;
				tex_option[3].Width    = pPng->width;
				tex_option[3].Height   = pPng->height;
				tex_option[3].Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_option[3].Vram 	   = gsKit_vram_alloc(gsGlobal,
			 								gsKit_texture_size(tex_option[3].Width, tex_option[3].Height, tex_option[3].PSM), 
			 								GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_option[3]);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}

	if ((pPng = pngOpenRAW(&option_format_mc, size_option_format_mc)) > 0) { // tex size = 0x4000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_option[4].PSM 	   = GS_PSM_CT32;
				tex_option[4].Mem 	   = (u32 *)pImgData;
				tex_option[4].VramClut = 0;
				tex_option[4].Clut 	   = NULL;
				tex_option[4].Width    = pPng->width;
				tex_option[4].Height   = pPng->height;
				tex_option[4].Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_option[4].Vram 	   = gsKit_vram_alloc(gsGlobal,
			 								gsKit_texture_size(tex_option[4].Width, tex_option[4].Height, tex_option[4].PSM), 
			 								GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_option[4]);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}

	if ((pPng = pngOpenRAW(&option_uninstall, size_option_uninstall)) > 0) { // tex size = 0x8000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_option[5].PSM 	   = GS_PSM_CT32;
				tex_option[5].Mem 	   = (u32 *)pImgData;
				tex_option[5].VramClut = 0;
				tex_option[5].Clut 	   = NULL;
				tex_option[5].Width    = pPng->width;
				tex_option[5].Height   = pPng->height;
				tex_option[5].Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_option[5].Vram 	   = gsKit_vram_alloc(gsGlobal,
			 								gsKit_texture_size(tex_option[5].Width, tex_option[5].Height, tex_option[5].PSM), 
			 								GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_option[5]);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}
						
	if ((pPng = pngOpenRAW(&credits_coded, size_credits_coded)) > 0) { // tex size = 0x14000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_credits_coded.PSM 	   = GS_PSM_CT32;
				tex_credits_coded.Mem 	   = (u32 *)pImgData;
				tex_credits_coded.VramClut = 0;
				tex_credits_coded.Clut 	   = NULL;
				tex_credits_coded.Width    = pPng->width;
				tex_credits_coded.Height   = pPng->height;
				tex_credits_coded.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_credits_coded.Vram 	   = gsKit_vram_alloc(gsGlobal,
			 							  		gsKit_texture_size(tex_credits_coded.Width, tex_credits_coded.Height, tex_credits_coded.PSM), 
			 							  		GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_credits_coded);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}
		
	if ((pPng = pngOpenRAW(&credits_gui, size_credits_gui)) > 0) { // tex size = 0x8000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_credits_gui.PSM 	 = GS_PSM_CT32;
				tex_credits_gui.Mem 	 = (u32 *)pImgData;
				tex_credits_gui.VramClut = 0;
				tex_credits_gui.Clut 	 = NULL;
				tex_credits_gui.Width    = pPng->width;
				tex_credits_gui.Height   = pPng->height;
				tex_credits_gui.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_credits_gui.Vram 	 = gsKit_vram_alloc(gsGlobal,
			 							 		gsKit_texture_size(tex_credits_gui.Width, tex_credits_gui.Height, tex_credits_gui.PSM), 
			 							  		GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_credits_gui);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}
	
	if ((pPng = pngOpenRAW(&highlight, size_highlight)) > 0) { // tex size = 0x4000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_highlight.PSM 	   = GS_PSM_CT32;
				tex_highlight.Mem 	   = (u32 *)pImgData;
				tex_highlight.VramClut = 0;
				tex_highlight.Clut 	   = NULL;
				tex_highlight.Width    = pPng->width;
				tex_highlight.Height   = pPng->height;
				tex_highlight.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_highlight.Vram 	   = gsKit_vram_alloc(gsGlobal,
			 							 		gsKit_texture_size(tex_highlight.Width, tex_highlight.Height, tex_highlight.PSM), 
			 							  		GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_highlight);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}

	if ((pPng = pngOpenRAW(&highlight_bw, size_highlight_bw)) > 0) { // tex size = 0x4000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_highlight_bw.PSM 	  = GS_PSM_CT32;
				tex_highlight_bw.Mem 	  = (u32 *)pImgData;
				tex_highlight_bw.VramClut = 0;
				tex_highlight_bw.Clut 	  = NULL;
				tex_highlight_bw.Width    = pPng->width;
				tex_highlight_bw.Height   = pPng->height;
				tex_highlight_bw.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_highlight_bw.Vram 	  = gsKit_vram_alloc(gsGlobal,
			 							 		gsKit_texture_size(tex_highlight_bw.Width, tex_highlight_bw.Height, tex_highlight_bw.PSM), 
			 							  		GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_highlight_bw);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}
		
	if ((pPng = pngOpenRAW(&icon_ok, size_icon_ok)) > 0) {
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_icon_ok.PSM 	   = GS_PSM_CT32;
				tex_icon_ok.Mem 	   = (u32 *)pImgData;
				tex_icon_ok.VramClut = 0;
				tex_icon_ok.Clut 	   = NULL;
				tex_icon_ok.Width    = pPng->width;
				tex_icon_ok.Height   = pPng->height;
				tex_icon_ok.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_icon_ok.Vram 	   = gsKit_vram_alloc(gsGlobal,
		 							 		gsKit_texture_size(tex_icon_ok.Width, tex_icon_ok.Height, tex_icon_ok.PSM), 
		 							  		GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_icon_ok);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}
	
	if ((pPng = pngOpenRAW(&icon_warning, size_icon_warning)) > 0) {
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_icon_warning.PSM   	  = GS_PSM_CT32;
				tex_icon_warning.Mem 	  = (u32 *)pImgData;
				tex_icon_warning.VramClut = 0;
				tex_icon_warning.Clut 	  = NULL;
				tex_icon_warning.Width    = pPng->width;
				tex_icon_warning.Height   = pPng->height;
				tex_icon_warning.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_icon_warning.Vram 	  = gsKit_vram_alloc(gsGlobal,
		 							 			gsKit_texture_size(tex_icon_warning.Width, tex_icon_warning.Height, tex_icon_warning.PSM), 
		 							  			GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_icon_warning);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}

	if ((pPng = pngOpenRAW(&icon_error, size_icon_error)) > 0) {
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_icon_error.PSM   	= GS_PSM_CT32;
				tex_icon_error.Mem 	  	= (u32 *)pImgData;
				tex_icon_error.VramClut = 0;
				tex_icon_error.Clut 	= NULL;
				tex_icon_error.Width    = pPng->width;
				tex_icon_error.Height   = pPng->height;
				tex_icon_error.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_icon_error.Vram 	= gsKit_vram_alloc(gsGlobal,
		 							 			gsKit_texture_size(tex_icon_error.Width, tex_icon_error.Height, tex_icon_error.PSM), 
		 							  			GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_icon_error);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}
	
	if ((pPng = pngOpenRAW(&logo, size_logo)) > 0) { // tex size = 0x54000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_logo.PSM 	  = GS_PSM_CT32;
				tex_logo.Mem 	  = (u32 *)pImgData;
				tex_logo.VramClut = 0;
				tex_logo.Clut	  = NULL;
				tex_logo.Width    = pPng->width;
				tex_logo.Height   = pPng->height;
				tex_logo.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_logo.Vram 	  = gsKit_vram_alloc(gsGlobal,
	 						  			gsKit_texture_size(tex_logo.Width, tex_logo.Height, tex_logo.PSM), 
	 						  			GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_logo);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}	

	if ((pPng = pngOpenRAW(&font_verdana, size_font_verdana)) > 0) { // tex size = 0x20000
		if ((pImgData = malloc(pPng->width * pPng->height * (pPng->bit_depth / 8))) > 0) {
			if (pngReadImage( pPng, pImgData ) != -1) {
				tex_font_verdana.PSM 	   = GS_PSM_CT32;
				tex_font_verdana.Mem 	   = (u32 *)pImgData;
				tex_font_verdana.VramClut = 0;
				tex_font_verdana.Clut 	   = NULL;
				tex_font_verdana.Width    = pPng->width;
				tex_font_verdana.Height   = pPng->height;
				tex_font_verdana.Filter   = GS_FILTER_NEAREST;
				#ifdef DEBUG				
					printf("VRAM Pointer = %08x  ", gsGlobal->CurrentPointer);	
					printf("texture size = %x\n", gsKit_texture_size(pPng->width, pPng->height, GS_PSM_CT32));
				#endif	
				tex_font_verdana.Vram 	   = gsKit_vram_alloc(gsGlobal,
			 							 		gsKit_texture_size(tex_font_verdana.Width, tex_font_verdana.Height, tex_font_verdana.PSM), 
			 							  		GSKIT_ALLOC_USERBUFFER);
				gsKit_texture_upload(gsGlobal, &tex_font_verdana);
			}
			pngClose(pPng);
			free(pImgData);			
		}
	}

	#ifdef DEBUG				
		printf("Load_GUI last VRAM Pointer = %08x  \n", gsGlobal->CurrentPointer);	
	#endif	
}

//----------------------------------------------------

void gfx_set_defaults(void)
{
	// Careful !!! Must be called after Load_Texture function, and before drawing anything !
	// Init default for many gfx functions
	
	// Init common defaults to GUI, INTRO & OUTRO
	bar_delimiter_x[0] = -1;
	bar_delimiter_x[1] = 105;
	bar_delimiter_x[2] = 212;
	bar_delimiter_x[3] = 319;
	bar_delimiter_x[4] = 426;
	bar_delimiter_x[5] = 533;
	bar_delimiter_x[6] = SCREEN_WIDTH + 1;
	option_x[0] = 25;
	option_x[1] = 110;	
	option_x[2] = 240;
	option_x[3] = 330;
	option_x[4] = 453;
	option_x[5] = 538;

	// Init defaults for GUI	
	highlight_alpha = 0x80;
	logo_alpha = 0x80;			
	amount = 6;
	pause_pulse = 0;
	stop_pulse_done = 0;	
	log_alpha = 0x80;
	dialog_alpha = 0;
	control_alpha = 0;
	control_amount = 6;
	pause_control_pulse = 0;
	dialog_type = 0;
	internal_dialog_type = 0;
	dialog_icon = 0;		
	selected_dialog_button = 0;
		
	// Init defaults for INTRO & OUTRO	
	background_alpha = 0;
	up_panel_y = 0 - tex_bar_up.Height;
	down_panel_y = SCREEN_HEIGHT + tex_bar_down.Height + 1;
	logo_width = 0;
	logo_height = 0;
	logo_accel = 1.0f;
}

//----------------------------------------------------

void Setup_GS(int gs_vmode)
{
	// GS Init
	gsGlobal = gsKit_init_global_custom(gs_vmode,
		GS_RENDER_QUEUE_OS_POOLSIZE+GS_RENDER_QUEUE_OS_POOLSIZE/2, //eliminates overflow
		GS_RENDER_QUEUE_PER_POOLSIZE);
		
	// Clear Screen
	gsKit_clear(gsGlobal, Black);

	// Screen Position Init
	gsGlobal->StartX = SCREEN_X; 
	gsGlobal->StartY = SCREEN_Y;

	// Buffer Init
	gsGlobal->PrimAAEnable = GS_SETTING_ON;
	gsGlobal->DoubleBuffering = GS_SETTING_OFF;
	gsGlobal->ZBuffering      = GS_SETTING_OFF;
	gsGlobal->PSM = GS_PSM_CT32;
	gsGlobal->PSMZ = GS_PSMZ_16S;
	
	// Force Interlace and Field mode
	gsGlobal->Interlace = GS_INTERLACED;
	gsGlobal->Field     = GS_FIELD;

	// DMAC Init
	dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8, 1 << DMA_CHANNEL_GIF);
	//dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF, D_CTRL_STS_UNSPEC, D_CTRL_STD_OFF, D_CTRL_RCYC_8);
	dmaKit_chan_init(DMA_CHANNEL_GIF);
	dmaKit_chan_init(DMA_CHANNEL_FROMSPR);
	dmaKit_chan_init(DMA_CHANNEL_TOSPR);

	// Screen Init
	gsKit_init_screen(gsGlobal);
	gsKit_clear(gsGlobal, Black);	
	gsKit_mode_switch(gsGlobal, GS_ONESHOT);
}

//--------------------------------------------------------------

void Render_GUI(void)
{
	// Flips Framebuffers on VSync	
	gsKit_sync_flip(gsGlobal);

	// Normal User Draw Queue "Execution" (Kicks Oneshot and Persistent Queues)
	gsKit_queue_exec(gsGlobal);
	
	Play_Sound();		
}

//--------------------------------------------------------------		

void Clear_Screen(void)
{
	// Clear screen	
	gsKit_clear(gsGlobal, Black);
}

//--------------------------------------------------------------		

int Draw_INTRO(void)
{
	// return 1 if intro is finished
	// otherwise return 0
		
	int logo_width_grow_done = 0;	
	int logo_height_grow_done = 0;
	int up_panel_move_done = 0;
	int down_panel_move_done = 0;	
	int intro_done = 0;
	int background_fadein_done = 0;
		
	// Clear screen	
	gsKit_clear(gsGlobal, Black);
		
	// Set Alpha settings
	gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
	gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0,1,0,1,0), 0);
	gsKit_set_test(gsGlobal, GS_ATEST_OFF);

	// Calculates Logo width  & height to give grow effect
	logo_accel += (logo_accel * 0.1f);
	logo_height += logo_accel;
	logo_width += (logo_accel * 2.62f);
	
	// Check max values
	if (logo_width >= tex_logo.Width) { 
		logo_width = tex_logo.Width;
		logo_width_grow_done = 1;
	}
	if (logo_height >= tex_logo.Height) {
		logo_height = tex_logo.Height;	
		logo_height_grow_done = 1;
	}
		
	if (logo_width_grow_done && logo_height_grow_done) {
		// Here Logo grow effect is achieved

		// Calculates background alpha & Up and down panel y coordinates
		background_alpha += 3;
		up_panel_y += 2;
		down_panel_y -= 2;
		
		// Check max values
		if (up_panel_y >= 0) {
			up_panel_y = 0;		
			up_panel_move_done = 1;
		}
		if (down_panel_y <= SCREEN_HEIGHT) {
			down_panel_y = SCREEN_HEIGHT;		
			down_panel_move_done = 1;
		}
		if (background_alpha >= 0x80) {
			background_alpha = 0x80;
			background_fadein_done = 1;
		}
		
		// Draw Background
		draw_background(background_alpha);
		
		// Draw Up Panel
		draw_up_panel(up_panel_y, 0, 0, 0);
		
		// Draw Down panel
		draw_down_panel(down_panel_y);
		
		#ifdef DEBUG	
			printf("intro_background_alpha = %d\n", background_alpha);
			printf("up_panel_y = %d\n", up_panel_y);
			printf("down_panel_y = %d\n", down_panel_y);		
		#endif	
	}
		
	#ifdef DEBUG	
		if (!logo_width_grow_done || !logo_height_grow_done) {	
			printf("logo_accel = %f\n", logo_accel);
			printf("logo_width = %f\n", logo_width);
			printf("logo_height = %f\n", logo_height);	
		}
	#endif
	
	// Draw logo
	draw_logo(logo_width, logo_height, 0x80);
	
    gsKit_set_test(gsGlobal, GS_ATEST_ON);
    
    // Blend Alpha Primitives "Back To Front"
    gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);
	
    if (logo_width_grow_done && logo_height_grow_done && up_panel_move_done && down_panel_move_done && background_fadein_done)
    	intro_done = 1;     

    // Return 1 if intro have finished playing    	    
    if (intro_done) {
    	#ifdef DEBUG				
	    	printf("Intro is done\n");
	    #endif
	    
	    return 1;
    }
    return 0;    
}

//--------------------------------------------------------------		

int Draw_OUTRO(void)
{
	// return 1 if outro is finished
	// otherwise return 0
	
	int up_panel_move_done = 0;
	int down_panel_move_done = 0;	
	int outro_done = 0;
	int background_fadeout_done = 0;
		
	// Clear screen	
	gsKit_clear(gsGlobal, Black);
		
	// Set Alpha settings
	gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
	gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0,1,0,1,0), 0);
	gsKit_set_test(gsGlobal, GS_ATEST_OFF);

	// Calculates background alpha & Up and down panel y coordinates
	background_alpha -= 3;
	up_panel_y -= 2;
	down_panel_y += 2;

	// Check max values
	if (up_panel_y <= (0 - tex_bar_up.Height)) {
		up_panel_y = 0 - tex_bar_up.Height;		
		up_panel_move_done = 1;
	}
	if (down_panel_y >= (SCREEN_HEIGHT + tex_bar_down.Height + 1)) {
		down_panel_y = (SCREEN_HEIGHT + tex_bar_down.Height + 1);		
		down_panel_move_done = 1;
	}
	if (background_alpha <= 0x00) {
		background_alpha = 0x00;
		background_fadeout_done = 1;
	}
		
	#ifdef DEBUG	
		if (!up_panel_move_done || !down_panel_move_done || !background_fadeout_done) {
			printf("outro_background_alpha = %d\n", background_alpha);
			printf("up_panel_y = %d\n", up_panel_y);
			printf("down_panel_y = %d\n", down_panel_y);		
		}
	#endif
		
	// Draw Background
	draw_background(background_alpha);
		
	// Draw Up Panel
	draw_up_panel(up_panel_y, 0, 0, 0);
		
	// Draw Down panel
	draw_down_panel(down_panel_y);
	
    gsKit_set_test(gsGlobal, GS_ATEST_ON);
    
    // Blend Alpha Primitives "Back To Front"
    gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);
    	
    if (up_panel_move_done && down_panel_move_done && background_fadeout_done)
    	outro_done = 1;     

    // Return 1 if outro have finished playing    	    
    if (outro_done) {
    	#ifdef DEBUG				
	    	printf("Outro is done\n");
	    #endif

	    // Just to Play OUTRO more than Once
	    up_panel_y = 0;		
	    down_panel_y = SCREEN_HEIGHT;		
	    background_alpha = 0x80;
	    
	    return 1;
    }
    return 0;    
}

//--------------------------------------------------------------

int Draw_GUI(int logo, int selected_button, int highlight_pulse, int highlight_blw, int log, int dialog)
{
	// return 1 if logo is faded out and pulse is stopped at max
	// otherwise return 0
	
	int fade_logo_done = 0;
	int fadein_log_done = 0;
	int fadeout_log_done = 0;	
	int fadein_dialog_done = 0;
	int fadeout_dialog_done = 0;
	int i;
		
	// Clear screen	
	gsKit_clear(gsGlobal, Black);
		
	// Set Alpha settings
	gsGlobal->PrimAlphaEnable = GS_SETTING_ON;
	gsKit_set_primalpha(gsGlobal, GS_SETREG_ALPHA(0,1,0,1,0), 0);
	gsKit_set_test(gsGlobal, GS_ATEST_OFF);

	// Draw background	
	draw_background(0x80);
								
	// Draw Up panel
	draw_up_panel(0, selected_button, highlight_pulse, highlight_blw);
	
	// Draw down panel							
	draw_down_panel(SCREEN_HEIGHT);
							
	
	// Alpha calculation to control logo fade-in and fade-out		
	if (logo) {			
		if (logo_alpha < 0x80) {
			logo_alpha += 5;	
			if (logo_alpha > 0x80) logo_alpha = 0x80; 		
		}
	}
	else {
		fade_logo_done = 0;
		if (logo_alpha >= 0) {
			logo_alpha -= 5;	
			if (logo_alpha <= 0) {
				logo_alpha = 0; 	
				fade_logo_done = 1;
			}
		}
		else fade_logo_done = 1;
	}
	
	// Draw logo if needed	
 	if ((!stop_pulse_done || !fade_logo_done) || (logo))	{
	 	draw_logo(tex_logo.Width, tex_logo.Height, logo_alpha);
	}
				
	if (log) {
		// Alpha calculation to control log fade-in & out
		if (dialog) {
			fadeout_log_done = 0;
			if (log_alpha >= 0x06) {
				log_alpha -= 5;
				if (log_alpha <= 0x06) {
					log_alpha = 0x06; 		
					fadeout_log_done = 1;
				}
			}
		}
		else {
			fadein_log_done = 0;
			if (log_alpha <= 0x80) {
				log_alpha += 5;
				if (log_alpha >= 0x80) {
					log_alpha = 0x80;
					fadein_log_done = 1; 	
				}	
			}
		}
		
		// Draw log if needed
		draw_log(log_alpha);
	}

	fadeout_dialog_done = 0;		
	fadein_dialog_done = 0;							
	
	if (dialog) {
		// Alpha calculation to control dialog fade-in & out
		if (dialog_alpha <= 0x80) {
			dialog_alpha += 5;
			if (dialog_alpha >= 0x80) {
				dialog_alpha = 0x80; 		
				fadein_dialog_done = 1;
			}
		}
	}
	else {
		if (dialog_alpha >= 0) {
			dialog_alpha -= 5;
			if (dialog_alpha <= 0) {
				dialog_alpha = 0; 		
				fadeout_dialog_done = 1;
			}
		}
	}
	
	// While dialog is not completely faded-in, control alpha is equal to dialog alpha
	if (!fadein_dialog_done) {
		control_alpha = dialog_alpha;
	}
	else {
		// Calculate dialog control alpha to make a pulse
		control_alpha += control_amount;
		if (control_alpha >= 0xff) {
			control_alpha = 0xff;
			pause_control_pulse++;
			if (pause_control_pulse >= 12) {
				control_amount = -6;
				pause_control_pulse = 0;
			}
		} else if (control_alpha <= 0x40) {
			control_amount = 6;
			control_alpha = 0x40;
		}
	}
	
	// To fade-out dialog if it was printed
	if (!dialog && dialog_type > 0) {
		if (fadeout_dialog_done) {
			// Clean dialog
			dialog_type = 0;
			internal_dialog_type = 0;
			dialog_icon = 0;
			for (i=0; i<MAX_DIALOG_LINES; i++)
				dialog_buffer[i] = NULL;
		}
	}
	else {
		// In all other cases change dialog type  
		internal_dialog_type = dialog_type;
	}
		
	// Draw dialog
	draw_dialog(dialog_alpha, control_alpha);	
	
				
    gsKit_set_test(gsGlobal, GS_ATEST_ON);
    
    // Blend Alpha Primitives "Back To Front"
    gsKit_set_primalpha(gsGlobal, GS_BLEND_BACK2FRONT, 0);

   
    // if dialog is ON, log is ON and log and dialogs faded : Return 1
    // if dialog is ON, log is OFF and dialog faded : Return 1 else return 0
    if (dialog) {
	    if (log) {
	    	if (fadeout_log_done && fadein_dialog_done) {
    			#ifdef DEBUG				
	    			printf("Log is faded-out and dialog faded in\n");
	    		#endif
	    		return 1;
    		}
		}
    	else {
	    	if (fadein_dialog_done) {
    			#ifdef DEBUG				
	    			printf("Dialog-only faded in\n");
	    		#endif
	    		return 1;
    		}
    		return 0;
		}
    }
    
    // here dialog is OFF
    // if log is ON and dialog is faded out and log is faded in, return 1
    // if log is ON and else (above) return 0  
    if (log) { 
    	if (fadein_log_done && fadeout_dialog_done) {
   			#ifdef DEBUG				
    			printf("Log is faded-in and dialog faded out\n");
    		#endif
    		return 1;
   		}
   		return 0;
   	}
   	
    // here dialog is 0, log is 0
    // dialog IS fading out return 0
	if (dialog_type > 0) 
   		return 0;    
    
	// here dialog is 0, log is 0 and dialog is faded out   		
    // Return 1 if logo is faded-out and highlight pulse is at maximum
    if (!logo) {
    	if (stop_pulse_done && fade_logo_done) {
    		#ifdef DEBUG				
	    		printf("Logo is faded-out and highlight pulse is at maximum\n");
	    	#endif
	    	stop_pulse_done = 0;	
	    	fade_logo_done = 0;	
	    	
	    	return 1;
    	}
	}
	else {
    	if (stop_pulse_done) {
    		#ifdef DEBUG				
	    		printf("Highlight pulse is at maximum\n");
	    	#endif
	    	stop_pulse_done = 0;	
	    	
	    	return 1;
    	}
	}
	
    return 0;
}

//--------------------------------------------------------------
