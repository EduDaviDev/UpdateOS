/* ============================================================
 * updgr.c - UpdateOS Default Graphics Render
 * ============================================================ */

#include "updgr.h"
#include "math.h"
#include "drivers/video.h"

/* ============================================================ */
/*  Estado global                                              */
/* ============================================================ */

gfx_font_t current_font = {0};

#define UPDGR_MAX_NODES 256

/* Clip rect (inclusivo). x1 < x0 => inválido, precisa reset. */
static int clip_x0 = 0;
static int clip_y0 = 0;
static int clip_x1 = -1;
static int clip_y1 = -1;

static inline void clip_ensure(void)
{
	if (clip_x1 < clip_x0)
	{
		clip_x0 = 0;
		clip_y0 = 0;
		clip_x1 = (int)gfx_width  - 1;
		clip_y1 = (int)gfx_height - 1;
	}
}

/* ============================================================ */
/*  Info                                                       */
/* ============================================================ */

uint32_t gfx_get_width(void)  { return gfx_width;  }
uint32_t gfx_get_height(void) { return gfx_height; }
uint32_t gfx_get_bpp(void)    { return gfx_bpp;    }

/* ============================================================ */
/*  Cores                                                      */
/* ============================================================ */

uint32_t gfx_blend(uint32_t fg, uint32_t bg, uint8_t alpha)
{
	uint32_t a  = alpha;
	uint32_t ia = 255u - a;

	uint32_t r = (((fg >> 16) & 0xFF) * a + ((bg >> 16) & 0xFF) * ia) / 255u;
	uint32_t g = (((fg >>  8) & 0xFF) * a + ((bg >>  8) & 0xFF) * ia) / 255u;
	uint32_t b = (( fg        & 0xFF) * a + ( bg        & 0xFF) * ia) / 255u;

	return (r << 16) | (g << 8) | b;
}

/* ============================================================ */
/*  Clip                                                       */
/* ============================================================ */

void gfx_set_clip(int x, int y, int w, int h)
{
	clip_ensure();

	int x1 = x + w - 1;
	int y1 = y + h - 1;

	if (x  < clip_x0) x  = clip_x0;
	if (y  < clip_y0) y  = clip_y0;
	if (x1 > clip_x1) x1 = clip_x1;
	if (y1 > clip_y1) y1 = clip_y1;

	if (x > x1 || y > y1)
	{
		/* Clip vazio */
		clip_x0 = 0; clip_y0 = 0; clip_x1 = -1; clip_y1 = -1;
		return;
	}

	clip_x0 = x;
	clip_y0 = y;
	clip_x1 = x1;
	clip_y1 = y1;
}

void gfx_reset_clip(void)
{
	clip_x0 = 0;
	clip_y0 = 0;
	clip_x1 = (int)gfx_width  - 1;
	clip_y1 = (int)gfx_height - 1;
}

void gfx_get_clip(int *x, int *y, int *w, int *h)
{
	clip_ensure();
	if (x) *x = clip_x0;
	if (y) *y = clip_y0;
	if (w) *w = clip_x1 - clip_x0 + 1;
	if (h) *h = clip_y1 - clip_y0 + 1;
}

/* ============================================================ */
/*  put() inline — respeita clip                               */
/* ============================================================ */

static inline void put(int x, int y, uint32_t c)
{
	uint32_t *bb = gfx_bb_addr;
	if (!bb) return;
	if (x < clip_x0 || x > clip_x1) return;
	if (y < clip_y0 || y > clip_y1) return;
	c &= 0x00FFFFFFu;
	uint32_t *p = &bb[(uint32_t)y * gfx_width + (uint32_t)x];
	if (*p != c) *p = c;
}

static inline int updgr_absdiff(int a, int b) { return a > b ? a - b : b - a; }

