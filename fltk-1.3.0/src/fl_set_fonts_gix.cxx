extern const char* fl_encoding;

//fixme dpp
static int to_canonical(char *to, const char *from, size_t tolen) {
}

static int ultrasort(const void *aa, const void *bb) {

}

extern int gi_free_fonts_list(char** list, int n);


// turn a stored (with *'s) X font name into a pretty name:
const char* Fl::get_font_name(Fl_Font fnum, int* ap) {
}

static unsigned int fl_free_font = FL_FREE_FONT;

Fl_Font Fl::set_fonts(const char* xstarname) {
  if (fl_free_font > (unsigned)FL_FREE_FONT) // already been here
    return (Fl_Font)fl_free_font;
  fl_open_display();
  int xlistsize;
  char buf[20];
  if (!xstarname) {
    strcpy(buf,"-*-"); strcpy(buf+3,fl_encoding);
    xstarname = buf;
  }
  char **xlist = gi_list_fonts( xstarname, 10000, &xlistsize);
  if (!xlist) return (Fl_Font)fl_free_font;
  qsort(xlist, xlistsize, sizeof(*xlist), ultrasort);
  int used_xlist = 0;
  for (int i=0; i<xlistsize;) {
    int first_xlist = i;
    const char *p = xlist[i++];
    char canon[1024];
    int size = to_canonical(canon, p, sizeof(canon));
    if (size >= 0) {
      for (;;) { // find all matching fonts:
	if (i >= xlistsize) break;
	const char *q = xlist[i];
	char this_canon[1024];
	if (to_canonical(this_canon, q, sizeof(this_canon)) < 0) break;
	if (strcmp(canon, this_canon)) break;
	i++;
      }
      /*if (*p=='-' || i > first_xlist+1)*/ p = canon;
    }
    unsigned int j;
    for (j = 0;; j++) {
      /*if (j < FL_FREE_FONT) {
	// see if it is one of our built-in fonts:
	// if so, set the list of x fonts, since we have it anyway
	if (fl_fonts[j].name && !strcmp(fl_fonts[j].name, p)) break;
      } else */{
	j = fl_free_font++;
	if (p == canon) p = strdup(p); else used_xlist = 1;
	Fl::set_font((Fl_Font)j, p);
	break;
      }
    }
    if (!fl_fonts[j].xlist) {
      fl_fonts[j].xlist = xlist+first_xlist;
      fl_fonts[j].n = -(i-first_xlist);
      used_xlist = 1;
    }
  }
  if (!used_xlist) gi_free_fonts_list(xlist,xlistsize);
  return (Fl_Font)fl_free_font;
}

int Fl::get_font_sizes(Fl_Font fnum, int*& sizep) {
  Fl_Fontdesc *s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // empty slot in table, use entry 0
  if (!s->xlist) {
    fl_open_display();
    s->xlist = gi_list_fonts( s->name, 100, &(s->n));
    if (!s->xlist) return 0;
  }
  int listsize = s->n; if (listsize<0) listsize = -listsize;
  static int sizes[128];
  int numsizes = 0;
  for (int i = 0; i < listsize; i++) {
    char *q = s->xlist[i];
    char *d = fl_find_fontsize(q);
    if (!d) continue;
    int s = strtol(d,0,10);
    if (!numsizes || sizes[numsizes-1] < s) {
      sizes[numsizes++] = s;
    } else {
      // insert-sort the new size into list:
      int n;
      for (n = numsizes-1; n > 0; n--) if (sizes[n-1] < s) break;
      if (sizes[n] != s) {
	for (int m = numsizes; m > n; m--) sizes[m] = sizes[m-1];
	sizes[n] = s;
	numsizes++;
      }
    }
  }
  sizep = sizes;
  return numsizes;
}


