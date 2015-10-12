#include "commonEditor.h"
#include "pi64.h"

void ceditorDrawCursor(struct commonEditorConfig *CE) {
     
    int x = CE->cx*fontKerning;
    int y = CE->fb->height-((CE->cy+1)*fontHeight);
    int charmargin = (fontWidth-fontKerning)/2;

    x += CE->margin_left;
    y -= CE->margin_top;
    
    syscolor *color;
    
    if (!(CE->cblink & 0x80)) {
          if (pi64.mode==EDITOR) {
             color = sysscheme+RPI_EDITOR_CURSOR;
          } else {
             color = sysscheme+RPI_SHELL_CURSOR;
          }
          drawBox( CE->fb,x+charmargin,y,
                   x+charmargin+fontKerning-1,y+fontHeight-1,
                   color->r,color->g,color->b,color->a,1);
    }
    CE->cblink += 4;
}

void ceditorDrawPowerOff(struct commonEditorConfig *CE, int x, int y) {
    syscolor *color2 = sysscheme+RPI_FRAME;
    syscolor *color = sysscheme+RPI_SHELL;
    drawEllipse(CE->fb,x,y,12,12,color->r,color->g,color->b,color->a,1);
    drawEllipse(CE->fb,x,y,7,7,color2->r,color2->g,color2->b,color2->a,1);
    drawBox(CE->fb,x-4,y,x+4,y+12,color2->r,color2->g,color2->b,color2->a,1); 
    drawBox(CE->fb,x-2,y,x+2,y+14,color->r,color->g,color->b,color->a,1);   
}

void ceditorDrawSaveIcon(struct commonEditorConfig *CE, int x, int y) {
    syscolor *color = sysscheme+RPI_SHELL;
    drawBox(CE->fb,x-12,y-12,x+12,y+12,color->r,color->g,color->b,color->a,1);
    color = sysscheme+RPI_FRAME;
    drawBox(CE->fb,x-1,y+7,x+1,y+11,color->r,color->g,color->b,color->a,1);
    drawEllipse(CE->fb,x,y,4,4,color->r,color->g,color->b,color->a,1);
}

void ceditorDrawChars(struct commonEditorConfig *CE) {
    int y,x;
    erow *r;
    char buf[32];

    for (y = 0; y < CE->screenrows; y++) {
        int chary, filerow = CE->rowoff+y;

        if (filerow >= CE->numrows) break;
        chary = CE->fb->height-((y+1)*fontHeight);
        chary -= CE->margin_top;
        r = &CE->row[filerow];
        syscolor *color;
        
        if (pi64.mode==EDITOR) { 
           snprintf(buf,sizeof(buf),"%d",(filerow+1)%1000);
           color = sysscheme + RPI_FONT;
           bfWriteString(CE->fb,BitmapFont,0,chary,buf,strlen(buf),color->r,color->g,color->b,color->a,0);
        }
        
        for (x = 0; x < CE->screencols; x++) {
            int idx = x+CE->coloff;
            int charx;            

            if (idx >= r->size) break;
            charx = x*fontKerning;
            charx += CE->margin_left;
            if (pi64.mode!=EDITOR)
               color = hlscheme;
            else
               color = hlscheme+r->hl[idx];
               
            bfWriteChar(CE->fb,BitmapFont,charx,chary,r->chars[idx],
                        color->r,color->g,color->b,color->a);
        }
    }

    // if (CE->err!=NULL) bfWriteString(CE->fb,BitmapFont,CE->margin_left,10,CE->err,strlen(CE->err),0,0,0,255,0);
}

