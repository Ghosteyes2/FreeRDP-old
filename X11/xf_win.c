
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xf_win.h"
#include "xf_event.h"
#include "xf_colour.h"

static uint8 hatch_patterns[] = {
	0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00,	/* 0 - bsHorizontal */
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,	/* 1 - bsVertical */
	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01,	/* 2 - bsFDiagonal */
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,	/* 3 - bsBDiagonal */
	0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08,	/* 4 - bsCross */
	0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81	/* 5 - bsDiagCross */
};

static int
xf_set_rop3(xfInfo * xfi, int rop3)
{
	switch (rop3)
	{
		case 0x00: /* 0 */
			XSetFunction(xfi->display, xfi->gc, GXclear);
			break;		
		case 0x5a: /* D^P */
			XSetFunction(xfi->display, xfi->gc, GXxor);
			break;
		case 0x66: /* D^S */
			XSetFunction(xfi->display, xfi->gc, GXxor);
			break;
		case 0x88: /* D&S */
			XSetFunction(xfi->display, xfi->gc, GXand);
			break;
		case 0xcc: /* S */
			XSetFunction(xfi->display, xfi->gc, GXcopy);
			break;
		case 0xf0: /* P */
			XSetFunction(xfi->display, xfi->gc, GXcopy);
			break;		
		default:
			printf("xf_set_rop3: unknonw rop3 %x\n", rop3);
			break;
	}
	return 0;
}

static void
l_ui_error(struct rdp_inst * inst, char * text)
{
	printf("ui_error: %s", text);
}

static void
l_ui_warning(struct rdp_inst * inst, char * text)
{
	printf("ui_warning: %s\n", text);
}

static void
l_ui_unimpl(struct rdp_inst * inst, char * text)
{
	printf("ui_unimpl: %s\n", text);
}

static void
l_ui_begin_update(struct rdp_inst * inst)
{
	//printf("ui_begin_update: inst %p\n", inst);
}

static void
l_ui_end_update(struct rdp_inst * inst)
{
	//printf("ui_end_update: %p\n", inst);
}

static void
l_ui_desktop_save(struct rdp_inst * inst, int offset, int x, int y,
	int cx, int cy)
{
	printf("ui_desktop_save:\n");
}

static void
l_ui_desktop_restore(struct rdp_inst * inst, int offset, int x, int y,
	int cx, int cy)
{
	printf("ui_desktop_restore:\n");
}

static RD_HGLYPH
l_ui_create_glyph(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	xfInfo * xfi;
	XImage * image;
	Pixmap bitmap;
	int scanline;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_create_glyph: width %d height %d\n", width, height);
	scanline = (width + 7) / 8;
	bitmap = XCreatePixmap(xfi->display, xfi->wnd, width, height, 1);
	if (xfi->create_glyph_gc == 0)
	{
		xfi->create_glyph_gc = XCreateGC(xfi->display, bitmap, 0, NULL);
	}
	image = XCreateImage(xfi->display, xfi->visual, 1, ZPixmap, 0, (char *) data,
			     width, height, 8, scanline);
	image->byte_order = MSBFirst;
	image->bitmap_bit_order = MSBFirst;
	XInitImage(image);
	XPutImage(xfi->display, bitmap, xfi->create_glyph_gc, image, 0, 0, 0, 0, width, height);
	XFree(image);
	return (RD_HGLYPH) bitmap;
}

static void
l_ui_destroy_glyph(struct rdp_inst * inst, RD_HGLYPH glyph)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_destroy_glyph:\n");
	XFreePixmap(xfi->display, (Pixmap) glyph);
}

static RD_HBITMAP
l_ui_create_bitmap(struct rdp_inst * inst, int width, int height, uint8 * data)
{
	XImage * image;
	Pixmap bitmap;
	xfInfo * xfi;
	uint8 * cdata;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_create_bitmap: inst %p width %d height %d\n", inst, width, height);
	bitmap = XCreatePixmap(xfi->display, xfi->wnd, width, height, xfi->depth);
	if (xfi->create_bitmap_gc == 0)
	{
		xfi->create_bitmap_gc = XCreateGC(xfi->display, bitmap, 0, NULL);
	}
	cdata = xf_image_convert(xfi, inst->settings, width, height, data);
	image = XCreateImage(xfi->display, xfi->visual, xfi->depth, ZPixmap, 0,
		(char *) cdata, width, height, xfi->bitmap_pad, 0);
	XPutImage(xfi->display, bitmap, xfi->create_bitmap_gc, image, 0, 0, 0, 0, width, height);
	XFree(image);
	if (cdata != data)
	{
		free(cdata);
	}
	return (RD_HBITMAP) bitmap;
}

