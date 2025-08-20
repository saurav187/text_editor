#include<ctype.h>
#include<errno.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<termios.h>
#include<stdlib.h>

#define KILO_VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)

enum editorKey{
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN
};

struct editorConfig{
	int cx, cy ;                       // cursor position 
	int screenrows ;
	int screencols ;
	struct termios orig_termios ;
};

struct editorConfig E ;

void die(const char *s){
	write(STDOUT_FILENO, "\x1b[2J", 4) ;
	write(STDOUT_FILENO, "\x1b[H", 3) ;
	
	perror(s) ;
	exit(1);
}

void disableRawMode(){
	if( (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) ) die("tcssetarr") ;
}


void enableRawMode (){

	if ( tcgetattr ( STDIN_FILENO, &E.orig_termios ) == -1 ) die("tcgetatt") ;
	atexit(disableRawMode) ;

	struct termios raw = E.orig_termios;
	
	//to disable certain keys like ctrl + c , ctrl + v etc. 
  	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  	raw.c_oflag &= ~(OPOST);
 	raw.c_cflag |= (CS8);
  	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	
	raw.c_cc[VMIN] = 1 ;
	raw.c_cc[VTIME] = 0 ;


	if (tcsetattr ( STDIN_FILENO, TCSAFLUSH, &raw ) == -1) die("tcsetarrr") ;
}

int editorReadKey(){
	char c ;
	int nread ;

	while( ((nread = read(STDIN_FILENO, &c, 1)) != 1)){
		if( nread == -1 && errno != EAGAIN ) die("read") ;
	}

	if (c == '\x1b') {
		char seq[3] ;
		
		if (read (STDIN_FILENO, &seq[0], 1) != 1) return '\x1b' ;
		if (read (STDIN_FILENO, &seq[1], 1) != 1) return '\x1b' ;

		if (seq[0] == '['){
			switch (seq[1]) {
				case 'A' : return ARROW_UP ;
				case 'B' : return ARROW_DOWN ;
				case 'C' : return ARROW_RIGHT;
				case 'D' : return ARROW_LEFT ;
			}
		}

	return '\x1b' ;
	}else{
		return c ;
	}
}

int getWindowSize(int *rows, int *cols){
	struct winsize ws ;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0 ){
		return -1;
	}else{
		*cols = ws.ws_col;
		*rows = ws.ws_row;
		return 0 ;
	}
}

/**** append buffer ***/

struct abuf{
	char *b ;
	int len ;
};

#define ABUF_INIT {NULL, 0} 

void abAppend( struct abuf *ab, const char *s , int len ){
	char *new = realloc(ab -> b, ab -> len + len) ;
	if(new == NULL) return ;
	
	memcpy(&new[ab->len], s, len) ;
	ab -> b = new ;
	ab -> len += len ;
}

void abFree(struct abuf *ab){
	free(ab->b);
}

/**** output ***/

void editorDrawRows(struct abuf *ab){
	int y ;
	for (y = 0; y < E.screenrows ; y++ ){
		if (y == E.screenrows / 3){
			char welcome[80] ;
			int welcomelen = snprintf(welcome, sizeof(welcome),
					"Kilo Editor -- Version %s", KILO_VERSION) ;
		if (welcomelen > E.screencols) welcomelen = E.screencols ;

		int padding = (E.screencols - welcomelen) / 3 ;
		if (padding){
			abAppend (ab, "~", 1) ;
			padding-- ;
		}
		while ( padding-- ) abAppend (ab, " ", 1) ;

		abAppend (ab, welcome, welcomelen) ;
		} else {
			abAppend (ab, "~", 1) ;
		}
		
		abAppend(ab, "\x1b[K", 3);  //clears the cursor to the end of the line 
		if( y < E.screenrows - 1){
			abAppend(ab, "\r\n", 2) ;
		}
	}
}

void editorRefreshScreen(){
	struct abuf ab = ABUF_INIT ;

	abAppend(&ab, "\x1b[?25l", 6) ;     //hides the cursor to prevent annoyting flicker before refresh 
	abAppend(&ab, "\x1b[H", 3);

	editorDrawRows(&ab);

	char buf[32] ;
	snprintf (buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1) ;
	abAppend (&ab, buf, strlen(buf)) ;

	abAppend(&ab, "\x1b[?25h", 6) ;     // show cursor after the refresh 

	write(STDOUT_FILENO, ab.b, ab.len) ;
	abFree(&ab) ;
}

/**** input ***/

void editorMoveCursor (int key){
	switch(key){
		case ARROW_UP :
			if (E.cy != 0){
				E.cy-- ;
			}
			break ;
		case ARROW_LEFT :
			if (E.cx != 0){
				E.cx -- ;
			}
			break ;
		case ARROW_DOWN :
			if (E.cy != E.screenrows - 1){
				E.cy++ ;
			}
			break ;
		case ARROW_RIGHT :
			if (E.cx != E.screencols - 1){
				E.cx++ ;
			}
			break ;
	}
}

void editorProcessKeypress(){
	int c = editorReadKey() ;

	switch(c){
		case CTRL_KEY('q') :
			write(STDOUT_FILENO, "\x1b[2J", 4) ;
			write(STDOUT_FILENO, "\x1b[H", 3) ;
			exit(0);
			break ;
		
		case ARROW_UP :
		case ARROW_LEFT :
		case ARROW_DOWN :
		case ARROW_RIGHT :
			editorMoveCursor(c) ;
			break ;
		
	}
}
/*** init  ****/

void initEditor(){
	E.cx = 0 ;
	E.cy = 0 ;

	if (getWindowSize (&E.screenrows, &E.screencols) == -1) die("Get window size") ;	

}

int main(){

	enableRawMode() ;
	initEditor() ;

	while (1){ 
		editorRefreshScreen() ;
		editorProcessKeypress() ;
  	}

	return 0 ;
}
