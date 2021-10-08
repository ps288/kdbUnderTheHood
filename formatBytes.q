//format bytes


//////////////////////////////
//Dynamically loaded functions
//////////////////////////////


//takes any kdb object and returns the byte represention
rbx:`returnBytesX 2:(`returnBytesX;1);                          //takes any kdb object and returns the byte represention

getCount:`returnBytesX 2:(`getCount;1);                         //need this to get the count of objects we don't think of having a count eg. lamdas


//takes a memory location an returns byte represenation of kdb object there
//CAREFUL: If no kdb object there the result is nonesense
//A bad memory location will cause a crash
getMemLocation:`returnBytesX 2:(`getMemLocation;1);

//takes a memory location an returns the type of kdb object there
//same warnings above apply here
getTypeMemLocation:`returnBytesX 2:(`getTypeMemLocation;1);

//takes a memory location an returns the count of kdb object there
//same warnings above apply here
getCountMemLocation:`returnBytesX 2:(`getCountMemLocation;1);

getSymMemLocation:`returnBytesX 2:(`getSymMemLocation;1);

/////////////
//q functions
/////////////

//helper function to format a byte representaion of K object
cutInds:{[t;n]
  r:0 1 2 3 4 8;  //all K objects share common header positions
  //complex datatypes
  if[any t=99 105;:r,16 24];
  if[any t=98 102 103, 106+til 6;:r];
  if[(t within 20 76) or any t=77 100 104 112;:r,16+8*til n];
  if[t within 78 97;'`NYI];     //TODO these type

  //basic datatypes
  if[2=abs t;:r,16+16*til n];    //GUID atom is like a single element list
  $[1=signum t;
    [s:16+til[n]*      //uniform lists. Different cases for the 1,2,4,8 byte objects
      $[any t=1 4 10;
        1;
        any t=5;
        2;
        any t=6 8 13 14 17 18 19;
        4;
        8]];
    -1=signum t;       //atoms
      s:();
    s:16+til[n]*8      //mixed lists
  ];
  :r,s;
 };


//formatting functions
formatBytes:{[x] cutInds[type x;getCount x] cut rbx x};
formatBytesMem:{[x] cutInds[getTypeMemLocation x;getCountMemLocation x] cut getMemLocation x};
