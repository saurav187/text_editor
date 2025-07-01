#include<ctype.h>
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<stdlib.h>

struct termios orig_termios ;

void disableRawMode(){
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) ;
}


void enableRawMode (){

	tcgetattr ( STDIN_FILENO, &orig_termios ) ;
	atexit(disableRawMode) ;

	struct termios raw = orig_termios;
	
	//to disable certain keys like ctrl + c , ctrl + v etc. 
  	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  	raw.c_oflag &= ~(OPOST);
 	raw.c_cflag |= (CS8);
  	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	
	raw.c_cc[VMIN] = 1 ;
	raw.c_cc[VTIME] = 0 ;


	tcsetattr ( STDIN_FILENO, TCSAFLUSH, &raw ) ;
}


int main(){

	enableRawMode() ;
	
	while (1){
		char c = '\0' ;
  	
		read(STDIN_FILENO, &c, 1) ;
    		
		if (iscntrl(c)) {
      			printf("%d\r\n", c);
    			} else {
      			printf("%d ('%c')\r\n", c, c);
    		}
		if ( c == 'q' ) break ;
  	}

	return 0 ;
}
