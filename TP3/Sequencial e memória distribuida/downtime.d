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

/* Registo da criaÃao de threads */
proc:::lwp-create
/pid==$target/
{
	tempo[curlwpsinfo->pr_addr]=timestamp;
	passo[curlwpsinfo->pr_addr]="";
}

/* Calculo do tempo que demorou para uma thread entrar num cpu 
 *
 * So deve contar se quando saiu de um cpu, ainda estava no mesmo passo
 *
 * */
sched:::on-cpu
/tempo[curlwpsinfo->pr_addr] && passo[curlwpsinfo->pr_addr]==collect/
{
	@downtime[collect] = llquantize((timestamp - tempo[curlwpsinfo->pr_addr])/1000,10,1,3,10);
	/*@off_time[collect] = sum(timestamp - tempo[curlwpsinfo->pr_addr]);*/
}

/* Inicio da contabilizacao do tempo que uma thread passa fora de uma cpu*/
sched:::off-cpu
/tempo[curlwpsinfo->pr_addr]/
{
	@t[collect] = count();
	passo[curlwpsinfo->pr_addr] = collect;
	tempo[curlwpsinfo->pr_addr] = timestamp;
}