static void
l_ui_paint_bitmap(struct rdp_inst * inst, int x, int y, int cx, int cy, int width,
	int height, uint8 * data)
{
	XImage * image;
	xfInfo * xfi;
	uint8 * cdata;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_paint_bitmap: xfi->drw %p\n", (void *) (xfi->drw));
	XSetFunction(xfi->display, xfi->gc, GXcopy);
	XSetFillStyle(xfi->display, xfi->gc, FillSolid);
	cdata = xf_image_convert(xfi, inst->settings, width, height, data);
	image = XCreateImage(xfi->display, xfi->visual, xfi->depth, ZPixmap, 0,
		(char *) cdata, width, height, xfi->bitmap_pad, 0);
	XPutImage(xfi->display, xfi->backstore, xfi->gc, image, 0, 0, x, y, cx, cy);
	XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc, x, y, cx, cy, x, y);
	XFree(image);
	if (cdata != data)
	{
		free(cdata);
	}
}

static void
l_ui_destroy_bitmap(struct rdp_inst * inst, RD_HBITMAP bmp)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_destroy_bitmap:\n");
	XFreePixmap(xfi->display, (Pixmap) bmp);
}

static void
l_ui_line(struct rdp_inst * inst, uint8 opcode, int startx, int starty, int endx,
	int endy, RD_PEN * pen)
{
	printf("ui_line:\n");
}

static void
l_ui_rect(struct rdp_inst * inst, int x, int y, int cx, int cy, int colour)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	colour = xf_colour_convert(xfi, inst->settings, colour);
	//printf("ui_rect: inst %p x %d y %d cx %d cy %d colour %d\n", inst, x, y, cx, cy, colour);
	XSetFunction(xfi->display, xfi->gc, GXcopy);
	XSetFillStyle(xfi->display, xfi->gc, FillSolid);
	XSetForeground(xfi->display, xfi->gc, colour);
	XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
	if (xfi->drw == xfi->backstore)
	{
		XFillRectangle(xfi->display, xfi->wnd, xfi->gc, x, y, cx, cy);
	}
}

static void
l_ui_polygon(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, RD_POINT * point,
	int npoints, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_polygon:\n");
}

static void
l_ui_polyline(struct rdp_inst * inst, uint8 opcode, RD_POINT * points, int npoints,
	RD_PEN * pen)
{
	printf("ui_polyline:\n");
}

static void
l_ui_ellipse(struct rdp_inst * inst, uint8 opcode, uint8 fillmode, int x, int y,
	int cx, int cy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_ellipse:\n");
}

static void
l_ui_start_draw_glyphs(struct rdp_inst * inst, int bgcolour, int fgcolour)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	bgcolour = xf_colour_convert(xfi, inst->settings, bgcolour);
	fgcolour = xf_colour_convert(xfi, inst->settings, fgcolour);
	//printf("ui_start_draw_glyphs:\n");
	XSetFunction(xfi->display, xfi->gc, GXcopy);
	XSetForeground(xfi->display, xfi->gc, fgcolour);
	XSetBackground(xfi->display, xfi->gc, bgcolour);
	XSetFillStyle(xfi->display, xfi->gc, FillStippled);
}

static void
l_ui_draw_glyph(struct rdp_inst * inst, int x, int y, int cx, int cy,
	RD_HGLYPH glyph)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_draw_glyph:\n");
	XSetStipple(xfi->display, xfi->gc, (Pixmap) glyph);
	XSetTSOrigin(xfi->display, xfi->gc, x, y);
	XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
}

static void
l_ui_end_draw_glyphs(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_end_draw_glyphs:\n");
	if (xfi->drw == xfi->backstore)
	{
		XSetFillStyle(xfi->display, xfi->gc, FillSolid);
		XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc, x, y, cx, cy, x, y);
	}
}

static uint32
l_ui_get_toggle_keys_state(struct rdp_inst * inst)
{
	printf("ui_get_toggle_keys_state:\n");
	return 0;
}

static void
l_ui_bell(struct rdp_inst * inst)
{
	printf("ui_bell:\n");
}

