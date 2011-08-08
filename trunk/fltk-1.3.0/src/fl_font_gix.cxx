
//fixme dpp

#ifndef FL_DOXYGEN


int gi_free_fonts_list(char** list, int n)
{
	int i;

	if (!list)
	{
		return -1;
	}

	for (i=0; i<n; i++)
	{
		free(list[i]);
	}

	free(list);
	return 0;
}


Fl_Font_Descriptor::Fl_Font_Descriptor(const char* name, Fl_Fontsize size) {
  char buf[256];

  if (!name)
  {
	  font = font = gi_create_ufont("Vera/9");
	  return;
  }

  snprintf(buf,256,"%s/%d", name,size);
 
  font = gi_create_ufont( buf);
  if (!font) {
    Fl::warning("bad font: %s %d", name, size);
	snprintf(buf,256,"gbk/%d", size-2);
	//snprintf(buf,256,"Vera/%d", size);
    font = gi_create_ufont(buf);
	//gi_ufont_set_size(font, 10);
  }

}


Fl_Font_Descriptor::~Fl_Font_Descriptor() {

  if (this == fl_graphics_driver->font_descriptor()) {
    fl_graphics_driver->font_descriptor(NULL);
  }
  gi_delete_ufont(font);
}

////////////////////////////////////////////////////////////////

// WARNING: if you add to this table, you must redefine FL_FREE_FONT
// in Enumerations.H & recompile!!
static Fl_Fontdesc built_in_table[] = {
{"-*-helvetica-medium-r-normal--*"},
{"-*-helvetica-bold-r-normal--*"},
{"-*-helvetica-medium-o-normal--*"},
{"-*-helvetica-bold-o-normal--*"},
{"-*-courier-medium-r-normal--*"},
{"-*-courier-bold-r-normal--*"},
{"-*-courier-medium-o-normal--*"},
{"-*-courier-bold-o-normal--*"},
{"-*-times-medium-r-normal--*"},
{"-*-times-bold-r-normal--*"},
{"-*-times-medium-i-normal--*"},
{"-*-times-bold-i-normal--*"},
{"-*-symbol-*"},
{"-*-lucidatypewriter-medium-r-normal-sans-*"},
{"-*-lucidatypewriter-bold-r-normal-sans-*"},
{"-*-*zapf dingbats-*"}
};

Fl_Fontdesc* fl_fonts = built_in_table;

#define MAXSIZE 32767

// return dash number N, or pointer to ending null if none:
const char* fl_font_word(const char* p, int n) {
  while (*p) {if (*p=='-') {if (!--n) break;} p++;}
  return p;
}

// return a pointer to a number we think is "point size":
char* fl_find_fontsize(char* name) {
  char* c = name;
  // for standard x font names, try after 7th dash:
  if (*c == '-') {
    c = (char*)fl_font_word(c,7);
    if (*c++ && isdigit(*c)) return c;
    return 0; // malformed x font name?
  }
  char* r = 0;
  // find last set of digits:
  for (c++;* c; c++)
    if (isdigit(*c) && !isdigit(*(c-1))) r = c;
  return r;
}

//const char* fl_encoding = "iso8859-1";
const char* fl_encoding = "iso10646-1";

// return true if this matches fl_encoding:
int fl_correct_encoding(const char* name) {
  if (*name != '-') return 0;
  const char* c = fl_font_word(name,13);
  return (*c++ && !strcmp(c,fl_encoding));
}