void ceditorDraw(struct commonEditorConfig *CE) {         
    syscolor *color = sysscheme+RPI_FRAME; 
    drawBox(CE->fb,0,0,CE->fb->width-1,CE->fb->height-1,color->r,color->g,color->b,color->a,1);    
    if (pi64.mode==EDITOR) { 
        color = sysscheme+RPI_EDITOR;
        drawBox(CE->fb,
            CE->margin_left,
            CE->margin_bottom,
            CE->fb->width-1-CE->margin_right,
            CE->fb->height-1-CE->margin_top,color->r,color->g,color->b,color->a,1);                                 
    } else {    
       color = sysscheme+RPI_SHELL; 
       drawBox(CE->fb,
            CE->margin_left,
            CE->margin_bottom,
            CE->fb->width-1-CE->margin_right,
            CE->fb->height-1-CE->margin_top,color->r,color->g,color->b,color->a,1);
    }
    ceditorDrawChars(CE);
    if (pi64.mode==EDITOR || pi64.mode==INTERPRETER)
       ceditorDrawCursor(CE);
    /* Show buttons */
    ceditorDrawPowerOff(CE, POWEROFF_BUTTON_X,POWEROFF_BUTTON_Y);
    if (CE->dirty && CE->filename!=NULL) ceditorDrawSaveIcon(CE, SAVE_BUTTON_X,SAVE_BUTTON_Y);
    /* Show info about the current file */
    if (pi64.mode==EDITOR) {  
      color = sysscheme + RPI_FONT;     
      if (CE->filename!=NULL) 
        bfWriteString(CE->fb,BitmapFont,CE->margin_left,CE->fb->height-CE->margin_top+4,CE->filename,strlen(CE->filename), color->r,color->g,color->b,color->a,0);
      else 
        bfWriteString(CE->fb,BitmapFont,CE->margin_left,CE->fb->height-CE->margin_top+4,"PROGRAM EDITOR",14, color->r,color->g,color->b,color->a,0);
    }        
    
}

void csizeEditor(struct commonEditorConfig *CE, frameBuffer *fb) {  
    CE->fb = fb; 
    CE->screencols = (fb->width-CE->margin_left-CE->margin_right) / fontKerning;
    CE->screenrows = (fb->height-CE->margin_top-CE->margin_bottom) / fontHeight;
    ceditorDraw(CE);
}

/* ======================= Common Editor rows implementation ======================= */

/* Insert a row at the specified position, shifting the other rows on the bottom
 * if required. */
void ceditorInsertRow(struct commonEditorConfig *CE, int at, char *s) {
    if (at==-1) {
       at = CE->rowoff+CE->cy;
    }
    if (at > CE->numrows) return;
    CE->row = realloc(CE->row,sizeof(erow)*(CE->numrows+1));
    if (at != CE->numrows)
        memmove(CE->row+at+1,CE->row+at,sizeof(CE->row[0])*(CE->numrows-at));
    CE->row[at].size = strlen(s);
    
    // BUG v1.3 realloc buffer must be initialized with malloc!!!
    CE->row[at].chars = malloc(0);
    // patch ends
    
    CE->row[at].chars = strdup(s);
    CE->row[at].hl = NULL;
    ceditorUpdateSyntax(CE, CE->row+at);
    CE->numrows++;
    CE->dirty++;
}

/* Free row's heap allocated stuff. */
void ceditorFreeRow(erow *row) {
    free(row->chars);
    if (row->hl!=NULL)
       free(row->hl);
}

/* Remove the row at the specified position, shifting the remainign on the
 * top. */
void ceditorDelRow(struct commonEditorConfig *CE, int at) {
    erow *row;
    if (at==-1) {
       at = CE->rowoff+CE->cy;
    }
    if (at >= CE->numrows) return;
    row = CE->row+at;
    ceditorFreeRow(row);
    memmove(CE->row+at,CE->row+at+1,sizeof(CE->row[0])*(CE->numrows-at-1));
    CE->numrows--;
    CE->dirty++;
}

/* Turn the ceditor rows into a single heap-allocated string.
 * Returns the pointer to the heap-allocated string and populate the
 * integer pointed by 'buflen' with the size of the string, escluding
 * the final nulterm. */
char *ceditorRowsToString(struct commonEditorConfig *CE, int *buflen) {
    char *buf = NULL, *p;
    int totlen = 0;
    int j;

    /* Compute count of bytes */
    for (j = 0; j < CE->numrows; j++)
        totlen += CE->row[j].size+1; /* +1 is for "\n" at end of every row */
    *buflen = totlen;
    totlen++; /* Also make space for nulterm */

    p = buf = malloc(totlen);
    for (j = 0; j < CE->numrows; j++) {
        memcpy(p,CE->row[j].chars,CE->row[j].size);
        p += CE->row[j].size;
        *p = '\n';
        p++;
    }
    *p = '\0';
    return buf;
}

