#!/usr/sbin/dtrace -qs

#pragma D option quiet

BEGIN
{
	c=0;
	collect = "Zonas nao paralelizadas"
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
	collect = "Zonas nao paralelizadas";
}

dtget$target:::query-maxEntry
{
	collect = "TransformaGS";
}

dtget$target:::query-transformReturn
{
	collect = "Zonas nao paralelizadas";
}

profile-997
/*pid == $target*/
{
	estado = (pid==$target) ? curlwpsinfo->pr_state : 0;
	@name[curlwpsinfo->pr_sname] = count();
	@proc[collect] = lquantize(estado, 0, 10, 1);
}


