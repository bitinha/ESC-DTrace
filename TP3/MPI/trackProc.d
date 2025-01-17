#!/usr/sbin/dtrace -qs

#pragma D option quiet
#pragma D option destructive

BEGIN
{
	script = $1;
	n_procs = 0;
	processos[$target] = 1;
}


proc:::create
/pid==$target/
{
	n_procs++;
	/*processos[args[0]->pr_pid] = 1;*/
	system( "dtrace -s %s -p %d %d &", script, args[0]->pr_pid, pid );
}
/*
entry
/processos[pid] && wtime_calls[pid]==0/
{
	T_scatter[pid] = timestamp;
	wtime_calls[pid] = 1;
}

pid*::MPI_Wtime:entry
/processos[pid] && wtime_calls[pid]==1/
{
	T_scatter[pid] = timestamp - T1[pid];
	wtime_calls[pid] = 2;
}
pid*::MPI_Wtime:entry
/processos[pid] && wtime_calls[pid]==2/
{
	T_dt[pid] = timestamp;
	wtime_calls[pid] = 3;
}
pid*::MPI_Wtime:entry
/processos[pid] && wtime_calls[pid]==3/
{
	T_dt[pid] = timestamp - T_dt[pid];
	wtime_calls[pid] = 4;
}
pid*::MPI_Wtime:entry
/processos[pid] && wtime_calls[pid]==4/
{
	T_gather[pid] = timestamp;
	wtime_calls[pid] = 5;
}
pid*::MPI_Wtime:entry
/processos[pid] && wtime_calls[pid]==5/
{
	T_gather[pid] = timestamp - T_gather[pid];
	wtime_calls=6;
}




profile-997
/processos[args[0]->pr_pid]/
{
	estado = (processos[pid]==1) ? curlwpsinfo->pr_state : 0;
	@name[curlwpsinfo->pr_sname] = count();
	@proc = lquantize(estado, 0, 10, 1);
}

*/