/* Clamp de rect contra o clip. Retorna 0 se vazio. */
static int clip_rect(int *x, int *y, int *w, int *h)
{
	clip_ensure();
	int x0 = *x, y0 = *y;
	int x1 = x0 + *w - 1;
	int y1 = y0 + *h - 1;
	if (x0 < clip_x0) x0 = clip_x0;
	if (y0 < clip_y0) y0 = clip_y0;
	if (x1 > clip_x1) x1 = clip_x1;
	if (y1 > clip_y1) y1 = clip_y1;
	if (x0 > x1 || y0 > y1) return 0;
	*x = x0; *y = y0;
	*w = x1 - x0 + 1; *h = y1 - y0 + 1;
	return 1;
}

/* ============================================================ */
/*  Básico                                                     */
/* ============================================================ */

void gfx_putpixel(int x, int y, uint32_t color)
{
	clip_ensure();
	put(x, y, color);
}

uint32_t gfx_getpixel(int x, int y)
{
	uint32_t *bb = gfx_bb_addr;
	if (!bb) return 0;
	if (x < 0 || y < 0) return 0;
	if ((uint32_t)x >= gfx_width || (uint32_t)y >= gfx_height) return 0;
	return bb[(uint32_t)y * gfx_width + (uint32_t)x] & 0x00FFFFFFu;
}

void gfx_clear(uint32_t color)
{
	uint32_t *bb = gfx_bb_addr;
	if (!bb) return;
	clip_ensure();

	uint32_t c = color & 0x00FFFFFFu;
	for (int y = clip_y0; y <= clip_y1; y++)
	{
		uint32_t *row = bb + (uint32_t)y * gfx_width + (uint32_t)clip_x0;
		int n = clip_x1 - clip_x0 + 1;
		for (int i = 0; i < n; i++)
		{
			if (row[i] != c) row[i] = c;
		}
	}
}

void gfx_refresh(void)
{
	if (!gfx_bb_addr || !gfx_fb_addr) return;

	if (gfx_bpp == 32)
	{
		for (uint32_t y = 0; y < gfx_height; y++)
		{
			uint8_t  *dst = gfx_fb_addr + y * gfx_pitch;
			uint32_t *src = gfx_bb_addr + y * gfx_width;
			__builtin_memcpy(dst, src, gfx_width * 4);
		}
		return;
	}

	if (gfx_bpp == 24)
	{
		for (uint32_t y = 0; y < gfx_height; y++)
		{
			uint8_t  *dst = gfx_fb_addr + y * gfx_pitch;
			uint32_t *src = gfx_bb_addr + y * gfx_width;
			for (uint32_t x = 0; x < gfx_width; x++)
			{
				uint32_t c = src[x];
				dst[x*3+0] = (uint8_t)( c        & 0xFF);
				dst[x*3+1] = (uint8_t)((c >>  8) & 0xFF);
				dst[x*3+2] = (uint8_t)((c >> 16) & 0xFF);
			}
		}
		return;
	}

	/* 16/8bpp: copia direta em little-endian (aproximação). */
	for (uint32_t y = 0; y < gfx_height; y++)
	{
		uint8_t  *dst = gfx_fb_addr + y * gfx_pitch;
		uint32_t *src = gfx_bb_addr + y * gfx_width;
		uint32_t bpp_bytes = gfx_bpp / 8;
		for (uint32_t x = 0; x < gfx_width; x++)
		{
			uint32_t c = src[x];
			for (uint32_t b = 0; b < bpp_bytes; b++)
				dst[x * bpp_bytes + b] = (uint8_t)(c >> (8 * b));
		}
	}
}

/* ============================================================ */
/*  Linhas                                                     */
/* ============================================================ */

void gfx_draw_line(int x0, int y0, int x1, int y1, uint32_t color)
{
	if (!gfx_bb_addr) return;
	clip_ensure();

	int adx = updgr_absdiff(x1, x0);
	int ady = updgr_absdiff(y1, y0);
	int sx  = (x0 < x1) ? 1 : -1;
	int sy  = (y0 < y1) ? 1 : -1;
	int err = adx - ady;

	for (;;)
	{
		put(x0, y0, color);
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 > -ady) { err -= ady; x0 += sx; }
		if (e2 <  adx) { err += adx; y0 += sy; }
	}
}