static void
l_ui_destblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_destblt: xfi->drw %p opcode %d x %d y %d cx %d cy %d\n", xfi->drw, opcode, x, y, cx, cy);
	xf_set_rop3(xfi, opcode);
	XSetFillStyle(xfi->display, xfi->gc, FillSolid);
	XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
	if (xfi->drw == xfi->backstore)
	{
		XFillRectangle(xfi->display, xfi->wnd, xfi->gc, x, y, cx, cy);
	}
}

static void
l_ui_patblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	xfInfo * xfi;
	Pixmap fill;
	uint8 i, ipattern[8];

	xfi = (xfInfo *) (inst->param1);
	fgcolour = xf_colour_convert(xfi, inst->settings, fgcolour);
	bgcolour = xf_colour_convert(xfi, inst->settings, bgcolour);
	//printf("ui_patblt: style %d brush->bd %p\n", brush->style, brush->bd);
	//if (brush->bd != 0)
	//{
	//	printf("brush->bd->colour_code %d\n", brush->bd->colour_code);
	//}
	xf_set_rop3(xfi, opcode);
	switch (brush->style)
	{
		case 0:	/* Solid */
			XSetFillStyle(xfi->display, xfi->gc, FillSolid);
			XSetForeground(xfi->display, xfi->gc, fgcolour);
			XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
			if (xfi->drw == xfi->backstore)
			{
				XFillRectangle(xfi->display, xfi->wnd, xfi->gc, x, y, cx, cy);
			}
			return;
		case 2:	/* Hatch */
			fill = (Pixmap) l_ui_create_glyph(inst, 8, 8,
							hatch_patterns + brush->pattern[0] * 8);
			XSetForeground(xfi->display, xfi->gc, fgcolour);
			XSetBackground(xfi->display, xfi->gc, bgcolour);
			XSetFillStyle(xfi->display, xfi->gc, FillOpaqueStippled);
			XSetStipple(xfi->display, xfi->gc, fill);
			XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
			XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
			l_ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			break;
		case 3:	/* Pattern */
			if (brush->bd == 0)	/* rdp4 brush */
			{
				for (i = 0; i != 8; i++)
					ipattern[7 - i] = brush->pattern[i];
				fill = (Pixmap) l_ui_create_glyph(inst, 8, 8, ipattern);
				XSetForeground(xfi->display, xfi->gc, bgcolour);
				XSetBackground(xfi->display, xfi->gc, fgcolour);
				XSetFillStyle(xfi->display, xfi->gc, FillOpaqueStippled);
				XSetStipple(xfi->display, xfi->gc, fill);
				XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
				XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
				l_ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			else if (brush->bd->colour_code > 1)	/* > 1 bpp */
			{
				fill = (Pixmap) l_ui_create_bitmap(inst, 8, 8, brush->bd->data);
				XSetFillStyle(xfi->display, xfi->gc, FillTiled);
				XSetTile(xfi->display, xfi->gc, fill);
				XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
				XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
				l_ui_destroy_bitmap(inst, (RD_HBITMAP) fill);
			}
			else
			{
				fill = (Pixmap) l_ui_create_glyph(inst, 8, 8, brush->bd->data);
				XSetForeground(xfi->display, xfi->gc, bgcolour);
				XSetBackground(xfi->display, xfi->gc, fgcolour);
				XSetFillStyle(xfi->display, xfi->gc, FillOpaqueStippled);
				XSetStipple(xfi->display, xfi->gc, fill);
				XSetTSOrigin(xfi->display, xfi->gc, brush->xorigin, brush->yorigin);
				XFillRectangle(xfi->display, xfi->drw, xfi->gc, x, y, cx, cy);
				l_ui_destroy_glyph(inst, (RD_HGLYPH) fill);
			}
			break;
		default:
			printf("brush %d\n", brush->style);
			break;
	}
	if (xfi->drw == xfi->backstore)
	{
		XSetFunction(xfi->display, xfi->gc, GXcopy);
		XSetFillStyle(xfi->display, xfi->gc, FillSolid);
		XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc, x, y, cx, cy, x, y);
	}
}

static void
l_ui_screenblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	int srcx, int srcy)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_screenblt:\n");
	xf_set_rop3(xfi, opcode);
	XSetFillStyle(xfi->display, xfi->gc, FillSolid);
	XCopyArea(xfi->display, xfi->backstore, xfi->drw, xfi->gc, srcx, srcy, cx, cy, x, y);
	if (xfi->drw == xfi->backstore)
	{
		if (xfi->unobscured)
		{
			XCopyArea(xfi->display, xfi->wnd, xfi->wnd, xfi->gc, srcx, srcy, cx, cy, x, y);
		}
		else
		{
			XCopyArea(xfi->display, xfi->backstore, xfi->wnd, xfi->gc, x, y, cx, cy, x, y);
		}
	}
}

