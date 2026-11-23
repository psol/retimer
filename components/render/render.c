/*
 * Construction de l'image à afficher. Voir render.h.
 *
 * Les chiffres sont dessinés en segments (afficheur à 7 segments), à la
 * taille voulue, sans police ; les libellés utilisent la petite police 5 × 7.
 */
#include "render.h"

#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "font.h"

/* ---- Primitives --------------------------------------------------------- */

static void plot(render_image_t *im, int x, int y, bool black)
{
    if (x < 0 || y < 0 || x >= RENDER_WIDTH || y >= RENDER_HEIGHT) {
        return;
    }
    uint8_t *o = &im->bytes[y * RENDER_BYTES_PER_ROW + x / 8];
    uint8_t bit = (uint8_t)(0x80u >> (x % 8));
    if (black) {
        *o |= bit;
    } else {
        *o &= (uint8_t)~bit;
    }
}

bool render_pixel(const render_image_t *im, int x, int y)
{
    if (x < 0 || y < 0 || x >= RENDER_WIDTH || y >= RENDER_HEIGHT) {
        return false;
    }
    return (im->bytes[y * RENDER_BYTES_PER_ROW + x / 8] &
            (0x80u >> (x % 8))) != 0;
}

static void rectangle(render_image_t *im, int x, int y, int l, int h)
{
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + l; i++) {
            plot(im, i, j, true);
        }
    }
}

/* Inverse noir et blanc entre les lignes y0 et y1 incluses (alertes). */
static void invert_rows(render_image_t *im, int y0, int y1)
{
    for (int y = y0; y <= y1; y++) {
        for (int i = 0; i < RENDER_BYTES_PER_ROW; i++) {
            im->bytes[y * RENDER_BYTES_PER_ROW + i] ^= 0xFF;
        }
    }
}

/* ---- Chiffres à segments ------------------------------------------------ */

/*
 * Segments : a (haut), b (haut droite), c (bas droite), d (bas),
 * e (bas gauche), f (haut gauche), g (milieu).
 */
enum { SA = 1, SB = 2, SC = 4, SD = 8, SE = 16, SF = 32, SG = 64 };

static const uint8_t SEGMENTS[10] = {
    SA | SB | SC | SD | SE | SF,      /* 0 */
    SB | SC,                          /* 1 */
    SA | SB | SG | SE | SD,           /* 2 */
    SA | SB | SG | SC | SD,           /* 3 */
    SF | SG | SB | SC,                /* 4 */
    SA | SF | SG | SC | SD,           /* 5 */
    SA | SF | SG | SE | SC | SD,      /* 6 */
    SA | SB | SC,                     /* 7 */
    SA | SB | SC | SD | SE | SF | SG, /* 8 */
    SA | SB | SC | SD | SF | SG,      /* 9 */
};

typedef struct {
    int width;     /* d'un chiffre */
    int height;    /* d'un chiffre */
    int thickness; /* d'un segment */
    int space;     /* entre deux caractères */
    int colon; /* largeur du « : » */
} digit_size_t;

/* Segment horizontal à bouts biseautés, de x à x + l - 1, centré sur yc. */
static void segment_h(render_image_t *im, int x, int yc, int l, int e)
{
    int half = e / 2;
    for (int dy = -half; dy <= half; dy++) {
        int inset = dy < 0 ? -dy : dy;
        for (int i = x + inset; i < x + l - inset; i++) {
            plot(im, i, yc + dy, true);
        }
    }
}

/* Segment vertical à bouts biseautés, de y à y + h - 1, centré sur xc. */
static void segment_v(render_image_t *im, int xc, int y, int h, int e)
{
    int half = e / 2;
    for (int dx = -half; dx <= half; dx++) {
        int inset = dx < 0 ? -dx : dx;
        for (int j = y + inset; j < y + h - inset; j++) {
            plot(im, xc + dx, j, true);
        }
    }
}

