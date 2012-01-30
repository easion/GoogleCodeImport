extern const char* fl_encoding;
extern int gi_free_fonts_list(char** list, int n);

#define ENDOFBUFFER 127 // sizeof(Fl_Font.fontname)-1


// turn a stored (with *'s) X font name into a pretty name:
const char* Fl::get_font_name(Fl_Font fnum, int* ap) {
  Fl_Fontdesc *f = fl_fonts + fnum;
  if (!f->fontname[0]) {
    const char* p = f->name;
    if (!p || !*p) {if (ap) *ap = 0; return "";}
    strlcpy(f->fontname, p, ENDOFBUFFER);
    int type = 0;
    if (strstr(f->name, "Bold")) type |= FL_BOLD;
    if (strstr(f->name, "Italic")) type |= FL_ITALIC;
    f->fontname[ENDOFBUFFER] = (char)type;
  }
  if (ap) *ap = f->fontname[ENDOFBUFFER];
  return f->fontname;
}

static unsigned int fl_free_font = FL_FREE_FONT;

Fl_Font Fl::set_fonts(const char* xstarname) {
  if (fl_free_font > (unsigned)FL_FREE_FONT) // already been here
    return (Fl_Font)fl_free_font;
  fl_open_display();
  int xlistsize,i;
  char buf[20];
  if (!xstarname) {
    strcpy(buf,"*/*"); 
	//strcpy(buf+3,fl_encoding);
    xstarname = buf;
  }
  char **xlist = gi_list_fonts( xstarname, 10000, &xlistsize);
  if (!xlist) return (Fl_Font)fl_free_font;

  for (i=0; i<xlistsize; i++)
  {
	  char oName[64];
	  char dummy[64];
	  char bold[64];
	  char obli[64];
	  strncpy(oName,xlist[i],sizeof(oName));

	  //sscanf(xlist[i],"%s/%s/%s/%s/%s/%s/%s",
		//  dummy,dummy,oName,bold,obli,dummy,dummy);

	  //printf("find font %s for %s\n", xlist[i],xstarname);
	  Fl::set_font((Fl_Font)(fl_free_font++), strdup(oName));
  }
  
  gi_free_fonts_list(xlist,xlistsize);
  return (Fl_Font)fl_free_font;
}

static int array[128];
int Fl::get_font_sizes(Fl_Font fnum, int*& sizep) {
  Fl_Fontdesc *s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // empty slot in table, use entry 0
  int cnt = 0;

  // ATS supports all font size 
  array[0] = 0;
  sizep = array;
  cnt = 1;

  return cnt;
}
