#!/usr/sbin/dtrace -qs

#pragma D option quiet

BEGIN
{
	collect = "Zona nao paralelizada";
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

pid$target::transformaGS:entry
{
	collect = "transformaGS";
}

pid$target::transformaGS:return
{
	collect = "Zona nao paralelizada";
}
/*
cpc:::PAPI_l1_dca-user-10000
/pid==$target/
{
	@Accesses[collect]=count();
}
*/

cpc:::PAPI_l2_dcm-user-10000
/pid==$target/
{
        @Misses[collect] = count();
}

/*
cpc:::PAPI_br_tkn-user-10000
/pid==$target/
{
	@br[collect]=count();
}
*/
/*
cpc:::PAPI_tot_ins-user-1000000
/pid==$target/
{
	@ins[collect] = count();
}
*/
