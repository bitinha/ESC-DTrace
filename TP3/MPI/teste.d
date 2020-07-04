#!/usr/sbin/dtrace -qs

#pragma D option quiet

BEGIN
{
	T_gather = 0;
	T_scatter = 0;
	T_dt = 0;
	wtime_calls = 0;
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
	printf("PID=%d: Scatter=%dms; DT=%dms; Gather=%dms",$target,T_scatter/1000000,T_dt/1000000,T_gather/1000000);
}