/* Insert a character at the specified position in a row, moving the remaining
 * chars on the right if needed. */
void ceditorRowInsertChar(struct commonEditorConfig *CE, erow *row, int at, int c) {      
    if (at > row->size) {
        /* Pad the string with spaces if the insert location is outside the
         * current length by more than a single character. */
        int padlen = at-row->size;
        /* In the next line +2 means: new char and null term. */
        row->chars = realloc(row->chars,row->size+padlen+2);
        memset(row->chars+row->size,' ',padlen);
        row->chars[row->size+padlen+1] = '\0';
        row->size += padlen+1;
    } else {
        /* If we are in the middle of the string just make space for 1 new
         * char plus the (already existing) null term. */
        row->chars = realloc(row->chars,row->size+2);
        memmove(row->chars+at+1,row->chars+at,row->size-at+1);
        row->size++;
    }
    row->chars[at] = c;
    ceditorUpdateSyntax(CE, row);        
    CE->dirty++;
}

/* Append the string 's' at the end of a row */
void ceditorRowAppendString(struct commonEditorConfig *CE, erow *row, char *s) {
    int l = strlen(s);

    row->chars = realloc(row->chars,row->size+l+1);
    memcpy(row->chars+row->size,s,l);
    row->size += l;
    row->chars[row->size] = '\0';
    ceditorUpdateSyntax(CE, row);
    CE->dirty++;
}

void ceditorRowDelChar(struct commonEditorConfig *CE, erow *row, int at) {
    if (row->size <= at) return;
    memmove(row->chars+at,row->chars+at+1,row->size-at);
    ceditorUpdateSyntax(CE, row);
    row->size--;
    CE->dirty++;
}

void ceditorInsertChar(struct commonEditorConfig *CE, int c) {
    int filerow = CE->rowoff+CE->cy;
    int filecol = CE->coloff+CE->cx;
    erow *row = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];    
    /* If the row where the cursor is currently located does not exist in our
     * logical representaion of the file, add enough empty rows as needed. */
    
    if (!row) {        
        while(CE->numrows <= filerow) {                                 
            ceditorInsertRow(CE, CE->numrows,"");
        }
    }    
    row = &CE->row[filerow]; 
   
    ceditorRowInsertChar(CE,row,filecol,c);

    if (CE->cx == CE->screencols-1)
        CE->coloff++;
    else
        CE->cx++;
    CE->dirty++;
}

void ceditorCurrentRowToString(struct commonEditorConfig *CE, char **buffer) {
    int filerow = CE->rowoff+CE->cy;
    int filecol = CE->coloff+CE->cx;
    if (*buffer != NULL) free(*buffer);
    erow *row = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];
    *buffer = NULL;
    if (row != NULL && row->chars != NULL)
       *buffer = strdup(row->chars); 
}

/* Inserting a newline is slightly complex as we have to handle inserting a
 * newline in the middle of a line, splitting the line as needed. */
void ceditorInsertNewline(struct commonEditorConfig *CE) {
    int filerow = CE->rowoff+CE->cy;
    int filecol = CE->coloff+CE->cx;
    erow *row = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];

    if (!row) {
        if (filerow == CE->numrows) {
            ceditorInsertRow(CE,filerow,"");
            goto fixcursor;
        }
        return;
    }
    /* If the cursor is over the current line size, we want to conceptually
     * think it's just over the last character. */
    if (filecol >= row->size) filecol = row->size;
    if (pi64.mode==INTERPRETER) {
       // interpreterSetCommand(row->chars);
       if (INTP->command != NULL) free(INTP->command);
       INTP->command = NULL;
       if (row->chars != NULL)
          INTP->command = strdup(row->chars);          
    } else {
      if (filecol == 0) {
                  
        ceditorInsertRow(CE,filerow,"");                    
      } else {
        /* We are in the middle of a line. Split it between two rows. */
        ceditorInsertRow(CE,filerow+1,row->chars+filecol);
        row = &CE->row[filerow];
        row->chars[filecol] = '\0';
        row->size = filecol;
        ceditorUpdateSyntax(CE, row);
      }
    }