static void
l_ui_memblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_memblt: xfi->drw %p, opcode, %d, x %d, y %d, cx %d, cy %d, src %p srcx %d srcy %d\n",
	//	xfi->drw, opcode, x, y, cx, cy, src, srcx, srcy);
	xf_set_rop3(xfi, opcode);
	XSetFillStyle(xfi->display, xfi->gc, FillSolid);
	XCopyArea(xfi->display, (Pixmap) src, xfi->drw, xfi->gc, srcx, srcy, cx, cy, x, y);
	if (xfi->drw == xfi->backstore)
	{
		XCopyArea(xfi->display, (Pixmap) src, xfi->wnd, xfi->gc, srcx, srcy, cx, cy, x, y);
	}
}

static void
l_ui_triblt(struct rdp_inst * inst, uint8 opcode, int x, int y, int cx, int cy,
	RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolour, int fgcolour)
{
	printf("ui_triblt:\n");
}

static int
l_ui_select(struct rdp_inst * inst, int rdp_socket)
{
	//printf("ui_select: inst %p\n", inst);
	return 1;
}

static void
l_ui_set_clip(struct rdp_inst * inst, int x, int y, int cx, int cy)
{
	xfInfo * xfi;
	XRectangle clip_rect;

	xfi = (xfInfo *) (inst->param1);
	clip_rect.x = x;
	clip_rect.y = y;
	clip_rect.width = cx;
	clip_rect.height = cy;
	XSetClipRectangles(xfi->display, xfi->gc, 0, 0, &clip_rect, 1, YXBanded);
}

static void
l_ui_reset_clip(struct rdp_inst * inst)
{
	xfInfo * xfi;
	XRectangle clip_rect;

	xfi = (xfInfo *) (inst->param1);
	clip_rect.x = 0;
	clip_rect.y = 0;
	clip_rect.width = 4096;
	clip_rect.height = 4096;
	XSetClipRectangles(xfi->display, xfi->gc, 0, 0, &clip_rect, 1, YXBanded);
}

static void
l_ui_resize_window(struct rdp_inst * inst)
{
	printf("ui_resize_window:\n");
}

static void
l_ui_set_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
	printf("ui_set_cursor: inst %p\n", inst);
}

static void
l_ui_destroy_cursor(struct rdp_inst * inst, RD_HCURSOR cursor)
{
	printf("ui_destroy_cursor:\n");
}

static RD_HCURSOR
l_ui_create_cursor(struct rdp_inst * inst, unsigned int x, unsigned int y,
	int width, int height, uint8 * andmask, uint8 * xormask)
{
	printf("ui_create_cursor:\n");
	return (RD_HCURSOR) 1;
}

static void
l_ui_set_null_cursor(struct rdp_inst * inst)
{
	printf("ui_set_null_cursor:\n");
}

static void
l_ui_set_default_cursor(struct rdp_inst * inst)
{
	printf("ui_set_default_cursor:\n");
}

static RD_HCOLOURMAP
l_ui_create_colourmap(struct rdp_inst * inst, RD_COLOURMAP * colours)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	printf("ui_create_colourmap:\n");
	return xf_create_colourmap(xfi, inst->settings, colours);
}

static void
l_ui_move_pointer(struct rdp_inst * inst, int x, int y)
{
	printf("ui_move_pointer:\n");
}

static void
l_ui_set_colourmap(struct rdp_inst * inst, RD_HCOLOURMAP map)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	printf("ui_set_colourmap:\n");
	xf_set_colourmap(xfi, inst->settings, map);
}

static RD_HBITMAP
l_ui_create_surface(struct rdp_inst * inst, int width, int height, RD_HBITMAP old)
{
	Pixmap new;
	Window root;
	int x, y;
	uint32 oldwidth, oldheight, bw, dp;
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	new = XCreatePixmap(xfi->display, xfi->wnd, width, height, xfi->depth);
	if (old != 0)
	{
		XGetGeometry(xfi->display, (Drawable) old, &root, &x, &y, &oldwidth, &oldheight,
			&bw, &dp);
		if (oldwidth < width)
		{
			width = oldwidth;
		}
		if (oldheight < height)
		{
			height = oldheight;
		}
		XSetFunction(xfi->display, xfi->gc, GXcopy);
		XSetFillStyle(xfi->display, xfi->gc, FillSolid);
		XCopyArea(xfi->display, (Drawable) old, new, xfi->gc, 0, 0,
			width, height, 0, 0);
		XFreePixmap(xfi->display, (Pixmap) old);
	}
	if (xfi->drw == (Drawable) old)
	{
		xfi->drw = new;
	}
	//printf("ui_create_surface: returning %p xfi->drw %p\n", new, xfi->drw);
	return (RD_HBITMAP) new;
}

