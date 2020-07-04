#!/usr/sbin/dtrace -qs

#pragma D option quiet

BEGIN
{
	parent_process = $1;
	recv=0;
	receive_time=0;
	T_gather = 0;
	T_scatter = 0;
	T_dt = 0;
	wtime_calls = 0;
}
/*
pid$target::dt:entry
{
	printf("%d-%d\n",pid,timestamp);
}*/
pid$target::MPI_Recv:entry
{
/*	@e[recv]=sum(timestamp);
*/	self->ts =timestamp;
}
pid$target::MPI_Recv:return
/self->ts/
{
/*	@h[recv]=sum(timestamp);
*/	@rec=sum(timestamp-self->ts);
/*	@receive_time = quantize(timestamp-self->ts);
*/	self->ts=0;
/*	recv++;*/
}

pid$target::MPI_Wtime:entry
/wtime_calls==5/
{
	T_gather = timestamp - T_gather;
	wtime_calls=6;
}
pid$target::MPI_Wtime:entry
/wtime_calls==4/
{
	T_gather = timestamp;
	wtime_calls = 5;
}

pid$target::MPI_Wtime:entry
/wtime_calls==3/
{
	T_dt = timestamp - T_dt;
	wtime_calls = 4;
}


pid$target::MPI_Wtime:entry
/wtime_calls==2/
{
	T_dt = timestamp;
	wtime_calls = 3;
}
pid$target::MPI_Wtime:entry
/wtime_calls==1/
{
	T_scatter = timestamp - T_scatter;
	wtime_calls = 2;
}

pid$target::MPI_Wtime:entry
/wtime_calls==0/
{
	T_scatter = timestamp;
	wtime_calls = 1;
}


END
{
	printf("PID=%d: Scatter=%dms; DT=%dms; Gather=%dms\n",$target,T_scatter/1000000,T_dt/1000000,T_gather/1000000);
	normalize(@rec,1000000);
	printa("Tempo bloqueado em MPI_Recv(): %@dms\n",@rec);
}