fixcursor:
    if (CE->cy == CE->screenrows-1) {
        CE->rowoff++;
    } else {
        CE->cy++;
    }
    CE->cx = 0;
    CE->coloff = 0;
}

void ceditorDelChar(struct commonEditorConfig *CE) {
    int filerow = CE->rowoff+CE->cy;
    int filecol = CE->coloff+CE->cx;
    erow *row = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];
    // fake backspace
    if (!row && filecol==0 && filerow>0) {
       CE->cy--;
       filecol = CE->row[filerow-1].size; 
       CE->cx = filecol;
       if (CE->cx >= CE->screencols) {
            int shift = (CE->screencols-CE->cx)+1;
            CE->cx -= shift;
            CE->coloff += shift;
       }
       return;
    }
    if (!row || (filecol == 0 && filerow == 0)) return;
    if (filecol == 0) {
        /* Handle the case of column 0, we need to move the current line
         * on the right of the previous one. */
        filecol = CE->row[filerow-1].size;
        ceditorRowAppendString(CE, &CE->row[filerow-1],row->chars);
        ceditorDelRow(CE,filerow);
        row = NULL;
        if (CE->cy == 0)
            CE->rowoff--;
        else
            CE->cy--;
        CE->cx = filecol;
        if (CE->cx >= CE->screencols) {
            int shift = (CE->screencols-CE->cx)+1;
            CE->cx -= shift;
            CE->coloff += shift;
        }
    } else {
        ceditorRowDelChar(CE,row,filecol-1);
        if (CE->cx == 0 && CE->coloff)
            CE->coloff--;
        else
            CE->cx--;
    }
    if (row) ceditorUpdateSyntax(CE, row);
    CE->dirty++;
}

void ceditorDelCharAtCursor(struct commonEditorConfig *CE) {
    int filerow = CE->rowoff+CE->cy;
    int filecol = CE->coloff+CE->cx;
    erow *row = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];
    erow *nextrow = (filerow+1 >= CE->numrows) ? NULL : &CE->row[filerow+1];    
    if (!row) return;
    if (nextrow && filecol == row->size) {
        /* Handle the case of lastcolumn, we need to move the next line
         * on the right of the current one. */
        filecol = CE->row[filerow].size;        
        ceditorRowAppendString(CE,&CE->row[filerow],nextrow->chars);
        ceditorDelRow(CE,filerow+1);        
    } else {
        ceditorRowDelChar(CE,row,filecol);
    }
    // if (row) ceditorUpdateSyntax(row);
    CE->dirty++;
}

void ceditorDelCharsToCursor(struct commonEditorConfig *CE) {
    int filerow = CE->rowoff+CE->cy;
    int filecol = CE->coloff+CE->cx;
    erow *row = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];     
    if (!row) return;
    int i = 0;
    while(i<filecol) {
        ceditorRowDelChar(CE,row,0);
        CE->cx--;
        i++;
    }
}

void ceditorDelCharsFromCursor(struct commonEditorConfig *CE) {
    int filerow = CE->rowoff+CE->cy;
    int filecol = CE->coloff+CE->cx;
    erow *row = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];     
    if (!row) return;    
    int i = 0;
    if (row->chars != NULL) {
          i = strlen(row->chars);
          while(i>filecol) {
               ceditorRowDelChar(CE,row,i-1);
               i--;
          }
    }
}
 

int is_separator(int c) {
    return c == '\0' || isspace(c) || strchr(",.()+-/*=~%[];",c) != NULL;
}