static const char *find_best_font(const char *fname, int size) {
  static int cnt;
  static char **list = NULL;
// locate or create an Fl_Font_Descriptor for a given Fl_Fontdesc and size:
  if (list) gi_free_fonts_list(list, cnt);
  list = gi_list_fonts( fname, 100, &cnt);
  if (!list) return "fixed";

  // search for largest <= font size:
  char* name = list[0]; int ptsize = 0;     // best one found so far
  int matchedlength = 32767;
  char namebuffer[1024];        // holds scalable font name
  int found_encoding = 0;
  int m = cnt; if (m<0) m = -m;
  for (int n=0; n < m; n++) {
    char* thisname = list[n];
    if (fl_correct_encoding(thisname)) {
      if (!found_encoding) ptsize = 0; // force it to choose this
      found_encoding = 1;
    } else {
      if (found_encoding) continue;
    }
    char* c = (char*)fl_find_fontsize(thisname);
    int thissize = c ? atoi(c) : MAXSIZE;
    int thislength = strlen(thisname);
    if (thissize == size && thislength < matchedlength) {
      // exact match, use it:
      name = thisname;
      ptsize = size;
      matchedlength = thislength;
    } else if (!thissize && ptsize!=size) {
      // whoa!  A scalable font!  Use unless exact match found:
      int l = c-thisname;
      memcpy(namebuffer,thisname,l);
      l += sprintf(namebuffer+l,"%d",size);
      while (*c == '0') c++;
      strcpy(namebuffer+l,c);
      name = namebuffer;
      ptsize = size;
    } else if (!ptsize ||	// no fonts yet
	       (thissize < ptsize && ptsize > size) || // current font too big
	       (thissize > ptsize && thissize <= size) // current too small
      ) {
      name = thisname;
      ptsize = thissize;
      matchedlength = thislength;
    }
  }

  return name;
}

static char *put_font_size(const char *n, int size)
{
        int i = 0;
        char *buf;
        const char *ptr;
        const char *f;
        char *name;
        int nbf = 1;
        name = strdup(n);
        while (name[i]) {
                if (name[i] == ',') {nbf++; name[i] = '\0';}
                i++;
        }

        buf = (char*) malloc(nbf * 256);
        buf[0] = '\0';
        ptr = name;
        i = 0;
        while (ptr && nbf > 0) {
                f = find_best_font(ptr, size);
                while (*f) {
                        buf[i] = *f;
                        f++; i++;
                }
                nbf--;
                while (*ptr) ptr++;
                if (nbf) {
                        ptr++;
                        buf[i] = ',';
                        i++;
                }
                while(isspace(*ptr)) ptr++;
        }
        buf[i] = '\0';
        free(name);
        return buf;
}


char *fl_get_font_xfld(int fnum, int size) {
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // use font 0 if still undefined
  fl_open_display();
  return put_font_size(s->name, size);
}

// locate or create an Fl_Font_Descriptor for a given Fl_Fontdesc and size:
static Fl_Font_Descriptor* find(int fnum, int size) {
  char *name;
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // use font 0 if still undefined
  Fl_Font_Descriptor* f;
  for (f = s->first; f; f = f->next)
    if (f->size == size) return f;
  fl_open_display();

  name = put_font_size(s->name, size);
  f = new Fl_Font_Descriptor(name, size);
  f->size = size;
  f->next = s->first;
  s->first = f;
  free(name);
  return f;
}


////////////////////////////////////////////////////////////////
// Public interface:

void *fl_xftfont = 0;
static gi_gc_ptr_t font_gc;

gi_ufont_t* Fl_XFont_On_Demand::value() {
  return ptr;
}

void Fl_Xlib_Graphics_Driver::font(Fl_Font fnum, Fl_Fontsize size) {
  if (fnum==-1) {
    Fl_Graphics_Driver::font(0, 0);
    return;
  }
  if (fnum == Fl_Graphics_Driver::font() && size == Fl_Graphics_Driver::size()) return;
  Fl_Graphics_Driver::font(fnum, size);
  Fl_Font_Descriptor* f = find(fnum, size);
  if (f != this->font_descriptor()) {
    this->font_descriptor(f);
    font_gc = 0;
  }
}

int Fl_Xlib_Graphics_Driver::height() {
  gi_ufont_t *myfont;
  if (!font_descriptor())
	  return -1;

  myfont = font_descriptor()->font;
  return gi_ufont_ascent_get(myfont) + gi_ufont_descent_get(myfont);
}

