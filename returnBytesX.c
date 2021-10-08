#define KXVER 3
#include"k.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



K return_bytes(void *p, size_t len)
{
    K x=ktn(KG,len);
    memcpy(kG(x),p,x->n);
    return x;
}


//takes any K object as argument
//returns a a list of bytes (as a K object)
K returnBytesX(K x)
{
        switch(xt) {

                case   0: return return_bytes(x,16+8*xn); break;
                case  -1: return return_bytes(x,9); break;
                case  -2: return return_bytes(x,32); break;
                case  -4: return return_bytes(x,9); break;
                case  -5: return return_bytes(x,10); break;
                case  -6: return return_bytes(x,12); break;
                case  -7: return return_bytes(x,16); break;
                case  -8: return return_bytes(x,12); break;
                case  -9: return return_bytes(x,16); break;
                case -10: return return_bytes(x,9); break;
                case -11: return return_bytes(x,16); break;
                case -12: return return_bytes(x,16); break;
                case -13: return return_bytes(x,12); break;
                case -14: return return_bytes(x,12); break;
                case -15: return return_bytes(x,16); break;
                case -16: return return_bytes(x,16); break;
                case -17: return return_bytes(x,12); break;
                case -18: return return_bytes(x,12); break;
                case -19: return return_bytes(x,12); break;

                case   1: return return_bytes(x,16+xn); break;
                case   2: return return_bytes(x,16+16*xn); break;
                case   4: return return_bytes(x,16+xn); break;
                case   5: return return_bytes(x,16+2*xn); break;
                case   6: return return_bytes(x,16+4*xn); break;
                case   7: return return_bytes(x,16+8*xn); break;
                case   8: return return_bytes(x,16+4*xn); break;
                case   9: return return_bytes(x,16+8*xn); break;
                case   10: return return_bytes(x,16+xn); break;
                case   11: return return_bytes(x,16+8*xn); break;
                case   12: return return_bytes(x,16+8*xn); break;
                case   13: return return_bytes(x,16+4*xn); break;
                case   14: return return_bytes(x,16+4*xn); break;
                case   15: return return_bytes(x,16+8*xn); break;
                case   16: return return_bytes(x,16+8*xn); break;
                case   17: return return_bytes(x,16+4*xn); break;
                case   18: return return_bytes(x,16+4*xn); break;
                case   19: return return_bytes(x,16+4*xn); break;

                case   20 ... 76: return return_bytes(x,16+8*xn); break;

                case   77: return return_bytes(x,16+8*xn); break;

                case   98: return return_bytes(x,16); break;
                case   99: return return_bytes(x,32); break;
                case   100: return return_bytes(x,16+8*xn); break;
                case   102: return return_bytes(x,16); break;
                case   103: return return_bytes(x,16); break;
                case   104: return return_bytes(x,16+8*xn); break;
                case   105: return return_bytes(x,32); break;
                case   106 ... 111: return return_bytes(x,16); break;
                case   112: return return_bytes(x,16+8*xn); break;

                default: return krr("notimplemented"); break;
                //default: return return_bytes(x,32); break;
        }
        return (K) 0;
}

K getCount(K x)
{
        return kj(xn);
}

K getMemLocation(K x)
{
        if(4!=xt) return krr("type");
        if(8!=xn) return krr("length");
        return returnBytesX(*(K*) kG(x));
}

K getTypeMemLocation(K x)
{
        if(4!=xt) return krr("type");
        if(8!=xn) return krr("length");
        return kh((*kK(x))->t);
}

K getCountMemLocation(K x)
{
        if(4!=xt) return krr("type");
        if(8!=xn) return krr("length");
        return kj((*kK(x))->n);
}

K getSymMemLocation(K x)
{
        if(4!=xt) return krr("type");
        if(8!=xn) return krr("length");
        return ks(*(S*) kG(x));
}



//rbx:`returnBytesX 2:(`returnBytesX;1)
//getMemLocation:`returnBytesX 2:(`getMemLocation;1)
//getSymMemLocation:`returnBytesX 2:(`getSymMemLocation;1);