void gfx_draw_hline(int x, int y, int w, uint32_t color)
{
	if (!gfx_bb_addr || w <= 0) return;
	clip_ensure();

	int x1 = x + w - 1;
	if (x  < clip_x0) x  = clip_x0;
	if (x1 > clip_x1) x1 = clip_x1;
	if (y  < clip_y0 || y > clip_y1) return;
	if (x  > x1) return;

	uint32_t c = color & 0x00FFFFFFu;
	uint32_t *row = gfx_bb_addr + (uint32_t)y * gfx_width;
	for (int i = x; i <= x1; i++)
		if (row[i] != c) row[i] = c;
}

void gfx_draw_vline(int x, int y, int h, uint32_t color)
{
	if (!gfx_bb_addr || h <= 0) return;
	clip_ensure();

	int y1 = y + h - 1;
	if (y  < clip_y0) y  = clip_y0;
	if (y1 > clip_y1) y1 = clip_y1;
	if (x  < clip_x0 || x > clip_x1) return;
	if (y  > y1) return;

	uint32_t c = color & 0x00FFFFFFu;
	uint32_t *bb = gfx_bb_addr;
	for (int j = y; j <= y1; j++)
	{
		uint32_t *p = bb + (uint32_t)j * gfx_width + (uint32_t)x;
		if (*p != c) *p = c;
	}
}

/* ============================================================ */
/*  Retângulos                                                 */
/* ============================================================ */

void gfx_draw_rect(int x, int y, int width, int height, uint32_t color)
{
	if (!gfx_bb_addr || width <= 0 || height <= 0) return;
	clip_ensure();

	int x1 = x + width  - 1;
	int y1 = y + height - 1;

	gfx_draw_hline(x,  y,  width, color);
	gfx_draw_hline(x,  y1, width, color);
	gfx_draw_vline(x,  y,  height, color);
	gfx_draw_vline(x1, y,  height, color);
}

void gfx_fill_rect(int x, int y, int width, int height, uint32_t color)
{
	if (!gfx_bb_addr) return;
	if (!clip_rect(&x, &y, &width, &height)) return;

	uint32_t c = color & 0x00FFFFFFu;
	uint32_t *bb = gfx_bb_addr;

	for (int j = 0; j < height; j++)
	{
		uint32_t *row = bb + (uint32_t)(y + j) * gfx_width + (uint32_t)x;
		for (int i = 0; i < width; i++)
			if (row[i] != c) row[i] = c;
	}
}

void gfx_fill_rect_grad_v(int x, int y, int w, int h, uint32_t c0, uint32_t c1)
{
	if (!gfx_bb_addr || w <= 0 || h <= 0) return;
	if (!clip_rect(&x, &y, &w, &h)) return;

	for (int j = 0; j < h; j++)
	{
		uint8_t a = (uint8_t)((j * 255) / (h > 1 ? h - 1 : 1));
		uint32_t c = gfx_blend(c1, c0, a);
		uint32_t *row = gfx_bb_addr + (uint32_t)(y + j) * gfx_width + (uint32_t)x;
		for (int i = 0; i < w; i++)
			if (row[i] != c) row[i] = c;
	}
}

void gfx_fill_rect_grad_h(int x, int y, int w, int h, uint32_t c0, uint32_t c1)
{
	if (!gfx_bb_addr || w <= 0 || h <= 0) return;
	if (!clip_rect(&x, &y, &w, &h)) return;

	for (int j = 0; j < h; j++)
	{
		uint32_t *row = gfx_bb_addr + (uint32_t)(y + j) * gfx_width + (uint32_t)x;
		for (int i = 0; i < w; i++)
		{
			uint8_t a = (uint8_t)((i * 255) / (w > 1 ? w - 1 : 1));
			uint32_t c = gfx_blend(c1, c0, a);
			if (row[i] != c) row[i] = c;
		}
	}
}