static void digit(render_image_t *im, int x, int y, uint8_t segs,
                  const digit_size_t *t)
{
    int e = t->thickness;
    int half = e / 2;
    int l = t->width;
    int h = t->height;
    int gap = 1; /* écart entre segments voisins */
    int top = y + half;
    int middle = y + h / 2;
    int bottom = y + h - 1 - half;
    int left = x + half;
    int right = x + l - 1 - half;

    if (segs & SA) {
        segment_h(im, left + gap, top, right - left - 2 * gap + 1, e);
    }
    if (segs & SG) {
        segment_h(im, left + gap, middle, right - left - 2 * gap + 1, e);
    }
    if (segs & SD) {
        segment_h(im, left + gap, bottom, right - left - 2 * gap + 1, e);
    }
    if (segs & SF) {
        segment_v(im, left, top + gap, middle - top - 2 * gap + 1, e);
    }
    if (segs & SB) {
        segment_v(im, right, top + gap, middle - top - 2 * gap + 1, e);
    }
    if (segs & SE) {
        segment_v(im, left, middle + gap, bottom - middle - 2 * gap + 1, e);
    }
    if (segs & SC) {
        segment_v(im, right, middle + gap, bottom - middle - 2 * gap + 1, e);
    }
}

static int char_width(char c, const digit_size_t *t)
{
    return c == ':' ? t->colon : t->width;
}

static int digits_text_width(const char *text,
                             const digit_size_t *t)
{
    int l = 0;
    for (const char *p = text; *p; p++) {
        if (p != text) {
            l += t->space;
        }
        l += char_width(*p, t);
    }
    return l;
}

/* Dessine « 12:34 », « 1:05:00 » ou « 12:-- » ; x est le bord gauche. */
static void digits_text(render_image_t *im, int x, int y, const char *text,
                        const digit_size_t *t)
{
    for (const char *p = text; *p; p++) {
        char c = *p;
        if (c >= '0' && c <= '9') {
            digit(im, x, y, SEGMENTS[c - '0'], t);
        } else if (c == '-') {
            digit(im, x, y, SG, t);
        } else if (c == ':') {
            int side = t->thickness;
            int cx = x + (t->colon - side) / 2;
            rectangle(im, cx, y + t->height * 3 / 10 - side / 2, side, side);
            rectangle(im, cx, y + t->height * 7 / 10 - side / 2, side, side);
        }
        x += char_width(c, t) + t->space;
    }
}

/* Plus grande taille, parmi la liste, qui tient dans la largeur donnée. */
static const digit_size_t *size_for(const char *text,
                                    const digit_size_t *sizes,
                                    int count, int max_width)
{
    for (int i = 0; i < count; i++) {
        if (digits_text_width(text, &sizes[i]) <= max_width) {
            return &sizes[i];
        }
    }
    return &sizes[count - 1];
}

/* Tailles de la minuterie globale, de la plus grande à la plus petite. */
static const digit_size_t TOTAL_SIZES[] = {
    {34, 66, 8, 6, 12},
    {26, 54, 7, 5, 11},
    {20, 46, 6, 4, 9},
};

/* Tailles de la minuterie rappel. */
static const digit_size_t REMINDER_SIZES[] = {
    {21, 44, 6, 4, 10},
    {13, 34, 4, 3, 7},
};

#define SIZE_COUNT(t) ((int)(sizeof(t) / sizeof((t)[0])))

/* ---- Texte -------------------------------------------------------------- */

/* Décode un caractère UTF-8 ; avance *p. */
static uint32_t utf8_next(const char **p)
{
    const unsigned char *s = (const unsigned char *)*p;
    uint32_t c = s[0];
    if (c < 0x80) {
        *p += 1;
        return c;
    }
    if ((c & 0xE0) == 0xC0 && s[1] != 0) {
        *p += 2;
        return ((c & 0x1F) << 6) | (s[1] & 0x3F);
    }
    *p += 1;
    return '?';
}

static int utf8_length(const char *text)
{
    int n = 0;
    const char *p = text;
    while (*p) {
        utf8_next(&p);
        n++;
    }
    return n;
}

/* Largeur d'un texte en police 5 × 7 à l'échelle donnée. */
static int text_width(const char *text, int scale)
{
    int n = utf8_length(text);
    return n > 0 ? (n * (FONT_WIDTH + 1) - 1) * scale : 0;
}

