#!/usr/sbin/dtrace -qs

#pragma D option quiet

BEGIN
{
	collect = "Zona nao paralelizada"
}

dtget$target:::query-primeiropasso
{
	collect = "T1";
}
dtget$target:::query-segundopasso
{
	collect = "T2";
}
dtget$target:::query-terceiroquartopasso
{
	collect = "T3+T4";
}
dtget$target:::query-fim
{
	collect = "Zona nao paralelizada";
}

dtget$target:::query-maxEntry
{
	collect = "transformaGS";
}

dtget$target:::query-transformReturn
{
	collect = "Zona nao paralelizada";
}

plockstat$target:::mutex-acquire
{ 
	@acquire[collect]=count();
	@iter[collect] = sum(arg2);
}

plockstat$target:::mutex-block
{
	@block[collect] = count();
}

plockstat$target:::mutex-spin
{
	@spin[collect] = count();
}

END
{
	printf("\nacquire");
	printa(@acquire);
	printf("\nblock");
	printa(@block);
	printf("\nspin");
	printa(@spin);
	printf("\niterations");
	printa(@iter);
}