/* ============================================================ */
/*  Círculos                                                   */
/* ============================================================ */

void gfx_draw_circle(int x, int y, int radius, uint32_t color)
{
	if (!gfx_bb_addr || radius < 0) return;
	clip_ensure();

	int px = 0, py = radius, d = 3 - 2 * radius;
	while (px <= py)
	{
		put(x + px, y + py, color);
		put(x - px, y + py, color);
		put(x + px, y - py, color);
		put(x - px, y - py, color);
		put(x + py, y + px, color);
		put(x - py, y + px, color);
		put(x + py, y - px, color);
		put(x - py, y - px, color);
		if (d < 0) d += 4 * px + 6;
		else { d += 4 * (px - py) + 10; py--; }
		px++;
	}
}

void gfx_fill_circle(int x, int y, int radius, uint32_t color)
{
	if (!gfx_bb_addr || radius < 0) return;
	clip_ensure();

	long r2 = (long)radius * (long)radius;
	uint32_t c = color & 0x00FFFFFFu;

	for (int dy = -radius; dy <= radius; dy++)
	{
		long rem = r2 - (long)dy * (long)dy;
		if (rem < 0) continue;
		int dx = (int)sqrt((double)rem);

		int yy = y + dy;
		if (yy < clip_y0 || yy > clip_y1) continue;

		int x0 = x - dx;
		int x1 = x + dx;
		if (x0 < clip_x0) x0 = clip_x0;
		if (x1 > clip_x1) x1 = clip_x1;
		if (x0 > x1) continue;

		uint32_t *row = gfx_bb_addr + (uint32_t)yy * gfx_width;
		for (int xx = x0; xx <= x1; xx++)
			if (row[xx] != c) row[xx] = c;
	}
}

/* ============================================================ */
/*  Elipses                                                    */
/* ============================================================ */

void gfx_draw_ellipse(int x, int y, int width, int height, uint32_t color)
{
	long a = width / 2;
	long b = height / 2;
	if (!gfx_bb_addr || a <= 0 || b <= 0) return;
	clip_ensure();

	int cx = x + (int)a;
	int cy = y + (int)b;

	long a2 = a * a;
	long b2 = b * b;

	long px = 0, py = b;
	long sigma = 2 * b2 + a2 * (1 - 2 * b);

	while (b2 * px <= a2 * py)
	{
		put(cx + (int)px, cy + (int)py, color);
		put(cx - (int)px, cy + (int)py, color);
		put(cx + (int)px, cy - (int)py, color);
		put(cx - (int)px, cy - (int)py, color);
		if (sigma >= 0) { sigma += 4 * a2 * (1 - py); py--; }
		sigma += b2 * (4 * px + 6);
		px++;
	}

	px = a; py = 0;
	sigma = 2 * a2 + b2 * (1 - 2 * a);

	while (a2 * py <= b2 * px)
	{
		put(cx + (int)px, cy + (int)py, color);
		put(cx - (int)px, cy + (int)py, color);
		put(cx + (int)px, cy - (int)py, color);
		put(cx - (int)px, cy - (int)py, color);
		if (sigma >= 0) { sigma += 4 * b2 * (1 - px); px--; }
		sigma += a2 * (4 * py + 6);
		py++;
	}
}