/*
 * Écrit un texte ; y est le haut des majuscules (les accents débordent de
 * 2 lignes au-dessus).
 */
static void text(render_image_t *im, int x, int y, const char *t, int scale)
{
    const char *p = t;
    while (*p) {
        const uint8_t *g = font_glyph(utf8_next(&p));
        if (g != NULL) {
            for (int row = 0; row < FONT_HEIGHT; row++) {
                for (int col = 0; col < FONT_WIDTH; col++) {
                    if (g[row] & (0x10u >> col)) {
                        rectangle(im, x + col * scale,
                                  y + (row - 2) * scale, scale,
                                  scale);
                    }
                }
            }
        }
        x += (FONT_WIDTH + 1) * scale;
    }
}

static void text_centered(render_image_t *im, int y, const char *t, int scale)
{
    text(im, (RENDER_WIDTH - text_width(t, scale)) / 2, y, t, scale);
}

static void text_right(render_image_t *im, int x_right, int y,
                       const char *t, int scale)
{
    text(im, x_right - text_width(t, scale), y, t, scale);
}

/* ---- Format des durées -------------------------------------------------- */

void render_format_duration(int64_t duration_ms, bool reduced, char *out)
{
    if (duration_ms < 0) {
        duration_ms = 0;
    }
    if (duration_ms > CST_TOTAL_MAX_MS) {
        duration_ms = CST_TOTAL_MAX_MS;
    }
    if (reduced) {
        /* MAT-04 : minutes arrondies au-dessus, sans les secondes. */
        int minutes = (int)((duration_ms + MINUTE_MS - 1) / MINUTE_MS);
        int h = (minutes / 60) % 100;
        int m = minutes % 60;
        if (h > 0) {
            snprintf(out, 10, "%d:%02d:--", h, m);
        } else {
            snprintf(out, 10, "%02d:--", m);
        }
        return;
    }
    int seconds = (int)((duration_ms + SECOND_MS - 1) / SECOND_MS);
    int h = (seconds / 3600) % 100;
    int m = (seconds / 60) % 60;
    int s = seconds % 60;
    if (h > 0) {
        snprintf(out, 10, "%d:%02d:%02d", h, m, s);
    } else {
        snprintf(out, 10, "%02d:%02d", m, s);
    }
}

/* ---- Écrans ------------------------------------------------------------- */

#define MARGIN 6
#define REMINDER_ZONE_Y (RENDER_SEPARATOR_Y + 2)

/* ERR-07 : petite pile barrée en haut à droite. */
static void battery_indicator(render_image_t *im)
{
    int x = RENDER_WIDTH - MARGIN - 26;
    int y = MARGIN;
    /* Contour 24 × 12 et borne. */
    rectangle(im, x, y, 24, 2);
    rectangle(im, x, y + 10, 24, 2);
    rectangle(im, x, y, 2, 12);
    rectangle(im, x + 22, y, 2, 12);
    rectangle(im, x + 24, y + 3, 2, 6);
    /* Charge presque vide. */
    rectangle(im, x + 4, y + 4, 4, 4);
    /* Point d'exclamation à gauche. */
    rectangle(im, x - 6, y, 3, 8);
    rectangle(im, x - 6, y + 10, 3, 2);
}

static void total_zone(render_image_t *im, const ktimer_view_t *view,
                       bool mask)
{
    text(im, MARGIN, MARGIN + 2, "TOTAL", 2);
    if (mask) {
        return; /* DEM-02 : phase éteinte du clignotement */
    }
    char t[10];
    render_format_duration(view->total_ms, view->reduced, t);
    const digit_size_t *size = size_for(
        t, TOTAL_SIZES, SIZE_COUNT(TOTAL_SIZES),
        RENDER_WIDTH - 2 * MARGIN);
    int l = digits_text_width(t, size);
    int zone_top = MARGIN + 2 + 14 + 4;
    int y = zone_top + (RENDER_SEPARATOR_Y - zone_top - size->height) / 2;
    digits_text(im, (RENDER_WIDTH - l) / 2, y, t, size);
}

