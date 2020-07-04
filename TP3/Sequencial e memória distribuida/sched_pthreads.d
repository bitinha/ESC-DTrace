#!/usr/sbin/dtrace -qs

#pragma D option quiet

BEGIN
{
	collect = "Zona nao paralelizada"
}

dtget$target:::query-primeiropasso
{
	col[curlwpsinfo->pr_lwpid] = "T1";
	collect = "T1";
}
dtget$target:::query-segundopasso
{
	col[curlwpsinfo->pr_lwpid] = "T2";
	collect = "T2";
}
dtget$target:::query-terceiroquartopasso
{
	col[curlwpsinfo->pr_lwpid] = "T3+T4";
	collect = "T3+T4";
}
dtget$target:::query-fim
{
	col[curlwpsinfo->pr_lwpid] = "Zona nao paralelizada";
	collect = "Zona nao paralelizada";
}

dtget$target:::query-maxEntry
{
	col[curlwpsinfo->pr_lwpid] = "TransformaGS";
	collect = "transformaGS";
}

dtget$target:::query-transformReturn
{
	col[curlwpsinfo->pr_lwpid] = "Zona nao paralelizada";
	collect = "Zona nao paralelizada";
}

sched:::sleep
/pid == $target/
{
	@sleep[curlwpsinfo->pr_lwpid,col[curlwpsinfo->pr_lwpid]] = count();
}

END
{
	printa("Thread %2d, Zona %-30s: %@d\n",@sleep);
}