static void
l_ui_set_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	if (surface != 0)
	{
		xfi->drw = (Drawable) surface;
	}
	else
	{
		xfi->drw = xfi->backstore;
	}
	//printf("ui_set_surface: xfi->drw %p xfi->backstore %p\n", (void *) (xfi->drw), (void *) (xfi->backstore));
}

static void
l_ui_destroy_surface(struct rdp_inst * inst, RD_HBITMAP surface)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	//printf("ui_destroy_surface:\n");
	if (xfi->drw == (Drawable) surface)
	{
		printf("l_ui_destroy_surface: freeing active surface!");
		xfi->drw = xfi->backstore;
	}
	if (surface != 0)
	{
		XFreePixmap(xfi->display, (Pixmap) surface);
	}
}

static int
xf_assign_callbacks(rdpInst * inst)
{
	inst->ui_error = l_ui_error;
	inst->ui_warning = l_ui_warning;
	inst->ui_unimpl = l_ui_unimpl;
	inst->ui_begin_update = l_ui_begin_update;
	inst->ui_end_update = l_ui_end_update;
	inst->ui_desktop_save = l_ui_desktop_save;
	inst->ui_desktop_restore = l_ui_desktop_restore;
	inst->ui_create_bitmap = l_ui_create_bitmap;
	inst->ui_paint_bitmap = l_ui_paint_bitmap;
	inst->ui_destroy_bitmap = l_ui_destroy_bitmap;
	inst->ui_line = l_ui_line;
	inst->ui_rect = l_ui_rect;
	inst->ui_polygon = l_ui_polygon;
	inst->ui_polyline = l_ui_polyline;
	inst->ui_ellipse = l_ui_ellipse;
	inst->ui_start_draw_glyphs = l_ui_start_draw_glyphs;
	inst->ui_draw_glyph = l_ui_draw_glyph;
	inst->ui_end_draw_glyphs = l_ui_end_draw_glyphs;
	inst->ui_get_toggle_keys_state = l_ui_get_toggle_keys_state;
	inst->ui_bell = l_ui_bell;
	inst->ui_destblt = l_ui_destblt;
	inst->ui_patblt = l_ui_patblt;
	inst->ui_screenblt = l_ui_screenblt;
	inst->ui_memblt = l_ui_memblt;
	inst->ui_triblt = l_ui_triblt;
	inst->ui_create_glyph = l_ui_create_glyph;
	inst->ui_destroy_glyph = l_ui_destroy_glyph;
	inst->ui_select = l_ui_select;
	inst->ui_set_clip = l_ui_set_clip;
	inst->ui_reset_clip = l_ui_reset_clip;
	inst->ui_resize_window = l_ui_resize_window;
	inst->ui_set_cursor = l_ui_set_cursor;
	inst->ui_destroy_cursor = l_ui_destroy_cursor;
	inst->ui_create_cursor = l_ui_create_cursor;
	inst->ui_set_null_cursor = l_ui_set_null_cursor;
	inst->ui_set_default_cursor = l_ui_set_default_cursor;
	inst->ui_create_colourmap = l_ui_create_colourmap;
	inst->ui_move_pointer = l_ui_move_pointer;
	inst->ui_set_colourmap = l_ui_set_colourmap;
	inst->ui_create_surface = l_ui_create_surface;
	inst->ui_set_surface = l_ui_set_surface;
	inst->ui_destroy_surface = l_ui_destroy_surface;
	return 0;
}