void gfx_fill_ellipse(int x, int y, int width, int height, uint32_t color)
{
	long a = width / 2;
	long b = height / 2;
	if (!gfx_bb_addr || a <= 0 || b <= 0) return;
	clip_ensure();

	int cx = x + (int)a;
	int cy = y + (int)b;

	double a2 = (double)a * (double)a;
	double b2 = (double)b * (double)b;
	uint32_t c = color & 0x00FFFFFFu;

	for (long dy = -b; dy <= b; dy++)
	{
		double rem = b2 - (double)dy * (double)dy;
		if (rem < 0.0) continue;
		long dx = (long)sqrt(a2 * rem / b2);

		int yy = cy + (int)dy;
		if (yy < clip_y0 || yy > clip_y1) continue;

		int x0 = cx - (int)dx;
		int x1 = cx + (int)dx;
		if (x0 < clip_x0) x0 = clip_x0;
		if (x1 > clip_x1) x1 = clip_x1;
		if (x0 > x1) continue;

		uint32_t *row = gfx_bb_addr + (uint32_t)yy * gfx_width;
		for (int xx = x0; xx <= x1; xx++)
			if (row[xx] != c) row[xx] = c;
	}
}

/* ============================================================ */
/*  Triângulos                                                 */
/* ============================================================ */

void gfx_draw_triangle(int x0, int y0, int x1, int y1,
                       int x2, int y2, uint32_t color)
{
	gfx_draw_line(x0, y0, x1, y1, color);
	gfx_draw_line(x1, y1, x2, y2, color);
	gfx_draw_line(x2, y2, x0, y0, color);
}

/* Scanline: para cada Y, acha x_min e x_max nas 3 arestas. */
void gfx_fill_triangle(int x0, int y0, int x1, int y1,
                       int x2, int y2, uint32_t color)
{
	if (!gfx_bb_addr) return;
	clip_ensure();

	int ymin = y0, ymax = y0;
	if (y1 < ymin) ymin = y1;
	if (y2 < ymin) ymin = y2;
	if (y1 > ymax) ymax = y1;
	if (y2 > ymax) ymax = y2;

	uint32_t c = color & 0x00FFFFFFu;

	for (int y = ymin; y <= ymax; y++)
	{
		if (y < clip_y0 || y > clip_y1) continue;

		int xs[3];
		int n = 0;

		/* Arestas: (0,1), (1,2), (2,0) */
		int ex[3] = { x0, x1, x2 };
		int ey[3] = { y0, y1, y2 };

		for (int e = 0; e < 3; e++)
		{
			int i = e, j = (e + 1) % 3;
			int yi = ey[i], yj = ey[j];
			int xi = ex[i], xj = ex[j];

			if ((yi < y && yj >= y) || (yj < y && yi >= y))
			{
				long num = (long)(y - yi) * (xj - xi);
				long den = (long)(yj - yi);
				xs[n++] = xi + (int)(num / den);
			}
		}

		if (n < 2) continue;

		int xmin = xs[0], xmax = xs[0];
		for (int i = 1; i < n; i++)
		{
			if (xs[i] < xmin) xmin = xs[i];
			if (xs[i] > xmax) xmax = xs[i];
		}

		if (xmin < clip_x0) xmin = clip_x0;
		if (xmax > clip_x1) xmax = clip_x1;
		if (xmin > xmax) continue;

		uint32_t *row = gfx_bb_addr + (uint32_t)y * gfx_width;
		for (int xx = xmin; xx <= xmax; xx++)
			if (row[xx] != c) row[xx] = c;
	}
}

/* ============================================================ */
/*  Polígonos                                                  */
/* ============================================================ */

void gfx_draw_polygon(const int positions[][2], int count, uint32_t color)
{
	if (!gfx_bb_addr || count < 2) return;

	for (int i = 0; i < count; i++)
	{
		int j = (i + 1) % count;
		gfx_draw_line(positions[i][0], positions[i][1],
		              positions[j][0], positions[j][1], color);
	}
}

