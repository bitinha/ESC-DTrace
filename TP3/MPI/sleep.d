#!/usr/sbin/dtrace -qs

#pragma D option quiet

BEGIN
{
	parent_process=$1;
	T_gather = 0;
	T_scatter = 0;
	T_dt = 0;
	wtime_calls = 0;
	collect = "Zona sequencial";
}


pid$target::MPI_Wtime:entry
/wtime_calls==5/
{
	collect = "Zona sequencial";
	T_gather = timestamp - T_gather;
	wtime_calls=6;
}
pid$target::MPI_Wtime:entry
/wtime_calls==4/
{
	collect = "gather";
	T_gather = timestamp;
	wtime_calls = 5;
}

pid$target::MPI_Wtime:entry
/wtime_calls==3/
{
	collect = "transformaGS";
	T_dt = timestamp - T_dt;
	wtime_calls = 4;
}


pid$target::MPI_Wtime:entry
/wtime_calls==2/
{
	collect = "dt";
	T_dt = timestamp;
	wtime_calls = 3;
}
pid$target::MPI_Wtime:entry
/wtime_calls==1/
{
	collect = "Zona sequencial";
	T_scatter = timestamp - T_scatter;
	wtime_calls = 2;
}

pid$target::MPI_Wtime:entry
/wtime_calls==0/
{
	collect = "scatter";
	T_scatter = timestamp;
	wtime_calls = 1;
}

sched:::sleep
/pid == $target/
{
	@sleep[collect] = count();
}

END
{
	printf("PID=%d: Scatter=%dms; DT=%dms; Gather=%dms",$target,T_scatter/1000000,T_dt/1000000,T_gather/1000000);
	printa("Zona %-30s: %@d\n",@sleep);
}

