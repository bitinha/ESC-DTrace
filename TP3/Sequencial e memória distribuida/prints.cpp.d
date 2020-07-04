#!/usr/sbin/dtrace -qs

#pragma D option quiet

dtget$target:::query-primeiropasso
{
	dtts = timestamp;
	self->ts = timestamp;
}
dtget$target:::query-segundopasso
{
	t1 = timestamp - self->ts;
	self->ts = 0;/*timestamp;*/
	tst2 = timestamp;
}
dtget$target:::query-terceiroquartopasso
{
	t2 = timestamp - tst2;/*self->ts;*/
	self->ts = timestamp;
}
dtget$target:::query-fim
{
	t3 = timestamp - self->ts;
	self->ts = 0;
	Tdt = timestamp - dtts;
}
dtget$target:::query-maxReturn
{
	maximo = arg0;
}
/*
pid$target::dt_p1:entry
{
	dtts = timestamp;
}
pid$target::dt_p2:return
{
	Tdt = timestamp - dtts;
}

pid$target::transformaGS:entry
{
	self->ts = timestamp;
}
pid$target::transformaGS:return
{
	tgs = timestamp - self->ts;
	self->ts = 0;
}*/
dtget$target:::query-transformEntry
{
	self->ts = timestamp;
}
dtget$target:::query-transformReturn
{
	tgs = timestamp - self->ts;
	self->ts = 0;
}
END
{
	printf("|---------- DTrace Output ----------\n");
	printf("| T1 = %d.%06d\n",t1/1000000000,(t1%1000000000)/1000);
	printf("| T2 = %d.%06d\n",t2/1000000000,(t2%1000000000)/1000);
	printf("| T3+T4 = %d.%06d\n",t3/1000000000,(t3%1000000000)/1000);
	printf("| Tdt = %d.%06d\n",Tdt/1000000000,(Tdt%1000000000)/1000);
	printf("| Max=%d\n",maximo);
	printf("| TtransformaGS = %d.%06d\n",tgs/1000000000,(tgs%1000000000)/1000);
}