void gfx_fill_polygon(const int positions[][2], int count, uint32_t color)
{
	if (!gfx_bb_addr || count < 3) return;
	clip_ensure();

	int ymin = positions[0][1];
	int ymax = positions[0][1];
	for (int i = 1; i < count; i++)
	{
		if (positions[i][1] < ymin) ymin = positions[i][1];
		if (positions[i][1] > ymax) ymax = positions[i][1];
	}

	int nodes[UPDGR_MAX_NODES];
	uint32_t c = color & 0x00FFFFFFu;

	for (int y = ymin; y <= ymax; y++)
	{
		if (y < clip_y0 || y > clip_y1) continue;

		int node_count = 0;
		int j = count - 1;
		for (int i = 0; i < count; i++)
		{
			int yi = positions[i][1], yj = positions[j][1];
			int xi = positions[i][0], xj = positions[j][0];

			if ((yi < y && yj >= y) || (yj < y && yi >= y))
			{
				if (node_count < UPDGR_MAX_NODES)
				{
					long num = (long)(y - yi) * (xj - xi);
					long den = (long)(yj - yi);
					nodes[node_count++] = xi + (int)(num / den);
				}
			}
			j = i;
		}

		/* insertion sort */
		for (int a = 1; a < node_count; a++)
		{
			int key = nodes[a];
			int b = a - 1;
			while (b >= 0 && nodes[b] > key)
			{
				nodes[b + 1] = nodes[b];
				b--;
			}
			nodes[b + 1] = key;
		}

		uint32_t *row = gfx_bb_addr + (uint32_t)y * gfx_width;
		for (int a = 0; a + 1 < node_count; a += 2)
		{
			int x0 = nodes[a];
			int x1 = nodes[a + 1];
			if (x0 < clip_x0) x0 = clip_x0;
			if (x1 > clip_x1) x1 = clip_x1;
			for (int xx = x0; xx <= x1; xx++)
				if (row[xx] != c) row[xx] = c;
		}
	}
}

/* ============================================================ */
/*  Blit                                                       */
/* ============================================================ */

void gfx_blit(gfx_surface_t *src, int dx, int dy)
{
	if (!gfx_bb_addr || !src || !src->pixels) return;
	clip_ensure();

	for (int sy = 0; sy < src->height; sy++)
	{
		int ty = dy + sy;
		if (ty < clip_y0 || ty > clip_y1) continue;

		uint32_t *srow = src->pixels + (uint32_t)sy * src->width;
		uint32_t *drow = gfx_bb_addr + (uint32_t)ty * gfx_width;

		for (int sx = 0; sx < src->width; sx++)
		{
			int tx = dx + sx;
			if (tx < clip_x0 || tx > clip_x1) continue;
			uint32_t c = srow[sx] & 0x00FFFFFFu;
			if (drow[tx] != c) drow[tx] = c;
		}
	}
}

void gfx_blit_colorkey(gfx_surface_t *src, int dx, int dy, uint32_t key)
{
	if (!gfx_bb_addr || !src || !src->pixels) return;
	clip_ensure();

	key &= 0x00FFFFFFu;

	for (int sy = 0; sy < src->height; sy++)
	{
		int ty = dy + sy;
		if (ty < clip_y0 || ty > clip_y1) continue;

		uint32_t *srow = src->pixels + (uint32_t)sy * src->width;
		uint32_t *drow = gfx_bb_addr + (uint32_t)ty * gfx_width;

		for (int sx = 0; sx < src->width; sx++)
		{
			int tx = dx + sx;
			if (tx < clip_x0 || tx > clip_x1) continue;
			uint32_t c = srow[sx] & 0x00FFFFFFu;
			if (c == key) continue;
			if (drow[tx] != c) drow[tx] = c;
		}
	}
}

void gfx_blit_alpha(gfx_surface_t *src, int dx, int dy, uint8_t alpha)
{
	if (!gfx_bb_addr || !src || !src->pixels) return;
	if (alpha == 0) return;
	clip_ensure();

	for (int sy = 0; sy < src->height; sy++)
	{
		int ty = dy + sy;
		if (ty < clip_y0 || ty > clip_y1) continue;

		uint32_t *srow = src->pixels + (uint32_t)sy * src->width;
		uint32_t *drow = gfx_bb_addr + (uint32_t)ty * gfx_width;

		for (int sx = 0; sx < src->width; sx++)
		{
			int tx = dx + sx;
			if (tx < clip_x0 || tx > clip_x1) continue;

			uint32_t c = srow[sx] & 0x00FFFFFFu;
			if (alpha < 255)
				c = gfx_blend(c, drow[tx] & 0x00FFFFFFu, alpha);

			if (drow[tx] != c) drow[tx] = c;
		}
	}
}