int Fl_Xlib_Graphics_Driver::descent() {
  if (font_descriptor()) return gi_ufont_descent_get(font_descriptor()->font);
  else return -1;
}

double Fl_Xlib_Graphics_Driver::width(const char* c, int n) {
  gi_ufont_t *myfont;
  int ww=0,hh=0;

  if (!font_descriptor())
	  return -1;

  myfont = font_descriptor()->font;

  gi_ufont_param(myfont,c,n, &ww, &hh);
  return (double) ww;
}

double Fl_Xlib_Graphics_Driver::width(unsigned int c) {
  gi_ufont_t *myfont;
  int ww=0,hh=0;
  char buf[2];

  if (!font_descriptor())
	  return -1;

  myfont = font_descriptor()->font;

  buf[0] = c;
  buf[1] = 0;

  gi_ufont_param(myfont,buf,1, &ww, &hh);
  return (double) ww;
  //if (font_descriptor()) return (double) XUtf8UcsWidth(font_descriptor()->font, c);
  //else return -1;
}

void Fl_Xlib_Graphics_Driver::text_extents(const char *c, int n, int &dx, int &dy, int &W, int &H) {
  if (font_gc != fl_gc) {
    if (!font_descriptor()) font(FL_HELVETICA, FL_NORMAL_SIZE);
    font_gc = fl_gc;
    //XSetFont(fl_display, fl_gc, font_descriptor()->font->fid);
  }
  int xx, yy, ww, hh;
  xx = yy = ww = hh = 0;

  gi_ufont_param(font_descriptor()->font,c,n, &ww, &hh);

  //if (fl_gc) XUtf8_measure_extents(fl_display, fl_window, 
  //font_descriptor()->font, fl_gc, &xx, &yy, &ww, &hh, c, n);

  W = ww; H = hh; dx = xx; dy = yy;

}

void Fl_Xlib_Graphics_Driver::draw(const char* c, int n, int x, int y) {
  gi_ufont_t *myfont;

  if (font_gc != fl_gc) {
    if (!font_descriptor()) this->font(FL_HELVETICA, FL_NORMAL_SIZE);
    font_gc = fl_gc;
    //XSetFont(fl_display, fl_gc, font_descriptor()->font->fid);
  }

  //if (!font_descriptor())
	//  return -1;

  myfont = font_descriptor()->font;

  if (fl_gc){
	  unsigned char R=0,G=255,B=0;
	  gi_ufont_set_text_attr(fl_gc,myfont,FALSE,GI_RGB(R,G,B),0);

	  gi_ufont_set_format(myfont, GI_RENDER_a8);

	//if (y>gi_ufont_ascent_get(myfont))
	  {
		  y-=gi_ufont_ascent_get(myfont);
	  }

	  gi_gc_attch_window(fl_gc, fl_window); //bind for gc
	  gi_ufont_draw( fl_gc,myfont,c,x,y,  n);
  }
  //if (fl_gc) XUtf8DrawString(fl_display, fl_window, myfont, fl_gc, x, y, c, n);
}

void Fl_Xlib_Graphics_Driver::draw(int angle, const char *str, int n, int x, int y) {
  fprintf(stderr,"ROTATING TEXT NOT IMPLEMENTED\n");
  this->draw(str, n, (int)x, (int)y);
}

void Fl_Xlib_Graphics_Driver::rtl_draw(const char* c, int n, int x, int y) {
  if (font_gc != fl_gc) {
    if (!font_descriptor()) this->font(FL_HELVETICA, FL_NORMAL_SIZE);
    font_gc = fl_gc;
  }
  //if (fl_gc) XUtf8DrawRtlString(fl_display, fl_window, myfont, fl_gc, x, y, c, n);
}
#endif // FL_DOXYGEN

