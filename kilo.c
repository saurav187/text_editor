#include<ctype.h>
#include<errno.h>
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<stdlib.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios orig_termios ;

void die(const char *s){
	perror(s) ;
	exit(1);
}

void disableRawMode(){
	if( (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) ) die("tcssetarr") ;
}


void enableRawMode (){

	if ( tcgetattr ( STDIN_FILENO, &orig_termios ) == -1 ) die("tcgetatt") ;
	atexit(disableRawMode) ;

	struct termios raw = orig_termios;
	
	//to disable certain keys like ctrl + c , ctrl + v etc. 
  	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  	raw.c_oflag &= ~(OPOST);
 	raw.c_cflag |= (CS8);
  	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	
	raw.c_cc[VMIN] = 1 ;
	raw.c_cc[VTIME] = 0 ;


	if (tcsetattr ( STDIN_FILENO, TCSAFLUSH, &raw ) == -1) die("tcsetarrr") ;
}


int main(){

	enableRawMode() ;
	
	while (1){
		char c = '\0' ;
  	
		if ( read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN ) die("read") ;
    		
		if (iscntrl(c)) {
      			printf("%d\r\n", c);
    			} else {
      			printf("%d ('%c')\r\n", c, c);
    		}
		if ( c == CTRL_KEY('q')) break ;
  	}

	return 0 ;
}