/* ============================================================ */
/*  Fontes bitmap                                              */
/* ============================================================ */

/* Desenha um único glifo. Retorna a largura avançada (f->width). */
static int draw_glyph(int cx, int cy, unsigned char ch, uint32_t c)
{
	gfx_font_t *f = &current_font;
	if (!f->code) return 0;

	int row_bytes = (f->width + 7) / 8;
	int char_size = row_bytes * f->height;

	if (ch < f->first_char || ch > f->last_char) return f->width;
	int idx = ch - f->first_char;
	if (idx >= f->charcount) return f->width;

	const uint8_t *glyph = f->code + idx * char_size;

	const int flip_h = (f->flags & GFX_FLAG_FLIP_H) ? 1 : 0;
	const int flip_v = (f->flags & GFX_FLAG_FLIP_V) ? 1 : 0;

	for (int gy = 0; gy < f->height; gy++)
	{
		int yy = cy + gy;
		if (yy < clip_y0 || yy > clip_y1) continue;

		int src_y = flip_v ? (f->height - 1 - gy) : gy;
		const uint8_t *grow = glyph + src_y * row_bytes;
		uint32_t *brow = gfx_bb_addr + (uint32_t)yy * gfx_width;

		for (int gx = 0; gx < f->width; gx++)
		{
			int src_x = flip_h ? (f->width - 1 - gx) : gx;
			int bit = 7 - (src_x & 7);
			if (!(grow[src_x >> 3] & (1u << bit))) continue;

			int xx = cx + gx;
			if (xx < clip_x0 || xx > clip_x1) continue;
			if (brow[xx] != c) brow[xx] = c;
		}
	}
	return f->width;
}

void gfx_print_char(int x, int y, char ch, uint32_t color)
{
	if (!gfx_bb_addr || !current_font.code) return;
	clip_ensure();
	draw_glyph(x, y, (unsigned char)ch, color & 0x00FFFFFFu);
}

void gfx_print(int x, int y, const char *s, uint32_t color)
{
	gfx_font_t *f = &current_font;
	if (!gfx_bb_addr || !f->code || f->width <= 0 || f->height <= 0 || !s)
		return;
	clip_ensure();

	uint32_t c = color & 0x00FFFFFFu;
	int cx = x, cy = y;

	for (const char *p = s; *p; ++p)
	{
		unsigned char ch = (unsigned char)*p;

		if (ch == '\n') { cx = x; cy += f->height; continue; }
		if (ch == '\r') { cx = x; continue; }

		draw_glyph(cx, cy, ch, c);
		cx += f->width;
	}
}

int gfx_text_width(const char *s)
{
	gfx_font_t *f = &current_font;
	if (!f->width || !s) return 0;

	int maxw = 0, cur = 0;
	for (const char *p = s; *p; ++p)
	{
		if (*p == '\n') { if (cur > maxw) maxw = cur; cur = 0; continue; }
		if (*p == '\r') { cur = 0; continue; }
		cur += f->width;
	}
	if (cur > maxw) maxw = cur;
	return maxw;
}

int gfx_text_height(void)
{
	return current_font.height;
}

void gfx_setfont(gfx_font_t font) { current_font = font; }

void gfx_setfpack(gfx_fpack_t fpack, int font)
{
	if (!fpack.fonts || fpack.fontcount <= 0) return;
	if (font < 0 || font >= fpack.fontcount) return;
	current_font = fpack.fonts[font];
}