/*
 * Zone du rappel : libellé à gauche, centré verticalement ; valeur à droite,
 * dans la place qui reste.
 */
static void reminder_zone(render_image_t *im, const ktimer_view_t *view,
                          bool mask)
{
    int top = REMINDER_ZONE_Y;
    int height = RENDER_HEIGHT - top;
    int middle = top + height / 2;
    text(im, MARGIN, middle - 7, "RAPPEL", 2);
    if (mask) {
        return;
    }
    int value_left = MARGIN + text_width("RAPPEL", 2) + 8;
    int available_width = RENDER_WIDTH - MARGIN - value_left;
    switch (view->reminder) {
    case REMINDER_OFF:
    case REMINDER_SPENT:
        /*
         * ECH-07 : plus d'échéance de rappel avant la fin. Même affichage
         * que le rappel à l'arrêt : « APRÈS LA FIN » déroutait (essai du
         * 23 novembre 2026).
         */
        text_right(im, RENDER_WIDTH - MARGIN, middle - 10, "ARRÊT", 3);
        return;
    case REMINDER_SET:
    case REMINDER_RUNNING:
    case REMINDER_ALERT: {
        char t[10];
        bool reduced = view->reminder == REMINDER_RUNNING && view->reduced;
        render_format_duration(view->reminder_ms, reduced, t);
        const digit_size_t *size =
            size_for(t, REMINDER_SIZES, SIZE_COUNT(REMINDER_SIZES),
                     available_width);
        int l = digits_text_width(t, size);
        digits_text(im, RENDER_WIDTH - MARGIN - l,
                    middle - size->height / 2, t, size);
        return;
    }
    }
}

static void idle_screen(render_image_t *im)
{
    /* MAT-12, ETA-08 : l'e-ink garde ce message sans consommer. */
    text_centered(im, 34, "RETIMER", 4);
    text_centered(im, 84, "EN VEILLE", 2);
    rectangle(im, 0, RENDER_SEPARATOR_Y, RENDER_WIDTH, 2);
    text_centered(im, REMINDER_ZONE_Y + 18, "MOLETTE :", 2);
    text_centered(im, REMINDER_ZONE_Y + 40, "RÉGLER", 2);
}

static void finished_screen(render_image_t *im, const ktimer_view_t *view)
{
    /* ETA-07 : « 00:00 ». */
    total_zone(im, view, false);
    rectangle(im, 0, RENDER_SEPARATOR_Y, RENDER_WIDTH, 2);
    text_centered(im, REMINDER_ZONE_Y + 20, "FIN", 5);
}

void render_message(render_image_t *im, const char *line1,
                    const char *line2)
{
    memset(im->bytes, 0, sizeof im->bytes);
    if (line1 != NULL) {
        text_centered(im, 60, line1, 4);
    }
    if (line2 != NULL) {
        text_centered(im, 120, line2, 2);
    }
}

void render_draw(render_image_t *im, const ktimer_view_t *view,
                 const render_options_t *options)
{
    memset(im->bytes, 0, sizeof im->bytes);

    switch (view->state) {
    case STATE_IDLE:
        idle_screen(im);
        break;

    case STATE_FINISHED:
        finished_screen(im, view);
        break;

    case STATE_SETTING:
    case STATE_STARTING:
    case STATE_RUNNING:
        total_zone(im, view, view->blink_hidden);
        rectangle(im, 0, RENDER_SEPARATOR_Y, RENDER_WIDTH, 2);
        reminder_zone(im, view, view->blink_hidden);
        break;
    }

    if (options != NULL && options->low_battery) {
        battery_indicator(im);
    }

    /* Alertes à l'écran (CLAUDE.md) : zone du rappel ou écran entier. */
    if (view->end_alert && view->alert_inverted) {
        invert_rows(im, 0, RENDER_HEIGHT - 1);
    } else if (view->reminder_alert && view->alert_inverted) {
        invert_rows(im, REMINDER_ZONE_Y, RENDER_HEIGHT - 1);
    }
}
