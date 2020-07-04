provider dtget{
    probe query__primeiropasso();
    probe query__segundopasso();
    probe query__terceiroquartopasso();
    probe query__terceiropasso();
    probe query__quartopasso();
    probe query__fim();
    probe query__maxEntry(unsigned int *,int);
    probe query__maxReturn(int);
    probe query__transformEntry();
    probe query__transformReturn();
};
