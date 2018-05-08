// wcl -bcl=dos -os -s tonystub.c

int main() {

static char msg[]="\r\n"
                  "wget http://www.japheth.de/Download/HXRT.ZIP\r\n\r\n"
                  "( or visit http://www.bttr-software.de/forum/ )"
                  "\r\n"
                  "$";

   _asm {
       mov ah,9
       mov dx,offset msg
       int 21h
   }

     return 0;
}