static int
xf_get_pixmap_info(rdpInst * inst, xfInfo * xfi)
{
	int vi_count;
	int pf_count;
	int index;
	XVisualInfo * vis;
	XVisualInfo * vi;
	XVisualInfo template;
	XPixmapFormatValues * pfs;
	XPixmapFormatValues * pf;

	pfs = XListPixmapFormats(xfi->display, &pf_count);
	if (pfs == NULL)
	{
		printf("xf_get_pixmap_info: XListPixmapFormats failed\n");
		return 1;
	}
	for (index = 0; index < pf_count; index++)
	{
		pf = pfs + index;
		if (pf->depth == xfi->depth)
		{
			xfi->bitmap_pad = pf->scanline_pad;
			xfi->bpp = pf->bits_per_pixel;
			break;
		}
	}
	XFree(pfs);
	memset(&template, 0, sizeof(template));
	template.class = TrueColor;
	template.screen = xfi->screen_num;
	vis = XGetVisualInfo(xfi->display, VisualClassMask | VisualScreenMask, &template,
		&vi_count);
	if (vis == NULL)
	{
		printf("xf_get_pixmap_info: XGetVisualInfo failed\n");
		return 1;
	}
	for (index = 0; index < vi_count; index++)
	{
		vi = vis + index;
		if (vi->depth == xfi->depth)
		{
			xfi->visual = vi->visual;
			break;
		}
	}
	XFree(vis);
	if ((xfi->visual == NULL) || (xfi->bitmap_pad == 0))
	{
		return 1;
	}
	return 0;
}

int
xf_pre_connect(rdpInst * inst)
{
	xfInfo * xfi;

	xf_assign_callbacks(inst);
	xfi = (xfInfo *) malloc(sizeof(xfInfo));
	inst->param1 = xfi;
	memset(xfi, 0, sizeof(xfInfo));
	xfi->display = XOpenDisplay(NULL);
	if (xfi->display == NULL)
	{
		printf("xf_init: failed to open display: %s\n", XDisplayName(NULL));
		return 1;
	}
	xfi->x_socket = ConnectionNumber(xfi->display);
	xfi->screen_num = DefaultScreen(xfi->display);
	xfi->screen = ScreenOfDisplay(xfi->display, xfi->screen_num);
	xfi->depth = DefaultDepthOfScreen(xfi->screen);
	xfi->xserver_be = (ImageByteOrder(xfi->display) == MSBFirst);
	return 0;
}

int
xf_post_connect(rdpInst * inst)
{
	xfInfo * xfi;
	XEvent xevent;
	int input_mask;
	int width;
	int height;

	xfi = (xfInfo *) (inst->param1);
	if (xf_get_pixmap_info(inst, xfi) != 0)
	{
		return 1;
	}
	width = inst->settings->width;
	height = inst->settings->height;
	xfi->wnd = XCreateSimpleWindow(xfi->display, RootWindowOfScreen(xfi->screen),
		 0, 0, width, height, 0, 0, 0);
	XStoreName(xfi->display, xfi->wnd, "freerdp");
	input_mask =
		KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask |
		VisibilityChangeMask | FocusChangeMask | StructureNotifyMask |
		PointerMotionMask | ExposureMask | EnterWindowMask | LeaveWindowMask;
	XSelectInput(xfi->display, xfi->wnd, input_mask);
	XMapWindow(xfi->display, xfi->wnd);
	/* wait for VisibilityNotify */
	do
	{
		XMaskEvent(xfi->display, VisibilityChangeMask, &xevent);
	}
	while (xevent.type != VisibilityNotify);
	xfi->unobscured = xevent.xvisibility.state == VisibilityUnobscured;
	xfi->gc = XCreateGC(xfi->display, xfi->wnd, 0, NULL);
	xfi->backstore = XCreatePixmap(xfi->display, xfi->wnd,
		width, height, xfi->depth);
	XSetForeground(xfi->display, xfi->gc, BlackPixelOfScreen(xfi->screen));
	XFillRectangle(xfi->display, xfi->backstore, xfi->gc, 0, 0, width, height);
	xfi->drw = xfi->backstore;
	return 0;
}

void
xf_deinit(rdpInst * inst)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	XCloseDisplay(xfi->display);
	free(xfi);
}

int
xf_get_fds(rdpInst * inst, void ** read_fds, int * read_count,
	void ** write_fds, int * write_count)
{
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	read_fds[*read_count] = (void *) (xfi->x_socket);
	(*read_count)++;
	return 0;
}

int
xf_check_fds(rdpInst * inst)
{
	XEvent xevent;
	xfInfo * xfi;

	xfi = (xfInfo *) (inst->param1);
	while (XPending(xfi->display))
	{
		memset(&xevent, 0, sizeof(xevent));
		XNextEvent(xfi->display, &xevent);
		//printf("-----------------got event %d\n", xevent.type);
		if (xf_handle_event(inst, xfi, &xevent) != 0)
		{
			return 1;
		}
	}
	return 0;
}
