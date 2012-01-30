


#ifndef FL_DOXYGEN

/*

find font arial@Bitstream Vera Sans Roman
find font fixed@helvetica Regular
find font gbk@FZXiHeiGBK-YS01 Regular
find font heiti@FZXiHeiGBK-YS01 Regular
find font helvB12@Helvetica Bold


*/

//FL_FREE_FONT
// FL_FREE_FONT
static Fl_Fontdesc built_in_table[16] = {
{"Arial"},
{"Arial Bold"},
{"Arial Italic"},
{"Arial Bold Italic"},
{"Courier New"},
{"Courier New Bold"},
{"Courier New Italic"},
{"Courier New Bold Italic"},
{"Times New Roman"},
{"Times New Roman Bold"},
{"Times New Roman Italic"},
{"Times New Roman Bold Italic"},
{"Symbol"},
{"Monaco"},
{"Andale Mono"}, // there is no bold Monaco font on standard Mac
{"Webdings"},
};

Fl_Fontdesc* fl_fonts = built_in_table;


//const char* fl_encoding = "iso8859-1";
const char* fl_encoding = "iso10646-1";



Fl_Font_Descriptor::Fl_Font_Descriptor(const char* name, Fl_Fontsize size) {
  char buf[256];
  const char *p;
  char* fname = (char*)name;

  if (!fname){
	  fname = (char*)built_in_table[0].name;
  }

  //Verdana:size=18:bold:dpi=75  
  snprintf(buf,256,"%s size=%d:dpi=75", fname,size);
  //snprintf(buf,256,"*/%d/%s/*",size, fname);
 
  font = gi_create_ufont( buf);
  if (!font) {	  
    Fl::warning("create bad font:%s - %s %d",buf, fname, size);
  }
  else
  {
	gi_ufont_set_format(font, GI_RENDER_a8);
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

// locate or create an Fl_Font_Descriptor for a given Fl_Fontdesc and size:
static Fl_Font_Descriptor* find(int fnum, int size) { 
  fl_open_display();
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // use 0 if fnum undefined
  Fl_Font_Descriptor* f;
  for (f = s->first; f; f = f->next)
    if (f->size == size) return f;
  f = new Fl_Font_Descriptor(s->name, size);
  f->next = s->first;
  s->first = f;
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
  int h = 0;
  if (!font_descriptor())
	  return -1;

  myfont = font_descriptor()->font;
  h = gi_ufont_ascent_get(myfont) + gi_ufont_descent_get(myfont);

  //printf("D/A : %d,%d\n", gi_ufont_ascent_get(myfont) , gi_ufont_descent_get(myfont));

  return h;
}


int Fl_Xlib_Graphics_Driver::descent() {
   gi_ufont_t *myfont;
  //int pad = 0;

  if (!font_descriptor())
	  return -1;

  myfont = font_descriptor()->font;
  //pad = gi_ufont_ascent_get(myfont);
  
  return  gi_ufont_descent_get(myfont) ;
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
}

void Fl_Xlib_Graphics_Driver::text_extents(const char *c, int n, int &dx, int &dy, int &W, int &H) {
  if (font_gc != fl_gc) {
    if (!font_descriptor()) font(FL_HELVETICA, FL_NORMAL_SIZE);
    font_gc = fl_gc;
  }
  int xx, yy, ww, hh;
  xx = yy = ww = hh = 0;

  gi_ufont_param(font_descriptor()->font,c,n, &ww, &hh);

  W = ww; H = hh; dx = xx; dy = yy;

}

void Fl_Xlib_Graphics_Driver::draw(const char* c, int n, int x, int y) {
  gi_ufont_t *myfont;

  if (font_gc != fl_gc) {
    if (!font_descriptor()) this->font(FL_HELVETICA, FL_NORMAL_SIZE);
    font_gc = fl_gc;
  }

  if (!font_descriptor())
	  return ;

  myfont = font_descriptor()->font;

  if (fl_gc){
	  gi_gc_attch_window(fl_gc, fl_window); //bind for gc
	  gi_ufont_draw( fl_gc,myfont,c,x,y,  n);
  }

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

}
#endif // FL_DOXYGEN