void ceditorUpdateSyntax(struct commonEditorConfig *CE, erow *row ) {
    
    if (pi64.mode!=EDITOR) {
       return;
    }
    
    int i, prev_sep, in_string;
    char *p;    
    
    row->hl = realloc(row->hl,row->size);
    memset(row->hl,HL_NORMAL,row->size);
    
    /* Point to the first non-space char. */
    p = row->chars;
    i = 0; /* Current char offset */
    while(*p && isspace(*p)) {
        p++;
        i++;
    }
    prev_sep = 1; /* Tell the parser if 'i' points to start of word. */
    in_string = 0; /* Are we inside "" or '' ? */
    while(*p) {
        if (prev_sep && *p == '-' && *(p+1) == '-') {
            /* From here to end is a comment */
            memset(row->hl+i,HL_COMMENT,row->size-i);
            return;
        }
        /* Handle "" and '' */
        if (in_string) {
            row->hl[i] = HL_STRING;
            if (*p == '\\') {
                row->hl[i+1] = HL_STRING;
                p += 2; i += 2;
                prev_sep = 0;
                continue;
            }
            if (*p == in_string) in_string = 0;
            p++; i++;
            continue;
        } else {
            if (*p == '"' || *p == '\'') {
                in_string = *p;
                row->hl[i] = HL_STRING;
                p++; i++;
                prev_sep = 0;
                continue;
            }
        }
        /* Handle numbers */
        if ((isdigit(*p) && (prev_sep || row->hl[i-1] == HL_NUMBER)) ||
            (*p == '.' && i >0 && row->hl[i-1] == HL_NUMBER)) {
            row->hl[i] = HL_NUMBER;
            p++; i++;
            prev_sep = 0;
            continue;
        }

        /* Handle keywords and lib calls */
        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int lib = keywords[j][klen-1] == '.';

                if (!lib && !memcmp(p,keywords[j],klen) &&
                    is_separator(*(p+klen)))
                {
                    /* Keyword */
                    memset(row->hl+i,HL_KEYWORD,klen);
                    p += klen;
                    i += klen;
                    break;
                }
                if (lib && !memcmp(p,keywords[j],klen)) {
                    /* Library call */
                    memset(row->hl+i,HL_LIB,klen);
                    p += klen;
                    i += klen;
                    while(!is_separator(*p)) {
                        row->hl[i] = HL_LIB;
                        p++;
                        i++;
                    }
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue; /* We had a keyword match */
            }
        }

        /* Not special chars */
        prev_sep = is_separator(*p);
        p++; i++;
    }

}

void ceditorMouseClicked(struct commonEditorConfig *CE, int x, int y, int button) {
    if (abs(x-POWEROFF_BUTTON_X) < 15 && abs(y-POWEROFF_BUTTON_Y) < 15 &&
        button == 1 && CE->modifiers & CTRL_MASK)
    {
       pi64.resetFlag = 1;       
    } else if (abs(x-POWEROFF_BUTTON_X) < 15 && abs(y-POWEROFF_BUTTON_Y) < 15 &&
        button == 1)
    {
        exit(1);
    } else if (abs(x-SAVE_BUTTON_X) < 15 && abs(y-SAVE_BUTTON_Y) < 15 &&
               button == 1) {
        if (editorSave(CE->filename) == 0) CE->dirty = 0;
    } else if (x >= CE->margin_left && x <= CE->fb->width-1-CE->margin_right &&
               y >= CE->margin_bottom && y <= CE->fb->height-1-CE->margin_top)
    {
        if (button == 4) {
            if (CE->rowoff) {
                CE->rowoff--;
                if (CE->cy < CE->screenrows - 1) CE->cy++;
            }
        }
        else if (button == 5) {
            if (CE->rowoff + CE->screenrows < CE->numrows) {
                CE->rowoff++;
                if (CE->cy > 0) CE->cy--;
            }
        }
        else {
            int realheight = CE->fb->height - CE->margin_top - CE->margin_bottom;
            int realy = y - CE->margin_bottom;
            int row = (realheight-realy)/fontHeight;
            int col = (x-CE->margin_left)/fontKerning;
            int filerow = CE->rowoff+row;
            int filecol = CE->coloff+col;
            erow *r = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];
        
            CE->cblink = 0;
            if (filerow == CE->numrows) {
                CE->cx = 0;
                CE->cy = filerow-CE->rowoff;
            } else if (r) {
                if (filecol >= r->size)
                    CE->cx = r->size-CE->coloff;
                else
                    CE->cx = filecol-CE->coloff;
                CE->cy = filerow-CE->rowoff;
            }
        }
    }
}

int getFirstNonSpace(erow *row) {
  int i;
  for (i = 0; i < row->size; i++) {
      if (row->chars[i] != ' ' && row->chars[i] != '\t') {
          return i;
      }
  }
  return -1;
}

