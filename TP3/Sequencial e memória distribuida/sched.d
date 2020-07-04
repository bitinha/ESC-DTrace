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

sched:::sleep
/pid == $target/
{
	@sleep[curlwpsinfo->pr_lwpid,collect] = count();
}

END
{
	printa("Thread %2d, Zona %-30s: %@d\n",@sleep);
}