void ceditorMoveCursor(struct commonEditorConfig *CE, int key) {
    int filerow = CE->rowoff+CE->cy;
    int filecol = CE->coloff+CE->cx;
    int rowlen;
    erow *row = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];
    int temp;

    switch(key) {
    case SDLK_LEFT:
        if (CE->cx == 0) {
            if (CE->coloff) CE->coloff--;
        } else {
            CE->cx -= 1;
        }
        break;
    case SDLK_RIGHT:
        if (row && filecol < row->size) {
            if (CE->cx == CE->screencols-1) {
                CE->coloff++;
            } else {
                CE->cx += 1;
            }
        }
        break;
    case SDLK_UP:
        if (CE->cy == 0) {
            if (CE->rowoff) CE->rowoff--;
        } else {
            CE->cy -= 1;
        }
        break;
    case SDLK_DOWN:
        if (filerow < CE->numrows) {
            if (CE->cy == CE->screenrows-1) {
                CE->rowoff++;
            } else {
                CE->cy += 1;
            }
        }
        break;
    case SDLK_PAGEUP:
        if (CE->rowoff) {
            CE->rowoff -= CE->screenrows - 1;
            if (CE->rowoff < 0) {
                CE->rowoff = 0;
                CE->cy = 0;
            }
        }
        else {
            if (CE->cy > 0) CE->cy = 0;
        }
        break;
    case SDLK_PAGEDOWN:
        if (CE->rowoff + CE->screenrows - 1 < CE->numrows) {
            CE->rowoff += CE->screenrows - 1;
            if (CE->rowoff + CE->screenrows - 1 > CE->numrows) CE->cy = CE->numrows - CE->rowoff - 1;
        }
        else {
            CE->cy = CE->numrows - CE->rowoff - 1;
        }
        break;
    case SDLK_HOME:
        if (CE->modifiers & CTRL_MASK) {
            CE->rowoff = CE->coloff = CE->cy = CE->cx = 0;
        }
        else {
            if (row && filecol != 0) {
                temp = getFirstNonSpace(row);
                if (temp > -1) {
                    if (filecol > temp) {
                        CE->cx = temp;
                        CE->coloff = 0;
                    }
                    else {
                        CE->cx = CE->coloff = 0;
                    }
                }
            }
        }
        break;
    case SDLK_END:
        if (CE->modifiers & CTRL_MASK && CE->numrows > CE->screenrows) {              
            CE->rowoff = CE->numrows - CE->screenrows;            
            CE->cy = CE->screenrows - 1;            
            CE->coloff = CE->cx = 0;
        } else if (CE->modifiers & CTRL_MASK) {
            // PGDOWN
            if (CE->rowoff + CE->screenrows - 1 < CE->numrows) {
               CE->rowoff += CE->screenrows - 1;
               if (CE->rowoff + CE->screenrows - 1 > CE->numrows) CE->cy = CE->numrows - CE->rowoff - 1;
            }
            else {
                 CE->cy = CE->numrows - CE->rowoff - 1;
            }       
        }
        else {
            if (row && filecol < row->size) {
                if (row->size - CE->screencols + 1 > 0) {
                    CE->coloff = row->size - CE->screencols + 1;
                }
                CE->cx = row->size - CE->coloff;
            }
        }
        break;
    }
    /* Fix cx if the current line has not enough chars. */
    filerow = CE->rowoff+CE->cy;
    filecol = CE->coloff+CE->cx;
    row = (filerow >= CE->numrows) ? NULL : &CE->row[filerow];
    rowlen = row ? row->size : 0;
    if (filecol > rowlen) {
        CE->cx -= filecol-rowlen;
        if (CE->cx < 0) {
            CE->coloff += CE->cx;
            CE->cx = 0;
        }
    }
}

#define KEY_REPEAT_PERIOD 2
#define KEY_REPEAT_PERIOD_FAST 1
#define KEY_REPEAT_DELAY 8
int pressed_or_repeated(int counter) {
    int period;

    if (counter > KEY_REPEAT_DELAY+(KEY_REPEAT_PERIOD*3)) {
        period = KEY_REPEAT_PERIOD_FAST;
    } else {
        period = KEY_REPEAT_PERIOD;
    }
    if (counter > 1 && counter < KEY_REPEAT_DELAY) return 0;
    return ((counter+period-1) % period) == 0;
}
